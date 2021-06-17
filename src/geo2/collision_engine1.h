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
    const Polygon *polygon;
    int idx; //index of owner
    uint16_t shape_id;
    bool is_des;

    CollisionEng1Obj()
    {}
    CollisionEng1Obj(const Polygon *polygon_, int idx_, uint16_t shape_id_, bool is_des_):
        polygon(polygon_),
        idx(idx_),
        shape_id(shape_id_),
        is_des(is_des_)
    {}
    static bool cmp_idx(const CollisionEng1Obj &a, const CollisionEng1Obj &b)
    {
        return a.idx < b.idx;
    }
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
    constexpr static int GRID_LEN = 128; //power of 2 is faster cuz mult turns into bitshift

    //fastish spatial partition grid
    template<class T> class Grid
    {
        std::unique_ptr<std::vector<T>[]> vals;
    public:
        Grid():
            vals(std::make_unique<std::vector<T>[]>(GRID_LEN*GRID_LEN))
        {}
        void reset()
        {
            for(int i=0; i<GRID_LEN; i++)
                for(int j=0; j<GRID_LEN; j++)
                    get_ref(i, j).clear();
        }
        inline std::vector<T>& get_ref(int a, int b)
        {
            return vals[a*GRID_LEN + b];
        }
        inline const std::vector<T>& get_const_ref(int a, int b) const
        {
            return vals[a*GRID_LEN + b];
        }
        inline void remove_one_with_idx(int x, int y, int idx)
        {
            //remember to preserve ordering of indexes!
            auto &grid_xy = get_ref(x, y);
            for(size_t i=0; i<grid_xy.size(); i++) {
                if(grid_xy[i].idx == idx) {
                    grid_xy.erase(grid_xy.begin() + i);
                    return;
                }
            }
            //no matches found!
            k_assert(false);
        }
    };

    /*
    template<class T> class Grid
    {
        kx::FixedSizeArray<T> vals;
        std::array<int, GRID_LEN*GRID_LEN> offset;
        std::array<int, GRID_LEN*GRID_LEN> cur_size;
    public:
        Grid()
        {}
        void init_with_max_sizes(nonstd::span<int> max_sizes)
        {
            std::fill(std::begin(cur_size), std::end(cur_size), 0);
            offset[0] = 0;
            for(int i=1; i<GRID_LEN*GRID_LEN; i++)
                offset[i] = offset[i-1] + max_sizes[i-1];
            vals = decltype(vals)(max_sizes[GRID_LEN*GRID_LEN-1] + offset[GRID_LEN*GRID_LEN-1]);
        }
        inline std::vector<T>& get_ref(int a, int b)
        {
            return vals[a*GRID_LEN + b];
        }
    }
    */

    std::shared_ptr<class ThreadPool> thread_pool;

    kx::FixedSizeArray<const Collidable*> map_objs;
    std::function<bool(const Collidable&, const Collidable&)> collision_could_matter;
    std::vector<CollisionEng1Obj> cur;
    std::vector<CollisionEng1Obj> des;
    std::vector<CollisionEng1Obj> active_objs;
    kx::FixedSizeArray<MoveIntent> move_intent;
    Grid<CollisionEng1Obj> grid;

    AABB global_AABB;
    float grid_rect_w;
    float grid_rect_h;
    float max_AABB_w;
    float max_AABB_h;

    int x_to_grid_x(float x) const;
    int y_to_grid_y(float y) const;
    void remove_obj_from_grid(std::vector<CollisionEng1Obj> &obj_vec, int idx);
    void add_obj_to_grid(std::vector<CollisionEng1Obj> &obj_vec,
                         int idx,
                         std::vector<CEng1Collision> *collisions);
    void find_and_add_collisions_neq(std::vector<CEng1Collision> *add_to,
                                     const CollisionEng1Obj &ceng_obj) const;
    void find_and_add_collisions_gt(std::vector<CEng1Collision> *add_to,
                                    const CollisionEng1Obj &ceng_obj) const;
public:
    CollisionEngine1(std::shared_ptr<ThreadPool> thread_pool_);
    void reset();
    ///cur_ and des_ must be sorted (for efficiency reasons)
    void set_cur_des(std::vector<CollisionEng1Obj> &&cur_,
                     std::vector<CollisionEng1Obj> &&des_);
    void set2(kx::FixedSizeArray<const Collidable*> &&map_objs_,
              std::function<bool(const Collidable&, const Collidable&)> collision_could_matter_,
              kx::FixedSizeArray<MoveIntent> &&move_intent_);
    void precompute(); ///depends on set_cur_des, but not set2
    std::vector<CEng1Collision> find_collisions();
    void update_intent(int idx, MoveIntent intent, std::vector<CEng1Collision> *add_to);
    void steal_cur_into(std::vector<CollisionEng1Obj> *vec);
    void steal_des_into(std::vector<CollisionEng1Obj> *vec);
};
}
