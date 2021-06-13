#pragma once

#include "kx/gfx/font.h"
#include "kx/gfx/renderer_types.h"
#include "kx/sdl_deleter.h"
#include "kx/util.h"
#include "kx/time.h"

#include <SDL2/SDL.h>
#include "glad/glad.h"

#include "span_lite.h"

#include <string>
#include <map>
#include <memory>
#include <set>
#include <queue>

namespace kx { namespace gfx {

template<class Deleter> class _GLUniqueObj final
{
public:
    GLuint id;

    _GLUniqueObj() = default;
    _GLUniqueObj(GLuint id_):
        id(id_)
    {}
    ~_GLUniqueObj()
    {
        Deleter()(id);
    }

    _GLUniqueObj(const _GLUniqueObj &other) = delete;
    _GLUniqueObj &operator = (const _GLUniqueObj &other) = delete;

    _GLUniqueObj(_GLUniqueObj &&other)
    {
        id = other.id;
        other.id = 0;
    }
    _GLUniqueObj &operator = (_GLUniqueObj &&other)
    {
        Deleter()(id);
        id = other.id;
        other.id = 0;
        return *this;
    }
};

struct _GLDeleteShader final
{
    void operator()(GLuint id) const {glDeleteShader(id);}
};
struct _GLDeleteShaderProgram final
{
    void operator()(GLuint id) const {glDeleteProgram(id);}
};
struct _GLDeleteBuffer final
{
    void operator()(GLuint id) const {glDeleteBuffers(1, &id);}
};
struct _GLDeleteVAO final
{
    void operator()(GLuint id) const {glDeleteVertexArrays(1, &id);}
};
struct _GLDeleteTexture final
{
    void operator()(GLuint id) const {glDeleteTextures(1, &id);}
};
struct _GLDeleteFramebuffers final
{
    void operator()(GLuint id) const {glDeleteFramebuffers(1, &id);}
};

struct ColorMod
{
    float r;
    float g;
    float b;

    ColorMod() = default;
    ColorMod(float r_, float g_, float b_);
};

class Texture
{
public:
    enum class Format {RGBA8888, RGB888, RGBA16F};
private:
    friend class Renderer;

    static GLuint active_texture;

    _GLUniqueObj<_GLDeleteTexture> texture;
    _GLUniqueObj<_GLDeleteFramebuffers> framebuffer;
    bool is_srgb_;
    bool has_mipmaps;
    int w;
    int h;
    Format format;
    ColorMod color_mod;
    float alpha_mod;
    GLenum binding_point; ///GL_TEXTURE_2D or GL_TEXTURE_2D_MULTISAMPLE
    int samples;

    Texture(int w_, int h_, Format format_, bool is_srgb__, int samples_);
    Texture(SDL_Surface *s, bool is_srgb__);
public:
    Texture(const Texture&) = delete;
    Texture &operator = (const Texture&) = delete;
    void set_color_mod(float r, float g, float b);
    void set_alpha_mod(float a);

    int get_w() const;
    int get_h() const;
    Format get_format() const;
    bool is_srgb() const;
    bool is_multisample() const;

    void make_targetable();

    enum class FilterAlgo {Nearest = GL_NEAREST, Linear = GL_LINEAR};

    void set_min_filter(FilterAlgo algo);
    void set_mag_filter(FilterAlgo algo);
};

class Shader
{
    friend class ShaderProgram;
protected:
    _GLUniqueObj<_GLDeleteShader> shader;

    Shader();
    Shader(const std::string &file_name, GLenum shader_type);
public:
};

class VertShader final: public Shader
{
    friend class Renderer;
    VertShader() = default;
    VertShader(const std::string &file_name);
};

class FragShader final: public Shader
{
    friend class Renderer;
    FragShader() = default;
    FragShader(const std::string &file_name);
};

using UniformLoc = GLint;
using UBIndex = GLuint;

class ShaderProgram
{
    friend Renderer;
    Renderer *owner;
    _GLUniqueObj<_GLDeleteShaderProgram> program;

    ShaderProgram(Renderer *owner_, const VertShader &vert_shader, const FragShader &frag_shader);
public:
    UniformLoc get_uniform_loc(const GLchar *name);

    //all set_uniform functions require the current program to be bound
    void set_uniform1i(GLint loc, GLint val);

    void set_uniform1f(GLint loc, GLfloat v0);
    void set_uniform2f(GLint loc, GLfloat v0, GLfloat v1);
    void set_uniform3f(GLint loc, GLfloat v0, GLfloat v1, GLfloat v3);
    void set_uniform4f(GLint loc, GLfloat v0, GLfloat v1, GLfloat v2, GLfloat v3);

