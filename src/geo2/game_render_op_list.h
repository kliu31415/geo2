#pragma once

#include "geo2/render_op.h"

namespace geo2 {

class Game;

class GameRenderOpList final: public RenderOpList
{
    kx::gfx::Renderer *cur_renderer;
public:
    struct Shaders
    {
        std::unique_ptr<GShader> test_terrain1;
        std::unique_ptr<GShader> outlined_tri;

        void init(kx::gfx::Renderer *rdr, kx::Passkey<GameRenderOpList>);
    };
    Shaders shaders;

    GameRenderOpList(kx::gfx::Renderer *renderer);
    void render(const Game &game, kx::gfx::KWindowRunning *kwin_r, int render_w, int render_h);
};

}
