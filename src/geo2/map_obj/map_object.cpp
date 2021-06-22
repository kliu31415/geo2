#include "geo2/map_obj/map_object.h"

#include "kx/io.h"

#include <cmath>

namespace geo2 { namespace map_obj {

//MapObject
void MapObject::run1_mt([[maybe_unused]] const MapObjRun1Args &args) {}
void MapObject::run2_st([[maybe_unused]] const MapObjRun2Args &args) {}
void MapObject::add_render_objs([[maybe_unused]] const MapObjRenderArgs &args) {}
bool MapObject::collision_could_matter([[maybe_unused]] const MapObject &other) const
{
    return true;
}
MoveIntent MapObject::handle_collision([[maybe_unused]] const CosmeticMapObj &other,
                                       [[maybe_unused]] const HandleCollisionArgs &args)
{
    return MoveIntent::StayAtCurrentPos;
}

//CosmeticMapObj
MoveIntent CosmeticMapObj::handle_collision(MapObject *other, const HandleCollisionArgs &args) const
{
    return other->handle_collision(*this, args);
}
MoveIntent CosmeticMapObj::handle_collision([[maybe_unused]] const Wall_Type1 &other,
                                            [[maybe_unused]] const HandleCollisionArgs &args)
{
    return MoveIntent::StayAtCurrentPos;
}
MoveIntent CosmeticMapObj::handle_collision([[maybe_unused]] const Unit_Type1 &other,
                                            [[maybe_unused]] const HandleCollisionArgs &args)
{
    return MoveIntent::StayAtCurrentPos;
}
MoveIntent CosmeticMapObj::handle_collision([[maybe_unused]] const Projectile_Type1 &other,
                                            [[maybe_unused]] const HandleCollisionArgs &args)
{
    return MoveIntent::StayAtCurrentPos;
}

}}
