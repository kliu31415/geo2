#include "kx/plot/numeric_graph_render.h"
#include "kx/util.h"
#include "kx/gfx/gfx.h"
#include "kx/gfx/renderer.h"
#include "kx/debug.h"

#include <algorithm>
#include <cmath>
#include <iomanip>

namespace kx { namespace plot {

static double round_up_2_10(double val)
{
    k_expects(val > 0);
    k_expects(!std::isnan(val));

    int exponent = 0;

    while(val < 1) {
        val *= 10;
        exponent--;
    }
    while(val >= 10) {
        val /= 10;
        exponent++;
    }
    if(val < 2) {
        return 2 * std::pow(10, exponent);
    } else
        return 10 * std::pow(10, exponent);
}
void NumericGraphRender::draw_axes_and_grid(gfx::KWindowRunning *kwin_r)
{
    if(show_axes_toggle) {
        switch(style) {
        case Style::Calculator: {
            double x_axis_pos = graph_render_rect.h *
                                (1 - (0 - bounds->y1) / (bounds->y2 - bounds->y1)); //the y coordinate of the x axis
            double y_axis_pos = graph_render_rect.w *
                                (0 - bounds->x1) / (bounds->x2 - bounds->x1); //the x coordinate of the y axis

            x_axis_pos = std::clamp(x_axis_pos, 0.0, (double)graph_render_rect.h - 1); //the x axis can't go off screen
            y_axis_pos = std::clamp(y_axis_pos, 0.0, (double)graph_render_rect.w - 1); //the y axis can't go off screen
            kwin_r->rdr()->set_color(gfx::SRGB_Color(0.0, 0.0, 0.0, 0.6));
            kwin_r->rdr()->fill_rect(gfx::Rect(y_axis_pos - 1, 0, 3, graph_render_rect.h)); //y axis
            kwin_r->rdr()->fill_rect(gfx::Rect(0, x_axis_pos - 1, graph_render_rect.w, 3)); //x axis
            break;}
        case Style::AxesAtBorder:{
            break;}
        }
    }

    int AXIS_TEXT_SIZE = 2 + 0.010*std::sqrt(graph_render_rect.w * graph_render_rect.h);
    constexpr double FINENESS = 0.055; //higher fineness = more tick mark texts
    constexpr double Y_FINENESS = 1500;
    constexpr float SMALL_AXIS_TICK_LEN = 3;
    constexpr float LARGE_AXIS_TICK_LEN = 5;

    double x_res = round_up_2_10((bounds->x2 - bounds->x1) / (FINENESS * graph_render_rect.w));
    double y_res = round_up_2_10((bounds->y2 - bounds->y1) / (FINENESS * Y_FINENESS));
    k_ensures(x_res > 0 && !std::isinf(x_res) && !std::isnan(x_res));
    k_ensures(y_res > 0 && !std::isinf(y_res) && !std::isnan(y_res));
    //this shouldn't fail unless there are precision errors
    constexpr double PAST_EDGE_MULT = 10;
    double past_edge = PAST_EDGE_MULT * AXIS_TEXT_SIZE;
    k_ensures((bounds->x2 / x_res + past_edge) - (bounds->x1 / x_res - past_edge) < 1e4);

    Coord bounds_center((bounds->x1 + bounds->x2) / 2, (bounds->y1 + bounds->y2) / 2);

    for(int64_t i = bounds->x1 / x_res - past_edge; i <= bounds->x2 / x_res + past_edge; i++) {
        if(style==Style::Calculator && i==0) //don't mark the origin in calculator style
            continue;

        auto x_tick_loc = get_pixel_loc(Coord(i * x_res, 0));
        switch(style) {
        case Style::Calculator:
            x_tick_loc.y = std::clamp(x_tick_loc.y, 0.0f, graph_render_rect.h - 1);
            break;
        case Style::AxesAtBorder:
            x_tick_loc.y = graph_render_rect.h - 1;
            break;
        }

        if(show_grid_toggle) {
            float grid_alpha;
            if(i % 5 != 0)
                grid_alpha = 0.10;
            else
                grid_alpha = 0.25;
            gfx::Point grid_line_p1(x_tick_loc.x, 0);
            gfx::Point grid_line_p2(x_tick_loc.x, graph_render_rect.h-1);
            kwin_r->rdr()->set_color(gfx::SRGB_Color(0.0, 0.0, 0.0, grid_alpha));
            kwin_r->rdr()->draw_line(grid_line_p1, grid_line_p2);
        }

        if(show_axes_toggle) {
            if(i % 5 != 0) { //draw minor axis tick marks
                kwin_r->rdr()->set_color(gfx::SRGB_Color(0.0, 0.0, 0.0, 0.5));
                auto point1 = gfx::Point(x_tick_loc.x, x_tick_loc.y - SMALL_AXIS_TICK_LEN);
                auto point2 = gfx::Point(x_tick_loc.x, x_tick_loc.y + SMALL_AXIS_TICK_LEN);
                kwin_r->rdr()->draw_line(point1, point2);
            } else { //occasionally note positions of "major" axis tick marks and draw text for them
                kwin_r->rdr()->set_color(gfx::SRGB_Color(0.0, 0.0, 0.0, 1.0));
                auto point1 = gfx::Point(x_tick_loc.x, x_tick_loc.y - LARGE_AXIS_TICK_LEN);
                auto point2 = gfx::Point(x_tick_loc.x, x_tick_loc.y + LARGE_AXIS_TICK_LEN);
                kwin_r->rdr()->draw_line(point1, point2);

                //draw x tick position text
                gfx::SRGB_Color text_color;
                if(i > 0)
                    text_color = gfx::SRGB_Color(0.0, 0.5, 0.0);
                else if(i < 0)
                    text_color = gfx::SRGB_Color(0.5, 0.0, 0.0);
                else
                    text_color = gfx::SRGB_Color(0.5, 0.5, 0.0);

                //if the text is an integer value, display it as an integer (looks prettier).
                //e.g. prefer 1234500 over 1.2345e+06
                std::string text_str;
                double x_axis_mark_val = std::fabs(i * x_res);
                if(x_axis_mark_val==(int)x_axis_mark_val && x_axis_mark_val<1e8)
                    text_str = to_str((int)x_axis_mark_val);
                else
                    text_str = to_str(x_axis_mark_val);

                if(i < 0)
                    text_str = "(" + text_str + ")";
                auto texture = kwin_r->rdr()->get_text_texture(text_str, AXIS_TEXT_SIZE, text_color);
                int text_w = texture->get_w();
                int text_h = texture->get_h();
                gfx::Rect dst = gfx::IRect((int)(x_tick_loc.x - text_w/2),
                                           (int)(x_tick_loc.y - LARGE_AXIS_TICK_LEN - text_h),
                                           texture->get_w(),
                                           texture->get_h());

                dst.y = std::max(dst.y, LARGE_AXIS_TICK_LEN);
                kwin_r->rdr()->draw_texture(*texture, dst);
            }
        }
    }

    //this shouldn't fail unless there are precision errors
    k_ensures((bounds->y2 / y_res + past_edge) - (bounds->y1 / y_res - past_edge) < 1e4);

    for(int64_t i = bounds->y1 / y_res - past_edge; i <= bounds->y2 / y_res + past_edge; i++) {
        if(style==Style::Calculator && i==0) //don't mark the origin in calculator style
            continue;

        auto y_tick_loc = get_pixel_loc(Coord(0, i * y_res));
        switch(style) {
        case Style::Calculator:
            y_tick_loc.x = std::clamp(y_tick_loc.x, 0.0f, graph_render_rect.w - 1);
            break;
        case Style::AxesAtBorder:
            y_tick_loc.x = 0.0f;
            break;
        }

        if(show_grid_toggle) {
            float grid_alpha;
            if(i % 5 != 0)
                grid_alpha = 0.10;
            else
                grid_alpha = 0.25;

            gfx::Point grid_line_p1(0, y_tick_loc.y);
            gfx::Point grid_line_p2(graph_render_rect.w-1, y_tick_loc.y);
            kwin_r->rdr()->set_color(gfx::SRGB_Color(0.0, 0.0, 0.0, grid_alpha));
            kwin_r->rdr()->draw_line(grid_line_p1, grid_line_p2);
        }

        if(show_axes_toggle) {
            if(i % 5 != 0) { //draw minor axis tick marks
                kwin_r->rdr()->set_color(gfx::SRGB_Color(0.0, 0.0, 0.0, 0.5));
                auto point1 = gfx::Point(y_tick_loc.x - SMALL_AXIS_TICK_LEN, y_tick_loc.y);
                auto point2 = gfx::Point(y_tick_loc.x + SMALL_AXIS_TICK_LEN, y_tick_loc.y);
                kwin_r->rdr()->draw_line(point1, point2);
            } else { //occasionally note positions of "major" axis tick marks and draw text for them
                kwin_r->rdr()->set_color(gfx::SRGB_Color(0.0, 0.0, 0.0, 1.0));
                auto point1 = gfx::Point(y_tick_loc.x - LARGE_AXIS_TICK_LEN, y_tick_loc.y);
                auto point2 = gfx::Point(y_tick_loc.x + LARGE_AXIS_TICK_LEN, y_tick_loc.y);
                kwin_r->rdr()->draw_line(point1, point2);

                //draw y position text
                gfx::SRGB_Color text_color;
                if(i > 0)
                    text_color = gfx::SRGB_Color(0.0, 0.5, 0.0);
                else if(i < 0)
                    text_color = gfx::SRGB_Color(0.5, 0.0, 0.0);
                else
                    text_color = gfx::SRGB_Color(0.5, 0.5, 0.0);
                auto text_str = to_str(std::fabs(i * y_res));
                if(i < 0)
                    text_str = "(" + text_str + ")";
                auto texture = kwin_r->rdr()->get_text_texture(text_str, AXIS_TEXT_SIZE, text_color);
                int text_w = texture->get_w();
                int text_h = texture->get_h();
                gfx::Rect dst = gfx::IRect((int)(y_tick_loc.x + LARGE_AXIS_TICK_LEN),
                                           (int)(y_tick_loc.y - text_h/2),
                                           texture->get_w(),
                                           texture->get_h());
                dst.x = std::min(dst.x, graph_render_rect.w - text_w - LARGE_AXIS_TICK_LEN);
                kwin_r->rdr()->draw_texture(*texture, dst);
            }
        }
    }
}
/*we could run into precision issues with bounds
  e.g. if the user wants to x1=1e100 and x2=0.1 + 1e100
  This could cause issues in draw_axes, which uses
  the reciprocals of x2-x1 and y2-y1 and converts them
  to int64s. Therefore, ensure that x1 and x2 relative to the
  width and y1 and y2 relative to the height are never too big.
  Another issue is that grid lines may become distorted because
  they use floats, but I think that will only cause minor graphical
  bugs, so it's not a giant deal.

  Another issue that could pop up is if the adjustments become
  much smaller than the actual bounds, then the adjustments will
  just be rounded to 0 (e.g. if x1=1e100 and you try to add
  5 to it, nothing will happen due to precision issues).
*/

static constexpr double MAX_ZOOM_FACTOR = 1e12;

void NumericGraphRender::zoom_x(double dzoom, double pos)
{
    //a zoom <= 1 causes the bounds to have negative length.
    //we also don't want too extreme of a zoom because that
    //might cause precision issues
    k_expects(dzoom>-0.5);
    k_expects(std::fabs(std::log10(1 + 2*dzoom)) < 1);

    k_expects(graph_render_rect.w > 0);
    k_expects(pos>=0 && pos<=1); //ensure that our mouse is within our render rect

    double old_x_len = (bounds->x2 - bounds->x1);
    double new_x1 = bounds->x1 + old_x_len * pos       * dzoom;
    double new_x2 = bounds->x2 - old_x_len * (1 - pos) * dzoom;
    //make sure we don't zoom too far in
    double new_x_len = new_x2 - new_x1;
    if(std::fabs(new_x2 / new_x_len) > MAX_ZOOM_FACTOR ||
       std::fabs(new_x1 / new_x_len) > MAX_ZOOM_FACTOR)
    {
        //sanity check to make sure our bounds were in range before
        //(i.e. x1 and x2 were not larger than x_len by a factor of
        //of more than MAX_ZOOM_FACTOR)
        k_expects(dzoom > 0);
    } else {
        bounds->x1 = new_x1;
        bounds->x2 = new_x2;
    }
    k_ensures(bounds->x1 < bounds->x2);
}
void NumericGraphRender::zoom_y(double dzoom, double pos)
{
    //a zoom <= 1 causes the bounds to have negative length.
    //we also don't want too extreme of a zoom because that
    //might cause precision issues
    k_expects(dzoom>-0.5);
    k_expects(std::fabs(std::log10(1 + 2*dzoom)) < 1);

    k_expects(graph_render_rect.h > 0);
    k_expects(pos>=0 && pos<=1); //ensure that our mouse is within our render rect

    double old_y_len = (bounds->y2 - bounds->y1);
    double new_y1 = bounds->y1 + old_y_len * pos       * dzoom;
    double new_y2 = bounds->y2 - old_y_len * (1 - pos) * dzoom;
    //make sure we don't zoom too far in
    double new_y_len = new_y2 - new_y1;
    if(std::fabs(new_y2 / new_y_len) > MAX_ZOOM_FACTOR ||
       std::fabs(new_y1 / new_y_len) > MAX_ZOOM_FACTOR)
    {
        //sanity check to make sure our bounds were in range before
        //(i.e. y1 and y2 were not larger than y_len by a factor of
        //of more than MAX_ZOOM_FACTOR)
        k_expects(dzoom > 0);
    } else {
        bounds->y1 = new_y1;
        bounds->y2 = new_y2;
    }
    k_ensures(bounds->y1 < bounds->y2);
}
void NumericGraphRender::autocalc_bounds()
{
    for(const auto &graph: graphs)
        bounds = Bounds::combine(bounds.get(), graph->get_bounds().get());
    if(bounds == nullptr) { //if bounds are still not set, set them to some default value
        bounds = std::make_unique<Bounds>(0, 0, 1, 1);
    } else { //otherwise, prevent getting a divide by 0 later by making sure the bounds
             //have nonzero area. Also make sure you don't divide by too small of a number
             //by making x2-x1 and y2-y1 sufficiently large relative to x1, x2 and y1, y2

        if(bounds->x1 >= bounds->x2) {
            bounds->x1 -= 1;
            bounds->x2 += 1;
            k_ensures(bounds->x1 < bounds->x2);

            while(true) {
                double x_len = bounds->x2 - bounds->x1;
                if(bounds->x2 / x_len > MAX_ZOOM_FACTOR ||
                   bounds->x1 / x_len > MAX_ZOOM_FACTOR)
                {
                    bounds->x2 -= x_len;
                    bounds->x1 += x_len;
                } else
                    break;
            }
        }
        if(bounds->y1 >= bounds->y2) {
            bounds->y1 -= 1;
            bounds->y2 += 1;
            k_ensures(bounds->y1 < bounds->y2);

            while(true) {
                double y_len = bounds->y2 - bounds->y1;
                if(bounds->y2 / y_len > MAX_ZOOM_FACTOR ||
                   bounds->y1 / y_len > MAX_ZOOM_FACTOR)
                {
                    bounds->y2 -= y_len;
                    bounds->y1 += y_len;
                } else
                    break;
            }
        }
        //zoom out a bit
        zoom_x(std::log1p(-0.1), 0.5);
        zoom_y(std::log1p(-0.1), 0.5);
    }
}
void NumericGraphRender::process_input(gfx::KWindowRunning *kwin_r)
{
    auto &kpfc = key_press_frame_count; //typing this out every time is annoying, so abbreviate it
    if(!kwin_r->is_focused()) { //don't process input if the window isn't focused
        std::fill(std::begin(kpfc), std::end(kpfc), 0); //reset kpfc (act as if no keys are pressed)
        return;
    }
    int numkeys;
    auto modstate = SDL_GetModState();
    const Uint8 *raw_keystate = SDL_GetKeyboardState(&numkeys);
    std::vector<Uint8> keystate(numkeys);
    std::copy(raw_keystate, raw_keystate + numkeys, keystate.begin());

    std::vector<int> wheel_ys;
    //mouse scroll
    for(auto &inp: kwin_r->input_deque) {
        switch(inp->type)
        {
        case SDL_TEXTINPUT:
            inp.reset();
            break;
        case SDL_MOUSEWHEEL: {
            wheel_ys.push_back(inp->wheel.y);
            inp.reset();
            break;}
        case SDL_KEYDOWN:
            //if we're lagging (say 2fps) then unless the key happens to be pressed at the time of this loop,
            //it won't get processed, e.g. if this loop gets processed at timestamps 1.1s and 1.6s, but we
            //press the left arrow key from 1.2s-1.4s, the down press won't get processed, so use KEYDOWN
            //events to artificially update the keystate
            switch(inp->key.keysym.sym)
            {
            case SDLK_LEFT:
            case SDLK_RIGHT:
            case SDLK_DOWN:
            case SDLK_UP:
            case SDLK_z:
            case SDLK_x:
                keystate[SDL_GetScancodeFromKey(inp->key.keysym.sym)] = 1;
                break;
            default:
                break;
            }
            inp.reset();
            break;
        case SDL_KEYUP:
            inp.reset();
            break;
        default:
            break;
        }
    }

    //if we have really low fps, mouse_x and mouse_y might not be the same as they were when
    //the scroll wheel was zoomed. Unfortunately, there's no good way to compensate for this
    //because we can't get where the mouse was in the past.
    int mouse_x, mouse_y;
    SDL_GetMouseState(&mouse_x, &mouse_y);

    for(const auto &wheel_y: wheel_ys) {
        double x_pos = (mouse_x - graph_render_rect.x) / (double)graph_render_rect.w;
        double y_pos = (mouse_y - graph_render_rect.y) / (double)graph_render_rect.h;
        if(x_pos>=0 && x_pos<=1 && y_pos>=0 && y_pos<=1) { //ensure the mouse is within our rendering rect
            int iterations = std::abs(wheel_y);
            double zoom_rate;

            int zoom_rate_mod = ((modstate & KMOD_CTRL)>0) - ((modstate & KMOD_SHIFT)>0);
            if(wheel_y < 0) {
                zoom_rate = std::log1p(-0.075 * std::pow(4, zoom_rate_mod));
            } else
                zoom_rate = std::log1p( 0.075 * std::pow(4, zoom_rate_mod));

            for(int i=0; i<iterations; i++) {
                //Z key = horizontal zoom, X key = vertical zoom
                if((!keystate[SDL_SCANCODE_Z]) ^ keystate[SDL_SCANCODE_X]) { //both or neither are pressed
                    zoom_x(zoom_rate, x_pos);
                    zoom_y(zoom_rate, 1 - y_pos);
                } else if(keystate[SDL_SCANCODE_X]) {
                    zoom_x(zoom_rate, x_pos);
                } else if(keystate[SDL_SCANCODE_Z]) {
                    zoom_y(zoom_rate, 1 - y_pos);
                } else
                    log_error("can't process zoom due to weird keystate");
            }
        }
    }

    //arrow keys
    constexpr int left_idx=0, right_idx=1, up_idx=2, down_idx=3;

    auto frame_len = Time::Delta(1.0 / std::max(kwin_r->rdr()->get_fps(), 1), Time::Length::second);

    kpfc[left_idx]  = ((keystate[SDL_SCANCODE_LEFT]  | keystate[SDL_SCANCODE_A]) ? kpfc[left_idx] + 1 : 0);
    kpfc[right_idx] = ((keystate[SDL_SCANCODE_RIGHT] | keystate[SDL_SCANCODE_D]) ? kpfc[right_idx] + 1: 0);
    kpfc[up_idx]    = ((keystate[SDL_SCANCODE_UP]    | keystate[SDL_SCANCODE_W]) ? kpfc[up_idx] + 1   : 0);
    kpfc[down_idx]  = ((keystate[SDL_SCANCODE_DOWN]  | keystate[SDL_SCANCODE_S]) ? kpfc[down_idx] + 1 : 0);

    double dbounds_x = 1.1 * frame_len.to_double(Time::Length::second) * (bounds->x2 - bounds->x1);
    double dbounds_y = 1.1 * frame_len.to_double(Time::Length::second) * (bounds->y2 - bounds->y1);

    if(modstate & KMOD_CTRL) {
        dbounds_x *= 4;
        dbounds_y *= 4;
    }
    if(modstate & KMOD_SHIFT) {
        dbounds_x /= 4;
        dbounds_y /= 4;
    }

    //note that if the left and right keys are pressed at the same time, then left will override
    //the right due to the <= (and similarly, down overrides up)
    if(kpfc[left_idx]>0 && (kpfc[right_idx]==0 || kpfc[left_idx]<=kpfc[right_idx])) {
        bounds->x1 -= dbounds_x;
        bounds->x2 -= dbounds_x;
    } else if(kpfc[right_idx]>0 && (kpfc[left_idx]==0 || kpfc[right_idx]<=kpfc[left_idx])) {
        bounds->x1 += dbounds_x;
        bounds->x2 += dbounds_x;
    }
    if(kpfc[down_idx]>0 && (kpfc[up_idx]==0 || kpfc[down_idx]<=kpfc[up_idx])) {
        bounds->y1 -= dbounds_y;
        bounds->y2 -= dbounds_y;
    } else if(kpfc[up_idx]>0 && (kpfc[down_idx]==0 || kpfc[up_idx]<=kpfc[down_idx])) {
        bounds->y1 += dbounds_y;
        bounds->y2 += dbounds_y;
    }

}
static std::string double_to_pretty_str(double val)
{
    if(std::fabs(val) < 1e-20)
        return "0.000";

    std::stringstream ss;
    ss << std::showpos;
    if(std::fabs(std::log10(std::fabs(val))) > 5)
        ss << std::scientific << std::setprecision(4);
    else
        ss << std::fixed << std::setprecision(6 - std::max(0.0, std::log10(std::fabs(val))));
    ss << val;
    return ss.str();
}
static void draw_text_to_fit_y(gfx::Renderer *renderer,
                               const std::string &text, int size, gfx::SRGB_Color color,
                               float x, float y, float h)
{
    if(!text.empty()) {
        auto texture = renderer->get_text_texture(text, size, color);
        if(texture->get_h() > 0) {
            float dst_w = texture->get_w() * h / texture->get_h();
            gfx::Rect dst = gfx::IRect(x, y, dst_w, h);
            renderer->draw_texture(*texture, dst);
        }
    }
}
std::shared_ptr<gfx::Texture> NumericGraphRender::render(gfx::KWindowRunning *kwin_r)
{
    k_expects(!std::isnan(bounds->x1));
    k_expects(!std::isnan(bounds->x2));
    k_expects(!std::isnan(bounds->y1));
    k_expects(!std::isnan(bounds->y2));

    k_expects(!std::isinf(bounds->x1));
    k_expects(!std::isinf(bounds->x2));
    k_expects(!std::isinf(bounds->y1));
    k_expects(!std::isinf(bounds->y2));

    k_expects(bounds->x1 < bounds->x2);
    k_expects(bounds->y1 < bounds->y2);

    //render all the graphs to buffers
    std::vector<std::shared_ptr<gfx::Texture>> graph_textures;
    for(const auto &graph: graphs) {
        graph_textures.push_back(graph->run(kwin_r, this));
        erase_remove(&kwin_r->input_deque, std::nullopt);
        //I think using nullopt works but might as well make sure
        for(auto &i: kwin_r->input_deque)
            k_assert(i.has_value());
    }

    auto composite_graph_texture(kwin_r->rdr()->make_texture_target(graph_render_rect.w, graph_render_rect.h));
    kwin_r->rdr()->set_target(composite_graph_texture.get());
    kwin_r->rdr()->set_color(gfx::Color::WHITE);
    kwin_r->rdr()->clear();

    //render all the graph textures in opposite order
    gfx::Rect render_graph_to(0, 0, graph_render_rect.w, graph_render_rect.h);
    for(int i=(int)graph_textures.size()-1; i>=0; i--)
        kwin_r->rdr()->draw_texture(*graph_textures[i], render_graph_to);

    //render axes
    draw_axes_and_grid(kwin_r);

    auto return_texture(kwin_r->rdr()->make_texture_target(render_rect.w, render_rect.h));

    kwin_r->rdr()->set_target(return_texture.get());
    kwin_r->rdr()->set_color(gfx::SRGB_Color(0.9f, 0.9f, 0.9f, 1.0f));
    kwin_r->rdr()->clear();

    //relative graph render rect (the x and y are relative to render_rect's)
    gfx::Rect rel_gr_rect(graph_render_rect.x - render_rect.x,
                          graph_render_rect.y - render_rect.y,
                          graph_render_rect.w,
                          graph_render_rect.h);
    //there are useful things that we will draw in the border if it's nonzero size
    if(border_size > 0) {
        //draw title
        kwin_r->rdr()->set_font(gfx::Font::DEFAULT);
        double left_border_end_x = border_size;

        draw_text_to_fit_y(kwin_r->rdr(),
                           title, rel_gr_rect.y, gfx::Color::BLACK,
                           left_border_end_x, 0, rel_gr_rect.y);

        draw_text_to_fit_y(kwin_r->rdr(),
                           x_axis_title, rel_gr_rect.y, gfx::Color::BLACK,
                           left_border_end_x, render_rect.h - rel_gr_rect.y, rel_gr_rect.y);

        //show mouse coords
        int mouse_x, mouse_y;
        SDL_GetMouseState(&mouse_x, &mouse_y);
        if(graph_render_rect.point_inside(gfx::Point(mouse_x, mouse_y))) {
            kwin_r->rdr()->set_font(gfx::Font::MONO_DEFAULT);

            double bounds_w = bounds->x2 - bounds->x1;
            double bounds_h = bounds->y2 - bounds->y1;
            Coord mouse_loc_coord(bounds->x1 + bounds_w*(mouse_x - graph_render_rect.x)/(graph_render_rect.w),
                                  bounds->y2 - bounds_h*(mouse_y - graph_render_rect.y)/(graph_render_rect.h));

            std::string mouse_loc_text_x = "(" + double_to_pretty_str(mouse_loc_coord.x) + ",";
            std::string mouse_loc_text_y = double_to_pretty_str(mouse_loc_coord.y) + ")";

            auto x_color = mouse_loc_coord.x>=0? gfx::Color::BLACK: gfx::Color::DARK_RED;
            auto y_color = mouse_loc_coord.y>=0? gfx::Color::BLACK: gfx::Color::DARK_RED;

            auto mouse_loc_texture_x = kwin_r->rdr()->get_text_texture(mouse_loc_text_x, rel_gr_rect.y, x_color);
            auto mouse_loc_texture_y = kwin_r->rdr()->get_text_texture(mouse_loc_text_y, rel_gr_rect.y, y_color);

            if(mouse_loc_texture_x->get_h() > 0) {
                //y coordinate
                float dsty_w = mouse_loc_texture_y->get_w() * rel_gr_rect.y / mouse_loc_texture_y->get_h();
                gfx::Rect dst = gfx::IRect(render_rect.w - dsty_w,
                                           render_rect.h - rel_gr_rect.y,
                                           dsty_w,
                                           rel_gr_rect.y);
                kwin_r->rdr()->draw_texture(*mouse_loc_texture_y, dst);

                static constexpr float MAGIC_TEXT_DIST_MULT = 5;
                float text_dist = rel_gr_rect.y * MAGIC_TEXT_DIST_MULT;
                //x coordinate
                float dstx_w = mouse_loc_texture_x->get_w() * rel_gr_rect.y / mouse_loc_texture_x->get_h();
                dst = gfx::IRect(render_rect.w - std::max(text_dist, dsty_w) - std::max(text_dist, dstx_w),
                                 render_rect.h - rel_gr_rect.y,
                                 dstx_w,
                                 rel_gr_rect.y);
                kwin_r->rdr()->draw_texture(*mouse_loc_texture_x, dst);
            }
        }
    }

    kwin_r->rdr()->draw_texture(*composite_graph_texture, rel_gr_rect);

    return return_texture;
}
NumericGraphRender::NumericGraphRender(const gfx::Rect &render_rect_):
    KItem(render_rect_),
    style(Style::AxesAtBorder),
    show_axes_toggle(true),
    show_grid_toggle(true),
    last_frame_timestamp(Time::now())
{
    set_border_size(30);
    std::fill(std::begin(key_press_frame_count), std::end(key_press_frame_count), 0);
}
void NumericGraphRender::add_graph_front(std::shared_ptr<NumericGraph> graph)
{
    graphs.push_front(std::move(graph));
}
gfx::Point NumericGraphRender::get_pixel_loc(Coord c) const
{
    return gfx::Point(pixel_loc_x_mult * (c.x - bounds->x1),
                      graph_render_rect.h - pixel_loc_y_mult * (c.y - bounds->y1));
}
void NumericGraphRender::set_bounds(Bounds *new_bounds)
{
    if(new_bounds == nullptr) {
        bounds = nullptr;
    } else {
        bounds = std::make_unique<Bounds>(*new_bounds); //copy by value because the input is a non-owning pointer
    }
}
std::shared_ptr<gfx::Texture> NumericGraphRender::run(gfx::KWindowRunning *kwin_r)
{
    if(bounds == nullptr)
        autocalc_bounds();

    process_input(kwin_r);

    last_frame_timestamp = Time::now();

    k_expects(bounds != nullptr);
    k_expects(bounds->x1 < bounds->x2);
    k_expects(bounds->y1 < bounds->y2);

    //some precalculations
    pixel_loc_x_mult = graph_render_rect.w / (bounds->x2 - bounds->x1);
    pixel_loc_y_mult = graph_render_rect.h / (bounds->y2 - bounds->y1);

    return render(kwin_r);
}
void NumericGraphRender::set_title(std::string new_title)
{
    title = std::move(new_title);
}
void NumericGraphRender::set_x_axis_title(std::string new_title)
{
    x_axis_title = std::move(new_title);
}
/*void NumericGraphRender::set_y_axis_title(std::string new_title)
{
    y_axis_title = std::move(new_title);
}*/
void NumericGraphRender::set_style(Style new_style)
{
    style = new_style;
}
void NumericGraphRender::set_border_size(double new_border_size)
{
    border_size = std::clamp(new_border_size, 0.0, std::min(render_rect.w, render_rect.h) / 2.0);
    graph_render_rect = gfx::Rect((int)(render_rect.x + border_size),
                                  (int)(render_rect.y + border_size),
                                  (int)(render_rect.w  - 2*border_size),
                                  (int)(render_rect.h - 2*border_size));
}
void NumericGraphRender::show_axes(bool toggle)
{
    show_axes_toggle = toggle;
}
void NumericGraphRender::show_grid(bool toggle)
{
    show_grid_toggle = toggle;
}
gfx::Rect NumericGraphRender::get_graph_render_rect() const
{
    return graph_render_rect;
}
std::unique_ptr<Bounds> NumericGraphRender::get_bounds() const
{
    return std::make_unique<Bounds>(*bounds);
}
}}
