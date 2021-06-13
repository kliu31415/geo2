#include "geo2/collision_engine1.h"

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
int CollisionEngine1::x_to_grid_x(float x)
{
    int x_ = std::min((int)((x - global_AABB.x1) / grid_rect_w), Grid::LEN - 1);
    return x_;
}
int CollisionEngine1::y_to_grid_y(float y)
{
    int y_ = std::min((int)((y - global_AABB.y1) / grid_rect_h), Grid::LEN - 1);
    return y_;
}
void CollisionEngine1::remove_obj_from_grid(std::vector<CollisionEng1Obj> &obj_vec, int idx)
{
    //note that idx doesn't mean obj_vec[idx]
    auto ptr = std::lower_bound(obj_vec.begin(), obj_vec.end(), idx, cmp_idx_int);
    k_expects(ptr->idx==idx); //there should be at least one shape
    while(ptr<obj_vec.end() && ptr->idx==idx) {
        auto aabb = ptr->shape->get_AABB();
        int x = x_to_grid_x(0.5f * (aabb.x1 + aabb.x2));
        int y = y_to_grid_y(0.5f * (aabb.y1 + aabb.y2));
        bool found = false;
        for(size_t i=0; i<grid[x][y].size(); i++) {
            if(grid[x][y][i].idx == idx) {
                grid[x][y][i] = grid[x][y].back();
                grid[x][y].pop_back();
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
        auto aabb = ptr->shape->get_AABB();
        int x = x_to_grid_x(0.5f * (aabb.x1 + aabb.x2));
        int y = y_to_grid_y(0.5f * (aabb.y1 + aabb.y2));
        CollisionEng1ObjAndAABB ceng_and_aabb(*ptr, aabb);
        find_and_add_collisions(collisions, ceng_and_aabb);
        grid[x][y].push_back(ceng_and_aabb);
        ptr++;
    }
}
void CollisionEngine1::find_and_add_collisions(std::vector<CEng1Collision> *add_to,
                                               const CollisionEng1ObjAndAABB &ceng_obj)
{

    int x1 = x_to_grid_x(ceng_obj.aabb.x1 - 0.5f*max_AABB_w);
    int x2 = x_to_grid_x(ceng_obj.aabb.x2 + 0.5f*max_AABB_w);
    int y1 = y_to_grid_y(ceng_obj.aabb.y1 - 0.5f*max_AABB_h);
    int y2 = y_to_grid_y(ceng_obj.aabb.y2 + 0.5f*max_AABB_h);
    x1 = std::clamp(x1, 0, Grid::LEN - 1);
    x2 = std::clamp(x2, 0, Grid::LEN - 1);
    y1 = std::clamp(y1, 0, Grid::LEN - 1);
    y2 = std::clamp(y2, 0, Grid::LEN - 1);

    for(int x=x1; x<=x2; x++) {
        for(int y=y1; y<=y2; y++) {
            for(const auto &other: grid[x][y]) {
                //filter out a potential collision if:
                //-the .idx (owner) is the same
                //-the AABBs don't overlap
                //-the collision wouldn't matter anyway
                if(ceng_obj.idx!=other.idx &&
                   ceng_obj.aabb.overlaps(other.aabb) &&
                   collision_could_matter(*map_objs[ceng_obj.idx], *map_objs[other.idx]))
                {
                    if(ceng_obj.shape->has_collision(*other.shape)) {
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

CollisionEngine1::CollisionEngine1(kx::FixedSizeArray<const Collidable*> &&map_objs_,
                    std::function<bool(const Collidable&, const Collidable&)> collision_could_matter_,
                                   std::vector<CollisionEng1Obj> &&cur_,
                                   std::vector<CollisionEng1Obj> &&des_,
                                   kx::FixedSizeArray<MoveIntent> &&move_intent_):
    map_objs(map_objs_),
    collision_could_matter(collision_could_matter_),
    cur(cur_),
    des(des_),
    move_intent(move_intent_),
    grid()
{
    k_expects(std::is_sorted(cur.begin(), cur.end(), cmp_idx));
    k_expects(std::is_sorted(des.begin(), des.end(), cmp_idx));
    k_expects(map_objs.size() == move_intent.size());
}

std::vector<CEng1Collision> CollisionEngine1::find_collisions()
{
    //find the grid AABBs. The AABBs are integral with dimensions in the range [0, Grid::LEN).
    auto get_ceng_obj_AABB = [](const CollisionEng1Obj &c) -> AABB
                                 {return c.shape->get_AABB();};
    cur_AABB = decltype(cur_AABB)(cur.size());
    std::transform(cur.begin(),
                   cur.end(),
                   cur_AABB.begin(),
                   get_ceng_obj_AABB);

    des_AABB = decltype(des_AABB)(des.size());
    std::transform(des.begin(),
                   des.end(),
                   des_AABB.begin(),
                   get_ceng_obj_AABB);

    auto get_AABB_w = [](const AABB &rect) -> float
                        {
                            return rect.x2 - rect.x1;
                        };

    auto get_AABB_h = [](const AABB &rect) -> float
                        {
                            return rect.y2 - rect.y1;
                        };

    auto get_max_float = [](float a, float b) -> float
                            {
                                return a > b? a: b;
                            };


    max_AABB_w = std::transform_reduce(cur_AABB.begin(),
                                       cur_AABB.end(),
                                       0.0f,
                                       get_max_float,
                                       get_AABB_w);

    max_AABB_w = std::transform_reduce(des_AABB.begin(),
                                       des_AABB.end(),
                                       max_AABB_w,
                                       get_max_float,
                                       get_AABB_w);

    max_AABB_h = std::transform_reduce(cur_AABB.begin(),
                                       cur_AABB.end(),
                                       0.0f,
                                       get_max_float,
                                       get_AABB_h);

    max_AABB_h = std::transform_reduce(des_AABB.begin(),
                                       des_AABB.end(),
                                       max_AABB_h,
                                       get_max_float,
                                       get_AABB_h);


     auto combine_AABB = [](const AABB &a, const AABB &b) -> AABB
                            {AABB ret = a; ret.combine(b); return ret;};

     global_AABB = std::reduce(cur_AABB.begin(),
                               cur_AABB.end(),
                               AABB(),
                               combine_AABB);

     global_AABB = std::reduce(des_AABB.begin(),
                               des_AABB.end(),
                               global_AABB,
                               combine_AABB);


    grid_rect_w = (global_AABB.x2 - global_AABB.x1) / Grid::LEN;
    grid_rect_h = (global_AABB.y2 - global_AABB.y1) / Grid::LEN;

    //put things in their grid rects
    size_t cur_idx = 0;
    size_t des_idx = 0;

    std::vector<CollisionEng1ObjAndAABB> active_objs;

    auto add_active_obj = [&]
         (int i,
          decltype(cur) *objs_,
          size_t *obj_idx,
          const decltype(cur_AABB) &AABBs)
            -> void
          {
              while(*obj_idx < objs_->size() && (*objs_)[*obj_idx].idx<=i) {
                  if((*objs_)[*obj_idx].idx==i) {
                      active_objs.emplace_back((*objs_)[*obj_idx], AABBs[*obj_idx]);

                      auto mx = 0.5f * (AABBs[*obj_idx].x1 + AABBs[*obj_idx].x2);
                      auto my = 0.5f * (AABBs[*obj_idx].y1 + AABBs[*obj_idx].y2);

                      int x = x_to_grid_x(mx);
                      int y = y_to_grid_y(my);

                      grid[x][y].emplace_back((*objs_)[*obj_idx], AABBs[*obj_idx]);
                  }
                  (*obj_idx)++;
              }
          };

    for(size_t i=0; i<move_intent.size(); i++) {
        if(move_intent[i] == MoveIntent::GoToDesiredPos)
            add_active_obj(i, &des, &des_idx, des_AABB);
        else if(move_intent[i] == MoveIntent::StayAtCurrentPos)
            add_active_obj(i, &cur, &cur_idx, cur_AABB);
    }

    std::vector<CEng1Collision> collisions;

    //find collisions
    for(size_t i=0; i<active_objs.size(); i++) {
        find_and_add_collisions(&collisions, active_objs[i]);
    }

    return collisions;
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

}
