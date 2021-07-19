#pragma once

#include "kx/sdl_deleter.h"

#include <memory>
#include <string>
#include <array>

struct FT_FaceRec_;

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
    FT_FaceRec_ *ft_face;

    Font();
public:
    ///System fonts loaded upon gfx::init
    static std::shared_ptr<Font> DEFAULT;
    static std::shared_ptr<Font> MONO_DEFAULT;
    static std::shared_ptr<Font> ROBOTO_MONO_REGULAR;
    static std::shared_ptr<Font> ROBOTO_MONO_LIGHT;
    static std::shared_ptr<Font> BLACK_CHANCERY;

    static std::shared_ptr<Font> get_font(const std::string &font_name);
    static std::shared_ptr<Font> load(const std::string &font_name, const std::string &file);
    static int close(const std::string &font_name);
    static void init(const InitPkey&);
    static void quit(const QuitPkey&);

    int get_height(int size) const;
    int get_recommended_line_skip(int size) const;

    ///Fonts aren't copyable because they have unique ownership of a _TTF_Font
    Font(const Font&) = delete;
    Font &operator = (const Font&) = delete;

    ///Fonts aren't movable because there's an internal map that holds pointers to them
    Font(Font&&) = delete;
    Font &operator = (Font&&) = delete;
};

}}
