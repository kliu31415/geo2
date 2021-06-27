#pragma once

#include "geo2/map_obj/map_object.h"

namespace geo2 { namespace map_obj {

class Projectile_Type1: public MapObject
{
protected:
    std::weak_ptr<Unit_Type1> owner;
    int team;

    Projectile_Type1(const std::shared_ptr<Unit_Type1> &owner_);
public:
    virtual ~Projectile_Type1() = default;

    void handle_collision(MapObject *other, const HandleCollisionArgs &args) const override;
    HANDLE_COLLISION_FUNC_DECLARATION(Wall_Type1) override;
    HANDLE_COLLISION_FUNC_DECLARATION(Unit_Type1) override;
    HANDLE_COLLISION_FUNC_DECLARATION(Projectile_Type1) override;
};

}}
