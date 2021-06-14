#pragma once

#include "geo2/map_objs/map_object.h"

namespace geo2 { namespace map_obj {

/** Generic rectangular wall
 */
class Wall_Type1: public MapObject
{
protected:
    MapRect position;
    std::unique_ptr<Polygon> polygon;
    Wall_Type1(const MapRect &position_);
public:
    void run1_mt(const MapObjRun1Args &args) override;

    MoveIntent handle_collision(MapObject *other,
                                const HandleCollisionArgs &args) const override final;
    HANDLE_COLLISION_FUNC_DECLARATION(Wall_Type1) override final;
    HANDLE_COLLISION_FUNC_DECLARATION(Unit_Type1) override final;
    HANDLE_COLLISION_FUNC_DECLARATION(Projectile_Type1) override final;
};

}}
