#include "geo2/map_obj/unit_type1/pig_1.h"
#include "geo2/map_obj/map_obj_args.h"

#include <random>

namespace geo2 { namespace map_obj {

Pig_1::Pig_1(MapCoord position_):
    Unit_Type1(Team::Enemy, position_, 6.0)
{}

const kx::gfx::LinearColor DEFAULT_INNER_COLOR(1.0f, 0.3f, 0.3f, 1.0f);
const kx::gfx::LinearColor DEFAULT_BORDER_COLOR(0.0f, 0.0f, 0.0f, 1.0f);
constexpr float BORDER_THICKNESS = 0.1f;

constexpr float PART_W[]{2.5f, 1.0f, 0.375f};
constexpr float PART_H[]{2.0f, 1.5f, 0.5f};
constexpr float X_BEGIN = -2.0f;
constexpr MapVec v0[]{{X_BEGIN, -0.5f*PART_H[0]},
                      {X_BEGIN + PART_W[0], -0.5f*PART_H[1]},
                      {X_BEGIN + PART_W[0] + PART_W[1], -0.5f*PART_H[2]}};

//starts at the bottom left and goes CCW
const std::unique_ptr<Polygon> BASE_SHAPE = Polygon::make(
    std::vector<float>{v0[0].x, v0[0].y + PART_H[0],
                       v0[1].x, v0[0].y + PART_H[0],
                       v0[1].x, v0[1].y + PART_H[1],
                       v0[2].x, v0[1].y + PART_H[1],
                       v0[2].x, v0[2].y + PART_H[2],
                       v0[2].x + PART_W[2], v0[2].y + PART_H[2],
                       v0[2].x + PART_W[2], v0[2].y,
                       v0[2].x, v0[2].y,
                       v0[2].x, v0[1].y,
                       v0[1].x, v0[1].y,
                       v0[1].x, v0[0].y,
                       v0[0].x, v0[0].y});

void Pig_1::init(const MapObjInitArgs &args)
{
    args.add_current_pos(Polygon::make_with_num_sides(12));
    args.add_desired_pos(Polygon::make_with_num_sides(12));
}

constexpr double MAX_SPEED = 13;
constexpr double MAX_ANGULAR_SPEED = 8;
constexpr double MAX_ACCEL = 90;
constexpr double MAX_ANGULAR_ACCEL = 40;

void Pig_1::run1_mt([[maybe_unused]] const MapObjRun1Args &args)
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
    cur->copy_from(BASE_SHAPE.get());
    cur->rotate_about_origin(movement_algo.get_current_angle());
    cur->translate(current_position.x, current_position.y);

    auto des = args.get_sole_desired_pos();
    des->copy_from(BASE_SHAPE.get());
    des->rotate_about_origin(movement_algo.get_desired_angle());
    auto desired_position = movement_algo.get_desired_position();
    des->translate(desired_position.x, desired_position.y);

