#pragma once

#include "geo2/map_obj/unit_type1/unit_type1.h"
#include "geo2/game_render_op_list.h"

namespace geo2 { namespace map_obj {

class Pig_1 final: public Unit_Type1
{
    static const std::unique_ptr<const Polygon> BASE_SHAPE;

    std::unique_ptr<Polygon> cur_shape;
    std::unique_ptr<Polygon> des_shape;

    std::shared_ptr<RenderOpShader> op;
    std::shared_ptr<RenderOpGroup> op_group;
    nonstd::span<float> op_iu;
public:
    Pig_1(MapCoord position_);

    void run1_mt(const MapObjRun1Args &args) override;

    HANDLE_COLLISION_FUNC_DECLARATION(Wall_Type1) override;
    HANDLE_COLLISION_FUNC_DECLARATION(Unit_Type1) override;
    HANDLE_COLLISION_FUNC_DECLARATION(Projectile_Type1) override;

    void add_render_objs(const MapObjRenderArgs &args) override;
};

}}
