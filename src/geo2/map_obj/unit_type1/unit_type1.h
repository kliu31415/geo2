#pragma once

#include "geo2/map_obj/map_object.h"

namespace geo2 { namespace map_obj {

class Unit_Type1: public MapObject
{
protected:
    int team;
    MoveIntent move_intent;
    MapCoord position;
    MapCoord desired_position;
    double collision_damage;

    Unit_Type1(int team_, MapCoord position_, double collision_damage_=0);
public:
    virtual ~Unit_Type1() = default;

    MoveIntent handle_collision(MapObject *other, const HandleCollisionArgs &args) const override;
    HANDLE_COLLISION_FUNC_DECLARATION(Wall_Type1) override;
    HANDLE_COLLISION_FUNC_DECLARATION(Unit_Type1) override;
    HANDLE_COLLISION_FUNC_DECLARATION(Projectile_Type1) override;

    MapCoord get_pos() const;
    int get_team() const;
};

}}