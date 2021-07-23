#pragma once

#include "geo2/render_op.h"

namespace geo2 {

class Game;

class GameRenderOpList final: public RenderOpList
{
    kx::gfx::Renderer *cur_renderer;
public:
    class Shaders
    {
        std::map<std::string, std::unique_ptr<IShader>, std::less<>> shaders_map;
    public:
        void init(kx::gfx::Renderer *rdr, kx::Passkey<GameRenderOpList>);
        const IShader *get(std::string_view name) const;
    };
    Shaders shaders;

    class Fonts
    {
        std::map<std::string, std::unique_ptr<FontAtlas>, std::less<>> fonts_map;
    public:
        void init(kx::gfx::FontLibrary *font_library, kx::gfx::Renderer *rdr, kx::Passkey<GameRenderOpList>);
        const FontAtlas *get(std::string_view name) const;
    };
    Fonts fonts;

    GameRenderOpList(kx::gfx::FontLibrary *font_library, kx::gfx::Renderer *renderer);
    void render(const Game &game, kx::gfx::KWindowRunning *kwin_r, int render_w, int render_h);
};

}
