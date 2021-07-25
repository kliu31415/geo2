#pragma once

#include "geo2/game.h"
#include "geo2/library_pointers.h"

#include "kx/gfx/kwindow.h"

namespace geo2 {

void run(const LibraryPointers &libraries);

/** -Note that the game primarily uses normalized coordinates to draw
 */
class MasterInstance final: public kx::gfx::KItem
{
    enum class State {MainMenu, InGame};
    State state;

    LibraryPointers libraries;

    std::unique_ptr<Game> game;
    std::unique_ptr<class MasterInstanceGfxImpl> gfx;
public:
    MasterInstance(const LibraryPointers &libraries_, const kx::gfx::Rect &render_rect_);

    ///noncopyable and nonmovable for safety
    MasterInstance(const MasterInstance&) = delete;
    MasterInstance &operator = (const MasterInstance&) = delete;
    MasterInstance(MasterInstance&&) = delete;
    MasterInstance &operator = (MasterInstance&&) = delete;

    std::shared_ptr<kx::gfx::Texture> run(kx::gfx::KWindowRunning *kwin_r) override;
};

}
