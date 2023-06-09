#pragma once

#include "geo2/map_obj/unit/unit.h"
#include "geo2/map_obj/unit/movement/algo1.h"
#include "geo2/game_render_scene_graph.h"

#include <array>
#include <functional>

namespace geo2 { namespace map_obj {

class Hexfly_1 final: public Unit
{
    unit_movement::Algo1 movement_algo;
    std::unique_ptr<Polygon> base_shape;
    float side_len;
    double wing_freq;

    std::array<std::shared_ptr<RenderOpShader>, 8> ops;
    std::array<kx::kx_span<float>, 8> op_ius;
    std::shared_ptr<RenderOpGroup> op_group;

    float eye_w;
    float eye_h;
    float eye_x_offset;
    float eye_y_offset;

    Hexfly_1(MapCoord position_, float side_len_);
public:
    static std::shared_ptr<Hexfly_1> make_standard(MapCoord position);

    void init(const MapObjInitArgs &args) override;
    void run1_mt(const MapObjRun1Args &args) override;
    void run3_mt(const MapObjRun3Args &args) override;

    void add_render_ops(const MapObjRenderArgs &args) override;

    double get_collision_damage() const override;
};

}}
