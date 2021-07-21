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
#include <cstring>

namespace kx { namespace gfx {

std::atomic<Font::ID_t> num_fonts(0);

Font::Font():
    id(num_fonts.fetch_add(1, std::memory_order_relaxed))
{
    //The following assert fails if we're loading a ton of fonts, which really shouldn't be happening.
    k_expects(id != std::numeric_limits<ID_t>::max());
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

static std::atomic<int> init_count(0);
FontLibrary::FontLibrary()
{
    //this should be a singleton (at least for now)
    k_assert(init_count.fetch_add(1) == 0);

    if(FT_Init_FreeType(&ft_library)) {
        log_error("FT_Init_FreeType failed");
    }
    if(TTF_Init() < 0) {
        log_error((std::string)"TTF_Init error: " + TTF_GetError());
        return;
    }

    //make sure to set all of these to nullptr when closing
    font_default = load(FONT_DEFAULT, "kx_data/fonts/roboto_mono_regular.ttf");
    font_mono_default = load(FONT_MONO_DEFAULT, "kx_data/fonts/roboto_mono_regular.ttf");
    font_roboto_mono_regular = load(FONT_ROBOTO_MONO_REGULAR, "kx_data/fonts/roboto_mono_regular.ttf");
    font_roboto_mono_light = load(FONT_ROBOTO_MONO_LIGHT, "kx_data/fonts/roboto_mono_light.ttf");
    font_black_chancery = load(FONT_BLACK_CHANCERY, "kx_data/fonts/black_chancery.ttf");
}

FontLibrary::~FontLibrary()
{
    //make sure there are no more references to these
    font_default = nullptr;
    font_mono_default = nullptr;
    font_roboto_mono_regular = nullptr;
    font_roboto_mono_light = nullptr;
    font_black_chancery = nullptr;

    for(const auto &font: fonts) {
        if(font.second.use_count() > 1)
            log_error("attempting to close all fonts, but font \"" + font.first +
                      "\" still has " + to_str(font.second.use_count()-1) + " other owners");
    }
    fonts.clear();

    FT_Done_FreeType(ft_library);
    TTF_Quit();
}

std::shared_ptr<Font> FontLibrary::get_font(std::string_view font_name)
{
    auto font = fonts.find(font_name);
    if(font == fonts.end())
        return nullptr;
    else
        return font->second;
}
std::shared_ptr<Font> FontLibrary::load(std::string_view font_name, std::string_view file)
{
    if(fonts.find(font_name) != fonts.end()) {
        log_error((std::string)"Attempted to load a font with name \"" + to_str(font_name) +
                  "\", but a font with that name already exists");
        return nullptr;
    }

    std::shared_ptr<Font> font = std::shared_ptr<Font>(new Font());

    if(FT_New_Face(ft_library, file.data(), 0, &font->ft_face)) {
        log_error("error loading FT_Face from file \"" + (std::string)file + "\"");
    }
    if(!(font->ft_face->face_flags & FT_FACE_FLAG_SCALABLE)) {
        log_error("FT_Face is not scalable");
    }

    for(int i=0; i<=Font::MAX_FONT_SIZE; i++)
        font->font_of_size[i] = unique_ptr_sdl<TTF_Font>(TTF_OpenFont(file.data(), i));

    if(font->font_of_size[0] == nullptr) {
        log_error((std::string)"TTF_GetError(): " + TTF_GetError());
        return nullptr;
    }

    fonts.emplace(font_name, font);

    return font;
}
void FontLibrary::close(std::string_view font_name)
{
    auto font = fonts.find(font_name);
    if(font != fonts.end()) {
        if(font->second.use_count() != 1) {
            log_warning("removing font " + to_str(font_name) + " from the db, but " +
                        to_str(font->second.use_count() - 1) +
                        " other owning pointers exist, so the memory won't be freed");
        }
        fonts.erase(font);
    } else {
        log_error("Failed to erase font with name \"" + to_str(font_name) +
                  "\", as no fonts with that name exist");
    }
}

}}
