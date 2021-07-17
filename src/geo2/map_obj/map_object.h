#pragma once

#include "geo2/geometry.h"

namespace geo2 { namespace map_obj {

enum class Team {
    NotSet, Neutral, Ally, Enemy
};

static inline bool are_enemies(Team t1, Team t2)
{
    return t1 != t2;
}

class MapObjInitArgs;
class MapObjRun1Args;
class MapObjRun3Args;
class MapObjRenderArgs;
class HandleCollisionArgs;
class EndHandleCollisionBlockArgs;

#define HANDLE_COLLISION_FUNC_DECLARATION(T) \
    void handle_collision(class T *other, const HandleCollisionArgs &args)

class MapObject
{
public:
    MapObject() = default;

    ///delete copy because it usually doesn't make sense
    MapObject(const MapObject&) = delete;
    MapObject& operator = (const MapObject&) = delete;

    ///delete move for safety (we might keep pointers to things)
    MapObject(MapObject&&) = delete;
    MapObject& operator = (MapObject&&) = delete;

    virtual ~MapObject() = default;

    ///default behavior for these functions = do nothing
    virtual void init(const MapObjInitArgs &args);
    virtual void run1_mt(const MapObjRun1Args &args);
    virtual void run3_mt(const MapObjRun3Args &args);
    virtual void add_render_objs(const MapObjRenderArgs &args);

    ///default return value = true
    virtual bool collision_could_matter(const MapObject &other) const;

    /** "other" handles the collision and reports its intent, so this function is
     *  const. Note that this function exists solely to perform the first level of
     *  double dispatch.
     */
    virtual void handle_collision(MapObject *other, const HandleCollisionArgs &args) = 0;
    ///for the below functions, "this" handles the collision and reports its intent
    virtual HANDLE_COLLISION_FUNC_DECLARATION(CosmeticMapObj);
    virtual HANDLE_COLLISION_FUNC_DECLARATION(Wall_Type1) = 0;
    virtual HANDLE_COLLISION_FUNC_DECLARATION(Unit_Type1) = 0;
    virtual HANDLE_COLLISION_FUNC_DECLARATION(Projectile_Type1) = 0;

    virtual void end_handle_collision_block(const EndHandleCollisionBlockArgs &args);

    virtual Team get_team() const;
};

/** Note that cosmetic map objects CAN collide with things; it's just that something
 *  collides with a cosmetic map object, the effect will be purely cosmetic; i.e.
 *  a floating blob might change the color of every unit it collides with.
 */
class CosmeticMapObj: public MapObject
{
    void handle_collision(MapObject *other, const HandleCollisionArgs &args) override;
    HANDLE_COLLISION_FUNC_DECLARATION(Wall_Type1) override;
    HANDLE_COLLISION_FUNC_DECLARATION(Unit_Type1) override;
    HANDLE_COLLISION_FUNC_DECLARATION(Projectile_Type1) override;
};

}}
