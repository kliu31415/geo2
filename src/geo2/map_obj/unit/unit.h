#pragma once

#include "geo2/map_obj/map_obj_args.h"
#include "geo2/map_obj/map_object.h"
#include "geo2/map_obj/alive_status.h"

#include "kx/gfx/renderer_types.h"

#include <map>

namespace geo2 { namespace map_obj {

class Unit: public MapObject
{
    std::map<Unit*, double> last_collision_damage_time;
    double last_damaged_at_time;
protected:
    //make ctors explicit to prevent accidental implicit casts from HandleCollisionArgs

    struct HandleWallCollisionArgs final: public HandleCollisionArgs
    {
        MoveIntent move_intent_upon_collision;

        explicit HandleWallCollisionArgs(const HandleCollisionArgs &args):
            HandleCollisionArgs(args),
            move_intent_upon_collision(MoveIntent::NotSet)
        {}
    };
    struct HandleUnitCollisionArgs final: public HandleCollisionArgs
    {
        MoveIntent move_intent_upon_collision;

        explicit HandleUnitCollisionArgs(const HandleCollisionArgs &args):
            HandleCollisionArgs(args),
            move_intent_upon_collision(MoveIntent::NotSet)
        {}
    };
    struct HandleProjCollisionArgs final: public HandleCollisionArgs
    {
        MoveIntent move_intent_upon_collision;

        explicit HandleProjCollisionArgs(const HandleCollisionArgs &args):
            HandleCollisionArgs(args),
            move_intent_upon_collision(MoveIntent::NotSet)
        {}
    };

    MapCoord current_position;
    Team team;
    AliveStatus alive_status;
    double max_health;
    double health;
    double death_time;

    kx::gfx::LinearColor apply_color_mod(const kx::gfx::LinearColor &color,
                                         double cur_level_time) const;

    void handle_wall_collision(Wall_Type1 *other, const HandleWallCollisionArgs &args);
    void handle_unit_collision(Unit *other, const HandleUnitCollisionArgs &args);
    void handle_proj_collision(Projectile_Type1 *other, const HandleProjCollisionArgs &args);

    Unit(Team team_, MapCoord position_, double health);
public:
    virtual ~Unit() = default;

    void handle_collision(MapObject *other, const HandleCollisionArgs &args) override;

    /** Note that these collision handling functions have some useful functionality;
     *  you usually wouldn't want to replace their functionality completely. Most
     *  commonly, you'd want to modify their functionality, which can be done by
     *  calling handle_****_collision and customizing the values in the second argument
     *  of that function; the below functions internally call handle_****_collision
     *  with some default values in the second argument that work well for most cases.
     */
    HANDLE_COLLISION_FUNC_DECLARATION(Wall_Type1) override;
    HANDLE_COLLISION_FUNC_DECLARATION(Unit) override;
    HANDLE_COLLISION_FUNC_DECLARATION(Projectile_Type1) override;

    void end_handle_collision_block(const EndHandleCollisionBlockArgs &args) override;

    virtual double get_collision_damage() const; ///defaults to 0

    MapCoord get_position() const;
    bool is_dead() const;
    bool is_completely_faded_out(double cur_level_time) const;

    Team get_team() const override;
    double get_health() const;
    double get_max_health() const;
};

}}
