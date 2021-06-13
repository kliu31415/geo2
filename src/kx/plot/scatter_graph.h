#pragma once

#include "kx/plot/numeric_graph.h"

#include <vector>
#include <memory>

namespace kx { namespace plot {

class NumericGraphRender;

class ScatterGraph final: public NumericGraph
{
    std::vector<Coord> data;
    gfx::SRGB_Color color;
    Shape shape;
    int shape_size;
public:
    ScatterGraph(const std::vector<Coord> &data_, gfx::SRGB_Color color_,
                 Shape shape_, int shape_size_);

    std::shared_ptr<gfx::Texture> run(gfx::KWindowRunning *kwin_r, NumericGraphRender *context) override;
    std::unique_ptr<Bounds> get_bounds() const override;
};

}}
