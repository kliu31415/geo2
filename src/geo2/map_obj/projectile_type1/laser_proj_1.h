#pragma once

#include "geo2/map_obj/projectile_type1/basic_proj_1.h"

namespace geo2 {class RenderOpShader; class RenderOpGroup;}

namespace geo2 { namespace map_obj {

class LaserProj_1 final: public BasicProj_1
{
    std::shared_ptr<RenderOpShader> op;
    std::shared_ptr<RenderOpGroup> op_group;
    nonstd::span<float> op_iu;
    std::unique_ptr<Polygon> base_shape;
public:
    LaserProj_1(const std::shared_ptr<MapObject> &owner_,
                double damage_,
                double lifespan_,
                MapCoord pos_,
                MapVec velocity_,
                double rot);

    void init(const MapObjInitArgs &args) override;
    void add_render_ops(const MapObjRenderArgs &args) override;

    void copy_base_shape_into(Polygon *p) const override;
};

}}
