#include "kx/gfx/font.h"
#include "renderer.h" //needed to get sizeof(Texture) because unique_ptr complains
#include "kx/debug.h"

#include <SDL2/SDL_ttf.h>
#include <limits>
#include <type_traits>
#include <map>
#include <memory>
#include <string>

namespace kx {namespace gfx {

int Font::num_fonts = 0;

Font::Font(): id(num_fonts++)
{
    static_assert(std::is_same<decltype(num_fonts), std::remove_const<decltype(id)>::type>::value);
    //The following assert fails if we're loading a ton of fonts, which really shouldn't be happening.
    k_ensures(id != std::numeric_limits<decltype(num_fonts)>::max());
}

static std::map<std::string, std::shared_ptr<Font> > fonts;
std::shared_ptr<const Font> Font::DEFAULT;
std::shared_ptr<const Font> Font::MONO_DEFAULT;
std::shared_ptr<const Font> Font::ROBOTO_MONO_REGULAR;
std::shared_ptr<const Font> Font::ROBOTO_MONO_LIGHT;

std::shared_ptr<const Font> Font::get_font(const std::string &font_name)
{
    auto font = fonts.find(font_name);
    if(font == fonts.end())
        return nullptr;
    else
        return font->second;
}
std::shared_ptr<const Font> Font::load(const std::string &font_name, const std::string &file)
{
    if(fonts.find(font_name) != fonts.end()) {
        log_error((std::string)"Attempted to load a font with name \"" + font_name +
                  "\", but a font with that name already exists");
        return nullptr;
    }

    std::shared_ptr<Font> font = std::shared_ptr<Font>(new Font()); //private constructor, have to use new

    for(int i=0; i<Font::NUM_FONT_SIZES; i++)
        font->font_size[i] = unique_ptr_sdl<TTF_Font>(TTF_OpenFont(file.c_str(), i));

    if(font->font_size[0] == nullptr) {
        log_error((std::string)"TTF_GetError(): " + TTF_GetError());
        return nullptr;
    }

    fonts[font_name] = font;
    /*if(!TTF_FontFaceIsFixedWidth(font->font_size[Font::NUM_FONT_SIZES-1].get())) {
        log_warning("Warning: font may not be monospaced, which may cause rendering issues");
    }*/

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
void Font::load_system_fonts(const InitPkey&)
{
    //make sure to set all of these to nullptr when closing
    DEFAULT = load("kx.default", "kx_data/fonts/roboto_mono_regular.ttf");
    MONO_DEFAULT = load("kx.mono.default", "kx_data/fonts/roboto_mono_regular.ttf");
    ROBOTO_MONO_REGULAR = load("kx.roboto_mono.regular", "kx_data/fonts/roboto_mono_regular.ttf");
    ROBOTO_MONO_LIGHT = load("kx.roboto_mono.light", "kx_data/fonts/roboto_mono_light.ttf");
}

void Font::close_all(const QuitPkey&)
{
    //make sure there are no more references to these
    DEFAULT = nullptr;
    MONO_DEFAULT = nullptr;
    ROBOTO_MONO_REGULAR = nullptr;
    ROBOTO_MONO_LIGHT = nullptr;

    for(const auto &font: fonts) {
        if(font.second.use_count() > 1)
            log_error("attempting to close all fonts, but font \"" + font.first +
                      "\" still has " + to_str(font.second.use_count()-1) + " other owners");
    }
    fonts.clear();
}
int Font::get_height(int size) const
{
    k_expects(size>=1 && size<=NUM_FONT_SIZES);
    return TTF_FontHeight(font_size[size].get());
}
int Font::get_recommended_line_skip(int size) const
{
    k_expects(size>=1 && size<=NUM_FONT_SIZES);
    return TTF_FontLineSkip(font_size[size].get());
}

MonofontAtlas::MonofontAtlas(int font_size_, int char_w_, int char_h_):
    font_size(font_size_),
    char_w(char_w_),
    char_h(char_h_)
{
    //the character textures should be >0 size
    k_expects(char_w > 0);
    k_expects(char_h > 0);
}

}}
