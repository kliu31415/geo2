#include "geo2/map_obj/unit_type1/pig_1.h"

namespace geo2 { namespace map_obj {

Pig_1::Pig_1(MapCoord position_):
    Unit_Type1(2, position_, 2.0)
{}

void Pig_1::run1_mt([[maybe_unused]] const MapObjRun1Args &args)
{

}

MoveIntent Pig_1::handle_collision([[maybe_unused]] const Wall_Type1 &other,
                                   [[maybe_unused]] const HandleCollisionArgs &args)
{
    move_intent = MoveIntent::StayAtCurrentPos;
    return MoveIntent::StayAtCurrentPos;
}
MoveIntent Pig_1::handle_collision([[maybe_unused]] const Unit_Type1 &other,
                                   [[maybe_unused]] const HandleCollisionArgs &args)
{
    move_intent = MoveIntent::StayAtCurrentPos;
    //deal damage? apply effects?
    return MoveIntent::StayAtCurrentPos;
}
MoveIntent Pig_1::handle_collision([[maybe_unused]] const Projectile_Type1 &other,
                                   [[maybe_unused]] const HandleCollisionArgs &args)
{
    //deal damage? apply effects?
    return move_intent;
}
void Pig_1::add_render_objs(const MapObjRenderArgs &args)
{
    if(op == nullptr) {
        op_group = std::make_shared<RenderOpGroup>(2000.0);
        op = std::make_shared<RenderOpShader>(*args.shaders->pig_1);
        op_group->add_op(op);

        auto iu_map = op->map_instance_uniform(0);
        op_iu = {(float*)iu_map.begin(), (float*)iu_map.end()};

        //interior color
        op_iu[4] = 1.0f;
        op_iu[5] = 0.25f;
        op_iu[6] = 0.3f;
        op_iu[7] = 1.0f;

        //border color
        op_iu[8] = 0.0f;
        op_iu[9] = 0.0f;
        op_iu[10] = 0.0f;
        op_iu[11] = 1.0f;

        //eye color
        op_iu[12] = 0.02f;
        op_iu[13] = 0.02f;
        op_iu[14] = 0.02f;
        op_iu[15] = 1.0f;

        //rotation center
        op_iu[18] = 0.5f;
        op_iu[19] = 0.5f;

        //eye center
        op_iu[20] = 0.85f;
        op_iu[21] = 0.4f;

        //eye radius squared
        op_iu[22] = std::pow(0.15f, 2);

        //border thickness
        op_iu[23] = 0.03f;

        //pig shape
        op_iu[28] = 0.0f;
        op_iu[29] = 0.8f;

        op_iu[30] = 0.3f;
        op_iu[31] = 1.0f;

        op_iu[32] = 0.65f;
        op_iu[33] = 1.0f;

        op_iu[34] = 1.0f;
        op_iu[35] = 0.7f;
    }


    constexpr float PIG_W = 3.5f;
    constexpr float PIG_H = 2.0f;

    //position
    op_iu[0] = args.x_to_ndc(position.x - 0.5f*PIG_W);
    op_iu[1] = args.y_to_ndc(position.y - 0.5f*PIG_H);
    op_iu[2] = std::fabs(args.x_to_ndc(position.x + 0.5f*PIG_W) - op_iu[0]);
    op_iu[3] = std::fabs(args.y_to_ndc(position.y + 0.5f*PIG_H) - op_iu[1]);

    //rotation values
    op_iu[16] = 1.0f;
    op_iu[17] = 0.0f;

    op_iu[24] = PIG_W;
    op_iu[25] = PIG_H;

    args.add_op_group(op_group);
}

}}
