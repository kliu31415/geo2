#include "geo2/collision_engine1.h"
#include "geo2/multithread/thread_pool.h"

#include "geo2/timer.h"

#include <cmath>
#include <algorithm>
#include <execution>

namespace geo2 {

bool cmp_idx_int(const CollisionEng1Obj &a, int b)
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
//note: obj_vec is either cur or des
void CollisionEngine1::remove_obj_from_grid(std::vector<CollisionEng1Obj> &obj_vec, int idx)
{
    //note that idx doesn't mean obj_vec[idx]
    auto ptr = std::lower_bound(obj_vec.begin(), obj_vec.end(), idx, cmp_idx_int);
    k_expects(ptr->idx==idx); //there should be at least one shape
    while(ptr<obj_vec.end() && ptr->idx==idx) {
        auto aabb = ptr->polygon->get_AABB();
        int x = x_to_grid_x(aabb.x1);
        int y = y_to_grid_y(aabb.y1);
        grid.remove_one_with_idx(x, y, idx);
        ptr++;
    }
}
//note: obj_vec is either cur or des
void CollisionEngine1::add_obj_to_grid(std::vector<CollisionEng1Obj> &obj_vec,
                                       int idx,
                                       std::vector<CEng1Collision> *collisions)
{
    //note that idx doesn't mean obj_vec[idx]
    auto ptr = std::lower_bound(obj_vec.begin(), obj_vec.end(), idx, cmp_idx_int);
    k_expects(ptr->idx==idx); //there should be at least one shape
    while(ptr<obj_vec.end() && ptr->idx==idx) {
        auto aabb = ptr->polygon->get_AABB();
        int x = x_to_grid_x(aabb.x1);
        int y = y_to_grid_y(aabb.y1);
        find_and_add_collisions_neq(collisions, *ptr);
        grid.get_ref(x, y).push_back(*ptr);
        ptr++;
    }
}
void CollisionEngine1::find_and_add_collisions_neq(std::vector<CEng1Collision> *add_to,
                                                   const CollisionEng1Obj &ceng_obj) const
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
                   aabb.overlaps(other.polygon->get_AABB()) &&
                   collision_could_matter(*map_objs[ceng_obj.idx], *map_objs[other.idx]))
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
                                                  const CollisionEng1Obj &ceng_obj) const
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

                //note that we assume the grid cells are ordered, so we can break
                //and move on to the next cell if our idx isn't greater than the other's idx
                if(ceng_obj.idx <= other.idx)
                    break;
                if(aabb.overlaps(other.polygon->get_AABB()) &&
                   collision_could_matter(*map_objs[ceng_obj.idx], *map_objs[other.idx]))
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
CollisionEngine1::CollisionEngine1(std::shared_ptr<ThreadPool> thread_pool_):
    thread_pool(std::move(thread_pool_))
{}
void CollisionEngine1::reset()
{
    grid.reset();
}
void CollisionEngine1::set_cur_des(std::vector<CollisionEng1Obj> &&cur_,
                                   std::vector<CollisionEng1Obj> &&des_)
{
    cur = std::move(cur_);
    des = std::move(des_);
    k_expects(std::is_sorted(cur.begin(), cur.end(), CollisionEng1Obj::cmp_idx));
    k_expects(std::is_sorted(des.begin(), des.end(), CollisionEng1Obj::cmp_idx));
}
void CollisionEngine1::set2(kx::FixedSizeArray<const Collidable*> &&map_objs_,
                            std::function<bool(const Collidable&, const Collidable&)> collision_could_matter_,
                            kx::FixedSizeArray<MoveIntent> &&move_intent_)
{
    map_objs = std::move(map_objs_);
    collision_could_matter = std::move(collision_could_matter_);
    move_intent = std::move(move_intent_);
    grid.reset();

    k_expects(map_objs.size() == move_intent.size());
}
void CollisionEngine1::precompute()
{
    auto get_AABB_w = [](const CollisionEng1Obj &obj) -> float
                        {
                            const auto &aabb = obj.polygon->get_AABB();
                            return aabb.x2 - aabb.x1;
                        };

    auto get_AABB_h = [](const CollisionEng1Obj &obj) -> float
                        {
                            const auto &aabb = obj.polygon->get_AABB();
                            return aabb.y2 - aabb.y1;
                        };

    auto get_max_float = [](float a, float b) -> float
                            {
                                return a > b? a: b;
                            };

    max_AABB_w = std::transform_reduce(cur.begin(),
                                       cur.end(),
                                       0.0f,
                                       get_max_float,
                                       get_AABB_w);

    max_AABB_w = std::transform_reduce(des.begin(),
                                       des.end(),
                                       max_AABB_w,
                                       get_max_float,
                                       get_AABB_w);

    max_AABB_h = std::transform_reduce(cur.begin(),
                                       cur.end(),
                                       0.0f,
                                       get_max_float,
                                       get_AABB_h);

    max_AABB_h = std::transform_reduce(des.begin(),
                                       des.end(),
                                       max_AABB_h,
                                       get_max_float,
                                       get_AABB_h);

    //we grid things based on their top left corner, so calculate the global
    //AABB based on top left corners only.
    global_AABB = AABB::make_maxbad_AABB();

    auto calc_global_AABB = [this](const CollisionEng1Obj &obj) -> void
    {
        global_AABB.x1 = std::min(global_AABB.x1, obj.polygon->get_AABB().x1);
        global_AABB.x2 = std::max(global_AABB.x2, obj.polygon->get_AABB().x1);
        global_AABB.y1 = std::min(global_AABB.y1, obj.polygon->get_AABB().y1);
        global_AABB.y2 = std::max(global_AABB.y2, obj.polygon->get_AABB().y1);
    };
    std::for_each(cur.begin(), cur.end(), calc_global_AABB);
    std::for_each(des.begin(), des.end(), calc_global_AABB);

    grid_rect_w = (global_AABB.x2 - global_AABB.x1) / GRID_LEN;
    grid_rect_h = (global_AABB.y2 - global_AABB.y1) / GRID_LEN;
}
std::vector<CEng1Collision> CollisionEngine1::find_collisions()
{
    //put things in their grid rects
    size_t cur_idx = 0;
    size_t des_idx = 0;

    active_objs.clear();

    auto add_active_obj = [this]
         (std::vector<CollisionEng1Obj> *active_objs_,
          int i,
          decltype(cur) __restrict *objs_,
          size_t __restrict *obj_idx)
            -> void
          {
              while(*obj_idx < objs_->size() && (*objs_)[*obj_idx].idx<=i) {
                  if((*objs_)[*obj_idx].idx==i) {
                      const auto &obj_ref = (*objs_)[*obj_idx];
                      const auto &aabb = obj_ref.polygon->get_AABB();
                      active_objs_->push_back(obj_ref);

                      int x = x_to_grid_x(aabb.x1);
                      int y = y_to_grid_y(aabb.y1);

                      grid.get_ref(x, y).push_back(obj_ref);
                  }
                  (*obj_idx)++;
              }
          };

    //~300-350us on Test2(40, 40), which adding to the grid takes ~200us
    for(size_t i=0; i<move_intent.size(); i++) {
        if(move_intent[i] == MoveIntent::GoToDesiredPos)
            add_active_obj(&active_objs, i, &des, &des_idx);
        else if(move_intent[i] == MoveIntent::StayAtCurrentPos)
            add_active_obj(&active_objs, i, &cur, &cur_idx);
    }

    size_t num_threads = 1 + thread_pool->size();

    std::vector<std::vector<CEng1Collision>> collisions(num_threads);
    std::vector<std::function<void()>> tasks(num_threads);
    std::vector<std::future<void>> is_done(num_threads);

    //note that processing an object has a complexity roughly linearly proportional
    //to its index because (a, b) is checked for overlap only if a.idx < b.idx, so
    //later objects require more work to check. Therefore, not every task should
    //consist of the same number of objects. Empirically, using a power of ~0.75
    //results in the best runtime.
    for(size_t i=0; i<num_threads; i++) {
        size_t idx1 = active_objs.size() * std::pow(i / (double)num_threads, 0.75);
        size_t idx2 = active_objs.size() * std::pow((i+1) / (double)num_threads, 0.75);
        tasks[i] = [num_threads, i, idx1, idx2, this, &collisions]
                    {
                        for(size_t j=idx1; j<idx2; j++)
                            find_and_add_collisions_gt(&collisions[i], active_objs[j]);
                    };
    }
    //the multithreaded version ~200-250us on Test2(40, 40)
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
    //takes ~700-800us on Test2(40, 40)
    std::vector<CEng1Collision> collisions;
    for(size_t i=0; i<active_objs.size(); i++) {
        find_and_add_collisions_gt(&collisions, active_objs[i]);
    }
    return collisions;
    */
}
void CollisionEngine1::update_intent(int idx, MoveIntent intent, std::vector<CEng1Collision> *add_to)
{
    k_expects(intent != MoveIntent::NotSet);

    //if the intent didn't change, do nothing
    if(intent == move_intent[idx])
        return;

    if(intent == MoveIntent::StayAtCurrentPos) {
        if(move_intent[idx] == MoveIntent::GoToDesiredPos)
            remove_obj_from_grid(des, idx);
        else
            k_assert(false);

        add_obj_to_grid(cur, idx, add_to);

    } else if(intent == MoveIntent::Delete) {
        if(move_intent[idx] == MoveIntent::GoToDesiredPos)
            remove_obj_from_grid(des, idx);
        else if(move_intent[idx] == MoveIntent::StayAtCurrentPos)
            remove_obj_from_grid(cur, idx);
        else
            k_assert(false);

    } else //this shouldn't happen
        k_assert(false);

    move_intent[idx] = intent;
}
void CollisionEngine1::steal_cur_into(std::vector<CollisionEng1Obj> *vec)
{
    *vec = std::move(cur);
}
void CollisionEngine1::steal_des_into(std::vector<CollisionEng1Obj> *vec)
{
    *vec = std::move(des);
}

}
