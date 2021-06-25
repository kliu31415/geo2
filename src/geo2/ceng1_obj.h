#pragma once

#include "geo2/geometry.h"

#include <cstdint>

namespace geo2 {

struct CEng1Obj
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

struct CEng1Collision
{
    int idx1;
    int idx2;
    void swap()
    {
        std::swap(idx1, idx2);
    }
};

}
