#pragma once

#include "geo2/map_objs/map_object.h"

namespace geo2 { namespace map_obj {

class Projectile_Type1: public MapObject
{
    MoveIntent handle_collision(MapObject *other, const HandleCollisionArgs &args) const override;
    HANDLE_COLLISION_FUNC_DECLARATION(Wall_Type1) override;
    HANDLE_COLLISION_FUNC_DECLARATION(Unit_Type1) override;
    HANDLE_COLLISION_FUNC_DECLARATION(Projectile_Type1) override;
};

}}
