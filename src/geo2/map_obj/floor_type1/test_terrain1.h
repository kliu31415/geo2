#pragma once

#include "geo2/map_obj/floor_type1/floor_type1.h"

#include "span_lite.h"

namespace geo2 {class RenderOpGroup; class RenderOpShader;}

namespace geo2 { namespace map_obj {

class TestTerrain1 final: public Floor_Type1
{
    std::shared_ptr<class RenderOpShader> op;
    std::shared_ptr<class RenderOpGroup> op_group;
    nonstd::span<float> op_iu;
public:
    TestTerrain1(MapCoord pos_);
    void add_render_objs(const MapObjRenderArgs &info) override;
};

}}
