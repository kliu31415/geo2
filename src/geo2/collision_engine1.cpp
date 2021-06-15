#include "geo2/collision_engine1.h"
#include "geo2/multithread/thread_pool.h"

#include "geo2/timer.h"

#include <cmath>
#include <algorithm>
#include <execution>

namespace geo2 {

bool cmp_idx(const CollisionEng1Obj &a, const CollisionEng1Obj &b)
{
    return a.idx < b.idx;
}
bool cmp_idx_int(const CollisionEng1Obj &a, int b)
{
    return a.idx < b;
}
int CollisionEngine1::x_to_grid_x(float x) const
{
    int x_ = std::min((int)((x - global_AABB.x1) / grid_rect_w), GRID_LEN - 1);
    return x_;
}
int CollisionEngine1::y_to_grid_y(float y) const
{
    int y_ = std::min((int)((y - global_AABB.y1) / grid_rect_h), GRID_LEN - 1);
    return y_;
}
void CollisionEngine1::remove_obj_from_grid(std::vector<CollisionEng1Obj> &obj_vec, int idx)
{
    //note that idx doesn't mean obj_vec[idx]
    auto ptr = std::lower_bound(obj_vec.begin(), obj_vec.end(), idx, cmp_idx_int);
    k_expects(ptr->idx==idx); //there should be at least one shape
    while(ptr<obj_vec.end() && ptr->idx==idx) {
        auto aabb = ptr->polygon->get_AABB();
        int x = x_to_grid_x(0.5f * (aabb.x1 + aabb.x2));
        int y = y_to_grid_y(0.5f * (aabb.y1 + aabb.y2));
        bool found = false;
        auto &grid_xy = grid.get_ref(x, y);
        for(size_t i=0; i<grid_xy.size(); i++) {
            if(grid_xy[i].idx == idx) {
                grid_xy[i] = grid_xy.back();
                grid_xy.pop_back();
                found = true;
                break;
            }
        }
        k_ensures(found);
        ptr++;
    }
}
void CollisionEngine1::add_obj_to_grid(std::vector<CollisionEng1Obj> &obj_vec,
                                       int idx,
                                       std::vector<CEng1Collision> *collisions)
{
    //note that idx doesn't mean obj_vec[idx]
    auto ptr = std::lower_bound(obj_vec.begin(), obj_vec.end(), idx, cmp_idx_int);
    k_expects(ptr->idx==idx); //there should be at least one shape
    while(ptr<obj_vec.end() && ptr->idx==idx) {
        auto aabb = ptr->polygon->get_AABB();
        int x = x_to_grid_x(0.5f * (aabb.x1 + aabb.x2));
        int y = y_to_grid_y(0.5f * (aabb.y1 + aabb.y2));
        CollisionEng1ObjAndAABB ceng_and_aabb(*ptr, aabb);
        find_and_add_collisions<std::not_equal_to<>>(collisions, ceng_and_aabb);
        grid.get_ref(x, y).push_back(ceng_and_aabb);
        ptr++;
    }
}
template<class IdxCmp>
void CollisionEngine1::find_and_add_collisions(std::vector<CEng1Collision> *add_to,
                                               const CollisionEng1ObjAndAABB &ceng_obj) const
{

    int x1 = x_to_grid_x(ceng_obj.aabb.x1 - 0.5f*max_AABB_w);
    int x2 = x_to_grid_x(ceng_obj.aabb.x2 + 0.5f*max_AABB_w);
    int y1 = y_to_grid_y(ceng_obj.aabb.y1 - 0.5f*max_AABB_h);
    int y2 = y_to_grid_y(ceng_obj.aabb.y2 + 0.5f*max_AABB_h);
    x1 = std::clamp(x1, 0, GRID_LEN - 1);
    x2 = std::clamp(x2, 0, GRID_LEN - 1);
    y1 = std::clamp(y1, 0, GRID_LEN - 1);
    y2 = std::clamp(y2, 0, GRID_LEN - 1);

    for(int x=x1; x<=x2; x++) {
        for(int y=y1; y<=y2; y++) {
            for(const auto &other: grid.get_const_ref(x, y)) {
                //filter out a potential collision if:
                //-the .idx (owner) is the same
                //-the AABBs don't overlap
                //-the collision wouldn't matter anyway
                if(IdxCmp()(ceng_obj.idx, other.idx) &&
                   ceng_obj.aabb.overlaps(other.aabb) &&
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
void CollisionEngine1::init(kx::FixedSizeArray<const Collidable*> &&map_objs_,
                            std::function<bool(const Collidable&, const Collidable&)> collision_could_matter_,
                            std::vector<CollisionEng1Obj> &&cur_,
                            std::vector<CollisionEng1Obj> &&des_,
                            kx::FixedSizeArray<MoveIntent> &&move_intent_)
{
    map_objs = std::move(map_objs_);
    collision_could_matter = std::move(collision_could_matter_);
    cur = std::move(cur_);
    des = std::move(des_);
    move_intent = std::move(move_intent_);

    for(int i=0; i<GRID_LEN; i++)
        for(int j=0; j<GRID_LEN; j++)
            grid.get_ref(i, j).clear();

    k_expects(std::is_sorted(cur.begin(), cur.end(), cmp_idx));
    k_expects(std::is_sorted(des.begin(), des.end(), cmp_idx));
    k_expects(map_objs.size() == move_intent.size());
}

std::vector<CEng1Collision> CollisionEngine1::find_collisions()
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

    auto combine_AABB = [](const AABB &a, const AABB &b) -> AABB
                            {AABB ret = a; ret.combine(b); return ret;};

    global_AABB = AABB::make_maxbad_AABB();
    for(const auto &obj: cur)
        global_AABB.combine(obj.polygon->get_AABB());
    for(const auto &obj: des)
        global_AABB.combine(obj.polygon->get_AABB());

    grid_rect_w = (global_AABB.x2 - global_AABB.x1) / GRID_LEN;
    grid_rect_h = (global_AABB.y2 - global_AABB.y1) / GRID_LEN;

    //put things in their grid rects
    size_t cur_idx = 0;
    size_t des_idx = 0;

    active_objs.clear();

    auto add_active_obj = [this]
         (std::vector<CollisionEng1ObjAndAABB> *active_objs_,
          int i,
          decltype(cur) __restrict *objs_,
          size_t __restrict *obj_idx)
            -> void
          {
              while(*obj_idx < objs_->size() && (*objs_)[*obj_idx].idx<=i) {
                  if((*objs_)[*obj_idx].idx==i) {
                      const auto &obj_ref = (*objs_)[*obj_idx];
                      const auto &aabb = obj_ref.polygon->get_AABB();
                      active_objs_->emplace_back(obj_ref, aabb);

                      auto mx = 0.5f * (aabb.x1 + aabb.x2);
                      auto my = 0.5f * (aabb.y1 + aabb.y2);

                      int x = x_to_grid_x(mx);
                      int y = y_to_grid_y(my);

                      grid.get_ref(x, y).emplace_back((*objs_)[*obj_idx], aabb);
                  }
                  (*obj_idx)++;
              }
          };

    for(size_t i=0; i<move_intent.size(); i++) {
        if(move_intent[i] == MoveIntent::GoToDesiredPos)
            add_active_obj(&active_objs, i, &des, &des_idx);
        else if(move_intent[i] == MoveIntent::StayAtCurrentPos)
            add_active_obj(&active_objs, i, &cur, &cur_idx);
    }

    size_t num_threads = 1 + thread_pool->size();

    std::vector<std::vector<CEng1Collision>> collisions(num_threads);
    std::vector<std::function<void()>> tasks(num_threads);
    std::vector<std::atomic<bool>> is_done(num_threads);
    for(auto &flag: is_done)
        flag.store(false, std::memory_order_relaxed);

    for(size_t i=0; i<tasks.size(); i++) {
        size_t idx1 = active_objs.size() * i / num_threads;
        size_t idx2 = active_objs.size() * (i+1) / num_threads;
        tasks[i] = [&is_done, i, idx1, idx2, this, &collisions]
                    {
                        for(size_t j=idx1; j<idx2; j++)
                            find_and_add_collisions<std::less<>>(&collisions[i], active_objs[j]);
                        is_done[i].store(true, std::memory_order_release);
                    };
    }

    for(size_t i=1; i<tasks.size(); i++)
        thread_pool->add_task(tasks[i]);
    tasks[0]();
    for(size_t i=1; i<tasks.size(); i++) {
        while(!is_done[i].load(std::memory_order_acquire))
        {}
    }

    for(size_t i=1; i<collisions.size(); i++)
        collisions[0].insert(collisions[0].end(), collisions[i].begin(), collisions[i].end());

    return collisions[0];

    /* //single threaded version
    std::vector<CEng1Collision> collisions;
    for(size_t i=0; i<active_objs.size(); i++) {
        find_and_add_collisions<std::less<>>(&collisions, active_objs[i]);
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
