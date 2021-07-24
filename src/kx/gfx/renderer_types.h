#pragma once

/** LinearColor and SRGB_Color are guaranteed to be 16 bytes in size, and
 *  the components appear in memory in order r, g, b, a; this is static asserted
 *  in the source file
 */

#ifdef KX_RENDERER_SDL2

#elif defined(KX_RENDERER_GL)

namespace kx { namespace gfx {

/** These classes don't need to be final, but there's more compatibility with the SDL2
 *  renderer (which needs these classes to be final) if we make them final here to
 *  prevent them from being parent classes.
 */

struct Color4f
{
    float r;
    float g;
    float b;
    float a;

    Color4f() = default;
    Color4f(float r_, float g_, float b_, float a_=1.0f);
};

struct SRGB_Color;

struct LinearColor final: public Color4f
{
    LinearColor() = default;
    LinearColor(float r_, float g_, float b_, float a_=1.0f);
    LinearColor(const SRGB_Color &color);
};

struct SRGB_Color final: public Color4f
{
    SRGB_Color() = default;
    SRGB_Color(float r_, float g_, float b_, float a_=1.0f);
    SRGB_Color(const LinearColor &color);
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
    Zero = 0,
    One = 1,
    SrcAlpha = 2,
    OneMinusSrcAlpha = 3
};

enum class DrawMode {
    Triangles = 0,
    TriangleFan = 1,
    TriangleStrip = 2
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
