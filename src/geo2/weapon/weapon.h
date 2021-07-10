#pragma once

#include "geo2/geometry.h"
#include "geo2/map_obj/map_object.h"
#include "geo2/game_render_op_list.h"
#include "geo2/render_args.h"
#include "geo2/rng_args.h"

#include <SDL2/SDL_mouse.h>
#include <cmath>
#include <memory>

namespace geo2 { namespace weapon {

struct WeaponOwnerInfo final
{
    MapCoord pos;
    double offset;

    WeaponOwnerInfo() = default;
    WeaponOwnerInfo(MapCoord pos_, double offset_):
        pos(pos_),
        offset(offset_)
    {}
};

class WeaponRunArgs final: public RNG_Args
{
    std::vector<std::shared_ptr<map_obj::MapObject>> *map_objs_to_add;
    WeaponOwnerInfo owner_info;
    MapCoord cursor_pos;
    double angle;
    Uint32 mouse_state;
public:
    double cur_level_time;
    double tick_len;

    inline bool is_lmb_down() const
    {
        return mouse_state & SDL_BUTTON(SDL_BUTTON_LEFT);
    }
    inline void set_map_objs_to_add(std::vector<std::shared_ptr<map_obj::MapObject>> *to_add)
    {
        map_objs_to_add = to_add;
    }
    inline void add_map_obj(const std::shared_ptr<map_obj::MapObject> &obj) const
    {
        map_objs_to_add->push_back(obj);
    }
    inline void set_owner_info(WeaponOwnerInfo info)
    {
        owner_info = info;
    }
    inline WeaponOwnerInfo get_owner_info() const
    {
        return owner_info;
    }
    inline void set_angle(double a)
    {
        angle = a;
    }
    inline double get_angle() const
    {
        return angle;
    }
    inline void set_mouse_state(Uint32 state)
    {
        mouse_state = state;
    }
    inline Uint32 get_mouse_state() const
    {
        return mouse_state;
    }
    inline void set_cursor_pos(MapCoord pos)
    {
        cursor_pos = pos;
    }
    inline MapCoord get_cursor_pos() const
    {
        return cursor_pos;
    }
    inline void set_tick_len(double len)
    {
        tick_len = len;
    }
    inline double get_tick_len() const
    {
        return tick_len;
    }
    inline void set_cur_level_time(double t)
    {
        cur_level_time = t;
    }
    inline double get_cur_level_time() const
    {
        return cur_level_time;
    }
};

/** Take care when rendering a weapon; you have to clear RenderOpGroup every
 *  time! This is because Weapon modifies the RenderOpGroups you give it,
 *  so you can't just cache the RenderOpGroup and keep reusing it.
 *
 */
class WeaponRenderArgs final: public RenderArgs
{
    std::shared_ptr<RenderOpGroup> op_group;
    WeaponOwnerInfo owner_info;
    double angle;
    float priority;
public:
    inline void set_op_group(const std::shared_ptr<RenderOpGroup> &group)
    {
        op_group = group;
    }
    inline void add_render_op(const std::shared_ptr<RenderOpShader> &op) const
    {
        op_group->add_op(op);
    }
    inline void set_render_priority(float priority_)
    {
        priority = priority_;
    }
    inline float get_render_priority() const
    {
        return priority;
    }
    inline void set_render_args(const RenderArgs &args)
    {
        *static_cast<RenderArgs*>(this) = args;
    }
    inline void set_owner_info(WeaponOwnerInfo info)
    {
        owner_info = info;
    }
    inline WeaponOwnerInfo get_owner_info() const
    {
        return owner_info;
    }
    inline void set_angle(double a)
    {
        angle = a;
    }
    inline double get_angle() const
    {
        return angle;
    }
};

class WeaponSwapInArgs final
{
public:
};

class WeaponSwapOutArgs final
{
public:
};

class Weapon
{
protected:
    std::weak_ptr<map_obj::MapObject> owner;
public:
    Weapon(const std::shared_ptr<map_obj::MapObject> &owner_);
    virtual ~Weapon() = default;

    virtual void run(const WeaponRunArgs &args) = 0;
    virtual void render(const WeaponRenderArgs &args) = 0;
    virtual void swap_in(const WeaponSwapInArgs &args);
    virtual void swap_out(const WeaponSwapOutArgs &args);
};

}}
