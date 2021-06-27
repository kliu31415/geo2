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
                   aabb.overlaps(other.polygon->get_AABB()) &&
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
CollisionEngine1::CollisionEngine1(std::shared_ptr<ThreadPool> thread_pool_):
    thread_pool(std::move(thread_pool_))
{}
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
void CollisionEngine1::precompute()
{
    auto get_AABB_dim = [this](const Polygon *polygon, int) -> void
                                {
                                    const auto &aabb = polygon->get_AABB();
                                    max_AABB_w = std::max(max_AABB_w, aabb.x2 - aabb.x1);
                                    max_AABB_h = std::max(max_AABB_h, aabb.y2 - aabb.y1);
                                };

    max_AABB_w = 0.0f;
    max_AABB_h = 0.0f;
    for(auto &cdata: *ceng_data)
        cdata.for_each(get_AABB_dim);

    //we grid things based on their top left corner, so calculate the global
    //AABB based on top left corners only.
    global_AABB = AABB::make_maxbad_AABB();

    auto calc_global_AABB = [this](const Polygon *polygon, int) -> void
    {
        global_AABB.x1 = std::min(global_AABB.x1, polygon->get_AABB().x1);
        global_AABB.x2 = std::max(global_AABB.x2, polygon->get_AABB().x1);
        global_AABB.y1 = std::min(global_AABB.y1, polygon->get_AABB().y1);
        global_AABB.y2 = std::max(global_AABB.y2, polygon->get_AABB().y1);
    };
    for(auto &cdata: *ceng_data)
        cdata.for_each(calc_global_AABB);

    grid_rect_w = (global_AABB.x2 - global_AABB.x1) / GRID_LEN;
    grid_rect_h = (global_AABB.y2 - global_AABB.y1) / GRID_LEN;
}
std::vector<CEng1Collision> CollisionEngine1::find_collisions()
{
    active_objs.clear();

    //~300-350us on Test2(40, 40), which adding to the grid takes ~200us
    for(size_t i=0; i<ceng_data->size(); i++) {
        auto add_active_obj = [this, i](const Polygon *polygon, int shape_idx) -> void
                            {
                                const auto &aabb = polygon->get_AABB();

                                int x = x_to_grid_x(aabb.x1);
                                int y = y_to_grid_y(aabb.y1);

                                CEng1Obj obj(polygon, i, shape_idx);

                                active_objs.push_back(obj);
                                grid.get_ref(x, y).push_back(obj);
                            };

        auto move_intent = (*ceng_data)[i].get_move_intent();
        if(move_intent == MoveIntent::StayAtCurrentPos)
            (*ceng_data)[i].for_each_cur(add_active_obj);
        else if(move_intent == MoveIntent::GoToDesiredPos)
            (*ceng_data)[i].for_each_des(add_active_obj);
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
        size_t idx1 = ceng_data->size() * std::pow(i / (double)num_threads, 0.75);
        size_t idx2 = ceng_data->size() * std::pow((i+1) / (double)num_threads, 0.75);
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
void CollisionEngine1::update_intent(int idx, MoveIntent prev_intent, std::vector<CEng1Collision> *add_to)
{
    //if the intent didn't change, do nothing
    auto new_intent = (*ceng_data)[idx].get_move_intent();
    if(new_intent == prev_intent)
        return;

    if(new_intent == MoveIntent::StayAtCurrentPos) {
        if(prev_intent == MoveIntent::GoToDesiredPos)
            remove_des_from_grid(idx);
        else
            k_assert(false);

        add_cur_to_grid(idx, add_to);
    } else if(new_intent == MoveIntent::Delete) {
        if(prev_intent == MoveIntent::GoToDesiredPos)
            remove_des_from_grid(idx);
        else if(prev_intent == MoveIntent::StayAtCurrentPos)
            remove_cur_from_grid(idx);
        else
            k_assert(false);

    } else //this shouldn't happen
        k_assert(false);

    (*ceng_data)[idx].set_move_intent(new_intent);
}

}
