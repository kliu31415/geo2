#pragma once

#include "geo2/map_obj/map_object.h"

namespace geo2 { namespace map_obj {

/** Generic rectangular wall
 */
class Wall_Type1: public MapObject
{
protected:
    MapRect position;
    Wall_Type1(const MapRect &position_);
public:
    virtual ~Wall_Type1() = default;
    void init(const MapObjInitArgs &args) override;
    void run1_mt(const MapObjRun1Args &args) override;

    void handle_collision(MapObject *other, const HandleCollisionArgs &args) override final;
    HANDLE_COLLISION_FUNC_DECLARATION(Wall_Type1) override final;
    HANDLE_COLLISION_FUNC_DECLARATION(Unit_Type1) override final;
    HANDLE_COLLISION_FUNC_DECLARATION(Projectile_Type1) override final;
};

}}
