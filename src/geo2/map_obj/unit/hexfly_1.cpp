#include "geo2/map_obj/unit/hexfly_1.h"
#include "geo2/map_obj/map_obj_args.h"

#include <cmath>
#include <random>

constexpr auto M_PI = 3.14159265358979323846;

namespace geo2 { namespace map_obj {

constexpr float H_MULT = std::sqrt(3.0f) / 2.0f;

Hexfly_1::Hexfly_1(MapCoord position_, float s):
    Unit(Team::Enemy, position_, 6.0),
    base_shape(Polygon::make(std::vector<float>{
         s    ,   0.0f,
         0.5f*s, -H_MULT*s,
        -0.5f*s, -H_MULT*s,
        -s    ,   0.0f,
        -0.5f*s,  H_MULT*s,
         0.5f*s,  H_MULT*s
        })),
    side_len(s)
{}

const kx::gfx::LinearColor DEFAULT_BODY_COLOR_1(0.2f, 0.4f, 0.0f, 1.0f);
const kx::gfx::LinearColor DEFAULT_BODY_COLOR_2(0.05f, 1.0f, 0.02f, 1.0f);
const kx::gfx::LinearColor DEFAULT_BODY_BORDER_COLOR(0.0f, 0.0f, 0.0f, 1.0f);

const kx::gfx::LinearColor DEFAULT_WING_COLOR_1(0.07f, 0.01f, 0.01f, 0.5f);
const kx::gfx::LinearColor DEFAULT_WING_COLOR_2(0.03f, 0.01f, 0.01f, 0.9f);
const kx::gfx::LinearColor DEFAULT_WING_BORDER_COLOR(0.0f, 0.0f, 0.0f, 1.0f);

constexpr float BORDER_THICKNESS = 0.17f;

//starts at right and goes CCW
void Hexfly_1::init(const MapObjInitArgs &args)
{
    args.add_current_pos_polygon_with_num_sides(6);
    args.add_desired_pos_polygon_with_num_sides(6);
    wing_freq = std::uniform_real_distribution<double>(9.0, 9.2)(*args.get_rng());
}

constexpr double MAX_SPEED = 13;
constexpr double MAX_ANGULAR_SPEED = 8;
constexpr double MAX_ACCEL = 90;
constexpr double MAX_ANGULAR_ACCEL = 40;

void Hexfly_1::run1_mt([[maybe_unused]] const MapObjRun1Args &args)
{
    if(alive_status.is_dead()) {
        if(is_completely_faded_out(args.cur_level_time))
            args.delete_me();
        return;
    }

    unit_movement::Algo1RunArgs algo_args;
    algo_args.rng = args.get_rng();
    algo_args.current_position = &current_position;
    algo_args.max_speed = MAX_SPEED;
    algo_args.max_angular_speed = MAX_ANGULAR_SPEED;
    algo_args.max_accel = MAX_ACCEL;
    algo_args.max_angular_accel = MAX_ANGULAR_ACCEL;
    algo_args.tick_len = args.tick_len;
    algo_args.cur_level_time = args.cur_level_time;
    movement_algo.run(algo_args);

    auto cur = args.get_sole_current_pos();
    cur->copy_from(*base_shape);
    cur->rotate_about_origin(movement_algo.get_current_angle());
    cur->translate(current_position.x, current_position.y);

    auto des = args.get_sole_desired_pos();
    des->copy_from(*base_shape);
    des->rotate_about_origin(movement_algo.get_desired_angle());
    auto desired_position = movement_algo.get_desired_position();
    des->translate(desired_position.x, desired_position.y);

    args.set_move_intent(MoveIntent::GoToDesiredPos);
}
void Hexfly_1::run3_mt([[maybe_unused]] const MapObjRun3Args &args)
{
    unit_movement::Algo1HandleMovementArgs algo_args;
    algo_args.rng = args.get_rng();
    algo_args.current_position = &current_position;
    algo_args.max_speed = MAX_SPEED;
    algo_args.max_angular_speed = MAX_ANGULAR_SPEED;
    algo_args.max_accel = MAX_ACCEL;
    algo_args.max_angular_accel = MAX_ANGULAR_ACCEL;
    algo_args.tick_len = args.tick_len;
    algo_args.cur_level_time = args.cur_level_time;
    algo_args.translating_time_param = 0.3;
    algo_args.rotating_time_param = 0.2;
    algo_args.resting_time_param = 2;

    if(args.get_move_intent() == MoveIntent::GoToDesiredPos) {
        movement_algo.handle_successful_movement(algo_args);
    } else {
        movement_algo.handle_unsuccessful_movement(algo_args);
    }
}

void Hexfly_1::add_render_ops(const MapObjRenderArgs &args)
{
    if(op_group == nullptr) {
        op_group = std::make_shared<RenderOpGroup>(args.get_NPC_render_priority());
        for(int i=0; i<8; i++) {
            ops[i] = std::make_shared<RenderOpShader>(*args.shaders->get("hexfly_1"));
            op_group->add_op(ops[i]);
            auto iu_map = ops[i]->map_instance_uniform(0);
            op_ius[i] = {(float*)iu_map.begin(), (float*)iu_map.end()};
        }

        //no borders yet
        for(int i=0; i<6; i++) {
            op_ius[i][6] = args.to_whole_pixels(side_len * H_MULT * BORDER_THICKNESS);
            op_ius[i][8] = 3;
            op_ius[i][7] = 1;
        }
        for(int i=6; i<8; i++) {
            op_ius[i][6] = -1;
            op_ius[i][8] = 2;
            op_ius[i][7] = 1;
        }

        //subroutine #
        for(int i=0; i<8; i++) {
            op_ius[i][7] = i;
        }
    }

    //body
    {
        auto inner_color_1 = apply_color_mod(DEFAULT_BODY_COLOR_1, args.cur_level_time);
        auto inner_color_2 = apply_color_mod(DEFAULT_BODY_COLOR_2, args.cur_level_time);
        auto border_color = apply_color_mod(DEFAULT_BODY_BORDER_COLOR, args.cur_level_time);

        auto cur_shape = base_shape->copy();
        cur_shape->rotate_about_origin_and_translate(movement_algo.get_current_angle(),
                                                     current_position - MapCoord::ORIGIN);
        for(int i=0; i<6; i++) {
            op_ius[i][0] = args.x_to_ndc(current_position.x);
            op_ius[i][1] = args.y_to_ndc(current_position.y);
            auto v1 = cur_shape->get_vertex(i);
            auto v2 = cur_shape->get_vertex(i+1);
            op_ius[i][2] = args.x_to_ndc(v1.x);
            op_ius[i][3] = args.y_to_ndc(v1.y);
            op_ius[i][4] = args.x_to_ndc(v2.x);
            op_ius[i][5] = args.y_to_ndc(v2.y);

            *reinterpret_cast<kx::gfx::LinearColor*>(&op_ius[i][12]) = border_color;
            *reinterpret_cast<kx::gfx::LinearColor*>(&op_ius[i][16]) = inner_color_1;
            *reinterpret_cast<kx::gfx::LinearColor*>(&op_ius[i][20]) = inner_color_2;
        }
    }

    //wings
    {
        auto inner_color_1 = apply_color_mod(DEFAULT_WING_COLOR_1, args.cur_level_time);
        auto inner_color_2 = apply_color_mod(DEFAULT_WING_COLOR_2, args.cur_level_time);
        auto border_color = apply_color_mod(DEFAULT_WING_BORDER_COLOR, args.cur_level_time);

        auto rot_mat = Matrix2::make_rotation_matrix(movement_algo.get_current_angle());

        for(int i=6; i<8; i++) {
            int flip_mult = (i==7? 1: -1);

            auto v0 = MapVec(0.7*side_len, 0.2*flip_mult*side_len);
            auto d_v0_v1 = MapVec(-1.4*side_len, 0.0*flip_mult*side_len);
            auto d_v0_v2 = MapVec(-0.9*side_len, 0.4*flip_mult*side_len);

            auto wing_angle = flip_mult * 0.26 * std::sin(args.cur_level_time * 2*M_PI * wing_freq);
            auto wing_rot_mat = Matrix2::make_rotation_matrix(wing_angle);

            auto v1 = v0 + wing_rot_mat * d_v0_v1;
            auto v2 = v0 + wing_rot_mat * d_v0_v2;

            auto v0_rot = current_position + rot_mat * v0;
            auto v1_rot = current_position + rot_mat * v1;
            auto v2_rot = current_position + rot_mat * v2;

            op_ius[i][0] = args.x_to_ndc(v0_rot.x);
            op_ius[i][1] = args.y_to_ndc(v0_rot.y);
            op_ius[i][2] = args.x_to_ndc(v1_rot.x);
            op_ius[i][3] = args.y_to_ndc(v1_rot.y);
            op_ius[i][4] = args.x_to_ndc(v2_rot.x);
            op_ius[i][5] = args.y_to_ndc(v2_rot.y);

            *reinterpret_cast<kx::gfx::LinearColor*>(&op_ius[i][12]) = border_color;
            *reinterpret_cast<kx::gfx::LinearColor*>(&op_ius[i][16]) = inner_color_1;
            *reinterpret_cast<kx::gfx::LinearColor*>(&op_ius[i][20]) = inner_color_2;
        }
    }

    args.add_op_group(op_group);
}
double Hexfly_1::get_collision_damage() const
{
    return 2.0;
}

}}
