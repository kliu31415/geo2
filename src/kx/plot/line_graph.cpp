#include "kx/plot/line_graph.h"
#include "kx/plot/numeric_graph_render.h"
#include "kx/gfx/gfx.h"
#include "kx/gfx/renderer.h"
#include "kx/debug.h"

#include <algorithm>
#include <cmath>

namespace kx { namespace plot {

static bool cmp_coord_x_le(const Coord &a, const Coord &b)
{
    return a.x < b.x;
}
LineGraph::LineGraph(const std::vector<Coord> &data_, gfx::SRGB_Color color_):
    data(data_),
    color(color_)
{
    #ifdef KX_DEBUG
    for(const auto &coord: data) {
        k_expects(!std::isinf(coord.x));
        k_expects(!std::isnan(coord.x));
        k_expects(!std::isinf(coord.y));
        k_expects(!std::isnan(coord.y));
    }
    k_expects(std::is_sorted(data.begin(), data.end(), cmp_coord_x_le));
    #endif
}
std::shared_ptr<gfx::Texture> LineGraph::run(gfx::KWindowRunning *kwin_r, NumericGraphRender *context)
{
    k_expects(kwin_r != nullptr);
    k_expects(kwin_r->rdr() != nullptr);
    k_expects(context != nullptr);
    auto render_rect = context->get_graph_render_rect();
    kwin_r->rdr()->set_color(color);
    auto return_texture(kwin_r->rdr()->make_texture_target(render_rect.w, render_rect.h));
    kwin_r->rdr()->set_target(return_texture.get());

    auto bounds = context->get_bounds();
    Coord dummy_x1(bounds->x1, 0);
    Coord dummy_x2(bounds->x2, 0);

    size_t first_idx = std::lower_bound(data.begin(), data.end(), dummy_x1, cmp_coord_x_le) - data.begin();
    size_t last_idx = std::upper_bound(data.begin(), data.end(), dummy_x2, cmp_coord_x_le) - data.begin();
    if(first_idx > 0)
        first_idx--;
    if(last_idx < data.size())
        last_idx++;

    std::vector<gfx::Point> render_points;
    render_points.resize(last_idx - first_idx);

    #pragma omp parallel for
    for(size_t i=first_idx; i<last_idx; i++)
        render_points[i-first_idx] = context->get_pixel_loc(data[i]);
    kwin_r->rdr()->draw_polyline(render_points);

    return return_texture;
}
std::unique_ptr<Bounds> LineGraph::get_bounds() const
{
    std::unique_ptr<Bounds> bounds = nullptr;
    for(const auto &data_point: data) {
        Bounds point_bounds = Bounds(data_point.x, data_point.y, data_point.x, data_point.y);
        bounds = Bounds::combine(bounds.get(), &point_bounds);
    }
    return bounds;
}

}}
