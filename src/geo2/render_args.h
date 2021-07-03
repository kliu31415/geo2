#pragma once

#include "kx/gfx/renderer.h"
#include "geo2/game_render_op_list.h"

namespace geo2 {

class RenderArgs
{
    const kx::gfx::Renderer *rdr;
    kx::gfx::Rect camera;
    float cam_w_inv;
    float cam_h_inv;
    const GameRenderOpList::Shaders *shaders;
public:
    inline void set_renderer(kx::gfx::Renderer *renderer)
    {
        rdr = renderer;
    }
    inline void set_camera(kx::gfx::Rect camera_)
    {
        camera = camera_;
        cam_w_inv = 1.0f / camera.w;
        cam_h_inv = 1.0f / camera.h;
    }
    inline const GameRenderOpList::Shaders *get_shaders() const
    {
        return shaders;
    }
    inline void set_shaders(GameRenderOpList::Shaders *shaders_)
    {
        shaders = shaders_;
    }
    inline bool is_x_line_ndc_in_view(float x1, float x2) const
    {
        return rdr->is_x_line_ndc_in_view(x1, x2);
    }
    inline bool is_y_line_ndc_in_view(float y1, float y2) const
    {
        return rdr->is_y_line_ndc_in_view(y1, y2);
    }
    inline bool is_x_ndc_in_view(float x) const
    {
        return rdr->is_x_ndc_in_view(x);
    }
    inline bool is_y_ndc_in_view(float y) const
    {
        return rdr->is_y_ndc_in_view(y);
    }
    inline bool is_coord_ndc_in_view(float x, float y) const
    {
        return is_x_ndc_in_view(x) && is_y_ndc_in_view(y);
    }
    inline float x_to_ndc(float x) const
    {
        return rdr->x_nc_to_ndc((x - camera.x) * cam_w_inv);
    }
    inline float y_to_ndc(float y) const
    {
        return rdr->y_nc_to_ndc((y - camera.y) * cam_h_inv);
    }
    inline bool is_AABB_in_view(const AABB &aabb) const
    {
        return is_x_line_ndc_in_view(x_to_ndc(aabb.x1), x_to_ndc(aabb.x2)) &&
               is_y_line_ndc_in_view(y_to_ndc(aabb.y1), y_to_ndc(aabb.y2));
    }
};

}
