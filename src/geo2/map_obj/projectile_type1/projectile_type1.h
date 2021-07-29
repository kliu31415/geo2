#pragma once

#include "geo2/map_obj/map_object.h"
#include "geo2/map_obj/alive_status.h"

namespace geo2 { namespace map_obj {

class Projectile_Type1: public MapObject
{
protected:
    std::weak_ptr<MapObject> owner;
    Team team;
    AliveStatus alive_status;

    Projectile_Type1(const std::shared_ptr<MapObject> &owner_);
public:
    virtual ~Projectile_Type1() = default;

    void handle_collision(MapObject *other, const HandleCollisionArgs &args) override;
    HANDLE_COLLISION_FUNC_DECLARATION(Wall_Type1) override;
    HANDLE_COLLISION_FUNC_DECLARATION(Unit) override;
    HANDLE_COLLISION_FUNC_DECLARATION(Projectile_Type1) override;

    void end_handle_collision_block(const EndHandleCollisionBlockArgs &args) override;

    bool is_dead() const;
    Team get_team() const override;

    virtual double get_damage() const = 0;
};

}}
