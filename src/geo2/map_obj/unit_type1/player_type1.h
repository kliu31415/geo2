#include "geo2/map_obj/unit_type1/unit_type1.h"
#include "geo2/weapon/weapon.h"

namespace geo2 {class RenderOpGroup; class RenderOpShader; class Game;}

namespace geo2 { namespace map_obj {

struct PlayerRunSpecialArgs final
{
    double tick_len;

    int mouse_x;
    int mouse_y;

    bool left_pressed;
    bool right_pressed;
    bool up_pressed;
    bool down_pressed;

    weapon::WeaponRunArgs weapon_run_args;
};

class Player_Type1 final: public Unit_Type1
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
    nonstd::span<float> op_iu1;
    nonstd::span<float> op_iu2;

    double weapon_angle;

    std::vector<std::shared_ptr<weapon::Weapon>> weapons;
    int cur_weapon_idx;
public:
    static constexpr float WEAPON_OFFSET = 0.5f;

    Player_Type1();
    void run_special(const PlayerRunSpecialArgs &args, kx::Passkey<Game>);
    void init(const MapObjInitArgs &args) override;
    void run1_mt(const MapObjRun1Args &args) override;
    void run2_st(const MapObjRun2Args &args) override;
    void add_render_objs(const MapObjRenderArgs &args) override;
    void set_position(MapCoord pos, kx::Passkey<Game>); ///called at level start
    double get_collision_damage() const override;

};

}}
