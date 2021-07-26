#pragma once

#include "kx/gfx/gfx.h"
#include "geo2/geometry.h"
#include "geo2/map_obj/map_object.h"
#include "geo2/game_render_scene_graph.h"
#include "geo2/render_args.h"
#include "geo2/rng_args.h"

#include <SDL2/SDL_mouse.h>
#include <cmath>
#include <memory>

namespace geo2 { namespace weapon {

class WeaponOwnerInfo final
{
    //this exists because it happens that if we're spending, say, 0.003 mana per tick,
    //then the player will be left over with a nonzero, tiny amount of mana, which is
    //rounded to 1 in the display. This is not aesthetically pleasing, so allow the
    //player to spend more mana than they have if there's a small mana cost and the
    //player has nonzero mana; the threshold for "small mana cost" is below:
    static constexpr double SMALL_MANA_COST_THRESHOLD = 0.05;

    double *mana;
    double max_mana;
    double mana_cost_multiplier;
    double cooldown_speed_multipler;
public:
    MapCoord position;
    double offset_from_center;

    void set_mana(double *mana_)
    {
        mana = mana_;
    }
    void set_max_mana(double max_mana_)
    {
        max_mana = max_mana_;
    }
    void set_mana_cost_multiplier(double multiplier)
    {
        mana_cost_multiplier = multiplier;
    }
    void set_cooldown_speed_multiplier(double multiplier)
    {
        cooldown_speed_multipler = multiplier;
    }
    bool can_afford_mana_expenditure(double mana_cost) const
    {
        if(*mana > 0 && mana_cost < SMALL_MANA_COST_THRESHOLD)
            return true;
        return mana_cost_multiplier * mana_cost <= *mana;
    }
    void spend_mana(double mana_cost) const
    {
        k_expects(can_afford_mana_expenditure(mana_cost));
        *mana -= mana_cost_multiplier * mana_cost;
        *mana = std::max(*mana, 0.0);
    }
};

class WeaponRunArgs final: public RNG_Args
{
    std::vector<std::shared_ptr<map_obj::MapObject>> *map_objs_to_add;
public:
    MapCoord cursor_pos;
    double angle;
    kx::gfx::mouse_state_t mouse_state;
    double cur_level_time;
    double tick_len;

    inline bool primary_attack() const
    {
        return mouse_state & SDL_BUTTON(SDL_BUTTON_LEFT);
    }
    inline bool special_attack_1() const
    {
        return mouse_state & SDL_BUTTON(SDL_BUTTON_RIGHT);
    }
    inline void set_map_objs_to_add(std::vector<std::shared_ptr<map_obj::MapObject>> *to_add)
    {
        map_objs_to_add = to_add;
    }
    inline void add_map_obj(const std::shared_ptr<map_obj::MapObject> &obj) const
    {
        map_objs_to_add->push_back(obj);
    }
};

/** Take care when rendering a weapon; the parent has to clear RenderOpGroup every
 *  time! This is because Weapon modifies the RenderOpGroups you give it, so the
 *  parent (e.g. Player_Type1) can't just cache the RenderOpGroup and keep reusing it.
 */
class WeaponRenderArgs final: public RenderArgs
{
    RenderOpGroup *op_group;
public:
    double angle;
    float render_priority;
    inline void set_op_group(RenderOpGroup *group)
    {
        op_group = group;
    }
    inline void add_render_op(const std::shared_ptr<RenderOpShader> &op) const
    {
        op_group->add_op(op);
    }
    inline void set_render_args(const RenderArgs &args)
    {
        *static_cast<RenderArgs*>(this) = args;
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

class WeaponStartNewLevelArgs final
{
public:
};

class WeaponOwner
{
public:
    ///this isn't const because the owner has to return pointers to mana and max mana
    virtual WeaponOwnerInfo get_weapon_owner_info() = 0;
};

class Weapon
{
protected:
    std::weak_ptr<WeaponOwner> owner;
public:
    Weapon(const std::shared_ptr<WeaponOwner> &owner_);
    virtual ~Weapon() = default;

    virtual void run(const WeaponRunArgs &args) = 0;
    virtual void render(const WeaponRenderArgs &args) = 0;
    virtual void start_new_level(const WeaponStartNewLevelArgs &args);
    virtual void swap_in(const WeaponSwapInArgs &args);
    virtual void swap_out(const WeaponSwapOutArgs &args);
};

}}
