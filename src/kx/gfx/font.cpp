#include "kx/gfx/font.h"
#include "kx/debug.h"

#include "ft2build.h"
#include FT_FREETYPE_H

#include <SDL2/SDL_ttf.h>
#include <limits>
#include <type_traits>
#include <map>
#include <memory>
#include <string>
#include <atomic>

namespace kx { namespace gfx {

FT_Library ft_library;

std::atomic<Font::ID_t> num_fonts(0);

Font::Font():
    id(num_fonts.fetch_add(1, std::memory_order_relaxed))
{
    //The following assert fails if we're loading a ton of fonts, which really shouldn't be happening.
    k_expects(id != std::numeric_limits<ID_t>::max());
}

std::map<std::string, std::shared_ptr<Font> > fonts;

std::shared_ptr<Font> Font::DEFAULT;
std::shared_ptr<Font> Font::MONO_DEFAULT;
std::shared_ptr<Font> Font::ROBOTO_MONO_REGULAR;
std::shared_ptr<Font> Font::ROBOTO_MONO_LIGHT;
std::shared_ptr<Font> Font::BLACK_CHANCERY;

std::shared_ptr<Font> Font::get_font(const std::string &font_name)
{
    auto font = fonts.find(font_name);
    if(font == fonts.end())
        return nullptr;
    else
        return font->second;
}
std::shared_ptr<Font> Font::load(const std::string &font_name, const std::string &file)
{
    if(fonts.find(font_name) != fonts.end()) {
        log_error((std::string)"Attempted to load a font with name \"" + font_name +
                  "\", but a font with that name already exists");
        return nullptr;
    }

    std::shared_ptr<Font> font = std::shared_ptr<Font>(new Font());

    if(FT_New_Face(ft_library, file.c_str(), 0, &font->ft_face)) {
        log_error("error loading FT_Face from file \"" + file + "\"");
    }
    if(!(font->ft_face->face_flags & FT_FACE_FLAG_SCALABLE)) {
        log_error("FT_Face is not scalable");
    }

    for(int i=0; i<=MAX_FONT_SIZE; i++)
        font->font_of_size[i] = unique_ptr_sdl<TTF_Font>(TTF_OpenFont(file.c_str(), i));

    if(font->font_of_size[0] == nullptr) {
        log_error((std::string)"TTF_GetError(): " + TTF_GetError());
        return nullptr;
    }

    fonts[font_name] = font;

    return font;
}
int Font::close(const std::string &font_name)
{
    auto font = fonts.find(font_name);
    if(font != fonts.end()) {
        if(font->second.use_count() != 1) {
            log_warning("removing font " + font_name + " from the db, but " +
                        to_str(font->second.use_count() - 1) +
                        " other owning pointers exist, so the memory won't be freed");
        }
        fonts.erase(font);
        return 0;
    }
    log_error("Failed to erase font with name \"" + font_name +
              "\", as no fonts with that name exist");
    return -1;
}
void Font::init(Passkey<Library>)
{
    if(FT_Init_FreeType(&ft_library)) {
        log_error("FT_Init_FreeType failed");
    }
    if(TTF_Init() < 0) {
        log_error((std::string)"TTF_Init error: " + TTF_GetError());
        return;
    }

    //make sure to set all of these to nullptr when closing
    DEFAULT = load("kx.default", "kx_data/fonts/roboto_mono_regular.ttf");
    MONO_DEFAULT = load("kx.mono.default", "kx_data/fonts/roboto_mono_regular.ttf");
    ROBOTO_MONO_REGULAR = load("kx.roboto_mono.regular", "kx_data/fonts/roboto_mono_regular.ttf");
    ROBOTO_MONO_LIGHT = load("kx.roboto_mono.light", "kx_data/fonts/roboto_mono_light.ttf");
    BLACK_CHANCERY = load("kx.black_chancery", "kx_data/fonts/black_chancery.ttf");
}

void Font::quit(Passkey<Library>)
{
    //make sure there are no more references to these
    DEFAULT = nullptr;
    MONO_DEFAULT = nullptr;
    ROBOTO_MONO_REGULAR = nullptr;
    ROBOTO_MONO_LIGHT = nullptr;
    BLACK_CHANCERY = nullptr;

    for(const auto &font: fonts) {
        if(font.second.use_count() > 1)
            log_error("attempting to close all fonts, but font \"" + font.first +
                      "\" still has " + to_str(font.second.use_count()-1) + " other owners");
    }
    fonts.clear();

    FT_Done_FreeType(ft_library);
    TTF_Quit();
}
int Font::get_height(int size) const
{
    k_expects(size>=1 && size<=MAX_FONT_SIZE);
    return TTF_FontHeight(font_of_size[size].get());
}
int Font::get_recommended_line_skip(int size) const
{
    k_expects(size>=1 && size<=MAX_FONT_SIZE);
    return TTF_FontLineSkip(font_of_size[size].get());
}

}}
