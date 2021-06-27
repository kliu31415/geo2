#include "geo2/map_obj/map_object.h"
#include "geo2/map_obj/map_obj_args.h"

#include "kx/io.h"

#include <cmath>

namespace geo2 { namespace map_obj {

//MapObject
void MapObject::init([[maybe_unused]] const MapObjInitArgs &args) {}
void MapObject::run1_mt([[maybe_unused]] const MapObjRun1Args &args) {}
void MapObject::run2_st([[maybe_unused]] const MapObjRun2Args &args) {}
void MapObject::add_render_objs([[maybe_unused]] const MapObjRenderArgs &args) {}

bool MapObject::collision_could_matter([[maybe_unused]] const MapObject &other) const
{
    return true;
}
void MapObject::handle_collision([[maybe_unused]] const CosmeticMapObj &other,
                                 const HandleCollisionArgs &args)
{
    args.set_move_intent(MoveIntent::StayAtCurrentPos);
}

//CosmeticMapObj
void CosmeticMapObj::handle_collision(MapObject *other, const HandleCollisionArgs &args) const
{
    other->handle_collision(*this, args);
}
void CosmeticMapObj::handle_collision([[maybe_unused]] const Wall_Type1 &other,
                                      [[maybe_unused]] const HandleCollisionArgs &args)
{
    args.set_move_intent(MoveIntent::StayAtCurrentPos);
}
void CosmeticMapObj::handle_collision([[maybe_unused]] const Unit_Type1 &other,
                                      [[maybe_unused]] const HandleCollisionArgs &args)
{
    args.set_move_intent(MoveIntent::StayAtCurrentPos);
}
void CosmeticMapObj::handle_collision([[maybe_unused]] const Projectile_Type1 &other,
                                      [[maybe_unused]] const HandleCollisionArgs &args)
{
    args.set_move_intent(MoveIntent::StayAtCurrentPos);
}

}}
