#pragma once

#include "kx/sdl_deleter.h"

#include <memory>
#include <string>
#include <array>

namespace kx {namespace gfx {

class InitPkey;
class QuitPkey;
class Renderer;

class Font final
{
public:
    static constexpr int MAX_FONT_SIZE = 128; ///any text larger than this will appear blurry
    using ID_t = int;
private:
    friend Renderer;

    const ID_t id;
    std::array<unique_ptr_sdl<_TTF_Font>, MAX_FONT_SIZE+1> font_of_size;

    Font();
public:
    ///System fonts loaded upon gfx::init
    static std::shared_ptr<const Font> DEFAULT;
    static std::shared_ptr<const Font> MONO_DEFAULT;
    static std::shared_ptr<const Font> ROBOTO_MONO_REGULAR;
    static std::shared_ptr<const Font> ROBOTO_MONO_LIGHT;

    static std::shared_ptr<const Font> get_font(const std::string &font_name);
    static std::shared_ptr<const Font> load(const std::string &font_name, const std::string &file);
    static int close(const std::string &font_name);
    static void load_system_fonts(const InitPkey&);
    static void close_all(const QuitPkey&);

    int get_height(int size) const;
    int get_recommended_line_skip(int size) const;

    ///Fonts aren't copyable because they have unique ownership of a _TTF_Font
    Font(const Font&) = delete;
    Font &operator = (const Font&) = delete;

    ///Fonts aren't movable because there's an internal map that holds pointers to them
    Font(Font&&) = delete;
    Font &operator = (Font&&) = delete;
};

class Texture;

struct MonofontAtlas
{
    static constexpr int MIN_CHAR=1, MAX_CHAR = 127;

    const int font_size, char_w, char_h;
    std::unique_ptr<Texture> characters[MAX_CHAR+1];

    MonofontAtlas(int font_size_, int char_w_, int char_h_);

    ///noncopyable because it has unique ownership of character textures
    MonofontAtlas(const MonofontAtlas&) = delete;
    MonofontAtlas &operator = (const MonofontAtlas&) = delete;

    ///nonmovable for safety (also can't move things with const fields anyway)
    MonofontAtlas(MonofontAtlas&&) = delete;
    MonofontAtlas &operator = (MonofontAtlas&&) = delete;
};

}}
