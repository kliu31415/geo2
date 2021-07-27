#pragma once

#include "geo2/render_op.h"

namespace geo2 {

class Game;

class GameRenderSceneGraph final: public RenderSceneGraph
{
    kx::gfx::Renderer *cur_renderer;
public:
    class Shaders
    {
        std::map<std::string, std::unique_ptr<IShader>, std::less<>> shaders_map;
    public:
        void init(kx::gfx::Renderer *rdr, kx::Passkey<GameRenderSceneGraph>);
        const IShader *get(std::string_view name) const;
    };
    Shaders shaders;

    class Fonts
    {
        std::map<std::string, std::shared_ptr<FontAtlas>, std::less<>> fonts_map;
    public:
        void init(kx::gfx::FontLibrary *font_library, kx::gfx::Renderer *rdr, kx::Passkey<GameRenderSceneGraph>);
        const FontAtlas *get(std::string_view name) const;
        const FontAtlas *get_default() const;
        const FontAtlas *get_black_chancery() const;
    };
    Fonts fonts;

    GameRenderSceneGraph(kx::Passkey<class MasterInstanceGfxImpl>,
                         kx::gfx::FontLibrary *font_library,
                         kx::gfx::Renderer *renderer);
};

}
