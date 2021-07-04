#pragma once

#ifdef KX_RENDERER_SDL2

#include "kx/sdl_deleter.h"

#include <SDL2/SDL_rect.h>

namespace kx { namespace gfx {

struct SRGB_Color;

struct LinearColor final
{
    float r, g, b, a;

    LinearColor() = default;
    LinearColor(float r_, float g_, float b_, float a_=1.0f);
    LinearColor(const SRGB_Color &color);
};

struct SRGB_Color final
{
    float r, g, b, a;

    SRGB_Color() = default;
    SRGB_Color(SDL_Color sdl_color);
    SRGB_Color(float r_, float g_, float b_, float a_=1.0f);
    SRGB_Color(const LinearColor &color);
    operator SDL_Color() const;
};

namespace Color
{
    const SRGB_Color BLACK(0.0, 0.0, 0.0);
    const SRGB_Color WHITE(1.0, 1.0, 1.0);
    const SRGB_Color RED(1.0, 0.0, 0.0);
    const SRGB_Color GREEN(0.0, 1.0, 0.0);
    const SRGB_Color BLUE(0.0, 0.0, 1.0);
    const SRGB_Color LIGHT_GRAY(0.75, 0.75, 0.75);
    const SRGB_Color MEDIUM_GRAY(0.5, 0.5, 0.5);
    const SRGB_Color DARK_GRAY(0.25, 0.25, 0.25);
    const SRGB_Color PINK(1.0, 0.75, 0.8);
    const SRGB_Color YELLOW(1.0, 1.0, 0.0);
    const SRGB_Color ORANGE(1.0, 0.65, 0.0);
    const SRGB_Color BROWN(0.55, 0.27, 0.08);
    const SRGB_Color CYAN(0.0, 1.0, 1.0);
    const SRGB_Color MAGENTA(1.0, 0.0, 1.0);
    const SRGB_Color DARK_RED(0.5, 0.0, 0.0);
    const SRGB_Color DARK_GREEN(0.0, 0.5, 0.0);
    const SRGB_Color DARK_BLUE(0.0, 0.0, 0.5);
    const SRGB_Color PURPLE(0.5, 0.0, 0.5);
    const SRGB_Color TEAL(0.0, 0.5, 0.5);
}

/** NOTE: we CAN'T add any additional fields to anything that inherits
 *  a SDL2 struct because then memory alignment would be fucked.
 *  This is because, for example, we might convert an array of Rects,
 *  which is of type Rect*, to SDL_Rect*. If Rect and SDL_Rect are
 *  of different sizes, then rect[i] != sdl_rect[i] for i!=0.
 *  The data structures below are just supposed to be C++ wrappers around
 *  C structs.
 */

struct Point final: public SDL_FPoint
{
    Point() = default;
    Point(float x_, float y_);
};
static_assert(sizeof(Point) == sizeof(SDL_FPoint));
static_assert(std::is_convertible<Point*, SDL_FPoint*>::value);

struct Line final
{
    Point p1, p2;

    Line() = default;
    Line(Point p1_, Point p2_);
};

struct Rect final: public SDL_FRect
{
    Rect() = default;
    Rect(float x_, float y_, float w_, float h_);
    Rect(const struct IRect &other);
    bool point_inside(gfx::Point point) const; ///checks if point.x in [x, x+w) and point.y in [y, y+h)
};
static_assert(sizeof(Rect) == sizeof(SDL_FRect));
static_assert(std::is_convertible<Rect*, SDL_FRect*>::value);

struct IRect final: public SDL_Rect
{
    IRect() = default;
    IRect(int x, int y, int w, int h);
    bool point_inside(gfx::Point point) const; ///checks if point.x in [x, x+w) and point.y in [y, y+h)
};
static_assert(sizeof(IRect) == sizeof(SDL_Rect));
static_assert(std::is_convertible<IRect*, SDL_Rect*>::value);

struct Circle final
{
    float x, y, r;

    Circle() = default;
    Circle(float x_, float y_, float r_);
    bool point_inside(gfx::Point point) const; ///checks if (x-point.x)**2 + (y-point.y)**2 < r**2
};

class Texture
{
    friend class Renderer;
    unique_ptr_sdl<SDL_Texture> sdl_texture;
    Texture(SDL_Texture *t);
public:
    Texture(const Texture&) = delete;
    Texture &operator = (const Texture&) = delete;
    void set_color_mod(float r, float g, float b);
    void set_alpha_mod(float a);
    int get_w() const;
    int get_h() const;
};

}}

#elif defined(KX_RENDERER_GL)

#include "glad/glad.h"
#include <SDL2/SDL_pixels.h>

