#include "geo2/map_obj/unit_type1/unit_type1.h"
#include "geo2/map_obj/projectile_type1/projectile_type1.h"
#include "geo2/map_obj/map_obj_args.h"

namespace geo2 { namespace map_obj {

using kx::gfx::LinearColor;

constexpr double COLLISION_DAMAGE_COLOR_LEN = 0.5;
constexpr double DEATH_TIME_FADE_LEN = 0.5;
LinearColor Unit_Type1::apply_color_mod(const LinearColor &color, double cur_level_time) const
{

    double t = 1 - std::min(1.0, (cur_level_time - last_damaged_at_time) / COLLISION_DAMAGE_COLOR_LEN);

    ///note that t is in [0, 1]
    auto ret = LinearColor(0.8*t * (color.g + color.b) + color.r,
                           (1 - 0.4*t) * color.g,
                           (1 - 0.4*t) * color.b,
                           color.a);

    if(alive_status.is_dead()) {
        //it'd be weird for a fully faded object to not be deleted
        k_expects(cur_level_time - death_time <= DEATH_TIME_FADE_LEN);
        ret.a *= 1 - (cur_level_time - death_time) / DEATH_TIME_FADE_LEN;
    }

    return ret;
}

Unit_Type1::Unit_Type1(Team team_, MapCoord position_, double health_):
    last_damaged_at_time(-1e9),
    current_position(position_),
    team(team_),
    health(health_)
{}

void Unit_Type1::handle_wall_collision([[maybe_unused]] Wall_Type1 *other,
                                       [[maybe_unused]] const HandleWallCollisionArgs &args)
{
    if(is_dead())
        return;

    args.set_move_intent(args.move_intent_upon_collision);
}
void Unit_Type1::handle_unit_collision(Unit_Type1 *other, const HandleUnitCollisionArgs &args)
{
    if(is_dead() || other->is_dead())
        return;

    args.set_move_intent(args.move_intent_upon_collision);

    if(!are_enemies(team, other->get_team()))
        return;

    auto other_in_map = last_collision_damage_time.find(other);
    if(other_in_map == last_collision_damage_time.end()) {
        last_collision_damage_time.emplace(other, args.cur_level_time);
        health -= other->get_collision_damage();
    } else {
        health -= other->get_collision_damage() *
                  std::min(COLLISION_DAMAGE_COLOR_LEN, args.cur_level_time - other_in_map->second);
        other_in_map->second = args.cur_level_time;
    }

    if(other->get_collision_damage() > 0) {
        last_damaged_at_time = args.cur_level_time;

        if(health <= 0) {
            alive_status.set_status_to_just_died();
            death_time = args.cur_level_time;
            args.set_move_intent(MoveIntent::RemoveShapes);
        }
    }
}
void Unit_Type1::handle_proj_collision(Projectile_Type1 *other, const HandleProjCollisionArgs &args)
{
    if(is_dead() || other->is_dead() || !are_enemies(get_team(), other->get_team()))
        return;

    auto damage = other->get_damage();
    if(damage > 0) {
        health -= damage;
        last_damaged_at_time = args.cur_level_time;

        if(health <= 0) {
            alive_status.set_status_to_just_died();
            death_time = args.cur_level_time;
            args.set_move_intent(MoveIntent::RemoveShapes);
        }
    }
}
void Unit_Type1::handle_collision(MapObject *other, const HandleCollisionArgs &args)
{
    other->handle_collision(this, args);
}
void Unit_Type1::handle_collision([[maybe_unused]] Wall_Type1 *other,
                                  [[maybe_unused]] const HandleCollisionArgs &args)
{
    HandleWallCollisionArgs new_args(args);

    if(args.get_move_intent() == MoveIntent::GoToDesiredPos)
        new_args.move_intent_upon_collision = MoveIntent::GoToDesiredPosIfOtherDoesntCollide;
    else if(args.get_move_intent() == MoveIntent::StayAtCurrentPos)
        new_args.move_intent_upon_collision = MoveIntent::StayAtCurrentPos;
    else if(args.get_move_intent() != MoveIntent::RemoveShapes)
        k_assert(false);

    handle_wall_collision(other, new_args);
}
void Unit_Type1::handle_collision([[maybe_unused]] Unit_Type1 *other,
                                  [[maybe_unused]] const HandleCollisionArgs &args)
{
    HandleUnitCollisionArgs new_args(args);

    if(args.get_move_intent() == MoveIntent::GoToDesiredPos)
        new_args.move_intent_upon_collision = MoveIntent::GoToDesiredPosIfOtherDoesntCollide;
    else if(args.get_move_intent() == MoveIntent::StayAtCurrentPos)
        new_args.move_intent_upon_collision = MoveIntent::StayAtCurrentPos;
    else if(args.get_move_intent() != MoveIntent::RemoveShapes)
        k_assert(false);

    handle_unit_collision(other, new_args);
}
void Unit_Type1::handle_collision(Projectile_Type1 *other,
                                  [[maybe_unused]] const HandleCollisionArgs &args)
{
    HandleProjCollisionArgs new_args(args);

    new_args.move_intent_upon_collision = args.get_move_intent();

    handle_proj_collision(other, new_args);
}
void Unit_Type1::end_handle_collision_block([[maybe_unused]] const EndHandleCollisionBlockArgs &args)
{
    alive_status.end_current_time_block();
}
double Unit_Type1::get_collision_damage() const
{
    return 0;
}
MapCoord Unit_Type1::get_position() const
{
    return current_position;
}
bool Unit_Type1::is_dead() const
{
    return alive_status.is_dead();
}
bool Unit_Type1::is_completely_faded_out(double cur_level_time) const
{
    k_expects(alive_status.is_dead());
    return cur_level_time - death_time > DEATH_TIME_FADE_LEN;
}
Team Unit_Type1::get_team() const
{
    return team;
}

}}
