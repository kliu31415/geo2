#pragma once

#include "kx/gfx/kwindow.h"
#include "kx/plot/numeric_graph.h"
#include "kx/time.h"

#include <memory>
#include <string>
#include <deque>
#include <array>

namespace kx { namespace plot {

/** NumericGraphRender renders a collection of graphs to a KWindow. Currently it only
 *  supports having one y-axis, but that'll probably change in the future. As such,
 *  try not to use set_y_axis_title(...).
 */
class NumericGraphRender final: public gfx::KItem
{
public:
    enum class Style {Calculator, AxesAtBorder};
private:
    Style style;
    std::string title;
    std::string x_axis_title;
    std::string y_axis_title;
    double border_size;
    bool show_axes_toggle;
    bool show_grid_toggle;

    std::deque<std::shared_ptr<NumericGraph>> graphs;
    std::unique_ptr<Bounds> bounds;

    std::array<int64_t, 4> key_press_frame_count;

    Time last_frame_timestamp;
    double pixel_loc_x_mult;
    double pixel_loc_y_mult;
    gfx::Rect graph_render_rect;

    void draw_axes_and_grid(gfx::KWindowRunning *kwin_r);
    void zoom_x(double dzoom, double pos);
    void zoom_y(double dzoom, double pos);
    void autocalc_bounds();
    void process_input(gfx::KWindowRunning *kwin_r);
    std::shared_ptr<gfx::Texture> render(gfx::KWindowRunning *kwin_r);
public:
    NumericGraphRender(const gfx::Rect &render_rect_);

    ///This could be made to be copyable, but that'd be weird, so disable it for safety.
    NumericGraphRender(const NumericGraphRender&) = delete;
    NumericGraphRender &operator = (const NumericGraphRender&) = delete;

    void add_graph_front(std::shared_ptr<NumericGraph> graph);

    /** get_pixel_loc(...) relies on precomputing pixel_loc_x_mult and pixel_loc_y_mult.
     *  This precomputation is done in NumericGraphRender.run(...). As such, you should ONLY
     *  this function in NumericGraph.run(), where it is guaranteed to work.
     */
    gfx::Point get_pixel_loc(Coord c) const;

    void set_bounds(Bounds *new_bounds);

    std::shared_ptr<gfx::Texture> run(gfx::KWindowRunning *kwin_r) override final;

    void set_style(Style new_style);

    void set_title(std::string new_title);
    void set_x_axis_title(std::string new_title);
    //void set_y_axis_title(std::string new_title); //don't use for now. Will be enabled later.

    void set_border_size(double new_border_size);

    void show_axes(bool toggle);
    void show_grid(bool toggle);

    ///graph_render_rect is the rect where the actual graph (i.e. excludes borders) is drawn
    gfx::Rect get_graph_render_rect() const;

    std::unique_ptr<Bounds> get_bounds() const;
};

}}
