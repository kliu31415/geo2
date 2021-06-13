#pragma once

#include "geo2/game.h"

#include "kx/gfx/kwindow.h"

namespace geo2 {

void run();

/** -Note that the game primarily uses normalized coordinates to draw
 */
class MasterInstance final: public kx::gfx::KItem
{
    enum class State {MainMenu, InGame};
    State state;

    int render_w;
    int render_h;

    std::unique_ptr<Game> game;
public:
    MasterInstance(const kx::gfx::Rect &render_rect_);

    ///noncopyable and nonmovable for safety
    MasterInstance(const MasterInstance&) = delete;
    MasterInstance &operator = (const MasterInstance&) = delete;
    MasterInstance(MasterInstance&&) = delete;
    MasterInstance &operator = (MasterInstance&&) = delete;

    std::shared_ptr<kx::gfx::Texture> run(kx::gfx::KWindowRunning *kwin_r) override;
};

}
