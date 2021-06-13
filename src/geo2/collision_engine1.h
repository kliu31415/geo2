#pragma once

#include "geo2/geometry.h"

#include "kx/fixed_size_array.h"

#include "span_lite.h"
#include <type_traits>
#include <cstdint>
#include <memory>
#include <array>
#include <vector>

namespace geo2 {

struct CollisionEng1Obj
{
    const Shape *shape;
    int idx; //index of owner
    uint16_t shape_id;
    bool is_des;

    CollisionEng1Obj() {}
    CollisionEng1Obj(const Shape *shape_, int idx_, uint16_t shape_id_, bool is_des_):
        shape(shape_),
        idx(idx_),
        shape_id(shape_id_),
        is_des(is_des_)
    {}
};

struct CEng1Collision
{
    int idx1;
    int idx2;
    void swap()
    {
        std::swap(idx1, idx2);
    }
};

class CollisionEngine1
{
    struct CollisionEng1ObjAndAABB: public CollisionEng1Obj
    {
        AABB aabb;
        CollisionEng1ObjAndAABB(CollisionEng1Obj obj, AABB aabb_):
            CollisionEng1Obj(obj),
            aabb(aabb_)
        {}
    };

    //fastish spatial partition grid
    class Grid
    {
        //class to enable 2D indexing
        class GridCol
        {
            std::vector<CollisionEng1ObjAndAABB> *row_ptr;
        public:
            GridCol(std::vector<CollisionEng1ObjAndAABB> *row_ptr_):
                row_ptr(row_ptr_)
            {}
            inline std::vector<CollisionEng1ObjAndAABB> &operator [](int idx)
            {
                return *(row_ptr + idx);
            }
        };

        std::unique_ptr<std::vector<CollisionEng1ObjAndAABB>[]> vals;
public:
        constexpr static int LEN = 128; //power of 2 is faster cuz mult turns into bitshift

        Grid():
            vals(std::make_unique<std::vector<CollisionEng1ObjAndAABB>[]>(LEN*LEN))
        {}
        inline GridCol operator [](int idx)
        {
            return GridCol(vals.get() + LEN*idx);
        }
    };

    kx::FixedSizeArray<const Collidable*> map_objs;
    std::function<bool(const Collidable&, const Collidable&)> collision_could_matter;
    std::vector<CollisionEng1Obj> cur;
    std::vector<CollisionEng1Obj> des;
    kx::FixedSizeArray<MoveIntent> move_intent;
    Grid grid;
    kx::FixedSizeArray<AABB> cur_AABB;
    kx::FixedSizeArray<AABB> des_AABB;
    AABB global_AABB;
    float grid_rect_w;
    float grid_rect_h;
    float max_AABB_w;
    float max_AABB_h;

    int x_to_grid_x(float x);
    int y_to_grid_y(float y);
    void remove_obj_from_grid(std::vector<CollisionEng1Obj> &obj_vec, int idx);
    void add_obj_to_grid(std::vector<CollisionEng1Obj> &obj_vec,
                         int idx,
                         std::vector<CEng1Collision> *collisions);
    void find_and_add_collisions(std::vector<CEng1Collision> *add_to,
                                 const CollisionEng1ObjAndAABB &ceng_obj);
public:
    ///cur_ and des_ must be sorted (for efficiency reasons)
    CollisionEngine1(kx::FixedSizeArray<const Collidable*> &&map_objs_,
                     std::function<bool(const Collidable&, const Collidable&)> collision_could_matter_,
                     std::vector<CollisionEng1Obj> &&cur_,
                     std::vector<CollisionEng1Obj> &&des_,
                     kx::FixedSizeArray<MoveIntent> &&move_intent_);
    /** find_collisions() can only be called once per instance; a second call is UB.
     *  This should make sense, as this simulates advancing the game a fraction of a
     *  tick, and you'd have to update the collision engine with new values if you
     *  wanted to advance the collision engine more
     */
    std::vector<CEng1Collision> find_collisions();
    void update_intent(int idx, MoveIntent intent, std::vector<CEng1Collision> *add_to);
};

}
