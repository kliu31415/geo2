#pragma once

#include "geo2/map_obj/map_object.h"

namespace geo2 { namespace map_obj {

class Unit_Type1: public MapObject
{
protected:
    MapCoord current_position;
    double collision_damage;
    Team team;

    Unit_Type1(Team team_, MapCoord position_, double collision_damage_=0);
public:
    virtual ~Unit_Type1() = default;

    void handle_collision(MapObject *other, const HandleCollisionArgs &args) const override;
    HANDLE_COLLISION_FUNC_DECLARATION(Wall_Type1) override;
    HANDLE_COLLISION_FUNC_DECLARATION(Unit_Type1) override;
    HANDLE_COLLISION_FUNC_DECLARATION(Projectile_Type1) override;

    MapCoord get_pos() const;
};

}}
