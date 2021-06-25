#include "geo2/map_obj/unit_type1/unit_type1.h"

namespace geo2 {class RenderOpGroup; class RenderOpShader; class Game;}

namespace geo2 { namespace map_obj {

struct PlayerInputArgs final
{
    double tick_len;

    int mouse_x;
    int mouse_y;

    bool left_pressed;
    bool right_pressed;
    bool up_pressed;
    bool down_pressed;
};

class Player_Type1 final: public Unit_Type1
{
    int left_pressed_tick_count;
    int right_pressed_tick_count;
    int up_pressed_tick_count;
    int down_pressed_tick_count;

    std::shared_ptr<RenderOpShader> op1;
    std::shared_ptr<RenderOpShader> op2;
    std::shared_ptr<RenderOpGroup> op_group;
    nonstd::span<float> op_iu1;
    nonstd::span<float> op_iu2;

    std::unique_ptr<Polygon> cur_shape;
    std::unique_ptr<Polygon> des_shape;

    MapVec velocity;
public:
    Player_Type1();
    void process_input(const PlayerInputArgs &args);
    void run1_mt(const MapObjRun1Args &args) override;
    void run2_st(const MapObjRun2Args &args) override;
    void add_render_objs(const MapObjRenderArgs &args) override;
    void set_position(MapCoord pos, kx::Passkey<Game>); ///called at level start
};

}}