    void set_uniform1fv(GLint loc, nonstd::span<GLfloat> vals);
    void set_uniform2fv(GLint loc, nonstd::span<GLfloat> vals);
    void set_uniform3fv(GLint loc, nonstd::span<GLfloat> vals);
    void set_uniform4fv(GLint loc, nonstd::span<GLfloat> vals);

    UBIndex get_UB_index(const GLchar *name);

    void bind_UB(UBIndex ub_index, GLuint binding);
};

template<_BufferType T> class Buffer final
{
    friend class VAO;
    friend class Renderer;

    Renderer *owner;
    _GLUniqueObj<_GLDeleteBuffer> buffer;

    Buffer(Renderer *owner_);
public:
    void ensure_active();
    void buffer_data(void *data, GLuint n);
    void buffer_sub_data(GLintptr offset, void *data, GLuint n);
    void invalidate();
};

template class Buffer<_BufferType::VBO>;
template class Buffer<_BufferType::EBO>;
template class Buffer<_BufferType::UBO>;

using VBO = Buffer<_BufferType::VBO>;
using EBO = Buffer<_BufferType::EBO>;
using UBO = Buffer<_BufferType::UBO>;

class VAO final
{
    friend class Renderer;

    Renderer *owner;
    _GLUniqueObj<_GLDeleteVAO> vao;
    std::vector<std::shared_ptr<VBO>> vbos;
    std::vector<std::shared_ptr<EBO>> ebos;

    VAO(Renderer *owner_);
public:
    void add_VBO(std::shared_ptr<VBO> vbo);
    void add_EBO(std::shared_ptr<EBO> ebo);
    void vertex_attrib_pointer_f(GLuint pos, GLuint size, GLsizei stride, uintptr_t offset);
    void enable_vertex_attrib_array(GLuint pos);
};

constexpr int NUM_SAMPLES_DEFAULT = 4;

/** -The origin (0, 0) is at the top left.
 *  -Functions suffixed with _nc take input coordinates in normalized form.
 *  The normalized equivalent of (x, y) is (x / screen_w, y / screen_h)
 */
class Renderer final
{
    enum class CoordSys {Absolute, NC};

    struct TextTextureCacheInfo final
    {
        const std::string text;
        const int font_id, size;
        const int wrap_length;
        const uint8_t r, g, b, a;

        TextTextureCacheInfo(std::string text_, int font_id_, int size_,
                             int wrap_length_,
                             uint8_t r_, uint8_t g_, uint8_t b_, uint8_t a_);
        bool operator < (const TextTextureCacheInfo &other) const;
    };

    struct TextTexture final
    {
        std::shared_ptr<Texture> text_texture;
        Time time_last_used;
        TextTexture(std::shared_ptr<Texture> text_texture_, Time time_last_used_);
    };

    struct GLDeleteContext
    {
        void operator()(void *context) const {SDL_GL_DeleteContext(context);}
    };

    SDL_Window *window;
    std::unique_ptr<void, GLDeleteContext> gl_context;

    std::map<TextTextureCacheInfo, TextTexture> text_cache;
    std::map<Time, std::set<TextTextureCacheInfo> > text_time_last_used;
    std::shared_ptr<const Font> current_font;

    //actually ints, but float for speed
    float renderer_w_div2;
    float renderer_h_div2;

    bool show_fps_toggle;
    std::shared_ptr<const Font> fps_font;
    gfx::SRGB_Color fps_color;
    std::queue<Time> frame_timestamps; /** used for keeping track of fps
                                        *  (contains all frame timestamps in the last second)
                                        */
    Texture *render_target;

    SRGB_Color draw_color;

    struct _Shaders
    {
        constexpr static size_t TRI_BUFFER_SIZE = 1e5;

        GLuint cur_program;
        GLuint active_VBO_id;
        GLuint active_EBO_id;
        GLuint active_UBO_id;

        std::unique_ptr<ShaderProgram> triangle;
        std::unique_ptr<VAO> triangle_vao;
        std::shared_ptr<VBO> triangle_vbo;
        std::vector<float> tri_buffer;
        decltype(tri_buffer.begin()) tri_buffer_ptr;

        std::unique_ptr<ShaderProgram> texture;
        std::unique_ptr<ShaderProgram> texture_ms;
        std::unique_ptr<VAO> texture_vao;
        std::shared_ptr<VBO> texture_vbo;
        std::array<float, 32> texture_buffer;

    } Shaders;

