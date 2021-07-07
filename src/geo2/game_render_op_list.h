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
        std::unique_ptr<IShader> test_terrain_1;
        std::unique_ptr<IShader> outlined_tri;
        std::unique_ptr<IShader> monoc_wall_1;
        std::unique_ptr<IShader> monoc_floor_1;
        std::unique_ptr<IShader> pig_1;
        std::unique_ptr<IShader> laser_proj_1;
        std::unique_ptr<IShader> laser_1;

        void init(kx::gfx::Renderer *rdr, kx::Passkey<GameRenderOpList>);
    };
    Shaders shaders;

    GameRenderOpList(kx::gfx::Renderer *renderer);
    void render(const Game &game, kx::gfx::KWindowRunning *kwin_r, int render_w, int render_h);
};

}
