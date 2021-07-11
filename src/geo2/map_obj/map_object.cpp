#include "geo2/map_obj/map_object.h"
#include "geo2/map_obj/map_obj_args.h"

#include "kx/io.h"

#include <cmath>

namespace geo2 { namespace map_obj {

//MapObject
void MapObject::init([[maybe_unused]] const MapObjInitArgs &args)
{}
void MapObject::run1_mt([[maybe_unused]] const MapObjRun1Args &args)
{}
void MapObject::run3_mt([[maybe_unused]] const MapObjRun3Args &args)
{}
void MapObject::add_render_objs([[maybe_unused]] const MapObjRenderArgs &args)
{}
void MapObject::end_handle_collision_block([[maybe_unused]] const EndHandleCollisionBlockArgs &args)
{}

bool MapObject::collision_could_matter([[maybe_unused]] const MapObject &other) const
{
    return true;
}
void MapObject::handle_collision([[maybe_unused]] CosmeticMapObj *other,
                                 [[maybe_unused]] const HandleCollisionArgs &args)
{

}
Team MapObject::get_team() const
{
    return Team::NotSet;
}

//CosmeticMapObj
void CosmeticMapObj::handle_collision(MapObject *other, const HandleCollisionArgs &args)
{
    other->handle_collision(this, args);
}
void CosmeticMapObj::handle_collision([[maybe_unused]] Wall_Type1 *other,
                                      [[maybe_unused]] const HandleCollisionArgs &args)
{

}
void CosmeticMapObj::handle_collision([[maybe_unused]] Unit_Type1 *other,
                                      [[maybe_unused]] const HandleCollisionArgs &args)
{

}
void CosmeticMapObj::handle_collision([[maybe_unused]] Projectile_Type1 *other,
                                      [[maybe_unused]] const HandleCollisionArgs &args)
{

}

}}