    template<CoordSys CS> inline float x_to_gl_coord(float x) const
    {
        if constexpr(CS == CoordSys::Absolute)
            return x/renderer_w_div2 - 1;
        else if constexpr(CS == CoordSys::NC)
            return x*2 - 1;
        else
            k_assert(false);
    }
    template<CoordSys CS> inline float y_to_gl_coord(float y) const
    {
        if constexpr(CS == CoordSys::Absolute)
            return 1 - y/renderer_h_div2;
        else if constexpr(CS == CoordSys::NC)
            return 1 - y*2;
        else
            k_assert(false);
    }

    void init_shaders();

    void flush_tri_buffer();

    template<CoordSys CS>
    void _fill_quad(const Point &p1, const Point &p2, const Point &p3, const Point &p4);
    template<CoordSys CS>
    void _draw_texture(const Texture &texture, const Rect &dst, const std::optional<Rect> &src_ = {});
public:
    ///how long a text texture can go unused for before getting evicted (around 1s works well)
    static const Time::Delta TEXT_TEXTURE_CACHE_TIME;

    Renderer(SDL_Window *window_, Uint32 flags);

    ///noncopyable because that's an AbstractWindow can only have one Renderer
    Renderer(const Renderer&) = delete;
    Renderer &operator = (const Renderer&) = delete;

    ///nonmovable because AbstractWindow holds a pointer to it
    Renderer(Renderer&&) = delete;
    Renderer &operator = (Renderer&&) = delete;

    inline float x_to_ndc(float x) const
    {
        return x_to_gl_coord<CoordSys::Absolute>(x);
    }
    inline float y_to_ndc(float y) const
    {
        return y_to_gl_coord<CoordSys::Absolute>(y);
    }
    inline float x_nc_to_ndc(float x) const
    {
        return x_to_gl_coord<CoordSys::NC>(x);
    }
    inline float y_nc_to_ndc(float y) const
    {
        return y_to_gl_coord<CoordSys::NC>(y);
    }

    inline float x_to_tex_coord(float x, [[maybe_unused]] int w) const
    {
        return x / w;
    }
    inline float y_to_tex_coord(float y, int h) const
    {
        return 1.0f - y / h;
    }
    inline float x_nc_to_tex_coord(float x, [[maybe_unused]] int w) const
    {
        return x;
    }
    inline float y_nc_to_tex_coord(float y, [[maybe_unused]] int h) const
    {
        return 1.0f - y;
    }

    /** cleans up old cached text textures. It's best to not call this
     *  directly and use the global clean_memory() function instead.
     */
    void clean_memory();

    void make_context_current();

    void set_color(SRGB_Color c);
    SRGB_Color get_color() const;

    void clear();

    void fill_quad(const Point &p1, const Point &p2, const Point &p3, const Point &p4);
    void fill_quad_nc(const Point &p1, const Point &p2, const Point &p3, const Point &p4);

    void fill_rect(const Rect &r);
    void fill_rect_nc(const Rect &r);
    void fill_rects(const std::vector<Rect> &rects);

    void draw_line(const Line &l);
    void draw_line(Point p1, Point p2);
    void draw_polyline(const std::vector<Point> &points);

    void draw_circle(Circle c); ///radius has to be in (1, 9999)
    void fill_circle(Circle c); ///radius has to be in (1, 9999)
    void fill_circles(const std::vector<Point> &centers, float r); ///radius has to be in (1, 9999)

    void fill_decaying_circle(Circle c); ///radius has to be in (1, 9999)
    void fill_decaying_circles(const std::vector<Point> &centers, float r); ///radius has to be in (1, 9999)

    void invert_rect_colors(const gfx::Rect &rect);

    void set_font(std::string font_name);
    void set_font(std::shared_ptr<const Font> font);
    const std::shared_ptr<const Font> &get_font() const;

    std::shared_ptr<Texture> get_text_texture(const std::string &text, int sz, SRGB_Color c = Color::BLACK);
    ///no const reference to the string because we mutate it
    std::shared_ptr<Texture> get_text_texture_wrapped(std::string text, int sz, int wrap_length,
                                                      SRGB_Color c = Color::BLACK);

    void draw_text(std::string text, float x, float y, int sz, SRGB_Color c = Color::BLACK);
    void draw_text_wrapped(std::string text, float x, float y, int sz, int wrap_length,
                           SRGB_Color c = Color::BLACK);

    std::unique_ptr<MonofontAtlas> create_monofont_atlas(const Font &font, int font_size, SRGB_Color color);

