#pragma once

#include "kx/gfx/renderer_types.h"
#include "kx/util.h"
#include "kx/kx_span.h"
#include "kx/time.h"

#include <string>
#include <memory>

struct SDL_Window;

namespace kx { namespace gfx {

template<class Deleter> class _GLUniqueObj final
{
public:
    uint32_t id;

    _GLUniqueObj() = default;
    _GLUniqueObj(uint32_t id_):
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
    void operator()(uint32_t id) const;
};
struct _GLDeleteShaderProgram final
{
    void operator()(uint32_t id) const;
};
struct _GLDeleteBuffer final
{
    void operator()(uint32_t id) const;
};
struct _GLDeleteVAO final
{
    void operator()(uint32_t id) const;
};
struct _GLDeleteTexture final
{
    void operator()(uint32_t id) const;
};
struct _GLDeleteFramebuffers final
{
    void operator()(uint32_t id) const;
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
    enum class Format {
        RGBA8888,
        RGB888,
        RGBA16F,
        RGB16F,
        R8
    };
private:
    friend class Renderer;

    static uint32_t active_texture;

    _GLUniqueObj<_GLDeleteTexture> texture;
    _GLUniqueObj<_GLDeleteFramebuffers> framebuffer;
    bool is_srgb_;
    bool has_mipmaps;
    int w;
    int h;
    Format format;
    ColorMod color_mod;
    float alpha_mod;
    uint32_t binding_point; //actually a GLenum
    int samples;

    Texture(int dim, int slices, int w_, int h_, Format format_, bool is_srgb__, int samples_, void *pixels);
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
    int get_num_samples() const;
    bool is_multisample() const;

    void make_targetable();

    void make_mipmaps();

    enum class FilterAlgo {
        Nearest = 0,
        Linear = 1,
        NearestMipmapNearest = 2,
        NearestMipmapLinear = 3,
        LinearMipmapNearest = 4,
        LinearMipmapLinear = 5
    };

    void set_min_filter(FilterAlgo algo);
    void set_mag_filter(FilterAlgo algo);
};

class Shader
{
    friend class ShaderProgram;
protected:
    _GLUniqueObj<_GLDeleteShader> shader;

    Shader();
    Shader(std::string_view file_name, uint32_t shader_type);
public:
};

class VertShader final: public Shader
{
    friend class Renderer;
    VertShader() = default;
    VertShader(std::string_view file_name);
};

class FragShader final: public Shader
{
    friend class Renderer;
    FragShader() = default;
    FragShader(std::string_view file_name);
};

using UniformLoc = int;
using UBIndex = uint32_t;

class ShaderProgram
{
    friend Renderer;
    Renderer *owner;
    _GLUniqueObj<_GLDeleteShaderProgram> program;

    ShaderProgram(Renderer *owner_, const VertShader &vert_shader, const FragShader &frag_shader);
public:
    UniformLoc get_uniform_loc(const char *name);

    //all set_uniform functions require the current program to be bound
    void set_uniform1i(int loc, int val);

    void set_uniform1f(int loc, float v0);
    void set_uniform2f(int loc, float v0, float v1);
    void set_uniform3f(int loc, float v0, float v1, float v3);
    void set_uniform4f(int loc, float v0, float v1, float v2, float v3);

    void set_uniform1fv(int loc, kx::kx_span<float> vals);
    void set_uniform2fv(int loc, kx::kx_span<float> vals);
    void set_uniform3fv(int loc, kx::kx_span<float> vals);
    void set_uniform4fv(int loc, kx::kx_span<float> vals);

    UBIndex get_UB_index(const char *name);

    void bind_UB(UBIndex ub_index, uint32_t binding);
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
    void buffer_data(const void *data, uint32_t n);
    void buffer_sub_data(intptr_t offset, const void *data, uint32_t n);
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
    void vertex_attrib_pointer_f(uint32_t pos, uint32_t size, int stride, uintptr_t offset);
    void enable_vertex_attrib_array(uint32_t pos);
};

struct GlyphMetrics
{
    float w;
    float h;
    float left_offset;
    float top_offset;
    float advance;
};

//this should probably have immutable fields
struct ASCII_Atlas
{
    static constexpr int MAX_ASCII_CHAR = 127;

    const Font *font;
    float font_size;
    float max_w;
    float max_h;

    float min_x1;
    float max_x2;
    float min_y1;
    float max_y2;

    std::unique_ptr<Texture> texture;
    std::array<bool, MAX_ASCII_CHAR+1> char_supported;
    std::array<GlyphMetrics, MAX_ASCII_CHAR+1> glyph_metrics;
    std::array<std::array<float, MAX_ASCII_CHAR+1>, MAX_ASCII_CHAR+1> kerning;
};

constexpr int NUM_SAMPLES_DEFAULT = 1;

using renderer_flags_t = uint32_t;
/** -The origin (0, 0) is at the top left.
 *  -Functions suffixed with _nc take input coordinates in normalized form.
 *  The normalized equivalent of (x, y) is (x / screen_w, y / screen_h)
 */
class Renderer final
{
    enum class CoordSys {Absolute, NC};

