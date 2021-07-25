#pragma once

#include <memory>

#include "kx/util.h"

namespace kx { namespace gfx {
class Texture;
class FontLibrary;
class KWindowRunning;
}}

namespace geo2 {
class Xorshift64RNG;
class CEng1Data;

namespace map_obj {
class MapObject;
class Player_Type1;
}

struct GameGfxRenderArgs
{
    using flag_t = uint32_t;
    static constexpr flag_t FLAG_SHOW_HITBOXES = 1<<0;

    flag_t flags;
    kx::gfx::KWindowRunning *kwin_r;
    class GameRenderSceneGraph *render_scene_graph;
    int render_w;
    int render_h;
    float tile_len;
    std::vector<std::shared_ptr<map_obj::MapObject>> *map_objs;
    std::vector<std::shared_ptr<map_obj::MapObject>> *gfx_only_map_objs;
    std::vector<CEng1Data> *ceng_data;
    map_obj::Player_Type1 *player;
    std::vector<Xorshift64RNG> *rngs;
    double cur_level_time;
};

class GameGfx
{
    class Impl;
    std::unique_ptr<Impl> impl;
public:
    GameGfx(kx::Passkey<class Game>);
    ~GameGfx();
    std::shared_ptr<kx::gfx::Texture> render(const GameGfxRenderArgs &args);
};

}
