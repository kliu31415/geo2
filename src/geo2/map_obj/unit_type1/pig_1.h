#pragma once

#include "geo2/map_obj/unit_type1/unit_type1.h"
#include "geo2/map_obj/unit_type1/movement/algo1.h"
#include "geo2/game_render_op_list.h"

#include <array>

namespace geo2 { namespace map_obj {

class Pig_1 final: public Unit_Type1, private unit_movement::Algo1
{
    std::array<std::shared_ptr<RenderOpShader>, 5> ops;
    std::array<nonstd::span<float>, 5> op_ius;
    std::shared_ptr<RenderOpGroup> op_group;

    float eye_w;
    float eye_h;
    float eye_x_offset;
    float eye_y_offset;
public:
    Pig_1(MapCoord position_);

    void init(const MapObjInitArgs &args) override;
    void run1_mt(const MapObjRun1Args &args) override;
    void run3_mt(const MapObjRun3Args &args) override;

    void add_render_objs(const MapObjRenderArgs &args) override;

    double get_collision_damage() const override;
};

}}
