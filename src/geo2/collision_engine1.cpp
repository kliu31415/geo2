#include "geo2/collision_engine1.h"
#include "geo2/multithread/thread_pool.h"

#include "geo2/timer.h"

#include <cmath>
#include <algorithm>
#include <execution>

namespace geo2 {

using namespace map_obj;

bool cmp_idx_int(const CEng1Obj &a, int b)
{
    return a.idx < b;
}
int CollisionEngine1::x_to_grid_x(float x) const
{
    int x_ = std::clamp((int)((x - global_AABB.x1) / grid_rect_w), 0, GRID_LEN - 1);
    return x_;
}
int CollisionEngine1::y_to_grid_y(float y) const
{
    int y_ = std::clamp((int)((y - global_AABB.y1) / grid_rect_h), 0, GRID_LEN - 1);
    return y_;
}
void CollisionEngine1::remove_cur_from_grid(int idx)
{
    auto remove_obj_from_grid = [this, idx](const Polygon *polygon, int) -> void
    {
        auto aabb = polygon->get_AABB();
        int x = x_to_grid_x(aabb.x1);
        int y = y_to_grid_y(aabb.y1);
        grid.remove_one_with_idx(x, y, idx);
    };
    (*ceng_data)[idx].for_each_cur(remove_obj_from_grid);
}
void CollisionEngine1::remove_des_from_grid(int idx)
{
    auto remove_obj_from_grid = [this, idx](const Polygon *polygon, int) -> void
    {
        auto aabb = polygon->get_AABB();
        int x = x_to_grid_x(aabb.x1);
        int y = y_to_grid_y(aabb.y1);
        grid.remove_one_with_idx(x, y, idx);
    };
    (*ceng_data)[idx].for_each_des(remove_obj_from_grid);
}
void CollisionEngine1::add_cur_to_grid(int idx, std::vector<CEng1Collision> *collisions)
{
    auto add_obj_to_grid = [this, idx, collisions](const Polygon *polygon, int shape_id)
                            {
                                auto aabb = polygon->get_AABB();
                                //we don't have to update global_AABB because that's only used
                                //for optimization (making the grid bounds as tight as possible)
                                max_AABB_w = std::max(max_AABB_w, aabb.x2 - aabb.x1);
                                max_AABB_h = std::max(max_AABB_h, aabb.y2 - aabb.y1);
                                int x = x_to_grid_x(aabb.x1);
                                int y = y_to_grid_y(aabb.y1);
                                CEng1Obj obj(polygon, idx, shape_id);
                                find_and_add_collisions_neq(collisions, obj);
                                grid.get_ref(x, y).push_back(obj);


                            };
    (*ceng_data)[idx].for_each_cur(add_obj_to_grid);
}
void CollisionEngine1::add_des_to_grid(int idx, std::vector<CEng1Collision> *collisions)
{
    auto add_obj_to_grid = [this, idx, collisions](const Polygon *polygon, int shape_id)
                            {
                                auto aabb = polygon->get_AABB();
                                //we don't have to update global_AABB because that's only used
                                //for optimization (making the grid bounds as tight as possible)
                                max_AABB_w = std::max(max_AABB_w, aabb.x2 - aabb.x1);
                                max_AABB_h = std::max(max_AABB_h, aabb.y2 - aabb.y1);
                                int x = x_to_grid_x(aabb.x1);
                                int y = y_to_grid_y(aabb.y1);
                                CEng1Obj obj(polygon, idx, shape_id);
                                find_and_add_collisions_neq(collisions, obj);
                                grid.get_ref(x, y).push_back(obj);
                            };
    (*ceng_data)[idx].for_each_des(add_obj_to_grid);
}
void CollisionEngine1::find_and_add_collisions_neq(std::vector<CEng1Collision> *add_to,
                                                   const CEng1Obj &ceng_obj)
                                                   const
{
    const auto &aabb = ceng_obj.polygon->get_AABB();
    int x1 = x_to_grid_x(aabb.x1 - max_AABB_w);
    int x2 = x_to_grid_x(aabb.x2);
    int y1 = y_to_grid_y(aabb.y1 - max_AABB_h);
    int y2 = y_to_grid_y(aabb.y2);

    for(int x=x1; x<=x2; x++) {
        for(int y=y1; y<=y2; y++) {
            for(const auto &other: grid.get_const_ref(x, y)) {
                //filter out a potential collision if:
                //-the .idx (owner) is the same
                //-the AABBs don't overlap
                //-the collision wouldn't matter anyway
                if(ceng_obj.idx != other.idx &&
                   collision_could_matter(*(*map_objs)[ceng_obj.idx], *(*map_objs)[other.idx]))
                {
                    if(ceng_obj.polygon->has_collision(*other.polygon)) {
                        CEng1Collision collision;
                        collision.idx1 = ceng_obj.idx;
                        collision.idx2 = other.idx;
                        add_to->push_back(collision);
                    }
                }
            }
        }
    }
}
void CollisionEngine1::find_and_add_collisions_gt(std::vector<CEng1Collision> *add_to,
                                                  const CEng1Obj &ceng_obj) const
{
    //Only look for collisions to the right! Break ties by x.
    const auto &aabb = ceng_obj.polygon->get_AABB();
    int x1 = x_to_grid_x(aabb.x1);
    int x2 = x_to_grid_x(aabb.x2);
    int y1 = y_to_grid_y(aabb.y1 - max_AABB_h);
    int y2 = y_to_grid_y(aabb.y2);

    //to optimize for performance, break this into two cases:
    //(1) same x grid value
    for(int y=y1; y<=y2; y++) {
        for(const auto &other: grid.get_const_ref(x1, y)) {
            auto other_AABB = other.polygon->get_AABB();

            //only consider other objects with AABBs to the right, breaking ties by index
            if(aabb.x1 < other_AABB.x1)
                continue;
            if(aabb.x1 == other_AABB.x1 && ceng_obj.idx <= other.idx)
                continue;

            if(ceng_obj.idx != other.idx &&
               collision_could_matter(*(*map_objs)[ceng_obj.idx], *(*map_objs)[other.idx]))
            {
                if(ceng_obj.polygon->has_collision(*other.polygon)) {
                    CEng1Collision collision;
                    collision.idx1 = ceng_obj.idx;
                    collision.idx2 = other.idx;
                    add_to->push_back(collision);
                }
            }
        }
    }

    //(2) higher x grid value
    for(int x=x1+1; x<=x2; x++) {
        for(int y=y1; y<=y2; y++) {
            for(const auto &other: grid.get_const_ref(x, y)) {
                //filter out a potential collision if:
                //-the .idx (owner) is the same
                //-the AABBs don't overlap
                //-the collision wouldn't matter anyway

                //note that we assume the grid cells are ordered, so we can break
                //and move on to the next cell if our idx isn't greater than the other's idx
                if(ceng_obj.idx != other.idx &&
                   collision_could_matter(*(*map_objs)[ceng_obj.idx], *(*map_objs)[other.idx]))
                {
                    if(ceng_obj.polygon->has_collision(*other.polygon)) {
                        CEng1Collision collision;
                        collision.idx1 = ceng_obj.idx;
                        collision.idx2 = other.idx;
                        add_to->push_back(collision);
                    }
                }
            }
        }
    }
}
bool CollisionEngine1::des_cur_has_collision(int idx1, int idx2) const
{
    bool collision = false;

    auto f1 = [this, &collision, idx2](const Polygon *polygon, int) -> void
        {
            auto f2 = [this, polygon, &collision](const Polygon *polygon2, int) -> void
                {
                    collision |= polygon->has_collision(*polygon2);
                };
            (*ceng_data)[idx2].for_each_cur(f2);
        };

    (*ceng_data)[idx1].for_each_des(f1);

    return collision;
}
CollisionEngine1::CollisionEngine1(std::shared_ptr<ThreadPool> thread_pool_):
    thread_pool(std::move(thread_pool_))
{

}
void CollisionEngine1::reset()
{
    grid.reset();
}
void CollisionEngine1::set_ceng_data(std::vector<CEng1Data> *data)
{
    ceng_data = data;
}
void CollisionEngine1::set2(const std::vector<std::shared_ptr<map_obj::MapObject>> *map_objs_,
                std::function<bool(const MapObject&, const MapObject&)> collision_could_matter_)
{
    map_objs = map_objs_;
    collision_could_matter = std::move(collision_could_matter_);
    grid.reset();

    k_expects(map_objs->size() == ceng_data->size());
}
std::vector<CEng1Collision> CollisionEngine1::find_collisions()
{
    //step 1
    active_objs.clear();

    //~50us on Test2(40, 40)
    for(size_t i=0; i<ceng_data->size(); i++) {
        auto add_active_obj = [this, i](const Polygon *polygon, int shape_idx) -> void
                            {
                                active_objs.emplace_back(polygon, i, shape_idx);
                            };

        auto move_intent = (*ceng_data)[i].get_move_intent();
        if(move_intent == MoveIntent::StayAtCurrentPos)
            (*ceng_data)[i].for_each_cur(add_active_obj);
        else if(move_intent == MoveIntent::GoToDesiredPos)
            (*ceng_data)[i].for_each_des(add_active_obj);
        else {
            //-NotSet is the correct move intent if we don't want to add any shapes
            //-RemoveShapes would also work, as it also results in no shapes being
            // added, but we prefer NotSet for cleanliness
            k_assert(move_intent == MoveIntent::NotSet);
        }
    }

    //step 2
    #if 0 //#ifdef __AVX2__
    //the spec for gather doesn't require any alignment, so I'll comment out this assert...
    //hopefully alignment isn't implicitly required
    //k_assert((uintptr_t)active_objs.data() % 8 == 0);
    auto active_objs_base_ptr = (long long int*)active_objs.data();
    auto mm_vec_iterator = sizeof(CEng1Obj) * _mm256_set_epi64x(3, 2, 1, 0);

    auto mm_max_dimensions = _mm256_set1_ps(0);
    auto mm_global_aabb_min = _mm256_set1_ps( std::numeric_limits<float>::max());
    auto mm_global_aabb_max = _mm256_set1_ps(-std::numeric_limits<float>::max());

    auto last_avx_idx = 4 * (active_objs.size() / 4);
    //we grid things based on their top left corner, so calculate the global
    //AABB based on top left corners only.
    for(size_t i=0; i<last_avx_idx; i+=4) {
        static_assert(offsetof(CEng1Obj, polygon) == 0);
        static_assert(Polygon::offset_of_aabb() == 0);

        static_assert(offsetof(AABB, x1) == 0);
        static_assert(offsetof(AABB, y1) == 4);
        static_assert(offsetof(AABB, x2) == 8);
        static_assert(offsetof(AABB, y2) == 12);

        static_assert(std::is_same_v<decltype(AABB::x1), float>);
        static_assert(std::is_same_v<decltype(AABB::y1), float>);
        static_assert(std::is_same_v<decltype(AABB::x2), float>);
        static_assert(std::is_same_v<decltype(AABB::y2), float>);

        //only holds for 64 bit compilers, but it shouldn't be hard to adapt this to 32-bit compilers in the future
        static_assert(sizeof(CEng1Obj::polygon) == 8);

        auto mm_x1y1_addr = _mm256_i64gather_epi64(active_objs_base_ptr, mm_vec_iterator, 1);
        auto mm_x2y2_addr = 8 + mm_x1y1_addr;

        auto mm_x1y1_epi64 = _mm256_i64gather_epi64(nullptr, mm_x1y1_addr, 1);
        auto mm_x2y2_epi64 = _mm256_i64gather_epi64(nullptr, mm_x2y2_addr, 1);

        auto mm_x1y1 = *reinterpret_cast<__m256*>(&mm_x1y1_epi64);
        auto mm_x2y2 = *reinterpret_cast<__m256*>(&mm_x2y2_epi64);

        mm_vec_iterator += sizeof(CEng1Obj) * 4;

        mm_global_aabb_min = _mm256_min_ps(mm_global_aabb_min, mm_x1y1);
        mm_global_aabb_max = _mm256_max_ps(mm_global_aabb_max, mm_x1y1);
        mm_max_dimensions = _mm256_max_ps(mm_max_dimensions, mm_x2y2 - mm_x1y1);
    }
    std::array<float, 8> mm_vals;

    _mm256_storeu_ps(mm_vals.data(), mm_max_dimensions);
    max_AABB_w = std::max(mm_vals[0], std::max(mm_vals[2], std::max(mm_vals[4], mm_vals[6])));
    max_AABB_h = std::max(mm_vals[1], std::max(mm_vals[3], std::max(mm_vals[5], mm_vals[7])));

    _mm256_storeu_ps(mm_vals.data(), mm_global_aabb_min);
    global_AABB.x1 = std::min(mm_vals[0], std::min(mm_vals[2], std::min(mm_vals[4], mm_vals[6])));
    global_AABB.y1 = std::min(mm_vals[1], std::min(mm_vals[3], std::min(mm_vals[5], mm_vals[7])));

    _mm256_storeu_ps(mm_vals.data(), mm_global_aabb_max);
    global_AABB.x2 = std::max(mm_vals[0], std::max(mm_vals[2], std::max(mm_vals[4], mm_vals[6])));
    global_AABB.y2 = std::max(mm_vals[1], std::max(mm_vals[3], std::max(mm_vals[5], mm_vals[7])));

    for(auto i=last_avx_idx; i<active_objs.size(); i++) {
        const auto &aabb = active_objs[i].polygon->get_AABB();

        max_AABB_w = std::max(max_AABB_w, aabb.x2 - aabb.x1);
        max_AABB_h = std::max(max_AABB_h, aabb.y2 - aabb.y1);

        global_AABB.x1 = std::min(global_AABB.x1, aabb.x1);
        global_AABB.y1 = std::min(global_AABB.y1, aabb.y1);
        global_AABB.x2 = std::max(global_AABB.x2, aabb.x1);
        global_AABB.y2 = std::max(global_AABB.y2, aabb.y1);
    }
    #else
    //this until processing the active objects takes ~80us on Test2(40, 40)
    max_AABB_w = 0.0f;
    max_AABB_h = 0.0f;

    //we grid things based on their top left corner, so calculate the global
    //AABB based on top left corners only.
    global_AABB = AABB::make_maxbad_AABB();
    for(const auto &obj: active_objs) {
        const auto &aabb = obj.polygon->get_AABB();

        max_AABB_w = std::max(max_AABB_w, aabb.x2 - aabb.x1);
        max_AABB_h = std::max(max_AABB_h, aabb.y2 - aabb.y1);

        global_AABB.x1 = std::min(global_AABB.x1, aabb.x1);
        global_AABB.x2 = std::max(global_AABB.x2, aabb.x1);
        global_AABB.y1 = std::min(global_AABB.y1, aabb.y1);
        global_AABB.y2 = std::max(global_AABB.y2, aabb.y1);
    }
    #endif

    grid_rect_w = (global_AABB.x2 - global_AABB.x1) / GRID_LEN;
    grid_rect_h = (global_AABB.y2 - global_AABB.y1) / GRID_LEN;

    //step 3
    //~100us on Test2(40, 40)
    for(const auto &obj: active_objs) {
        const auto &aabb = obj.polygon->get_AABB();
        auto x = x_to_grid_x(aabb.x1);
        auto y = y_to_grid_y(aabb.y1);
        grid.get_ref(x, y).push_back(obj);
    }

    //step 4
    size_t num_threads = 1 + thread_pool->size();

    std::vector<std::vector<CEng1Collision>> collisions(num_threads);
    std::vector<std::function<void()>> tasks(num_threads);
    std::vector<std::future<void>> is_done(num_threads);

    for(size_t i=0; i<num_threads; i++) {
        size_t idx1 = active_objs.size() * i / (double)num_threads;
        size_t idx2 = active_objs.size() * (i+1) / (double)num_threads;
        tasks[i] = [num_threads, i, idx1, idx2, this, &collisions]
                    {
                        for(size_t j=idx1; j<idx2; j++)
                            find_and_add_collisions_gt(&collisions[i], active_objs[j]);
                    };
    }
    //the multithreaded version ~160us on Test2(40, 40)
    for(size_t i=1; i<tasks.size(); i++)
        is_done[i] = thread_pool->add_task(tasks[i]);
    tasks[0]();

    for(size_t i=1; i<collisions.size(); i++)
        is_done[i].get();
    for(size_t i=1; i<collisions.size(); i++)
        collisions[0].insert(collisions[0].end(), collisions[i].begin(), collisions[i].end());

    return collisions[0];

    /*
    //single threaded version
    //tprobably takes ~500us on Test2(40, 40) (haven't benchmarked most recent version)
    std::vector<CEng1Collision> collisions;
    for(size_t i=0; i<active_objs.size(); i++) {
        find_and_add_collisions_gt(&collisions, active_objs[i]);
    }
    return collisions;
    */
}
void CollisionEngine1::update_intent_after_collision(int idx,
                                                     MoveIntent prev_intent,
                                                     int other_idx,
                                                     MoveIntent other_prev_intent,
                                                     std::vector<CEng1Collision> *add_to)
{
    //if the intent didn't change, do nothing
    auto new_intent = (*ceng_data)[idx].get_move_intent();
    k_expects(new_intent != MoveIntent::NotSet);
    if(new_intent == prev_intent)
        return;

    if(new_intent == MoveIntent::StayAtCurrentPos) {
        if(prev_intent == MoveIntent::GoToDesiredPos)
            remove_des_from_grid(idx);
        else
            k_assert(false);
        add_cur_to_grid(idx, add_to);
    } else if(new_intent == MoveIntent::RemoveShapes) {
        if(prev_intent == MoveIntent::GoToDesiredPos)
            remove_des_from_grid(idx);
        else if(prev_intent == MoveIntent::StayAtCurrentPos)
            remove_cur_from_grid(idx);
        else
            k_assert(false);
    } else if(new_intent == MoveIntent::GoToDesiredPosIfOtherDoesntCollide) {
        //Note that GoToDesiredPosIfOtherDoesntCollide is a transient state that is
        //immediately resolved after it is declared, i.e. the only place where this state
        //can legally be set is in handle_collision, and it is converted to either
        //GoToDesiredPos or StayAtCurrentPos by the block below in this function, and
        //this function is called right after handle_collision. This means that you can
        //almost never observe a unit's move intent being GoToDesiredPosIfOtherDoesntCollide,
        //and in particular, it's impossible for prev_intent to be it.
        //Also note that the only previous legal state is GoToDesiredPos (it doesn't make
        //sense to have any other previous state, and it could cause bugs unless the block
        //below is changed to support it).
        k_expects(prev_intent == MoveIntent::GoToDesiredPos);
        if(other_prev_intent == MoveIntent::StayAtCurrentPos) {
            //the other shape hasn't moved, so we have to move back
            (*ceng_data)[idx].set_move_intent(MoveIntent::StayAtCurrentPos);
            remove_des_from_grid(idx);
            add_cur_to_grid(idx, add_to);
        } else if(other_prev_intent == MoveIntent::GoToDesiredPos) {
            auto other_new_intent = (*ceng_data)[other_idx].get_move_intent();

            if(other_new_intent == MoveIntent::StayAtCurrentPos ||
               other_new_intent == MoveIntent::GoToDesiredPosIfOtherDoesntCollide)
            {
                if(!des_cur_has_collision(idx, other_idx))
                    (*ceng_data)[idx].set_move_intent(MoveIntent::GoToDesiredPos);
                else {
                    (*ceng_data)[idx].set_move_intent(MoveIntent::StayAtCurrentPos);
                    remove_des_from_grid(idx);
                    add_cur_to_grid(idx, add_to);
                }
            } else if(other_new_intent == MoveIntent::GoToDesiredPos) {
                (*ceng_data)[idx].set_move_intent(MoveIntent::StayAtCurrentPos);
                remove_des_from_grid(idx);
                add_cur_to_grid(idx, add_to);
            } else if(other_new_intent == MoveIntent::RemoveShapes) {
                (*ceng_data)[idx].set_move_intent(MoveIntent::GoToDesiredPos);
            } else
                k_assert(false);
        } else //shouldn't be able to happen
            k_assert(false);
    } else //we received a bogus move intent or an illegal change (e.g. prev intent=cur, new intent=des)
        k_assert(false);
}

}
