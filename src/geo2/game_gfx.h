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

namespace map_obj {
class MapObject;
class Player_Type1;
}

class GameGfx
{
    class Impl;
    std::unique_ptr<Impl> impl;
public:
    GameGfx(kx::Passkey<class Game>);
    ~GameGfx();
    std::shared_ptr<kx::gfx::Texture> render(kx::gfx::KWindowRunning *kwin_r,
                                             class GameRenderSceneGraph *render_scene_graph,
                                             int render_w, int render_h,
                                             float tile_len,
                                             std::vector<std::shared_ptr<map_obj::MapObject>> *map_objs,
                                             std::vector<std::shared_ptr<map_obj::MapObject>> *gfx_only_map_objs,
                                             map_obj::Player_Type1 *player,
                                             std::vector<Xorshift64RNG> *rngs,
                                             double cur_level_time);
};

}
