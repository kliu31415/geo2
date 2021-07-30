#pragma once

#include "geo2/map_obj/unit/unit.h"
#include "geo2/map_obj/unit/movement/algo1.h"
#include "geo2/map_obj/unit/unit_rect_placement_info.h"
#include "geo2/game_render_scene_graph.h"

#include <array>
#include <functional>

namespace geo2 { namespace map_obj {

class Pig_1 final: public Unit
{
    unit_movement::Algo1 movement_algo;

    std::array<std::shared_ptr<RenderOpShader>, 5> ops;
    std::array<kx::kx_span<float>, 5> op_ius;
    std::shared_ptr<RenderOpGroup> op_group;

    float eye_w;
    float eye_h;
    float eye_x_offset;
    float eye_y_offset;

    Pig_1(MapCoord position_);
public:
    static std::shared_ptr<Pig_1> make_standard(MapCoord position);
    static UnitRectPlacementInfo get_standard_pig_placement_info();

    void init(const MapObjInitArgs &args) override;
    void run1_mt(const MapObjRun1Args &args) override;
    void run3_mt(const MapObjRun3Args &args) override;

    void add_render_ops(const MapObjRenderArgs &args) override;

    double get_collision_damage() const override;
};

}}
