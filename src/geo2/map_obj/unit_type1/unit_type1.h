#pragma once

#include "geo2/map_obj/map_object.h"
#include "geo2/map_obj/alive_status.h"

#include "kx/gfx/renderer_types.h"

#include <map>

namespace geo2 { namespace map_obj {

class Unit_Type1: public MapObject
{
    std::map<Unit_Type1*, double> last_collision_damage_time;
    double last_damaged_at_time;
protected:
    MapCoord current_position;
    Team team;
    double health;
    AliveStatus alive_status;

    kx::gfx::LinearColor apply_color_mod(const kx::gfx::LinearColor &color,
                                         double cur_level_time) const;

    Unit_Type1(Team team_, MapCoord position_, double health);
public:
    virtual ~Unit_Type1() = default;

    /** If you override the collision handling function for Unit_Type1
     *  or Projectile_Type1, make sure to call the function here, i.e.
     *  call Unit_Type1::handle_collision(...) in your overriding function.
     */
    void handle_collision(MapObject *other, const HandleCollisionArgs &args) override;
    HANDLE_COLLISION_FUNC_DECLARATION(Wall_Type1) override;
    HANDLE_COLLISION_FUNC_DECLARATION(Unit_Type1) override;
    HANDLE_COLLISION_FUNC_DECLARATION(Projectile_Type1) override;

    void end_handle_collision_block(const EndHandleCollisionBlockArgs &args) override;

    virtual double get_collision_damage() const; ///defaults to 0

    MapCoord get_position() const;
    bool is_dead() const;

    Team get_team() const override;
};

}}