    void set_target(Texture *target);
    Texture *get_target() const;
    bool is_target_srgb() const;

    void set_viewport(const std::optional<IRect> &r);

    std::unique_ptr<Texture> make_texture_target(int w,
                                                 int h,
                                                 Texture::Format format = Texture::Format::RGBA8888,
                                                 bool is_srgb = true,
                                                 int sample = 1) const;
    [[deprecated]]
    std::unique_ptr<Texture> create_texture_target(int w,
                                                   int h,
                                                   Texture::Format format = Texture::Format::RGBA8888,
                                                   bool is_srgb = true,
                                                   int samples = 1) const;

    ///use these only for non-multisampled textures
    void draw_texture(const Texture &texture, const Rect &dst, const std::optional<Rect> &src_ = {});
    void draw_texture_nc(const Texture &texture, const Rect &dst, const std::optional<Rect> &src_ = {});
    void draw_texture_ex(const Texture &texture, const Rect &dst, const std::optional<Rect> &src_,
                         const std::optional<gfx::Point> &center, double angle, Flip flip);

    ///use this exclusively for multisampled textures
    void draw_texture_ms(const Texture &texture, const Rect &dst, const std::optional<Rect> &src_);

    void set_blend_factors(BlendFactor src, BlendFactor dst);

    /** These can be called without any preparation */
    template<class ...Args> std::unique_ptr<VAO> make_VAO(Args &&...args)
        {return std::unique_ptr<VAO>(new VAO(this, std::forward<Args>(args)...));}
    template<class ...Args> std::unique_ptr<VBO> make_VBO(Args &&...args)
        {return std::unique_ptr<VBO>(new VBO(this, std::forward<Args>(args)...));}
    template<class ...Args> std::unique_ptr<EBO> make_EBO(Args &&...args)
        {return std::unique_ptr<EBO>(new EBO(this, std::forward<Args>(args)...));}
    template<class ...Args> std::unique_ptr<UBO> make_UBO(Args &&...args)
        {return std::unique_ptr<UBO>(new UBO(this, std::forward<Args>(args)...));}

    template<class ...Args> std::unique_ptr<VertShader> make_vert_shader(Args &&...args)
        {return std::unique_ptr<VertShader>(new VertShader(std::forward<Args>(args)...));}
    template<class ...Args> std::unique_ptr<FragShader> make_frag_shader(Args &&...args)
        {return std::unique_ptr<FragShader>(new FragShader(std::forward<Args>(args)...));}
    template<class ...Args> std::unique_ptr<ShaderProgram> make_shader_program(Args &&...args)
        {return std::unique_ptr<ShaderProgram>(new ShaderProgram(this, std::forward<Args>(args)...));}

    GLuint get_cur_program(Passkey<ShaderProgram>);
    GLuint get_active_VBO_id(Passkey<VBO, VAO>);
    GLuint get_active_EBO_id(Passkey<EBO>);
    GLuint get_active_UBO_id(Passkey<UBO>);

    /** If you call draw_arrays/draw_elements with a custom shader, then you must call
     *  prepare_for_custom_shader before calling any of the opengl related
     *  functions in the block below it. This is necessary to flush any buffers.
     *  If you don't call this function and try to set up stuff for a custom shader,
     *  the buffers will get flushed if they are nonempty when draw_ is called,
     *  which involves likely undesired state changes (e.g. the VAO might be rebound
     *  in the process of flushing buffers).
     *
     *  Buffers are filled by the renderer when it is commanded to do predefined
     *  drawing commands. For example, when draw_quad is called, the renderer
     *  puts triangles in a buffer, and the triangles will be drawn when the buffer
     *  is flushed later.
     */
    void prepare_for_custom_shader();
    void bind_VAO(const VAO&);
    void bind_VBO(const VBO&);
    void bind_EBO(const EBO&);
    void bind_UBO(const UBO&);
    void bind_UB_base(GLuint index, const UBO&);
    void use_shader_program(const ShaderProgram&);
    void draw_arrays(DrawMode mode, GLint first, GLsizei count);
    void draw_arrays_instanced(DrawMode mode, GLint first, GLsizei count, GLsizei instance_cnt);
    void draw_elements(DrawMode mode, GLint first, GLsizei count);
    void set_active_texture(int tex_num);
    void bind_texture(const Texture &texture);

    void show_fps(bool toggle);
    int set_fps_font(std::shared_ptr<const Font> font);
    void set_fps_color(SRGB_Color color);
    void refresh();
    int get_fps() const;

    int get_num_samples() const;
};

}}