namespace kx { namespace gfx {

/** These classes don't need to be final, but there's more compatibility with the SDL2
 *  renderer (which needs these classes to be final) if we make them final here to
 *  prevent them from being parent classes.
 */

struct SRGB_Color;

struct LinearColor final
{
    float r;
    float g;
    float b;
    float a;

    LinearColor() = default;
    LinearColor(float r_, float g_, float b_, float a_=1.0f);
    LinearColor(const SRGB_Color &color);
};

struct SRGB_Color final
{
    float r;
    float g;
    float b;
    float a;

    SRGB_Color() = default;
    SRGB_Color(SDL_Color sdl_color);
    SRGB_Color(float r_, float g_, float b_, float a_=1.0f);
    SRGB_Color(const LinearColor &color);
    operator SDL_Color() const;
};

namespace Color
{
    const SRGB_Color BLACK(0.0, 0.0, 0.0);
    const SRGB_Color WHITE(1.0, 1.0, 1.0);
    const SRGB_Color RED(1.0, 0.0, 0.0);
    const SRGB_Color GREEN(0.0, 1.0, 0.0);
    const SRGB_Color BLUE(0.0, 0.0, 1.0);
    const SRGB_Color LIGHT_GRAY(0.75, 0.75, 0.75);
    const SRGB_Color MEDIUM_GRAY(0.5, 0.5, 0.5);
    const SRGB_Color DARK_GRAY(0.25, 0.25, 0.25);
    const SRGB_Color PINK(1.0, 0.75, 0.8);
    const SRGB_Color YELLOW(1.0, 1.0, 0.0);
    const SRGB_Color ORANGE(1.0, 0.65, 0.0);
    const SRGB_Color BROWN(0.55, 0.27, 0.08);
    const SRGB_Color CYAN(0.0, 1.0, 1.0);
    const SRGB_Color MAGENTA(1.0, 0.0, 1.0);
    const SRGB_Color DARK_RED(0.5, 0.0, 0.0);
    const SRGB_Color DARK_GREEN(0.0, 0.5, 0.0);
    const SRGB_Color DARK_BLUE(0.0, 0.0, 0.5);
    const SRGB_Color PURPLE(0.5, 0.0, 0.5);
    const SRGB_Color TEAL(0.0, 0.5, 0.5);
}

struct Point final
{
    float x;
    float y;

    Point() = default;
    Point(float x_, float y_);
};

struct Line final
{
    Point p1;
    Point p2;

    Line() = default;
    Line(Point p1_, Point p2_);
};

struct Rect final
{
    float x;
    float y;
    float w;
    float h;

    Rect() = default;
    Rect(float x_, float y_, float w_, float h_);
    Rect(const struct IRect &other);
    bool point_inside(gfx::Point point) const; ///checks if point.x in [x, x+w) and point.y in [y, y+h)
};

struct IRect final
{
    int x;
    int y;
    int w;
    int h;

    IRect() = default;
    IRect(int x, int y, int w, int h);
    bool point_inside(gfx::Point point) const; ///checks if point.x in [x, x+w) and point.y in [y, y+h)
};

struct Circle final
{
    float x, y, r;

    Circle() = default;
    Circle(float x_, float y_, float r_);
    bool point_inside(gfx::Point point) const; ///checks if (x-point.x)**2 + (y-point.y)**2 < r**2
};

enum class BlendFactor {
    Zero = GL_ZERO,
    One = GL_ONE,
    SrcAlpha = GL_SRC_ALPHA,
    OneMinusSrcAlpha = GL_ONE_MINUS_SRC_ALPHA
};

enum class DrawMode {
    Triangles = GL_TRIANGLES,
    TriangleFan = GL_TRIANGLE_FAN,
    TriangleStrip = GL_TRIANGLE_STRIP
};

enum class Flip {
    None, Horizontal, Vertical
};

enum class _BufferType {
    VBO, EBO, UBO
};

}}

#elif defined(KX_RENDERER_VK)

#else
#error "No renderer defined!"
#endif

static_assert(sizeof(kx::gfx::SRGB_Color) == 16);
static_assert(offsetof(kx::gfx::SRGB_Color, r) == 0);
static_assert(offsetof(kx::gfx::SRGB_Color, g) == 4);
static_assert(offsetof(kx::gfx::SRGB_Color, b) == 8);
static_assert(offsetof(kx::gfx::SRGB_Color, a) == 12);

static_assert(sizeof(kx::gfx::LinearColor) == 16);
static_assert(offsetof(kx::gfx::LinearColor, r) == 0);
static_assert(offsetof(kx::gfx::LinearColor, g) == 4);
static_assert(offsetof(kx::gfx::LinearColor, b) == 8);
static_assert(offsetof(kx::gfx::LinearColor, a) == 12);
