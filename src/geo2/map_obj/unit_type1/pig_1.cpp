#include "geo2/map_obj/unit_type1/pig_1.h"
#include "geo2/map_obj/map_obj_args.h"

namespace geo2 { namespace map_obj {

Pig_1::Pig_1(MapCoord position_):
    Unit_Type1(Team::Enemy, position_, 2.0)
{}

const kx::gfx::LinearColor INNER_COLOR(1.0f, 0.3f, 0.3f, 1.0f);
const kx::gfx::LinearColor BORDER_COLOR(0.0f, 0.0f, 0.0f, 1.0f);
constexpr float BORDER_THICKNESS = 0.1f;

constexpr float PART_W[]{2.0f, 0.7f, 0.3f, 0.15f, 0.15f};
constexpr float PART_H[]{1.5f, 1.0f, 0.4f, 0.15f, 0.15f};
constexpr MapVec v0[]{{-1.5f, -0.75f},
                      {0.5f - BORDER_THICKNESS, -0.5f},
                      {1.2f - BORDER_THICKNESS*2, -0.2f},
                      {0.85f - BORDER_THICKNESS, -0.3f},
                      {0.85f - BORDER_THICKNESS, 0.15f}};

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
void Pig_1::run1_mt([[maybe_unused]] const MapObjRun1Args &args)
{
    desired_position = current_position;

    auto cur = args.get_sole_current_pos();
    cur->copy_from(BASE_SHAPE.get());
    //cur->rotate_about_origin(0.7);
    cur->translate(current_position.x, current_position.y);

    auto des = args.get_sole_desired_pos();
    des->copy_from(BASE_SHAPE.get());
    //des->rotate_about_origin(0.7);
    des->translate(desired_position.x, desired_position.y);

    args.set_move_intent(MoveIntent::GoToDesiredPos);
}

void Pig_1::handle_collision([[maybe_unused]] const Wall_Type1 &other,
                             [[maybe_unused]] const HandleCollisionArgs &args)
{
    args.set_move_intent(MoveIntent::StayAtCurrentPos);
}
void Pig_1::handle_collision([[maybe_unused]] const Unit_Type1 &other,
                             [[maybe_unused]] const HandleCollisionArgs &args)
{
    args.set_move_intent(MoveIntent::StayAtCurrentPos);
    //deal damage? apply effects?
}
void Pig_1::handle_collision([[maybe_unused]] const Projectile_Type1 &other,
                             [[maybe_unused]] const HandleCollisionArgs &args)
{
    //deal damage? apply effects?
    //move_intent remains unchanged
}

void Pig_1::add_render_objs(const MapObjRenderArgs &args)
{
    if(op_group == nullptr) {
        op_group = std::make_shared<RenderOpGroup>(args.get_NPC_render_priority());
        for(int i=0; i<5; i++) {
            ops[i] = std::make_shared<RenderOpShader>(*args.get_shaders()->pig_1);
            op_group->add_op(ops[i]);
            auto iu_map = ops[i]->map_instance_uniform(0);
            op_ius[i] = {(float*)iu_map.begin(), (float*)iu_map.end()};

            *reinterpret_cast<kx::gfx::LinearColor*>(&op_ius[i][12]) = INNER_COLOR;
            *reinterpret_cast<kx::gfx::LinearColor*>(&op_ius[i][16]) = BORDER_COLOR;
        }

        for(int i=0; i<5; i++) {
            op_ius[i][8] = -0.5f * PART_W[i];
            op_ius[i][9] = -0.5f * PART_H[i];
            op_ius[i][10] = 0.5f * PART_W[i];
            op_ius[i][11] = 0.5f * PART_H[i];

            op_ius[i][6] = op_ius[i][10] - BORDER_THICKNESS;
            op_ius[i][7] = op_ius[i][11] - BORDER_THICKNESS;
        }

        //the entire eye is the border color
        for(int i=3; i<=4; i++) {
            op_ius[i][6] = -1;
            op_ius[i][7] = -1;
        }
    }

    auto rot_mat = Matrix2::make_rotation_matrix(0.0f);
    for(int i=0; i<5; i++) {
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

    args.add_op_group(op_group);
}

}}
