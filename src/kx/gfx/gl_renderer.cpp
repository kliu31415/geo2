#include "kx/gfx/renderer.h"

#ifdef KX_RENDERER_GL

#endif


#include "kx/gfx/renderer.h"
#include "kx/io.h"

#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL_image.h>
#include <algorithm>
#include <cmath>
#include <climits>

#include "kx/debug.h"

#ifdef KX_DEBUG
#define SDL_L0_FUNC(func, ...) {if(func(__VA_ARGS__) < 0) \
                               {log_error(#func + (std::string)" failed: " + SDL_GetError());}}
#else
#define SDL_L0_FUNC(func, ...) {func(__VA_ARGS__);}
#endif

namespace kx { namespace gfx {

//non-macro version of SDL_L0_FUNC (the macro version can get the name of the func with "#func",
//which is pretty useful, so the macro version is being used rn)
//I THINK this works if args... contains a reference (std::forward should handle it) but I'm not sure
/*template<class Func, class ...Args> void SDL_L0_FUNC(Func func, Args &&...args)
{
    #ifdef KX_DEBUG
    if(func(std::forward<Args>(args)...) < 0) {
        log_error("SDL_L0_FUNC failed: " + (std::string)SDL_GetError());
    }
    #else
    func(std::forward<Args>(args)...);
    #endif
}*/

static_assert(sizeof(LinearColor) == 16);
static_assert(sizeof(SRGB_Color) == 16);
static_assert(offsetof(LinearColor, r) == offsetof(SRGB_Color, r));
static_assert(offsetof(LinearColor, g) == offsetof(SRGB_Color, g));
static_assert(offsetof(LinearColor, b) == offsetof(SRGB_Color, b));
static_assert(offsetof(LinearColor, a) == offsetof(SRGB_Color, a));

LinearColor::LinearColor(float r_, float g_, float b_, float a_):
    r(r_), g(g_), b(b_), a(a_)
{}

static float srgb_to_linear(float in)
{
    if(in <= 0.04045)
        return in / 12.92;
    else
        return std::pow((in+0.055)/1.055, 2.4);
}
LinearColor::LinearColor(const SRGB_Color &color):
    r(srgb_to_linear(color.r)),
    g(srgb_to_linear(color.g)),
    b(srgb_to_linear(color.b)),
    a(color.a)
{}

SRGB_Color::SRGB_Color(SDL_Color sdl_color):
    r(sdl_color.r / 255.999),
    g(sdl_color.g / 255.999),
    b(sdl_color.b / 255.999),
    a(sdl_color.a / 255.999)
{}
SRGB_Color::SRGB_Color(float r_, float g_, float b_, float a_):
    r(r_), g(g_), b(b_), a(a_)
{}
static float linear_to_srgb(float in)
{
    if(in <= 0.0031308)
        return in * 12.92;
    else
        return 1.055 * std::pow(in, 1.0 / 2.4) - 0.055;
}
SRGB_Color::SRGB_Color(const LinearColor &color):
    r(linear_to_srgb(color.r)),
    g(linear_to_srgb(color.g)),
    b(linear_to_srgb(color.b)),
    a(color.a)
{}
SRGB_Color::operator SDL_Color() const
{
    return SDL_Color{(Uint8)(r*255.999), (Uint8)(g*255.999), (Uint8)(b*255.999), (Uint8)(a*255.999)};
}

Point::Point(float x_, float y_):
    x(x_),
    y(y_)
{}

Line::Line(Point p1_, Point p2_):
    p1(p1_),
    p2(p2_)
{}

Rect::Rect(float x_, float y_, float w_, float h_):
    x(x_), y(y_), w(w_), h(h_)
{}
Rect::Rect(const IRect &other):
    x(other.x), y(other.y), w(other.w), h(other.h)
{}
bool Rect::point_inside(gfx::Point point) const
{
    return point.x>=x && point.x<x+w && point.y>=y && point.y<y+h;
}

IRect::IRect(int x_, int y_, int w_, int h_):
    x(x_), y(y_), w(w_), h(h_)
{}
bool IRect::point_inside(gfx::Point point) const
{
    return point.x>=x && point.x<x+w && point.y>=y && point.y<y+h;
}

Circle::Circle(float x_, float y_, float r_):
    x(x_), y(y_), r(r_)
{}
bool Circle::point_inside(gfx::Point point) const
{
    return std::pow(x-point.x, 2) + std::pow(y-point.y, 2) < std::pow(r, 2);
}

ColorMod::ColorMod(float r_, float g_, float b_):
    r(r_),
    g(g_),
    b(b_)
{}

Texture::Texture(int w_, int h_, Texture::Format format_, bool is_srgb__, int samples_):
    framebuffer(0),
    is_srgb_(is_srgb__),
    has_mipmaps(false),
    w(w_),
    h(h_),
    format(format_),
    color_mod(1.0, 1.0, 1.0),
    alpha_mod(1.0),
    samples(samples_)
{
    if(samples == 1)
        binding_point = GL_TEXTURE_2D;
    else
        binding_point = GL_TEXTURE_2D_MULTISAMPLE;

    glGenTextures(1, &texture.id);
    glBindTexture(binding_point, texture.id);

    if(binding_point == GL_TEXTURE_2D) {
        glTexParameteri(binding_point, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(binding_point, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(binding_point, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
        glTexParameteri(binding_point, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    }

    switch(format) {
    case Texture::Format::RGBA8888:
        if(binding_point == GL_TEXTURE_2D)
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
        else
            glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, samples, GL_RGBA, w, h, false);
        break;
    case Texture::Format::RGBA16F:
        if(binding_point == GL_TEXTURE_2D)
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, w, h, 0, GL_RGBA, GL_FLOAT, nullptr);
        else
            glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, samples, GL_RGBA16F, w, h, false);
        break;
    case Texture::Format::RGB16F:
        if(binding_point == GL_TEXTURE_2D)
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, w, h, 0, GL_RGB, GL_FLOAT, nullptr);
        else
            glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, samples, GL_RGB16F, w, h, false);
        break;
    default:
        log_error("bad texture format given to ctor");
        break;
    }
}
Texture::Texture(SDL_Surface *s, bool is_srgb__):
    framebuffer(0),
    is_srgb_(is_srgb__),
    has_mipmaps(false),
    w(s->w),
    h(s->h),
    color_mod(1.0, 1.0, 1.0),
    alpha_mod(1.0),
    binding_point(GL_TEXTURE_2D),
    samples(1)
{
    //-Hasn't been tested for 24-bit textures without an alpha channel
    //-I believe we assume little endian here

    glGenTextures(1, &texture.id);
    glBindTexture(binding_point, texture.id);

    glTexParameteri(binding_point, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(binding_point, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(binding_point, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(binding_point, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);

    auto bpp = s->format->BytesPerPixel;
    //rn, there's no support for anything except 24-bit color with a possible alpha channel
    k_expects(bpp==3 || bpp==4);

    //ensure each of R, G, and B have 8 consecutive bits
    k_expects(s->format->Rmask / (s->format->Rmask & (-s->format->Rmask)) == 0xff);
    k_expects(s->format->Gmask / (s->format->Gmask & (-s->format->Gmask)) == 0xff);
    k_expects(s->format->Bmask / (s->format->Bmask & (-s->format->Bmask)) == 0xff);

    SDL_LockSurface(s);
    std::vector<uint8_t> pixels(w * h * bpp);
    int Rshift = std::log2(s->format->Rmask / 0xff);
    int Gshift = std::log2(s->format->Gmask / 0xff);
    int Bshift = std::log2(s->format->Bmask / 0xff);
    int Ashift = std::log2(s->format->Amask / 0xff);
    for(int r=0; r<h; r++) {
        for(int c=0; c<w; c++) {
            auto pixel = *(uint32_t*)((uint8_t*)s->pixels + r * s->pitch + c * bpp + 4 - bpp);
            pixels[bpp * ((h-r-1)*w + c) + 0] = (s->format->Rmask & pixel) >> Rshift;
            pixels[bpp * ((h-r-1)*w + c) + 1] = (s->format->Gmask & pixel) >> Gshift;
            pixels[bpp * ((h-r-1)*w + c) + 2] = (s->format->Bmask & pixel) >> Bshift;
            if(bpp == 4)
                pixels[bpp * ((h-r-1)*w + c) + 3] = (s->format->Amask & pixel) >> Ashift;
        }
    }

    SDL_UnlockSurface(s);

    if(bpp == 4) {
        glTexImage2D(binding_point, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());
        format = Texture::Format::RGBA8888;
    } else {
        glTexImage2D(binding_point, 0, GL_RGB, w, h, 0, GL_RGB, GL_UNSIGNED_BYTE, pixels.data());
        format = Texture::Format::RGB888;
    }
}
void Texture::set_color_mod(float r, float g, float b)
{
    constexpr float MULT = 255.999;
    color_mod = ColorMod(r*MULT, g*MULT, b*MULT);
}
void Texture::set_alpha_mod(float a)
{
    constexpr double MULT = 255.999;
    alpha_mod = a*MULT;
}
int Texture::get_w() const
{
    return w;
}
int Texture::get_h() const
{
    return h;
}
Texture::Format Texture::get_format() const
{
    return format;
}
bool Texture::is_srgb() const
{
    return is_srgb_;
}
int Texture::get_num_samples() const
{
    return samples;
}
bool Texture::is_multisample() const
{
    return binding_point == GL_TEXTURE_2D_MULTISAMPLE;
}
void Texture::make_targetable()
{
    //if we already have a framebuffer associated with this texture, don't create another
    if(framebuffer.id != 0)
        return;

    GLint fb;
    glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &fb);

    glGenFramebuffers(1, &framebuffer.id);
    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer.id);
    glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, texture.id, 0);
    glDrawBuffer(GL_COLOR_ATTACHMENT0);
    if(glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        log_error("failed to create framebuffer for texture");

    glBindFramebuffer(GL_FRAMEBUFFER, fb);
}
void Texture::set_min_filter(Texture::FilterAlgo algo)
{
    k_expects(!is_multisample());
    glBindTexture(binding_point, texture.id);
    glTexParameteri(binding_point, GL_TEXTURE_MIN_FILTER, (GLenum)algo);
}
void Texture::set_mag_filter(Texture::FilterAlgo algo)
{
    k_expects(!is_multisample());
    glBindTexture(binding_point, texture.id);
    glTexParameteri(binding_point, GL_TEXTURE_MAG_FILTER, (GLenum)algo);
}

Shader::Shader():
    shader(0)
{}

static constexpr size_t INFO_LOG_SIZE = 1024;
static thread_local std::string info_log(INFO_LOG_SIZE, '\0');

Shader::Shader(const std::string &file_name, GLenum shader_type)
{
    shader.id = glCreateShader(shader_type);
    auto shader_src_opt = io::read_binary_file(file_name.c_str());
    if(!shader_src_opt) {
        log_error("failed to open shader file \"" + file_name + "\"");
    }
    auto shader_src = *shader_src_opt;
    auto shader_src_cstr = shader_src.c_str();
    glShaderSource(shader.id, 1, &shader_src_cstr, nullptr);
    glCompileShader(shader.id);
    GLint success;
    glGetShaderiv(shader.id, GL_COMPILE_STATUS, &success);
    if(!success) {
        GLsizei len;
        glGetShaderInfoLog(shader.id, INFO_LOG_SIZE, &len, info_log.data());
        log_error("Shader \"" + file_name + "\" compilation failed: " + info_log.substr(0, len));
    }
}

VertShader::VertShader(const std::string &file_name):
    Shader(file_name, GL_VERTEX_SHADER)
{}

FragShader::FragShader(const std::string &file_name):
    Shader(file_name, GL_FRAGMENT_SHADER)
{}

ShaderProgram::ShaderProgram(Renderer *owner_, const VertShader &vert_shader, const FragShader &frag_shader):
    owner(owner_)
{
    program.id = glCreateProgram();
    glAttachShader(program.id, vert_shader.shader.id);
    glAttachShader(program.id, frag_shader.shader.id);
    glLinkProgram(program.id);
    GLint success;
    glGetProgramiv(program.id, GL_LINK_STATUS, &success);
    if(!success) {
        GLsizei len;
        glGetProgramInfoLog(program.id, INFO_LOG_SIZE, &len, info_log.data());
        log_error("Error: program compilation failed: " + info_log.substr(0, len));
    }
}
UniformLoc ShaderProgram::get_uniform_loc(const GLchar *name)
{
    return glGetUniformLocation(program.id, name);
}

void ShaderProgram::set_uniform1i(GLint loc, GLint val)
{
    k_expects(owner->get_cur_program({}) == program.id);
    glUniform1i(loc, val);
}

void ShaderProgram::set_uniform1f(GLint loc, GLfloat v0)
{
    k_expects(owner->get_cur_program({}) == program.id);
    glUniform1f(loc, v0);
}
void ShaderProgram::set_uniform2f(GLint loc, GLfloat v0, GLfloat v1)
{
    k_expects(owner->get_cur_program({}) == program.id);
    glUniform2f(loc, v0, v1);
}
void ShaderProgram::set_uniform3f(GLint loc, GLfloat v0, GLfloat v1, GLfloat v2)
{
    k_expects(owner->get_cur_program({}) == program.id);
    glUniform3f(loc, v0, v1, v2);
}
void ShaderProgram::set_uniform4f(GLint loc, GLfloat v0, GLfloat v1, GLfloat v2, GLfloat v3)
{
    k_expects(owner->get_cur_program({}) == program.id);
    glUniform4f(loc, v0, v1, v2, v3);
}

void ShaderProgram::set_uniform1fv(GLint loc, nonstd::span<GLfloat> vals)
{
    k_expects(owner->get_cur_program({}) == program.id);
    glUniform1fv(loc, vals.size(), vals.begin());
}
void ShaderProgram::set_uniform2fv(GLint loc, nonstd::span<GLfloat> vals)
{
    k_expects(owner->get_cur_program({}) == program.id);
    glUniform2fv(loc, vals.size(), vals.begin());
}
void ShaderProgram::set_uniform3fv(GLint loc, nonstd::span<GLfloat> vals)
{
    k_expects(owner->get_cur_program({}) == program.id);
    glUniform3fv(loc, vals.size(), vals.begin());
}
void ShaderProgram::set_uniform4fv(GLint loc, nonstd::span<GLfloat> vals)
{
    k_expects(owner->get_cur_program({}) == program.id);
    glUniform4fv(loc, vals.size(), vals.begin());
}

UBIndex ShaderProgram::get_UB_index(const GLchar *name)
{
    return glGetUniformBlockIndex(program.id, name);
}

void ShaderProgram::bind_UB(UBIndex ub_index, GLuint binding)
{
    glUniformBlockBinding(program.id, ub_index, binding);
}

template<_BufferType T> Buffer<T>::Buffer(Renderer *owner_):
    owner(owner_)
{
    glGenBuffers(1, &buffer.id);
}
template<_BufferType T> void Buffer<T>::ensure_active()
{
    #ifdef KX_DEBUG
    if constexpr(T == _BufferType::VBO)
        k_expects(owner->get_active_VBO_id({}) == buffer.id);
    else if constexpr(T == _BufferType::EBO)
        k_expects(owner->get_active_EBO_id({}) == buffer.id);
    else if constexpr(T == _BufferType::UBO)
        k_expects(owner->get_active_UBO_id({}) == buffer.id);
    else
        k_assert(false, "bad _BufferType in ensure_active()");
    #endif
}
template<_BufferType T> void Buffer<T>::buffer_data(void *data, GLuint n)
{
    ensure_active();
    if constexpr(T == _BufferType::VBO)
        glBufferData(GL_ARRAY_BUFFER, n, data, GL_DYNAMIC_DRAW);
    else if constexpr(T == _BufferType::EBO)
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, n, data, GL_DYNAMIC_DRAW);
    else if constexpr(T == _BufferType::UBO)
        glBufferData(GL_UNIFORM_BUFFER, n, data, GL_DYNAMIC_DRAW);
    else
        k_assert(false);
}
template<_BufferType T> void Buffer<T>::buffer_sub_data(GLintptr offset, void *data, GLuint n)
{
    ensure_active();
    if constexpr(T == _BufferType::VBO)
        glBufferSubData(GL_ARRAY_BUFFER, offset, n, data);
    else if constexpr(T == _BufferType::EBO)
        glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, offset, n, data);
    else if constexpr(T == _BufferType::UBO)
        glBufferSubData(GL_UNIFORM_BUFFER, offset, n, data);
    else
        k_assert(false);
}
template<_BufferType T> void Buffer<T>::invalidate()
{
    glInvalidateBufferData(buffer.id);
}

VAO::VAO(Renderer *owner_):
    owner(owner_)
{
    glGenVertexArrays(1, &vao.id);
}
void VAO::add_VBO(std::shared_ptr<VBO> vbo)
{
    vbos.push_back(std::move(vbo));
}
void VAO::add_EBO(std::shared_ptr<EBO> ebo)
{
    ebos.push_back(std::move(ebo));
}
void VAO::vertex_attrib_pointer_f(GLuint pos, GLuint size, GLsizei stride, uintptr_t offset)
{
    #ifdef KX_DEBUG
    bool found = false;
    auto active_vbo_id = owner->get_active_VBO_id({});
    for(const auto &vbo: vbos) {
        if(active_vbo_id == vbo->buffer.id) {
            found = true;
            break;
        }
    }
    k_expects(found, "VBO used in VAO but not assigned");
    #endif
    glVertexAttribPointer(pos, size, GL_FLOAT, GL_FALSE, stride, (void*)offset);
}
void VAO::enable_vertex_attrib_array(GLuint pos)
{
    glEnableVertexAttribArray(pos);
}

const Time::Delta Renderer::TEXT_TEXTURE_CACHE_TIME(1.1, Time::Length::second);

Renderer::TextTextureCacheInfo::TextTextureCacheInfo(std::string text_, int font_id_, int size_,
                                                     int wrap_length_,
                                                     uint8_t r_, uint8_t g_, uint8_t b_, uint8_t a_):
    text(std::move(text_)),
    font_id(font_id_),
    size(size_),
    wrap_length(wrap_length_),
    r(r_), g(g_), b(b_), a(a_)
{}
bool Renderer::TextTextureCacheInfo::operator < (const TextTextureCacheInfo &other) const
{
    if(text != other.text)
        return text < other.text;
    if(font_id != other.font_id)
        return font_id < other.font_id;
    if(size != other.size)
        return size < other.size;
    if(wrap_length != other.wrap_length)
        return wrap_length < other.wrap_length;
    if(r != other.r)
        return r < other.r;
    if(g != other.g)
        return g < other.g;
    if(b != other.b)
        return b < other.b;
    return a < other.a;
}

Renderer::TextTexture::TextTexture(std::shared_ptr<Texture> text_texture_, Time time_last_used_):
    text_texture(std::move(text_texture_)),
    time_last_used(time_last_used_)
{}

void Renderer::init_shaders()
{
    Shaders.cur_program = 0;
    Shaders.active_VBO_id = 0;
    Shaders.active_EBO_id = 0;
    Shaders.active_UBO_id = 0;

    //triangle
    Shaders.tri_buffer.resize(Shaders.TRI_BUFFER_SIZE);
    Shaders.tri_buffer_ptr = Shaders.tri_buffer.begin();

    auto tri_vert = make_vert_shader("kx_data/shaders/triangle.vert");
    auto tri_frag = make_frag_shader("kx_data/shaders/triangle.frag");
    Shaders.triangle = make_shader_program(*tri_vert, *tri_frag);
    Shaders.triangle_vao = make_VAO();
    Shaders.triangle_vbo = make_VBO();
    bind_VAO(*Shaders.triangle_vao);
    bind_VBO(*Shaders.triangle_vbo);
    Shaders.triangle_vbo->buffer_data(nullptr, Shaders.tri_buffer.size() * sizeof(Shaders.tri_buffer[0]));
    Shaders.triangle_vao->add_VBO(Shaders.triangle_vbo);
    Shaders.triangle_vao->vertex_attrib_pointer_f(0, 2, 6*sizeof(float), 0*sizeof(float)); //positions
    Shaders.triangle_vao->vertex_attrib_pointer_f(1, 4, 6*sizeof(float), 2*sizeof(float)); //colors
    Shaders.triangle_vao->enable_vertex_attrib_array(0);
    Shaders.triangle_vao->enable_vertex_attrib_array(1);

    //texture
    auto texture_vert = make_vert_shader("kx_data/shaders/texture.vert");
    auto texture_frag = make_frag_shader("kx_data/shaders/texture.frag");
    Shaders.texture = make_shader_program(*texture_vert, *texture_frag);

    auto texture_vert_ms = make_vert_shader("kx_data/shaders/texture_ms.vert");
    auto texture_frag_ms = make_frag_shader("kx_data/shaders/texture_ms.frag");
    Shaders.texture_ms = make_shader_program(*texture_vert_ms, *texture_frag_ms);

    Shaders.texture_vao = make_VAO();
    Shaders.texture_vbo = make_VBO();
    bind_VAO(*Shaders.texture_vao);
    bind_VBO(*Shaders.texture_vbo);
    Shaders.texture_vbo->buffer_data(nullptr, 32*sizeof(float));
    Shaders.texture_vao->add_VBO(Shaders.texture_vbo);
    Shaders.texture_vao->vertex_attrib_pointer_f(0, 2, 8*sizeof(float), 0*sizeof(float)); //positions
    Shaders.texture_vao->vertex_attrib_pointer_f(1, 2, 8*sizeof(float), 2*sizeof(float)); //tex coord
    Shaders.texture_vao->vertex_attrib_pointer_f(2, 4, 8*sizeof(float), 4*sizeof(float)); //color/alpha mods
    Shaders.texture_vao->enable_vertex_attrib_array(0);
    Shaders.texture_vao->enable_vertex_attrib_array(1);
    Shaders.texture_vao->enable_vertex_attrib_array(2);
}
void Renderer::flush_tri_buffer()
{
    use_shader_program(*Shaders.triangle);
    bind_VAO(*Shaders.triangle_vao);
    bind_VBO(*Shaders.triangle_vbo);

    auto &cpu_vbo = Shaders.tri_buffer;
    auto vbo_len = Shaders.tri_buffer_ptr - cpu_vbo.begin();
    Shaders.triangle_vbo->buffer_sub_data(0, cpu_vbo.data(), vbo_len*sizeof(cpu_vbo[0]));

    draw_arrays(DrawMode::TriangleStrip, 0, vbo_len);

    Shaders.triangle_vbo->invalidate();

    Shaders.tri_buffer_ptr = cpu_vbo.begin();
}
template<Renderer::CoordSys CS>
void Renderer::_fill_quad(const Point &p1, const Point &p2, const Point &p3, const Point &p4)
{
    if(Shaders.tri_buffer.end() - Shaders.tri_buffer_ptr < 36)
        flush_tri_buffer();

    //p1
    *Shaders.tri_buffer_ptr++ = x_to_gl_coord<CS>(p1.x);
    *Shaders.tri_buffer_ptr++ = y_to_gl_coord<CS>(p1.y);

    if(is_target_srgb()) {
        *Shaders.tri_buffer_ptr++ = draw_color.r;
        *Shaders.tri_buffer_ptr++ = draw_color.g;
        *Shaders.tri_buffer_ptr++ = draw_color.b;
        *Shaders.tri_buffer_ptr++ = draw_color.a;
    } else {
        LinearColor real_draw_color(draw_color);
        *Shaders.tri_buffer_ptr++ = real_draw_color.r;
        *Shaders.tri_buffer_ptr++ = real_draw_color.g;
        *Shaders.tri_buffer_ptr++ = real_draw_color.b;
        *Shaders.tri_buffer_ptr++ = real_draw_color.a;
    }

    for(int i=0; i<6; i++) {
        *Shaders.tri_buffer_ptr = *(Shaders.tri_buffer_ptr - 6);
        Shaders.tri_buffer_ptr++;
    }

    //p2
    *Shaders.tri_buffer_ptr++ = x_to_gl_coord<CS>(p2.x);
    *Shaders.tri_buffer_ptr++ = y_to_gl_coord<CS>(p2.y);
    for(int i=0; i<4; i++) {
        *Shaders.tri_buffer_ptr = *(Shaders.tri_buffer_ptr - 6);
        Shaders.tri_buffer_ptr++;
    }

    //p4
    *Shaders.tri_buffer_ptr++ = x_to_gl_coord<CS>(p4.x);
    *Shaders.tri_buffer_ptr++ = y_to_gl_coord<CS>(p4.y);
    for(int i=0; i<4; i++) {
        *Shaders.tri_buffer_ptr = *(Shaders.tri_buffer_ptr - 6);
        Shaders.tri_buffer_ptr++;
    }

    //p3
    *Shaders.tri_buffer_ptr++ = x_to_gl_coord<CS>(p3.x);
    *Shaders.tri_buffer_ptr++ = y_to_gl_coord<CS>(p3.y);
    for(int i=0; i<4; i++) {
        *Shaders.tri_buffer_ptr = *(Shaders.tri_buffer_ptr - 6);
        Shaders.tri_buffer_ptr++;
    }

    for(int i=0; i<6; i++) {
        *Shaders.tri_buffer_ptr = *(Shaders.tri_buffer_ptr - 6);
        Shaders.tri_buffer_ptr++;
    }
}
template<Renderer::CoordSys CS>
void Renderer::_draw_texture(const Texture &texture, const Rect &dst, const std::optional<Rect> &src_)
{
    k_expects(texture.binding_point == GL_TEXTURE_2D);

    Rect src;
    if(src_.has_value()) {
        float w = texture.get_w();
        float h = texture.get_h();
        if constexpr(CS == CoordSys::Absolute)
            src = Rect(src_->x / w, src_->y / h, src_->w / w, src_->h / h);
        else if constexpr(CS == CoordSys::NC)
            src = *src_;
        else
            log_error("unsupported coord system in kx::gfx::Renderer::_draw_texture<CS>");
    } else {
        src = Rect(0.0, 0.0, 1.0, 1.0);
    }

    //vertex 1
    Shaders.texture_buffer[0] = x_to_gl_coord<CS>(dst.x);
    Shaders.texture_buffer[1] = y_to_gl_coord<CS>(dst.y);
    Shaders.texture_buffer[2] = src.x;
    Shaders.texture_buffer[3] = src.y + src.h;
    Shaders.texture_buffer[4] = texture.color_mod.r;
    Shaders.texture_buffer[5] = texture.color_mod.g;
    Shaders.texture_buffer[6] = texture.color_mod.b;
    Shaders.texture_buffer[7] = texture.alpha_mod;

    //vertex 2
    Shaders.texture_buffer[8] = x_to_gl_coord<CS>(dst.x + dst.w);
    Shaders.texture_buffer[9] = y_to_gl_coord<CS>(dst.y);
    Shaders.texture_buffer[10] = src.x + src.w;
    Shaders.texture_buffer[11] = src.y + src.h;
    for(int i=0; i<4; i++)
        Shaders.texture_buffer[12 + i] = Shaders.texture_buffer[4 + i];

    //vertex 3
    Shaders.texture_buffer[16] = x_to_gl_coord<CS>(dst.x);
    Shaders.texture_buffer[17] = y_to_gl_coord<CS>(dst.y + dst.h);
    Shaders.texture_buffer[18] = src.x;
    Shaders.texture_buffer[19] = src.y;
    for(int i=0; i<4; i++)
        Shaders.texture_buffer[20 + i] = Shaders.texture_buffer[4 + i];

    //vertex 4
    Shaders.texture_buffer[24] = x_to_gl_coord<CS>(dst.x + dst.w);
    Shaders.texture_buffer[25] = y_to_gl_coord<CS>(dst.y + dst.h);
    Shaders.texture_buffer[26] = src.x + src.w;
    Shaders.texture_buffer[27] = src.y;
    for(int i=0; i<4; i++)
        Shaders.texture_buffer[28 + i] = Shaders.texture_buffer[4 + i];

    if(Shaders.tri_buffer_ptr != Shaders.tri_buffer.begin())
        flush_tri_buffer();

    use_shader_program(*Shaders.texture);
    bind_VAO(*Shaders.texture_vao);
    bind_VBO(*Shaders.texture_vbo);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(texture.binding_point, texture.texture.id);
    auto pid = Shaders.texture->program.id;
    glUniform1i(glGetUniformLocation(pid, "texture_in"), 0);
    int conversion_algo = is_target_srgb() - (int)texture.is_srgb();
    glUniform1i(glGetUniformLocation(pid, "conversion_algo"), conversion_algo);
    Shaders.texture_vbo->buffer_sub_data(0, Shaders.texture_buffer.data(), 32*sizeof(float));
    draw_arrays(DrawMode::TriangleStrip, 0, 4);
    Shaders.texture_vbo->invalidate();
}
Renderer::Renderer(SDL_Window *window_, [[maybe_unused]] Uint32 flags):
    window(window_),
    gl_context([=]() -> std::unique_ptr<void, GLDeleteContext>
    {
        k_expects(window != nullptr);
        auto context = std::unique_ptr<void, GLDeleteContext>((void*)SDL_GL_CreateContext(window), GLDeleteContext());

        if(gl_context == nullptr)
            log_error(SDL_GetError());

        if (!gladLoadGLLoader((GLADloadproc) SDL_GL_GetProcAddress))
            log_error(SDL_GetError());
        return context;
    }()),
    current_font(Font::DEFAULT),
    show_fps_toggle(false),
    fps_font(Font::DEFAULT),
    fps_color(gfx::Color::BLACK),
    render_target(nullptr),
    Shaders()
{
    glEnable(GL_MULTISAMPLE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    SDL_GL_SetSwapInterval(1);

    int w;
    int h;
    SDL_GetWindowSize(window_, &w, &h);
    glViewport(0, 0, w, h);
    renderer_w_div2 = w / 2.0;
    renderer_h_div2 = h / 2.0;

    init_shaders();
}
void Renderer::clean_memory()
{
    auto cutoff = Time::now() - TEXT_TEXTURE_CACHE_TIME;
    for(auto i = text_time_last_used.begin(); i != text_time_last_used.end(); ) {
        auto cached = *i;
        if(cached.first > cutoff) { //all the remaining text textures have been in use recently
            break;
        } else {
            for(const auto &cache_info: cached.second) { //clean up the old cached textures here
                auto cached_texture = text_cache.find(cache_info);
                k_ensures(cached_texture != text_cache.end()); //the cached texture doesn't exist somehow?

                if(cached_texture != text_cache.end())
                    text_cache.erase(cached_texture);
            }
            i = text_time_last_used.erase(i);
        }
    }
}
void Renderer::make_context_current()
{
    SDL_GL_MakeCurrent(window, gl_context.get());
}
void Renderer::set_color(SRGB_Color c)
{
    draw_color = c;
}
SRGB_Color Renderer::get_color() const
{
    return draw_color;
}
void Renderer::clear()
{
    glClearColor(draw_color.r, draw_color.g, draw_color.b, draw_color.a);
    glClear(GL_COLOR_BUFFER_BIT);
}

void Renderer::fill_quad(const Point &p1, const Point &p2, const Point &p3, const Point &p4)
{
    _fill_quad<Renderer::CoordSys::Absolute>(p1, p2, p3, p4);
}
void Renderer::fill_quad_nc(const Point &p1, const Point &p2, const Point &p3, const Point &p4)
{
    _fill_quad<Renderer::CoordSys::NC>(p1, p2, p3, p4);
}
void Renderer::fill_rect(const Rect &r)
{
    fill_quad({r.x, r.y}, {r.x + r.w, r.y}, {r.x + r.w, r.y + r.h}, {r.x, r.y + r.h});
}
void Renderer::fill_rect_nc(const Rect &r)
{
    fill_quad_nc({r.x, r.y}, {r.x + r.w, r.y}, {r.x + r.w, r.y + r.h}, {r.x, r.y + r.h});
}
void Renderer::fill_rects(const std::vector<Rect> &rects)
{
    for(const auto &rect: rects)
        fill_rect(rect);
}
void Renderer::draw_line([[maybe_unused]] const Line &l)
{
    k_assert(false);
}
void Renderer::draw_line([[maybe_unused]] Point p1, [[maybe_unused]] Point p2)
{
    k_assert(false);
}
void Renderer::draw_polyline([[maybe_unused]] const std::vector<Point> &points)
{
    k_assert(false);
}
void Renderer::draw_circle([[maybe_unused]] Circle c)
{
    k_assert(false);
}
void Renderer::fill_circle([[maybe_unused]] Circle c)
{
    k_assert(false);
}
void Renderer::fill_circles(const std::vector<Point> &centers, float r)
{
    for(const auto &c: centers)
        fill_circle({c.x, c.y, r});
}
void Renderer::fill_decaying_circle([[maybe_unused]] Circle c)
{
    k_assert(false);
}
void Renderer::fill_decaying_circles(const std::vector<Point> &centers, float r)
{
    for(const auto &c: centers)
        fill_decaying_circle({c.x, c.y, r});
}

void Renderer::invert_rect_colors([[maybe_unused]] const gfx::Rect &rect)
{
    k_assert(false);
}

void Renderer::set_font(std::string font_name)
{
    auto font = Font::get_font(font_name);
    if(font == nullptr) {
        log_error("Failed to find font with name \"" + font_name + "\", as no fonts with that name exist");
        return;
    }
    current_font = font;
}
void Renderer::set_font(std::shared_ptr<const Font> font)
{
    k_expects(font != nullptr);
    current_font = std::move(font);
}
const std::shared_ptr<const Font> &Renderer::get_font() const
{
    return current_font;
}
std::shared_ptr<Texture> Renderer::get_text_texture(const std::string &text, int sz, SRGB_Color c)
{
    k_expects(current_font != nullptr);

    //unwrapped text effectively has a wrap length of numeric_limits...max()
    TextTextureCacheInfo cache_info(text, current_font->id, sz,
                                    std::numeric_limits<decltype(TextTextureCacheInfo::wrap_length)>::max(),
                                    c.r*255.999, c.g*255.999, c.b*255.999, c.a*255.999);
    auto current_time = Time::now();
    auto cached = text_cache.find(cache_info);

    std::shared_ptr<Texture> text_texture;

    if(cached == text_cache.end()) { //the text texture doesn't exist in the cache, so create it
        if(text.size() > 0) { //passing in a string of length 0 returns an error
            auto desired_font = current_font->font_size[std::clamp(sz, 0, Font::NUM_FONT_SIZES-1)].get();
            unique_ptr_sdl<SDL_Surface> temp_surface = TTF_RenderText_Blended(desired_font, text.c_str(), c);
            if(temp_surface == nullptr)
                log_error("TTF_RenderText_Blended returned nullptr: " + (std::string)SDL_GetError());
            text_texture = std::shared_ptr<Texture>(new Texture(temp_surface.get(), true));
        }
        else //for empty strings, create a filler texture that's completely transparent
            text_texture = make_texture_target(1, 1); //we have to make it a texture of nonzero size

        text_cache.insert(std::make_pair(cache_info, TextTexture(text_texture, current_time)));
        text_time_last_used[current_time].insert(cache_info);
    } else {
        k_assert(cached->second.text_texture != nullptr);
        //the text is being used right now, so remove it from the time set where it used to be and
        //put it in the set corresponding to the current time
        auto prev_last_used_set = text_time_last_used.find(cached->second.time_last_used);
        if(prev_last_used_set != text_time_last_used.end()) {
            prev_last_used_set->second.erase(cache_info);
        } else
            log_warning("prev_last_used_set is nullptr (the cache isn't supposed to have cleaned it yet)");

        cached->second.time_last_used = current_time;
        text_time_last_used[current_time].insert(cache_info);
        text_texture = cached->second.text_texture;
    }
    return text_texture;
}
std::shared_ptr<Texture> Renderer::get_text_texture_wrapped(std::string text, int sz, int wrap_length, SRGB_Color c)
{
    k_expects(current_font != nullptr);

    TextTextureCacheInfo cache_info(text, current_font->id, sz, wrap_length,
                                    c.r*255.999, c.g*255.999, c.b*255.999, c.a*255.999);
    auto current_time = Time::now();
    auto cached = text_cache.find(cache_info);

    std::shared_ptr<Texture> text_texture;

    if(cached == text_cache.end()) { //the text texture doesn't exist in the cache, so create it
        if(text.size() > 0) { //passing in a string of length 0 returns an error
            std::vector<std::unique_ptr<Texture>> text_lines;
            std::reverse(text.begin(), text.end());
            while(text.size() > 0) {
                std::string this_line;
                while(text.size() > 0) {
                    if(text.back() == '\n') {
                        text.pop_back();
                        break;
                    }
                    int w;
                    //TODO: how fast is TTF_SizeText? Could this be a bottleneck?
                    auto desired_font = current_font->font_size[std::clamp(sz, 0, Font::NUM_FONT_SIZES-1)].get();
                    TTF_SizeText(desired_font, (this_line + text.back()).c_str(), &w, nullptr);
                    if(w > wrap_length) {
                        break;
                    }
                    this_line += text.back();
                    text.pop_back();
                }
                if(this_line.size() == 0) {
                    log_warning("text wrap length too small (can't find 1 character in a line)");
                    //putting 1 character on this line is probably the easiest way to solve this problem
                    this_line += text.back();
                    text.pop_back();
                }
                auto desired_font = current_font->font_size[std::clamp(sz, 0, Font::NUM_FONT_SIZES-1)].get();
                unique_ptr_sdl<SDL_Surface> temp_surface = TTF_RenderText_Blended(desired_font, this_line.c_str(), c);
                if(temp_surface == nullptr)
                    log_error("TTF_RenderText_Blended_Wrapped returned nullptr: " + (std::string)SDL_GetError());
                text_lines.emplace_back(new Texture(temp_surface.get(), true));
            }
            k_ensures(text_lines.size() > 0);

            int spacing = 1;
            int texture_h = text_lines[0]->get_h();

            text_texture = make_texture_target(wrap_length, spacing * texture_h * text_lines.size());
            auto prev_target = get_target();
            set_target(text_texture.get());
            int cur_y = 0;
            int first_h = -1;
            for(const auto &t: text_lines) {
                int w = t->get_w();
                int h = t->get_h();
                if(first_h == -1) {
                    first_h = h;
                } else
                    k_assert(first_h == h); //all text of the same font and size should have the same height
                Rect dst{0, (float)cur_y, (float)w, (float)h};
                draw_texture(*t, dst);
                cur_y += h * spacing;
            }
            set_target(prev_target);
        }
        else { //for empty strings, create a filler texture that's completely transparent
            text_texture = make_texture_target(1, 1); //we have to make it a texture of nonzero size
        }
        text_cache.insert(std::make_pair(cache_info, TextTexture(text_texture, current_time)));
        text_time_last_used[current_time].insert(cache_info);

    } else {
        k_assert(cached->second.text_texture != nullptr);
        //the text is being used right now, so remove it from the set where it used to be and
        //put it in the set corresponding to the current time
        auto prev_last_used_set = text_time_last_used.find(cached->second.time_last_used);
        if(prev_last_used_set != text_time_last_used.end()) {
            prev_last_used_set->second.erase(cache_info);
        } else
            log_warning("prev_last_used_set is nullptr (the cache isn't supposed to have cleaned it yet)");

        cached->second.time_last_used = current_time;
        text_time_last_used[current_time].insert(cache_info);
        text_texture = cached->second.text_texture;
    }
    return text_texture;
}
void Renderer::draw_text(std::string text, float x, float y, int sz, SRGB_Color c)
{
    auto text_texture = get_text_texture(std::move(text), sz, c);
    int w = text_texture->get_w();
    int h = text_texture->get_h();
    Rect dst{x, y, (float)w, (float)h};
    draw_texture(*text_texture, dst);
}
void Renderer::draw_text_wrapped(std::string text, float x, float y, int sz, int wrap_length, SRGB_Color c)
{
    auto text_texture = get_text_texture_wrapped(std::move(text), sz, wrap_length, c);
    int w = text_texture->get_w();
    int h = text_texture->get_h();
    Rect dst{x, y, (float)w, (float)h};
    draw_texture(*text_texture, dst);
}
std::unique_ptr<MonofontAtlas> Renderer::create_monofont_atlas([[maybe_unused]] const Font &font,
                                                               [[maybe_unused]] int font_size,
                                                               [[maybe_unused]] SRGB_Color color)
{
    k_assert(false);
    return nullptr;
}
void Renderer::set_target(Texture *target)
{
    if(Shaders.tri_buffer_ptr != Shaders.tri_buffer.begin())
        flush_tri_buffer();

    if(target == nullptr) {
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        int w;
        int h;
        SDL_GetWindowSize(window, &w, &h);
        glViewport(0, 0, w, h);
        renderer_w_div2 = w / 2.0;
        renderer_h_div2 = h / 2.0;
    } else {
        k_expects(target->framebuffer.id != 0);
        glBindFramebuffer(GL_FRAMEBUFFER, target->framebuffer.id);
        glViewport(0, 0, target->get_w(), target->get_h());
        renderer_w_div2 = target->get_w() / 2.0;
        renderer_h_div2 = target->get_h() / 2.0;
    }
    render_target = target;
}
Texture *Renderer::get_target() const
{
    return render_target;
}
bool Renderer::is_target_srgb() const
{
    auto target = get_target();
    if(target == nullptr)
        return true;
    return target->is_srgb();
}
void Renderer::set_viewport(const std::optional<IRect> &r)
{
    if(r.has_value()) {
        glViewport(r->x, r->y, r->w, r->h);
        renderer_w_div2 = r->w / 2.0;
        renderer_h_div2 = r->h / 2.0;
    } else {
        int w;
        int h;
        SDL_GetWindowSize(window, &w, &h);
        glViewport(0, 0, w, h);
        renderer_w_div2 = w / 2.0;
        renderer_h_div2 = h / 2.0;
    }
}
std::unique_ptr<Texture> Renderer::make_texture_target(int w,
                                                       int h,
                                                       Texture::Format format,
                                                       bool is_srgb,
                                                       int samples) const
{
    auto tex = std::unique_ptr<Texture>(new Texture(w, h, format, is_srgb, samples));
    tex->make_targetable();
    return tex;
}
std::unique_ptr<Texture> Renderer::create_texture_target(int w,
                                                         int h,
                                                         Texture::Format format,
                                                         bool is_srgb,
                                                         int samples) const
{
    return make_texture_target(w, h, format, is_srgb, samples);
}
void Renderer::draw_texture(const Texture &texture, const Rect &dst, const std::optional<Rect> &src_)
{
    _draw_texture<CoordSys::Absolute>(texture, dst, src_);
}
void Renderer::draw_texture_nc(const Texture &texture, const Rect &dst, const std::optional<Rect> &src_)
{
    _draw_texture<CoordSys::NC>(texture, dst, src_);
}
void Renderer::draw_texture_ex([[maybe_unused]] const Texture &texture,
                               [[maybe_unused]] const Rect &dst,
                               [[maybe_unused]] const std::optional<Rect> &src_,
                               [[maybe_unused]] const std::optional<gfx::Point> &center,
                               [[maybe_unused]] double angle,
                               [[maybe_unused]] Flip flip)
{
    k_assert(false);
}
void Renderer::draw_texture_ms(const Texture &texture, const Rect &dst, const std::optional<Rect> &src_)
{
    k_expects(texture.binding_point == GL_TEXTURE_2D_MULTISAMPLE);

    Rect src;
    if(src_.has_value()) {
        src = *src_;
    } else
        src = Rect(0.0, 0.0, texture.get_w(), texture.get_h());

    //vertex 1
    Shaders.texture_buffer[0] = x_to_gl_coord<CoordSys::Absolute>(dst.x);
    Shaders.texture_buffer[1] = y_to_gl_coord<CoordSys::Absolute>(dst.y);
    Shaders.texture_buffer[2] = src.x;
    Shaders.texture_buffer[3] = src.y + src.h;
    Shaders.texture_buffer[4] = texture.color_mod.r;
    Shaders.texture_buffer[5] = texture.color_mod.g;
    Shaders.texture_buffer[6] = texture.color_mod.b;
    Shaders.texture_buffer[7] = texture.alpha_mod;

    //vertex 2
    Shaders.texture_buffer[8] = x_to_gl_coord<CoordSys::Absolute>(dst.x + dst.w);
    Shaders.texture_buffer[9] = y_to_gl_coord<CoordSys::Absolute>(dst.y);
    Shaders.texture_buffer[10] = src.x + src.w;
    Shaders.texture_buffer[11] = src.y + src.h;
    for(int i=0; i<4; i++)
        Shaders.texture_buffer[12 + i] = Shaders.texture_buffer[4 + i];

    //vertex 3
    Shaders.texture_buffer[16] = x_to_gl_coord<CoordSys::Absolute>(dst.x);
    Shaders.texture_buffer[17] = y_to_gl_coord<CoordSys::Absolute>(dst.y + dst.h);
    Shaders.texture_buffer[18] = src.x;
    Shaders.texture_buffer[19] = src.y;
    for(int i=0; i<4; i++)
        Shaders.texture_buffer[20 + i] = Shaders.texture_buffer[4 + i];

    //vertex 4
    Shaders.texture_buffer[24] = x_to_gl_coord<CoordSys::Absolute>(dst.x + dst.w);
    Shaders.texture_buffer[25] = y_to_gl_coord<CoordSys::Absolute>(dst.y + dst.h);
    Shaders.texture_buffer[26] = src.x + src.w;
    Shaders.texture_buffer[27] = src.y;
    for(int i=0; i<4; i++)
        Shaders.texture_buffer[28 + i] = Shaders.texture_buffer[4 + i];

    if(Shaders.tri_buffer_ptr != Shaders.tri_buffer.begin())
        flush_tri_buffer();

    use_shader_program(*Shaders.texture_ms);
    bind_VAO(*Shaders.texture_vao);
    bind_VBO(*Shaders.texture_vbo);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(texture.binding_point, texture.texture.id);
    auto pid = Shaders.texture_ms->program.id;
    glUniform1i(glGetUniformLocation(pid, "texture_in"), 0);
    int conversion_algo = is_target_srgb() - (int)texture.is_srgb();
    glUniform1i(glGetUniformLocation(pid, "conversion_algo"), conversion_algo);
    glUniform1f(glGetUniformLocation(pid, "samples"), (float)texture.samples);
    Shaders.texture_vbo->buffer_sub_data(0, Shaders.texture_buffer.data(), 32*sizeof(float));
    draw_arrays(DrawMode::TriangleStrip, 0, 4);
    Shaders.texture_vbo->invalidate();
}
void Renderer::set_blend_factors(BlendFactor src, BlendFactor dst)
{
    glBlendFunc((GLenum)src, (GLenum)dst);
}
void Renderer::prepare_for_custom_shader()
{
    if(Shaders.tri_buffer_ptr != Shaders.tri_buffer.begin())
        flush_tri_buffer();
}
GLuint Renderer::get_cur_program(Passkey<ShaderProgram>)
{
    return Shaders.cur_program;
}
GLuint Renderer::get_active_VBO_id(Passkey<VBO, VAO>)
{
    return Shaders.active_VBO_id;
}
GLuint Renderer::get_active_EBO_id(Passkey<EBO>)
{
    return Shaders.active_EBO_id;
}
GLuint Renderer::get_active_UBO_id(Passkey<UBO>)
{
    return Shaders.active_UBO_id;
}

void Renderer::bind_VAO(const VAO &vao)
{
    glBindVertexArray(vao.vao.id);
}
void Renderer::bind_VBO(const VBO &vbo)
{
    glBindBuffer(GL_ARRAY_BUFFER, vbo.buffer.id);
    Shaders.active_VBO_id = vbo.buffer.id;
}
void Renderer::bind_EBO(const EBO &ebo)
{
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo.buffer.id);
    Shaders.active_EBO_id = ebo.buffer.id;
}
void Renderer::bind_UBO(const UBO &ubo)
{
    glBindBuffer(GL_UNIFORM_BUFFER, ubo.buffer.id);
    Shaders.active_UBO_id = ubo.buffer.id;
}
void Renderer::bind_UB_base(GLuint index, const UBO &ubo)
{
    glBindBufferBase(GL_UNIFORM_BUFFER, index, ubo.buffer.id);
}
void Renderer::use_shader_program(const ShaderProgram &program)
{
    if(program.program.id != Shaders.cur_program) {
        Shaders.cur_program = program.program.id;
        glUseProgram(program.program.id);
    }
}
void Renderer::draw_arrays(DrawMode mode, GLint first, GLsizei count)
{
    glDrawArrays((GLenum)mode, first, count);
}
void Renderer::draw_arrays_instanced(DrawMode mode, GLint first, GLsizei count, GLsizei instance_cnt)
{
    glDrawArraysInstanced((GLenum)mode, first, count, instance_cnt);
}
void Renderer::draw_elements(DrawMode mode, GLint first, GLsizei count)
{
    glDrawElements((GLenum)mode, count, GL_UNSIGNED_INT, (void*)(first*sizeof(GLuint)));
}
void Renderer::set_active_texture(int tex_num)
{
    //most implementations have more than 16 available, but 16 is the minimum guaranteed
    k_expects(tex_num>=0 && tex_num<16);
    glActiveTexture(GL_TEXTURE0 + tex_num);
}
void Renderer::bind_texture(const Texture &texture)
{
    glBindTexture(texture.binding_point, texture.texture.id);
}
void Renderer::show_fps(bool toggle)
{
    show_fps_toggle = toggle;
}
int Renderer::set_fps_font(std::shared_ptr<const Font> font)
{
    k_expects(font != nullptr);
    fps_font = std::move(font);
    return 0;
}
void Renderer::set_fps_color(SRGB_Color color)
{
    fps_color = color;
}
void Renderer::refresh()
{
    if(Shaders.tri_buffer_ptr != Shaders.tri_buffer.begin())
        flush_tri_buffer();

    auto cur_time = Time::now();
    auto cutoff = cur_time - Time::Delta((int64_t)1, Time::Length::second);
    while(!frame_timestamps.empty() && frame_timestamps.front() < cutoff)
        frame_timestamps.pop();
    frame_timestamps.push(cur_time);

    if(show_fps_toggle && fps_font!=nullptr) {
        auto cur_font = get_font();
        set_font(fps_font);
        draw_text(to_str(frame_timestamps.size()) + " fps", 0, 0, 10, fps_color);
        set_font(cur_font);
    }
    SDL_GL_SwapWindow(window);
}
int Renderer::get_fps() const
{
    return frame_timestamps.size();
}
int Renderer::get_num_samples() const
{
    return NUM_SAMPLES_DEFAULT;
}

}}

#undef SDL_L0_FUNC
