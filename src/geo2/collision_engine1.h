#pragma once

#include "geo2/ceng1_collision.h"
#include "geo2/ceng1_data.h"
#include "geo2/map_obj/map_object.h"
#include "geo2/geometry.h"

#include "kx/fixed_size_array.h"

#include "kx/kx_span.h"
#include <type_traits>
#include <cstdint>
#include <memory>
#include <array>
#include <vector>

namespace geo2 {

struct CEng1Obj final
{
    const Polygon *polygon;
    int idx; //index of owner
    uint16_t shape_id;

    CEng1Obj(const Polygon *polygon_, int idx_, uint16_t shape_id_):
        polygon(polygon_),
        idx(idx_),
        shape_id(shape_id_)
    {}

    static bool cmp_idx(const CEng1Obj &a, const CEng1Obj &b)
    {
        return a.idx < b.idx;
    }
};

/** The collision engine is, unsurprisingly, the bottleneck. An issue that arose was
 *  how to deal with Polygon ownership. The simplest solution is to use shared_ptr,
 *  in which case a MapObject and a CEng1Obj can both own a Polygon, but copying
 *  shared_ptr is slow, and shared_ptr needs to be copied every time a Polygon is
 *  put into the grid. It doesn't make much sense for the MapObject to be the owner,
 *  so the CollisionEngine1 will own the Polygons.
 */

class CollisionEngine1
{
    constexpr static int GRID_LEN = 128; //power of 2 is faster cuz mult turns into bitshift

    //fastish spatial partition grid
    template<class T> class Grid
    {
        std::array<std::vector<T>, GRID_LEN * GRID_LEN> vals;
    public:
        void reset()
        {
            for(auto &cell: vals)
                cell.clear();
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
            auto &grid_xy = get_ref(x, y);
            for(size_t i=0; i<grid_xy.size(); i++) {
                if(grid_xy[i].idx == idx) {
                    grid_xy[i] = grid_xy.back();
                    grid_xy.pop_back();
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
        void init_with_max_sizes(kx::kx_span<int> max_sizes)
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

    std::vector<CEng1Obj> active_objs;

    std::shared_ptr<class ThreadPool> thread_pool;

    const std::vector<std::shared_ptr<map_obj::MapObject>> *map_objs;
    std::function<bool(const map_obj::MapObject&, const map_obj::MapObject&)> collision_could_matter;

    //afaik, the only way we modify ceng_data is calling set_move_intent in update_intent_after_collision
    std::vector<CEng1Data> *ceng_data;
    Grid<CEng1Obj> grid;

    AABB global_AABB;
    float grid_rect_w;
    float grid_rect_h;
    float max_AABB_w;
    float max_AABB_h;

    int x_to_grid_x(float x) const;
    int y_to_grid_y(float y) const;
    void remove_cur_from_grid(int idx);
    void remove_des_from_grid(int idx);
    void add_cur_to_grid(int idx, std::vector<CEng1Collision> *collisions);
    void add_des_to_grid(int idx, std::vector<CEng1Collision> *collisions);
    void add_obj_to_grid(std::vector<CEng1Obj> &obj_vec,
                         int idx,
                         std::vector<CEng1Collision> *collisions);
    void find_and_add_collisions_neq(std::vector<CEng1Collision> *add_to,
                                     const CEng1Obj &ceng_obj) const;
    void find_and_add_collisions_gt(std::vector<CEng1Collision> *add_to,
                                    const CEng1Obj &ceng_obj) const;
    bool des_cur_has_collision(int idx1, int idx2) const;
public:
    CollisionEngine1(std::shared_ptr<ThreadPool> thread_pool_);

    void reset();
    ///cur_ and des_ must be sorted (for efficiency reasons)
    void set_ceng_data(std::vector<CEng1Data> *data);
    void set2(const std::vector<std::shared_ptr<map_obj::MapObject>> *map_objs_,
              std::function<bool(const map_obj::MapObject&,
                               const map_obj::MapObject&)> collision_could_matter_);
    std::vector<CEng1Collision> find_collisions();
    ///called only after a collision happens
    void update_intent_after_collision(int idx, MoveIntent prev_intent,
                                       int other_idx, MoveIntent other_prev_intent,
                                       std::vector<CEng1Collision> *add_to);
};
}
