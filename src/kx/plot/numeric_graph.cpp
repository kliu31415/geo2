#include "kx/plot/numeric_graph.h"
#include "kx/util.h"
#include "kx/gfx/gfx.h"
#include "kx/debug.h"

#include <SDL2/SDL.h>
#include <algorithm>
#include <cmath>

namespace kx { namespace plot {

Bounds::Bounds(double x1_, double y1_, double x2_, double y2_):
    x1(x1_), y1(y1_), x2(x2_), y2(y2_)
{
    //use <= because a temporary Bounds object can be created
    //from just one point, in which case x1==x2 and y1==y2
    k_expects(x1 <= x2, "make sure coordinates aren't nan or inf");
    k_expects(y1 <= y2, "make sure coordinates aren't nan or inf");
}
std::unique_ptr<Bounds> Bounds::combine(const Bounds *b1, const Bounds *b2)
{
    if(b1 == nullptr) {
        if(b2 == nullptr) {
            return nullptr;
        }
        return std::make_unique<Bounds>(*b2);
    }
    if(b2 == nullptr) {
        return std::make_unique<Bounds>(*b1);
    }
    return std::make_unique<Bounds>(std::min(b1->x1, b2->x1), std::min(b1->y1, b2->y1),
                                    std::max(b1->x2, b2->x2), std::max(b1->y2, b2->y2));
}

Coord::Coord(double x_, double y_):
    x(x_), y(y_)
{}

}}