    struct GLDeleteContext
    {
        void operator()(void *context) const;
    };

    SDL_Window *window;
    std::unique_ptr<void, GLDeleteContext> gl_context;

    //actually ints, but float for speed
    float renderer_w;
    float renderer_h;
    float renderer_w_div2;
    float renderer_h_div2;

    Time cur_frame_start_time;
    //pair of start time of current frame, end time of current frame
    std::deque<std::pair<Time, Time>> frame_timestamps;
    Texture *render_target;

    std::pair<BlendFactor, BlendFactor> blend_factors;

    struct _Shaders
    {
        constexpr static size_t TRI_BUFFER_SIZE = 1e5;

        uint32_t cur_program;
        uint32_t active_VBO_id;
        uint32_t active_EBO_id;
        uint32_t active_UBO_id;

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
    Renderer(SDL_Window *window_, uint32_t flags);

    ///noncopyable because that's an AbstractWindow can only have one Renderer
    Renderer(const Renderer&) = delete;
    Renderer &operator = (const Renderer&) = delete;

    ///nonmovable because AbstractWindow holds a pointer to it
    Renderer(Renderer&&) = delete;
    Renderer &operator = (Renderer&&) = delete;

    inline float is_x_ndc_in_view(float x) const
    {
        return x>-1.0f && x<1.0f;
    }
    inline float is_y_ndc_in_view(float y) const
    {
        return y>-1.0f && y<1.0f;
    }
    inline bool is_x_line_ndc_in_view(float x1, float x2) const
    {
        return !(x1<=-1 && x2<=-1) && !(x1>=1 && x2>=1);
    }
    inline bool is_y_line_ndc_in_view(float y1, float y2) const
    {
        return !(y1<=-1 && y2<=-1) && !(y1>=1 && y2>=1);
    }
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
    inline float x_nc_to_dc(float x) const
    {
        return x * renderer_w;
    }
    inline float y_nc_to_dc(float y) const
    {
        return y * renderer_h;
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

    /** Doesn't do anything right now, but might in the future
     */
    void clean_memory();

    void make_context_current();

    void clear(const Color4f& color);

    std::unique_ptr<ASCII_Atlas> make_ascii_atlas(Font *font, int font_size);

    void set_target(Texture *target);
    Texture *get_target() const;
    bool is_target_srgb() const;

    void set_viewport(const std::optional<IRect> &r);
    int get_render_w() const;
    int get_render_h() const;

    std::unique_ptr<Texture> make_texture_2d_array(int w,
                                                   int h,
                                                   int slices,
                                                   Texture::Format format = Texture::Format::RGBA8888,
                                                   bool is_srgb = true,
                                                   int samples = 1,
                                                   void *pixels = nullptr) const;

    std::unique_ptr<Texture> make_texture_target(int w,
                                                 int h,
                                                 Texture::Format format = Texture::Format::RGBA8888,
                                                 bool is_srgb = true,
                                                 int samples = 1,
                                                 void *pixels = nullptr) const;

    /** The draw_texture_* functions rebind the shader program, VBO, and VAO.
     */
    ///use these only for non-multisampled textures
    void draw_texture(const Texture &texture, const Rect &dst, const std::optional<Rect> &src_ = {});
    void draw_texture_nc(const Texture &texture, const Rect &dst, const std::optional<Rect> &src_ = {});
    void draw_texture_ex(const Texture &texture, const Rect &dst, const std::optional<Rect> &src_,
                         const std::optional<gfx::Point> &center, double angle, Flip flip);

    ///use this exclusively for multisampled textures
    void draw_texture_ms(const Texture &texture, const Rect &dst, const std::optional<Rect> &src_);

    void set_blend_factors(BlendFactor src, BlendFactor dst);
    void set_blend_factors(const std::pair<BlendFactor, BlendFactor> &factors);
    const std::pair<BlendFactor, BlendFactor>& get_blend_factors() const;

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

    uint32_t get_cur_program(Passkey<ShaderProgram>);
    uint32_t get_active_VBO_id(Passkey<VBO, VAO>);
    uint32_t get_active_EBO_id(Passkey<EBO>);
    uint32_t get_active_UBO_id(Passkey<UBO>);

    void bind_VAO(const VAO&);
    void bind_VBO(const VBO&);
    void bind_EBO(const EBO&);
    void bind_UBO(const UBO&);
    void bind_UB_base(uint32_t index, const UBO&);
    void use_shader_program(const ShaderProgram&);
    void draw_arrays(DrawMode mode, int first, int count);
    void draw_arrays_instanced(DrawMode mode, int first, int count, int instance_cnt);
    void draw_elements(DrawMode mode, int first, int count);
    void set_active_texture(int tex_num);
    void bind_texture(const Texture &texture);

    void refresh();
    float get_fps() const;
    //float get_estimated_program_load() const;

    int get_num_samples() const;
};

}}
