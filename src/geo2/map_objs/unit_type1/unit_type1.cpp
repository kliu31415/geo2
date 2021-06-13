#include "geo2/map_objs/unit_type1/unit_type1.h"

namespace geo2 { namespace map_obj {

Unit_Type1::Unit_Type1(int team_, MapCoord position_):
    team(team_),
    position(position_),
    velocity(0, 0)
{}
MoveIntent Unit_Type1::handle_collision(MapObject *other, const HandleCollisionArgs &args) const
{
    return other->handle_collision(*this, args);
}
MoveIntent Unit_Type1::handle_collision([[maybe_unused]] const Wall_Type1 &other,
                                        [[maybe_unused]] const HandleCollisionArgs &args)
{
    move_intent = MoveIntent::StayAtCurrentPos;
    return MoveIntent::StayAtCurrentPos;
}
MoveIntent Unit_Type1::handle_collision([[maybe_unused]] const Unit_Type1 &other,
                                        [[maybe_unused]] const HandleCollisionArgs &args)
{
    move_intent = MoveIntent::StayAtCurrentPos;
    //deal damage? apply effects?
    return MoveIntent::StayAtCurrentPos;
}
MoveIntent Unit_Type1::handle_collision([[maybe_unused]] const Projectile_Type1 &other,
                                        [[maybe_unused]] const HandleCollisionArgs &args)
{
    //deal damage? apply effects?
    return move_intent;
}
MapCoord Unit_Type1::get_pos() const
{
    return position;
}

}}
