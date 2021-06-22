#include "geo2/map_obj/unit_type1/player_type1.h"

#include <iostream>

namespace geo2 { namespace map_obj {

Player_Type1::Player_Type1():
    Unit_Type1(1, {}),
    left_pressed_tick_count(0),
    right_pressed_tick_count(0),
    up_pressed_tick_count(0),
    down_pressed_tick_count(0),
    velocity(MapVec(0, 0))
{

}

void Player_Type1::process_input(const PlayerInputArgs &args)
{
    auto &left_ptc = left_pressed_tick_count;
    auto &right_ptc = right_pressed_tick_count;
    auto &up_ptc = up_pressed_tick_count;
    auto &down_ptc = down_pressed_tick_count;

    //use keystate to update the key press tick counts
    if(args.left_pressed)
        left_ptc++;
    else
        left_ptc = 0;

    if(args.right_pressed)
        right_ptc++;
    else
        right_ptc = 0;

    if(args.up_pressed)
        up_ptc++;
    else
        up_ptc = 0;

    if(args.down_pressed)
        down_ptc++;
    else
        down_ptc = 0;

    //calculate intended horizontal and vertical input
    int horizontal_input;

    if(left_ptc>0 && (right_ptc==0 || left_ptc<right_ptc))
        horizontal_input = -1;
    else if(right_ptc>0 && (left_ptc==0 || right_ptc<left_ptc))
        horizontal_input = 1;
    else
        horizontal_input = 0;

    int vertical_input;

    if(up_ptc>0 && (down_ptc==0 || up_ptc<down_ptc))
        vertical_input = -1;
    else if(down_ptc>0 && (up_ptc==0 || down_ptc<up_ptc))
        vertical_input = 1;
    else
        vertical_input = 0;

    //Process raw input to get intended movement directions and move the player.
    //Note that if the player is moving right and either lets go of the
    //right key or presses the left key, he'll try to move left. This is
    //intended, as it helps the player stop moving faster. This also applies
    //to moving in other directions.
    double horizontal_dir;
    if(velocity.x>0 && horizontal_input<=0)
        horizontal_dir = -1;
    else if(velocity.x<0 && horizontal_input>=0)
        horizontal_dir = 1;
    else
        horizontal_dir = horizontal_input;

    double vertical_dir;
    if(velocity.y>0 && vertical_input<=0)
        vertical_dir = -1;
    else if(velocity.y<0 && vertical_input>=0)
        vertical_dir = 1;
    else
        vertical_dir = vertical_input;

    constexpr auto POWER = 120.0;
    constexpr auto MASS = 1.0;
    constexpr auto FRICTION_COEF = 4.0;
    constexpr auto GRAVITY = 9.80665;
    constexpr auto EPS = 1e-10;

    if(horizontal_dir!=0 || vertical_dir!=0) {
        double dir_norm = std::hypot(horizontal_dir, vertical_dir);
        vertical_dir /= dir_norm;
        horizontal_dir /= dir_norm;

        MapVec accel;
        accel.x = args.tick_len * FRICTION_COEF * horizontal_dir * POWER / MASS;
        accel.y = args.tick_len * FRICTION_COEF * vertical_dir * POWER / MASS;

        auto old_velocity = velocity;
        auto old_speed = old_velocity.norm();

        if(old_speed > EPS) {
            accel.x /= old_speed;
            accel.y /= old_speed;
        } else {
            accel.x /= EPS;
            accel.y /= EPS;
        }

        //F = W / v, aka Force = Power / velocity, but this results in a degenerate case
        //when v=0 or v is very small. Handle this manually by setting a max acceleration.
        //The max acceleration is set so that if you're at 0 speed, you accelerate so that
        //if you accelerated at this rate, your acceleration would be identical to your
        //acceleration X_s seconds from now. Picking X_s to roughly match the tick length
        //results in the most smooth acceleration, so X_s is set to 1ms. This effectively
        //means that if you're at rest, the zeroth and first ticks will have roughly the
        //same acceleration.
        constexpr auto X_S = 0.001;
        constexpr auto Q_a = X_S * MASS;
        constexpr auto Q_b = X_S * MASS * FRICTION_COEF * GRAVITY;
        constexpr auto Q_c = -FRICTION_COEF * POWER;
        constexpr auto MAX_ACCEL_PER_S = (-Q_b + std::sqrt(Q_b*Q_b - 4*Q_a*Q_c)) / (2 * Q_a);
        auto max_accel = MAX_ACCEL_PER_S * args.tick_len;

        auto accel_norm = accel.norm();
        if(accel_norm > max_accel) {
            accel.x *= max_accel / accel_norm;
            accel.y *= max_accel / accel_norm;
        }

        velocity.x += accel.x;
        velocity.y += accel.y;

        //if the player isn't holding any keys down, make sure not to wiggle
        //between small positive and negative velocities
        if(((velocity.x<0 && old_velocity.x>0) || (velocity.x>0 && old_velocity.x<0)) &&
           horizontal_input==0)
        {
            velocity.x = 0;
        }
        if(((velocity.y<0 && old_velocity.y>0) || (velocity.y>0 && old_velocity.y<0)) &&
           vertical_input==0)
        {
            velocity.y = 0;
        }
    }
    auto speed = velocity.norm();
    if(speed > EPS) {
        double friction_accel = args.tick_len * FRICTION_COEF * GRAVITY;
        auto scale_velocity = std::max(0.0, (speed - friction_accel) / speed);
        velocity.x *= scale_velocity;
        velocity.y *= scale_velocity;
    }
}
constexpr double PLAYER_SIDE_LEN = 1.6;
void Player_Type1::run1_mt(const MapObjRun1Args &args)
{
    constexpr auto PSL = PLAYER_SIDE_LEN;

    MapCoord cur_v[]{{position.x - 0.5*PSL, position.y - 0.5*PSL},
                     {position.x + 0.5*PSL, position.y - 0.5*PSL},
                     {position.x + 0.5*PSL, position.y + 0.5*PSL},
                     {position.x - 0.5*PSL, position.y + 0.5*PSL}};
    auto cur_span = nonstd::span<MapCoord>(std::begin(cur_v), std::end(cur_v));

    cur_shape = Polygon::make(cur_span);

    args.add_current_pos(cur_shape.get());

    if(velocity.x!=0 || velocity.y!=0) {
        move_intent = MoveIntent::GoToDesiredPos;
        desired_position.x = position.x + velocity.x * args.tick_len;
        desired_position.y = position.y + velocity.y * args.tick_len;

        MapCoord des_v[]{{desired_position.x - 0.5*PSL, desired_position.y - 0.5*PSL},
                         {desired_position.x + 0.5*PSL, desired_position.y - 0.5*PSL},
                         {desired_position.x + 0.5*PSL, desired_position.y + 0.5*PSL},
                         {desired_position.x - 0.5*PSL, desired_position.y + 0.5*PSL}};

        auto des_span = nonstd::span<MapCoord>(std::begin(des_v), std::end(des_v));
        des_shape = Polygon::make(des_span);
        args.add_desired_pos(des_shape.get());
    } else {
        move_intent = MoveIntent::StayAtCurrentPos;
    }
    args.set_move_intent(move_intent);
}
void Player_Type1::run2_st([[maybe_unused]] const MapObjRun2Args &args)
{
    if(move_intent == MoveIntent::GoToDesiredPos) {
        position = desired_position;
    }
}
void Player_Type1::add_render_objs(const MapObjRenderArgs &args)
{
    if(op1 == nullptr) {
        op_group = std::make_shared<RenderOpGroup>(3000.0);
        op1 = std::make_shared<RenderOpShader>(*args.shaders->outlined_tri);
        op_group->add_op(op1);
        op2 = std::make_shared<RenderOpShader>(*args.shaders->outlined_tri);
        op_group->add_op(op2);
        auto iu_map = op1->map_instance_uniform(0);
        op_iu1 = {(float*)iu_map.begin(), (float*)iu_map.end()};
        iu_map = op2->map_instance_uniform(0);
        op_iu2 = {(float*)iu_map.begin(), (float*)iu_map.end()};

        op_iu1[8] = 0.07f;
        op_iu1[9] = -1.0f;
        op_iu1[10] = 0.07f;

        op_iu1[12] = 0.2f;
        op_iu1[13] = 0.3f;
        op_iu1[14] = 0.6f;
        op_iu1[15] = 0.9f;

        op_iu1[16] = 0.05f;
        op_iu1[17] = 0.05f;
        op_iu1[18] = 0.05f;
        op_iu1[19] = 0.9f;

        op_iu2[8] = 0.07f;
        op_iu2[9] = -1.0f;
        op_iu2[10] = 0.07f;

        op_iu2[12] = 0.2f;
        op_iu2[13] = 0.3f;
        op_iu2[14] = 0.6f;
        op_iu2[15] = 0.9f;

        op_iu2[16] = 0.05f;
        op_iu2[17] = 0.05f;
        op_iu2[18] = 0.05f;
        op_iu2[19] = 0.9f;
    }

    op_iu1[0] = args.x_to_ndc(position.x - 0.5*PLAYER_SIDE_LEN);
    op_iu1[1] = args.y_to_ndc(position.y + 0.5*PLAYER_SIDE_LEN);
    op_iu1[2] = args.x_to_ndc(position.x - 0.5*PLAYER_SIDE_LEN);
    op_iu1[3] = args.y_to_ndc(position.y - 0.5*PLAYER_SIDE_LEN);
    op_iu1[4] = args.x_to_ndc(position.x + 0.5*PLAYER_SIDE_LEN);
    op_iu1[5] = args.y_to_ndc(position.y - 0.5*PLAYER_SIDE_LEN);

    op_iu2[0] = op_iu1[0];
    op_iu2[1] = op_iu1[1];
    op_iu2[2] = args.x_to_ndc(position.x + 0.5*PLAYER_SIDE_LEN);
    op_iu2[3] = args.y_to_ndc(position.y + 0.5*PLAYER_SIDE_LEN);
    op_iu2[4] = op_iu1[4];
    op_iu2[5] = op_iu1[5];

    args.add_op_group(op_group);
}
void Player_Type1::set_position(MapCoord pos, kx::Passkey<Game>)
{
    position = pos;
}

}}
