#pragma once

#include "geo2/geometry.h"
#include "geo2/map_obj/map_object.h"

#include <SDL2/SDL_mouse.h>
#include <cmath>

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

class WeaponRunArgs final
{
    std::vector<std::shared_ptr<map_obj::MapObject>> *map_objs_to_add;
    WeaponOwnerInfo owner_info;
    MapCoord cursor_pos;
    double tick_len;
    double cur_level_time;
    Uint32 mouse_state;
public:
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
    virtual void swap_in(const WeaponSwapInArgs &args);
    virtual void swap_out(const WeaponSwapOutArgs &args);
};

}}
