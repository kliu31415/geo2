#pragma once

#include "geo2/map_obj/floor_type1/floor_type1.h"

#include "kx/gfx/renderer_types.h"

namespace geo2 { namespace map_obj {

class MonochromaticFloor_1 final: public Floor_Type1
{
    kx::gfx::LinearColor color;
    std::shared_ptr<class RenderOpShader> op;
    std::shared_ptr<class RenderOpGroup> op_group;
    nonstd::span<float> op_iu;
public:
    MonochromaticFloor_1(const MapRect &position_, kx::gfx::LinearColor color_);
    void add_render_objs(const MapObjRenderArgs &args) override;
};

}}