    args.set_move_intent(MoveIntent::GoToDesiredPos);
}
void Pig_1::run3_mt([[maybe_unused]] const MapObjRun3Args &args)
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

void Pig_1::add_render_ops(const MapObjRenderArgs &args)
{
    if(op_group == nullptr) {
        op_group = std::make_shared<RenderOpGroup>(args.get_NPC_render_priority());
        for(int i=0; i<5; i++) {
            ops[i] = std::make_shared<RenderOpShader>(*args.shaders->pig_1);
            op_group->add_op(ops[i]);
            auto iu_map = ops[i]->map_instance_uniform(0);
            op_ius[i] = {(float*)iu_map.begin(), (float*)iu_map.end()};
        }

        auto adjusted_border_thickness = args.to_whole_pixels(BORDER_THICKNESS);

        for(int i=0; i<3; i++) {
            op_ius[i][8] = -0.5f * PART_W[i];
            op_ius[i][9] = -0.5f * PART_H[i];
            op_ius[i][10] = 0.5f * PART_W[i];
            op_ius[i][11] = 0.5f * PART_H[i];

            op_ius[i][6] = op_ius[i][10] - adjusted_border_thickness;
            op_ius[i][7] = op_ius[i][11] - adjusted_border_thickness;
        }

        //cheesy workaround to prevent the head and snout from having a left border
        for(int i=1; i<=2; i++) {
            op_ius[i][10] = 1.5f * PART_W[i];
            op_ius[i][6] = op_ius[i][10] - 2.0f*adjusted_border_thickness;
        }

        //eyes
        eye_w = args.to_whole_pixels(0.21f);
        eye_h = args.to_whole_pixels(0.21f);
        eye_x_offset = args.to_whole_pixels(0.6f);
        eye_y_offset = args.to_whole_pixels(0.23f);
        for(int i=3; i<=4; i++) {
            op_ius[i][8] = -0.5f * eye_w;
            op_ius[i][9] = -0.5f * eye_h;
            op_ius[i][10] = 0.5f * eye_w;
            op_ius[i][11] = 0.5f * eye_h;

            //the entire eye is the border color (any number <0 should suffice)
            op_ius[i][6] = -1.0f;
            op_ius[i][7] = -1.0f;
        }

        auto text = std::make_shared<RenderOpText>();

        text->set_text("The quick brown fox jumps over the lazy dog");
        text->set_x(500);
        text->set_y(500);
        text->set_font_size(64);
        text->set_font(args.fonts->default_.get());
        text->set_color(kx::gfx::LinearColor(0, 0, 8, 1));
        op_group->add_op(text);
    }

    auto rot_mat = Matrix2::make_rotation_matrix(movement_algo.get_current_angle());
    for(int i=0; i<=2; i++) {
        auto v0_rot = current_position + rot_mat * (v0[i]);
        auto v1_rot = current_position + rot_mat * (v0[i] + MapVec(PART_W[i], 0));
        auto v2_rot = current_position + rot_mat * (v0[i] + MapVec(0, PART_H[i]));

        op_ius[i][0] = args.x_to_ndc(v0_rot.x);
        op_ius[i][1] = args.y_to_ndc(v0_rot.y);
        op_ius[i][2] = args.x_to_ndc(v1_rot.x);
        op_ius[i][3] = args.y_to_ndc(v1_rot.y);
        op_ius[i][4] = args.x_to_ndc(v2_rot.x);
        op_ius[i][5] = args.y_to_ndc(v2_rot.y);
    }

    //upper eye
    {
        MapVec eye_v0(X_BEGIN + PART_W[0] + eye_x_offset, -eye_y_offset);
        auto v0_rot = current_position + rot_mat * (eye_v0);
        auto v1_rot = current_position + rot_mat * (eye_v0 + MapVec(eye_w, 0));
        auto v2_rot = current_position + rot_mat * (eye_v0 + MapVec(0, -eye_h));

        op_ius[3][0] = args.x_to_ndc(v0_rot.x);
        op_ius[3][1] = args.y_to_ndc(v0_rot.y);
        op_ius[3][2] = args.x_to_ndc(v1_rot.x);
        op_ius[3][3] = args.y_to_ndc(v1_rot.y);
        op_ius[3][4] = args.x_to_ndc(v2_rot.x);
        op_ius[3][5] = args.y_to_ndc(v2_rot.y);
    }

    //lower eye
    {
        MapVec eye_v0(X_BEGIN + PART_W[0] + eye_x_offset, eye_y_offset);
        auto v0_rot = current_position + rot_mat * (eye_v0);
        auto v1_rot = current_position + rot_mat * (eye_v0 + MapVec(eye_w, 0));
        auto v2_rot = current_position + rot_mat * (eye_v0 + MapVec(0, eye_h));

        op_ius[4][0] = args.x_to_ndc(v0_rot.x);
        op_ius[4][1] = args.y_to_ndc(v0_rot.y);
        op_ius[4][2] = args.x_to_ndc(v1_rot.x);
        op_ius[4][3] = args.y_to_ndc(v1_rot.y);
        op_ius[4][4] = args.x_to_ndc(v2_rot.x);
        op_ius[4][5] = args.y_to_ndc(v2_rot.y);
    }

    auto inner_color = apply_color_mod(DEFAULT_INNER_COLOR, args.cur_level_time);
    auto border_color = apply_color_mod(DEFAULT_BORDER_COLOR, args.cur_level_time);
    for(int i=0; i<5; i++) {
        *reinterpret_cast<kx::gfx::LinearColor*>(&op_ius[i][12]) = inner_color;
        *reinterpret_cast<kx::gfx::LinearColor*>(&op_ius[i][16]) = border_color;
    }

    args.add_op_group(op_group);
}
double Pig_1::get_collision_damage() const
{
    return 3.0;
}

}}
