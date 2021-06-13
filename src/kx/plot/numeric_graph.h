#pragma once

#include "kx/gfx/gfx.h"
#include "kx/gfx/kwindow.h"

#include <vector>
#include <memory>

namespace kx { namespace plot {
/** If you want to show a graph, first make sure that you have a KWindow.
 *  Create your desired graphs (e.g. ScatterGraph), and add all of them to a
 *  NumericGraphRender (the NumericGraphRender does the task of managing all
 *  the graphs (e.g. finding bounds that fit all of them)).
 *  Then, add NumericGraphRender to the KWindow, which will automatically render
 *  all the graphs every frame.
 */
struct Bounds final
{
    ///(x1, y1) is the lower left (in Cartesian coordinates, not SDL2 coordinates),
    ///so x1<=x2 and y1<=y2
    double x1;
    double y1;
    double x2;
    double y2;

    Bounds(double x1_, double y1_, double x2_, double y2_);
    static std::unique_ptr<Bounds> combine(const Bounds *b1, const Bounds *b2);
};

class NumericGraphRender;

class NumericGraph
{
public:
    virtual std::shared_ptr<gfx::Texture> run(gfx::KWindowRunning *kwin_r, NumericGraphRender *context) = 0;
    virtual std::unique_ptr<Bounds> get_bounds() const = 0;
    virtual ~NumericGraph() = default; /** just in case we add some fields to this in the future
                                        *  (it'd be bad to forget to add a virtual destructor)
                                        */
};

enum class Shape{DecayingCircle, Circle, Square};

struct Coord
{
    double x, y;
    Coord() = default;
    Coord(double x_, double y_);
    virtual ~Coord() = default;
};

template<class Container1, class Container2>
std::vector<Coord> make_coord_vector(const Container1 &X, const Container2 &Y)
{
    k_expects(X.size() == Y.size());
    std::vector<Coord> ret;
    ret.resize(X.size());
    for(size_t i=0; i<X.size(); i++) {
        ret[i] = Coord(X[i], Y[i]);
    }
    return ret;
}

}}
