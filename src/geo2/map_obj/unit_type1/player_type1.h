#pragma once

#include "geo2/map_obj/unit_type1/unit_type1.h"
#include "geo2/weapon/weapon.h"

namespace geo2 {class RenderOpGroup; class RenderOpShader; class Game;}

namespace geo2 { namespace map_obj {

struct PlayerRunSpecialArgs final
{
    double tick_len;

    bool left_pressed;
    bool right_pressed;
    bool up_pressed;
    bool down_pressed;

    weapon::WeaponRunArgs weapon_run_args;
};

class Player_Type1 final:
    public Unit_Type1,
    public std::enable_shared_from_this<Player_Type1>,
    public weapon::WeaponOwner
{
    MapCoord desired_position;
    MapVec velocity;

    int left_pressed_tick_count;
    int right_pressed_tick_count;
    int up_pressed_tick_count;
    int down_pressed_tick_count;

    std::shared_ptr<RenderOpShader> op1;
    std::shared_ptr<RenderOpShader> op2;
    std::shared_ptr<RenderOpGroup> op_group;
    kx::kx_span<float> op_iu1;
    kx::kx_span<float> op_iu2;

    double weapon_angle;
    double max_mana;
    double mana;

    std::vector<std::shared_ptr<weapon::Weapon>> weapons;
    int cur_weapon_idx;
public:
    Player_Type1();

    void start_new_level(MapCoord pos, kx::Passkey<Game>);
    void run_special(const PlayerRunSpecialArgs &args, kx::Passkey<Game>);

    void init(const MapObjInitArgs &args) override;
    void run1_mt(const MapObjRun1Args &args) override;
    void run3_mt(const MapObjRun3Args &args) override;
    void add_render_ops(const MapObjRenderArgs &args) override;

    double get_collision_damage() const override;

    weapon::WeaponOwnerInfo get_weapon_owner_info() override;

    double get_max_mana() const;
    double get_mana() const;
};

}}
