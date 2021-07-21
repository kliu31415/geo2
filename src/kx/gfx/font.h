#pragma once

#include "kx/sdl_deleter.h"
#include "kx/util.h"

#include <memory>
#include <string>
#include <array>
#include <map>

struct FT_FaceRec_;
struct FT_LibraryRec_;

namespace kx { namespace gfx {

class Renderer;

/** This and related functions (like Renderer::make_ASCII_atlas) aren't thread-safe
 *  because freetype2 isn't
 */
class Font final
{
    friend class FontLibrary;
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
    int get_height(int size) const;
    int get_recommended_line_skip(int size) const;

    ///Fonts aren't copyable because they have unique ownership of a _TTF_Font
    Font(const Font&) = delete;
    Font &operator = (const Font&) = delete;

    ///Fonts aren't movable because there's an internal map that holds pointers to them
    Font(Font&&) = delete;
    Font &operator = (Font&&) = delete;
};

class FontLibrary
{
    std::shared_ptr<Font> font_default;
    std::shared_ptr<Font> font_mono_default;
    std::shared_ptr<Font> font_roboto_mono_regular;
    std::shared_ptr<Font> font_roboto_mono_light;
    std::shared_ptr<Font> font_black_chancery;

    FT_LibraryRec_ *ft_library;
    std::map<std::string, std::shared_ptr<Font>, std::less<>> fonts;
public:
    static constexpr std::string_view FONT_DEFAULT = "kx.default";
    static constexpr std::string_view FONT_MONO_DEFAULT = "kx.mono.default";
    static constexpr std::string_view FONT_ROBOTO_MONO_REGULAR = "kx.roboto_mono.regular";
    static constexpr std::string_view FONT_ROBOTO_MONO_LIGHT = "kx.roboto_mono.light";
    static constexpr std::string_view FONT_BLACK_CHANCERY = "kx.black_chancery";

    FontLibrary();
    ~FontLibrary();

    std::shared_ptr<Font> get_font(std::string_view font_name);
    std::shared_ptr<Font> load(std::string_view font_name, std::string_view file);
    void close(std::string_view font_name);

    FontLibrary(const FontLibrary&) = delete;
    FontLibrary& operator = (const FontLibrary&) = delete;
    FontLibrary(FontLibrary&&) = delete;
    FontLibrary& operator = (FontLibrary&&) = delete;
};

}}
