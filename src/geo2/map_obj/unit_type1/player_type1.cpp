#include "geo2/map_obj/unit_type1/player_type1.h"
#include "geo2/map_obj/map_obj_args.h"
#include "geo2/weapon/laser_1.h"

#include <iostream>

namespace geo2 { namespace map_obj {

Player_Type1::Player_Type1():
    Unit_Type1(Team::Ally, {}, 20.0),
    velocity(MapVec(0, 0)),
    left_pressed_tick_count(0),
    right_pressed_tick_count(0),
    up_pressed_tick_count(0),
    down_pressed_tick_count(0),
    max_mana(20.0),
    mana(20.0)
{

}
void Player_Type1::start_new_level(MapCoord pos, kx::Passkey<Game>)
{
    current_position = pos;
    weapons[cur_weapon_idx]->start_new_level({});
}
void Player_Type1::run_special(const PlayerRunSpecialArgs &args, kx::Passkey<Game>)
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

    k_expects(cur_weapon_idx < (int)weapons.size());
    weapons[cur_weapon_idx]->run(args.weapon_run_args);
    weapon_angle = args.weapon_run_args.angle;
}
void Player_Type1::init([[maybe_unused]] const MapObjInitArgs &args)
{
    args.add_current_pos_polygon_with_num_sides(4);
    args.add_desired_pos_polygon_with_num_sides(4);

    try {
        weapons.push_back(std::make_shared<weapon::TestLaser1>(shared_from_this()));
    } catch(std::bad_weak_ptr &e) {
        k_assert(false);
    }
    weapon::WeaponSwapInArgs swap_in_args;
    weapons.back()->swap_in(swap_in_args);
    cur_weapon_idx = 0;
}
constexpr double PLAYER_SIDE_LEN = 1.6;
void Player_Type1::run1_mt(const MapObjRun1Args &args)
{
    constexpr auto PSL = PLAYER_SIDE_LEN;

    MapCoord cur_v[]{{current_position.x - 0.5*PSL, current_position.y - 0.5*PSL},
                     {current_position.x + 0.5*PSL, current_position.y - 0.5*PSL},
                     {current_position.x + 0.5*PSL, current_position.y + 0.5*PSL},
                     {current_position.x - 0.5*PSL, current_position.y + 0.5*PSL}};
    auto cur_span = kx::kx_span<MapCoord>(std::begin(cur_v), std::end(cur_v));

    args.get_sole_current_pos()->remake(cur_span);

    if(velocity.x!=0 || velocity.y!=0) {
        args.set_move_intent(MoveIntent::GoToDesiredPos);
        desired_position.x = current_position.x + velocity.x * args.tick_len;
        desired_position.y = current_position.y + velocity.y * args.tick_len;

        MapCoord des_v[]{{desired_position.x - 0.5*PSL, desired_position.y - 0.5*PSL},
                         {desired_position.x + 0.5*PSL, desired_position.y - 0.5*PSL},
                         {desired_position.x + 0.5*PSL, desired_position.y + 0.5*PSL},
                         {desired_position.x - 0.5*PSL, desired_position.y + 0.5*PSL}};

        auto des_span = kx::kx_span<MapCoord>(std::begin(des_v), std::end(des_v));
        args.get_sole_desired_pos()->remake(des_span);
    } else {
        args.set_move_intent(MoveIntent::StayAtCurrentPos);
    }
}
void Player_Type1::run3_mt([[maybe_unused]] const MapObjRun3Args &args)
{
    if(args.get_move_intent() == MoveIntent::GoToDesiredPos) {
        current_position = desired_position;
    }
}

constexpr float WEAPON_OFFSET = 0.5f;

using kx::gfx::LinearColor;
const LinearColor DEFAULT_INSIDE_COLOR(0.2f, 0.3f, 0.6f, 0.9f);
const LinearColor DEFAULT_OUTSIDE_COLOR(0.05f, 0.05f, 0.05f, 0.9f);

void Player_Type1::add_render_ops(const MapObjRenderArgs &args)
{
    if(op1 == nullptr) {
        op_group = std::make_shared<RenderOpGroup>(args.get_player_render_priority());
        op1 = std::make_shared<RenderOpShader>(*args.shaders->get("outlined_tri"));
        op2 = std::make_shared<RenderOpShader>(*args.shaders->get("outlined_tri"));
        auto iu_map = op1->map_instance_uniform(0);
        op_iu1 = {(float*)iu_map.begin(), (float*)iu_map.end()};
        iu_map = op2->map_instance_uniform(0);
        op_iu2 = {(float*)iu_map.begin(), (float*)iu_map.end()};

        float BORDER_SIZE = args.to_whole_pixels(0.07f * PLAYER_SIDE_LEN) / PLAYER_SIDE_LEN;

        op_iu1[8] = BORDER_SIZE;
        op_iu1[9] = -1.0f;
        op_iu1[10] = BORDER_SIZE;

        op_iu2[8] = BORDER_SIZE;
        op_iu2[9] = -1.0f;
        op_iu2[10] = BORDER_SIZE;
    }

    auto inside_color = apply_color_mod(DEFAULT_INSIDE_COLOR, args.cur_level_time);
    auto outside_color = apply_color_mod(DEFAULT_OUTSIDE_COLOR, args.cur_level_time);
    *reinterpret_cast<LinearColor*>(&op_iu1[12]) = inside_color;
    *reinterpret_cast<LinearColor*>(&op_iu1[16]) = outside_color;
    *reinterpret_cast<LinearColor*>(&op_iu2[12]) = inside_color;
    *reinterpret_cast<LinearColor*>(&op_iu2[16]) = outside_color;

    op_iu1[0] = args.x_to_ndc(current_position.x - 0.5*PLAYER_SIDE_LEN);
    op_iu1[1] = args.y_to_ndc(current_position.y + 0.5*PLAYER_SIDE_LEN);
    op_iu1[2] = args.x_to_ndc(current_position.x - 0.5*PLAYER_SIDE_LEN);
    op_iu1[3] = args.y_to_ndc(current_position.y - 0.5*PLAYER_SIDE_LEN);
    op_iu1[4] = args.x_to_ndc(current_position.x + 0.5*PLAYER_SIDE_LEN);
    op_iu1[5] = args.y_to_ndc(current_position.y - 0.5*PLAYER_SIDE_LEN);

    op_iu2[0] = op_iu1[0];
    op_iu2[1] = op_iu1[1];
    op_iu2[2] = args.x_to_ndc(current_position.x + 0.5*PLAYER_SIDE_LEN);
    op_iu2[3] = args.y_to_ndc(current_position.y + 0.5*PLAYER_SIDE_LEN);
    op_iu2[4] = op_iu1[4];
    op_iu2[5] = op_iu1[5];

    args.add_op_group(op_group);
    op_group->clear();
    op_group->add_op(op1);
    op_group->add_op(op2);

    weapon::WeaponRenderArgs weapon_render_args;
    weapon_render_args.set_render_args((RenderArgs)args);
    weapon_render_args.set_op_group(op_group.get());
    weapon_render_args.angle = weapon_angle;
    weapon_render_args.render_priority = args.get_player_render_priority();

    weapons[cur_weapon_idx]->render(weapon_render_args);
}
double Player_Type1::get_collision_damage() const
{
    return 2.0;
}
weapon::WeaponOwnerInfo Player_Type1::get_weapon_owner_info()
{
    weapon::WeaponOwnerInfo ret;
    ret.position = current_position;
    ret.offset_from_center = WEAPON_OFFSET;
    ret.set_mana_cost_multiplier(1);
    ret.set_cooldown_speed_multiplier(1);
    ret.set_mana(&mana);
    ret.set_max_mana(max_mana);
    return ret;
}
double Player_Type1::get_max_mana() const
{
    return max_mana;
}
double Player_Type1::get_mana() const
{
    return mana;
}

}}
