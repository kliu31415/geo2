#pragma once

#include "kx/plot/numeric_graph.h"

#include <vector>
#include <memory>

namespace kx { namespace plot {

class NumericGraphRender;

///The data for a LineGraph should be sorted by x coordinate
class LineGraph final: public NumericGraph
{
    std::vector<Coord> data;
    gfx::SRGB_Color color;
public:
    LineGraph(const std::vector<Coord> &data_, gfx::SRGB_Color color_);

    std::shared_ptr<gfx::Texture> run(gfx::KWindowRunning *kwin_r, NumericGraphRender *context) override;
    std::unique_ptr<Bounds> get_bounds() const override;
};

}}
