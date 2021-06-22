#pragma once

#include "geo2/geometry.h"
#include "geo2/map_obj/map_object.h"

#include <SDL2/SDL_mouse.h>
#include <cmath>

namespace geo2 { namespace weapon {

class WeaponRunArgs
{
    std::vector<std::shared_ptr<map_obj::MapObject>> *map_objs;
public:
    Uint32 mouse_state;
    bool is_mouse_in_game_area;
    MapCoord cursor_pos;
    MapCoord player_pos;
    double tick_len;
    double angle;

    void set_map_objs_vec(std::vector<std::shared_ptr<map_obj::MapObject>> *map_objs_)
    {
        map_objs = map_objs_;
    }

    inline bool is_lmb_down() const
    {
        return mouse_state & SDL_BUTTON(SDL_BUTTON_LEFT);
    }
    inline void add_map_obj(std::shared_ptr<map_obj::MapObject> obj) const
    {
        map_objs->push_back(std::move(obj));
    }
};

class Weapon
{
public:
    virtual ~Weapon() = default;

    virtual void run(const WeaponRunArgs &args) = 0;
};

}}
