#include "kx/plot/scatter_graph.h"
#include "kx/plot/numeric_graph_render.h"
#include "kx/gfx/renderer.h"
#include "kx/debug.h"

#include <cmath>

namespace kx { namespace plot {

ScatterGraph::ScatterGraph(const std::vector<Coord> &data_, gfx::SRGB_Color color_,
                           Shape shape_, int shape_size_):
    data(data_),
    color(color_),
    shape(shape_),
    shape_size(shape_size_)
{
    #ifdef KX_DEBUG
    for(const auto &coord: data) {
        k_expects(!std::isinf(coord.x));
        k_expects(!std::isnan(coord.x));
        k_expects(!std::isinf(coord.y));
        k_expects(!std::isnan(coord.y));
    }
    #endif
}

std::shared_ptr<gfx::Texture> ScatterGraph::run(gfx::KWindowRunning *kwin_r, NumericGraphRender *context)
{
    k_expects(kwin_r != nullptr);
    k_expects(context != nullptr);
    auto render_rect = context->get_graph_render_rect();
    kwin_r->rdr()->set_color(color);
    auto return_texture(kwin_r->rdr()->make_texture_target(render_rect.w, render_rect.h));
    kwin_r->rdr()->set_target(return_texture.get());
    switch(shape)
    {
    case Shape::Circle: {
        std::vector<gfx::Point> centers;
        centers.resize(data.size());
        #pragma omp parallel for
        for(size_t i=0; i<data.size(); i++)
            centers[i] = context->get_pixel_loc(data[i]);

        erase_remove_if(&centers,
            [=](const gfx::Point &center) -> bool {
                return center.x < -shape_size || center.x >= render_rect.w + shape_size ||
                       center.y < -shape_size || center.y >= render_rect.h + shape_size;
            }
            );
        kwin_r->rdr()->fill_circles(centers, shape_size);
        break;}
    case Shape::DecayingCircle: {
        std::vector<gfx::Point> centers;
        centers.resize(data.size());
        #pragma omp parallel for
        for(size_t i=0; i<data.size(); i++)
            centers[i] = context->get_pixel_loc(data[i]);

        erase_remove_if(&centers,
            [=](const gfx::Point &center) -> bool {
                return center.x < -shape_size || center.x >= render_rect.w + shape_size ||
                       center.y < -shape_size || center.y >= render_rect.h + shape_size;
            }
            );
        kwin_r->rdr()->fill_decaying_circles(centers, shape_size);
        break;}
    case Shape::Square: {
        std::vector<gfx::Rect> rects;
        rects.resize(data.size());
        #pragma omp parallel for
        for(size_t i=0; i<data.size(); i++) {
            auto center = context->get_pixel_loc(data[i]);
            rects[i] = gfx::Rect(center.x - shape_size/2.0, center.y - shape_size/2.0, shape_size, shape_size);
        }
        erase_remove_if(&rects,
            [=](const gfx::Rect &rect) -> bool {
                return rect.x + rect.w < 0 || rect.x >= render_rect.w ||
                       rect.y + rect.h < 0 || rect.y >= render_rect.h;
            }
            );
        kwin_r->rdr()->fill_rects(rects);
        break;}
    default:
        log_error("unsupported plot::Shape");
        break;
    }
    return return_texture;
}
std::unique_ptr<Bounds> ScatterGraph::get_bounds() const
{
    std::unique_ptr<Bounds> bounds = nullptr;
    for(const auto &data_point: data) {
        Bounds point_bounds = Bounds(data_point.x, data_point.y, data_point.x, data_point.y);
        bounds = Bounds::combine(bounds.get(), &point_bounds);
    }
    return bounds;
}

}}
