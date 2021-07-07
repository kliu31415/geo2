#include "geo2/map_obj/unit_type1/unit_type1.h"
#include "geo2/map_obj/map_obj_args.h"

namespace geo2 { namespace map_obj {

Unit_Type1::Unit_Type1(Team team_, MapCoord position_, double collision_damage_):
    current_position(position_),
    collision_damage(collision_damage_),
    team(team_)
{}
void Unit_Type1::handle_collision(MapObject *other, const HandleCollisionArgs &args) const
{
    other->handle_collision(*this, args);
}
void Unit_Type1::handle_collision([[maybe_unused]] const Wall_Type1 &other,
                                  [[maybe_unused]] const HandleCollisionArgs &args)
{
    args.set_move_intent(MoveIntent::StayAtCurrentPos);
}
void Unit_Type1::handle_collision([[maybe_unused]] const Unit_Type1 &other,
                                  [[maybe_unused]] const HandleCollisionArgs &args)
{
    args.set_move_intent(MoveIntent::StayAtCurrentPos);
    //deal damage? apply effects?
}
void Unit_Type1::handle_collision([[maybe_unused]] const Projectile_Type1 &other,
                                  [[maybe_unused]] const HandleCollisionArgs &args)
{
    //deal damage? apply effects?
    //move_intent stays the same
}
MapCoord Unit_Type1::get_position() const
{
    return current_position;
}

}}
