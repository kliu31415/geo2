#pragma once

#include "geo2/map_obj/wall_type1/wall_type1.h"

#include "kx/gfx/renderer_types.h"

namespace geo2 { namespace map_obj {

class MonochromaticWall_1 final: public Wall_Type1
{
    kx::gfx::LinearColor color;
    std::shared_ptr<class RenderOpShader> op;
    std::shared_ptr<class RenderOpGroup> op_group;
    nonstd::span<float> op_iu;
public:
    MonochromaticWall_1(const MapRect &position_, kx::gfx::LinearColor color_);
    void add_render_objs(const MapObjRenderArgs &args) override;
};

}}
