#include "geo2/map_objs/projectile_type1/projectile_type1.h"

namespace geo2 { namespace map_obj {

MoveIntent Projectile_Type1::handle_collision(MapObject *other, const HandleCollisionArgs &args) const
{
    return other->handle_collision(*this, args);
}
MoveIntent Projectile_Type1::handle_collision([[maybe_unused]] const Wall_Type1 &other,
                                              [[maybe_unused]] const HandleCollisionArgs &args)
{
    return MoveIntent::StayAtCurrentPos;
}
MoveIntent Projectile_Type1::handle_collision([[maybe_unused]] const Unit_Type1 &other,
                                              [[maybe_unused]] const HandleCollisionArgs &args)
{
    return MoveIntent::StayAtCurrentPos;
}
MoveIntent Projectile_Type1::handle_collision([[maybe_unused]] const Projectile_Type1 &other,
                                              [[maybe_unused]] const HandleCollisionArgs &args)
{
    return MoveIntent::StayAtCurrentPos;
}

}}
