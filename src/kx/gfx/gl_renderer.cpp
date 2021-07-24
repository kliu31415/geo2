#include "kx/gfx/renderer.h"
#include "kx/io.h"

#include "glad/glad.h"

#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL_image.h>

#include <algorithm>
#include <cmath>
#include <climits>

#include "ft2build.h"
#include FT_FREETYPE_H

#include "kx/debug.h"

#ifdef KX_DEBUG
#define SDL_L0_FUNC(func, ...) {if(func(__VA_ARGS__) < 0) \
                               {log_error(#func + (std::string)" failed: " + SDL_GetError());}}
#else
#define SDL_L0_FUNC(func, ...) {func(__VA_ARGS__);}
#endif

namespace kx { namespace gfx {

static_assert(std::is_same_v<GLenum, uint32_t>);

static_assert(sizeof(Color4f) == 16);

static_assert(sizeof(SRGB_Color) == 16);
static_assert(offsetof(SRGB_Color, r) == 0);
static_assert(offsetof(SRGB_Color, g) == 4);
static_assert(offsetof(SRGB_Color, b) == 8);
static_assert(offsetof(SRGB_Color, a) == 12);

static_assert(sizeof(LinearColor) == 16);
static_assert(offsetof(LinearColor, r) == 0);
static_assert(offsetof(LinearColor, g) == 4);
static_assert(offsetof(LinearColor, b) == 8);
static_assert(offsetof(LinearColor, a) == 12);

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

void _GLDeleteShader::operator()(GLuint id) const
{
    glDeleteShader(id);
}
void _GLDeleteShaderProgram::operator()(GLuint id) const
{
    glDeleteProgram(id);
}
void _GLDeleteBuffer::operator()(GLuint id) const
{
    glDeleteBuffers(1, &id);
}
void _GLDeleteVAO::operator()(GLuint id) const
{
    glDeleteVertexArrays(1, &id);
}
void _GLDeleteTexture::operator()(GLuint id) const
{
    glDeleteTextures(1, &id);
}
void _GLDeleteFramebuffers::operator()(GLuint id) const
{
    glDeleteFramebuffers(1, &id);
}

LinearColor::LinearColor(float r_, float g_, float b_, float a_):
    Color4f(r_, g_, b_, a_)
{}

static float srgb_to_linear(float in)
{
    if(in <= 0.04045)
        return in / 12.92;
    else
        return std::pow((in+0.055)/1.055, 2.4);
}
Color4f::Color4f(float r_, float g_, float b_, float a_):
    r(r_),
    g(g_),
    b(b_),
    a(a_)
{}
LinearColor::LinearColor(const SRGB_Color &color):
    Color4f(srgb_to_linear(color.r), srgb_to_linear(color.g), srgb_to_linear(color.b), color.a)
{}

SRGB_Color::SRGB_Color(float r_, float g_, float b_, float a_):
    Color4f(r_, g_, b_, a_)
{}
static float linear_to_srgb(float in)
{
    if(in <= 0.0031308)
        return in * 12.92;
    else
        return 1.055 * std::pow(in, 1.0 / 2.4) - 0.055;
}
SRGB_Color::SRGB_Color(const LinearColor &color):
    Color4f(linear_to_srgb(color.r), linear_to_srgb(color.g), linear_to_srgb(color.b), color.a)
{}

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

Texture::Texture(int dim,
                 int slices,
                 int w_,
                 int h_,
                 Texture::Format format_,
                 bool is_srgb__,
                 int samples_,
                 void *pixels):
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
    k_expects(dim == 2); //only 2d textures are support right now

    if(slices == 1) { //1 slice = standard non-array texture
        if(samples == 1)
            binding_point = GL_TEXTURE_2D;
        else
            binding_point = GL_TEXTURE_2D_MULTISAMPLE;
    } else {
        if(samples == 1)
            binding_point = GL_TEXTURE_2D_ARRAY;
        else {
            binding_point = GL_TEXTURE_2D_MULTISAMPLE_ARRAY;
        }
    }

    glGenTextures(1, &texture.id);
    glBindTexture(binding_point, texture.id);

    //this doesn't apply to multisample textures
    if(samples == 1) {
        glTextureParameteri(texture.id, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTextureParameteri(texture.id, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTextureParameteri(texture.id, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
        glTextureParameteri(texture.id, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    }

    GLenum arg1;
    GLenum arg2;
    GLenum type;

    switch(format) {
    case Texture::Format::R8:
        arg1 = GL_R8;
        arg2 = GL_RED;
        type = GL_UNSIGNED_BYTE;
        break;
    case Texture::Format::RGBA8888:
        arg1 = GL_RGBA8;
        arg2 = GL_RGBA;
        type = GL_UNSIGNED_BYTE;
        break;
    case Texture::Format::RGB888:
        arg1 = GL_RGB8;
        arg2 = GL_RGB;
        type = GL_UNSIGNED_BYTE;
        break;
    case Texture::Format::RGBA16F:
        arg1 = GL_RGBA16F;
        arg2 = GL_RGBA;
        type = GL_FLOAT;
        break;
    case Texture::Format::RGB16F:
        arg1 = GL_RGB16F;
        arg2 = GL_RGB;
        type = GL_FLOAT;
        break;
    default:
        arg1 = GL_RGBA8;
        arg2 = GL_RGBA;
        type = GL_UNSIGNED_BYTE;
        log_error("bad texture format given to ctor: defaulting to RGBA8");
        break;
    }
    if(binding_point == GL_TEXTURE_2D) {
        glTexImage2D(GL_TEXTURE_2D, 0, arg1, w, h, 0, arg2, type, pixels);
    } else if(binding_point == GL_TEXTURE_2D_MULTISAMPLE) {
        k_expects(pixels == nullptr);
        glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, samples, arg1, w, h, false);
    } else if(binding_point == GL_TEXTURE_2D_ARRAY) {
        glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, arg1, w, h, slices, 0, arg2, type, pixels);
    } else if(binding_point == GL_TEXTURE_2D_MULTISAMPLE_ARRAY) {
        k_expects(pixels == nullptr);
        glTexImage3DMultisample(GL_TEXTURE_2D_MULTISAMPLE_ARRAY, samples, arg1, w, h, slices, false);
    } else
        k_assert(false);
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

    glTextureParameteri(texture.id, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTextureParameteri(texture.id, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTextureParameteri(texture.id, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTextureParameteri(texture.id, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);

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
        glTexImage2D(binding_point, 0, GL_RGBA8, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());
        format = Texture::Format::RGBA8888;
    } else {
        glTexImage2D(binding_point, 0, GL_RGB8, w, h, 0, GL_RGB, GL_UNSIGNED_BYTE, pixels.data());
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
void Texture::make_mipmaps()
{
    glGenerateTextureMipmap(texture.id);
}

constexpr std::array<GLenum, 6> texture_filter_enum_convert {
    GL_NEAREST,
    GL_LINEAR,
    GL_NEAREST_MIPMAP_NEAREST,
    GL_NEAREST_MIPMAP_LINEAR,
    GL_LINEAR_MIPMAP_NEAREST,
    GL_LINEAR_MIPMAP_LINEAR
};
using TxFAlgo = Texture::FilterAlgo;
static_assert(texture_filter_enum_convert[(int)TxFAlgo::Nearest] == GL_NEAREST);
static_assert(texture_filter_enum_convert[(int)TxFAlgo::Linear] == GL_LINEAR);
static_assert(texture_filter_enum_convert[(int)TxFAlgo::NearestMipmapNearest] == GL_NEAREST_MIPMAP_NEAREST);
static_assert(texture_filter_enum_convert[(int)TxFAlgo::NearestMipmapLinear] == GL_NEAREST_MIPMAP_LINEAR);
static_assert(texture_filter_enum_convert[(int)TxFAlgo::LinearMipmapNearest] == GL_LINEAR_MIPMAP_NEAREST);
static_assert(texture_filter_enum_convert[(int)TxFAlgo::LinearMipmapLinear] == GL_LINEAR_MIPMAP_LINEAR);

void Texture::set_min_filter(Texture::FilterAlgo algo)
{
    k_expects(!is_multisample());
    glTextureParameteri(texture.id, GL_TEXTURE_MIN_FILTER, texture_filter_enum_convert[(int)algo]);
}
void Texture::set_mag_filter(Texture::FilterAlgo algo)
{
    k_expects(!is_multisample());
    glTextureParameteri(texture.id, GL_TEXTURE_MAG_FILTER, texture_filter_enum_convert[(int)algo]);
}

Shader::Shader():
    shader(0)
{}

static constexpr size_t INFO_LOG_SIZE = 1024;
static thread_local std::string info_log(INFO_LOG_SIZE, '\0');

Shader::Shader(std::string_view file_name, GLenum shader_type)
{
    shader.id = glCreateShader(shader_type);
    auto shader_src_opt = io::read_binary_file(file_name.data());
    if(!shader_src_opt) {
        log_error("failed to open shader file \"" + (std::string)file_name + "\"");
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
        log_error("Shader \"" + (std::string)file_name + "\" compilation failed: " + info_log.substr(0, len));
    }
}

VertShader::VertShader(std::string_view file_name):
    Shader(file_name, GL_VERTEX_SHADER)
{}

FragShader::FragShader(std::string_view file_name):
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

void ShaderProgram::set_uniform1fv(GLint loc, kx::kx_span<GLfloat> vals)
{
    k_expects(owner->get_cur_program({}) == program.id);
    glUniform1fv(loc, vals.size(), vals.begin());
}
void ShaderProgram::set_uniform2fv(GLint loc, kx::kx_span<GLfloat> vals)
{
    k_expects(owner->get_cur_program({}) == program.id);
    glUniform2fv(loc, vals.size(), vals.begin());
}
void ShaderProgram::set_uniform3fv(GLint loc, kx::kx_span<GLfloat> vals)
{
    k_expects(owner->get_cur_program({}) == program.id);
    glUniform3fv(loc, vals.size(), vals.begin());
}
void ShaderProgram::set_uniform4fv(GLint loc, kx::kx_span<GLfloat> vals)
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
template<_BufferType T> void Buffer<T>::buffer_data(const void *data, GLuint n)
{
    glNamedBufferData(buffer.id, n, data, GL_DYNAMIC_DRAW);
}
template<_BufferType T> void Buffer<T>::buffer_sub_data(GLintptr offset, const void *data, GLuint n)
{
    glNamedBufferSubData(buffer.id, offset, n, data);
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

void Renderer::GLDeleteContext::operator()(void *context) const
{
    SDL_GL_DeleteContext(context);
}

void Renderer::init_shaders()
{
    Shaders.cur_program = 0;
    Shaders.active_VBO_id = 0;
    Shaders.active_EBO_id = 0;
    Shaders.active_UBO_id = 0;

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

std::map<GLenum, std::string> debug_callback_map_source;
std::map<GLenum, std::string> debug_callback_map_type;
std::map<GLenum, std::string> debug_callback_map_severity;
std::once_flag gl_debug_once_flag;
void init_debug_callback_maps()
{
    debug_callback_map_source[GL_DEBUG_SOURCE_API] = "GL_DEBUG_SOURCE_API";
    debug_callback_map_source[GL_DEBUG_SOURCE_WINDOW_SYSTEM] = "GL_DEBUG_SOURCE_WINDOW_SYSTEM";
    debug_callback_map_source[GL_DEBUG_SOURCE_SHADER_COMPILER] = "GL_DEBUG_SOURCE_SHADER_COMPILER";
    debug_callback_map_source[GL_DEBUG_SOURCE_THIRD_PARTY] = "GL_DEBUG_SOURCE_THIRD_PARTY";
    debug_callback_map_source[GL_DEBUG_SOURCE_APPLICATION] = "GL_DEBUG_SOURCE_APPLICATION";
    debug_callback_map_source[GL_DEBUG_SOURCE_OTHER] = "GL_DEBUG_SOURCE_OTHER";

    debug_callback_map_type[GL_DEBUG_TYPE_ERROR] = "GL_DEBUG_TYPE_ERROR";
    debug_callback_map_type[GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR] = "GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR";
    debug_callback_map_type[GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR] = "GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR";
    debug_callback_map_type[GL_DEBUG_TYPE_PORTABILITY] = "GL_DEBUG_TYPE_PORTABILITY";
    debug_callback_map_type[GL_DEBUG_TYPE_PERFORMANCE] = "GL_DEBUG_TYPE_PERFORMANCE";
    debug_callback_map_type[GL_DEBUG_TYPE_MARKER] = "GL_DEBUG_TYPE_MARKER";
    debug_callback_map_type[GL_DEBUG_TYPE_PUSH_GROUP] = "GL_DEBUG_TYPE_PUSH_GROUP";
    debug_callback_map_type[GL_DEBUG_TYPE_POP_GROUP] = "GL_DEBUG_TYPE_POP_GROUP";
    debug_callback_map_type[GL_DEBUG_TYPE_OTHER] = "GL_DEBUG_TYPE_OTHER";

    debug_callback_map_severity[GL_DEBUG_SEVERITY_HIGH] = "GL_DEBUG_SEVERITY_HIGH";
    debug_callback_map_severity[GL_DEBUG_SEVERITY_MEDIUM] = "GL_DEBUG_SEVERITY_MEDIUM";
    debug_callback_map_severity[GL_DEBUG_SEVERITY_LOW] = "GL_DEBUG_SEVERITY_LOW";
    debug_callback_map_severity[GL_DEBUG_SEVERITY_NOTIFICATION] = "GL_DEBUG_SEVERITY_NOTIFICATION";
}
void GLAPIENTRY debug_callback(GLenum source,
                               GLenum type,
                               [[maybe_unused]] GLuint id,
                               GLenum severity,
                               [[maybe_unused]] GLsizei length,
                               const GLchar* message,
                               [[maybe_unused]] const void* user_param)
{
    if(severity == GL_DEBUG_SEVERITY_NOTIFICATION)
        return;
    if(strstr(message, "Fragment shader")!=nullptr && strstr(message, "recompiled")!=nullptr)
        return;

    std::call_once(gl_debug_once_flag, init_debug_callback_maps);

    auto source_find = debug_callback_map_source.find(source);
    auto type_find = debug_callback_map_type.find(type);
    auto severity_find = debug_callback_map_severity.find(severity);

    auto source_str = (source_find != debug_callback_map_source.end() ?
                       source_find->second :
                       to_str("int: ") + to_str((int)source));

    auto type_str = (type_find != debug_callback_map_type.end() ?
                     type_find->second :
                     to_str("int: ") + to_str((int)type));

    auto severity_str = (type_find != debug_callback_map_severity.end() ?
                         severity_find->second :
                         to_str("int: ") + to_str((int)severity));

    log_error("OpenGL debug callback: \n"
              "source = " + source_str + "\n"
              "type = " + type_str + "\n"
              "severity = " + severity_str + "\n"
              "message = " + to_str(message));
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
    render_target(nullptr),
    Shaders()
{
    glEnable(GL_DEBUG_OUTPUT);
    glDebugMessageCallback(debug_callback, nullptr);

    glEnable(GL_BLEND);

    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);


    set_blend_factors(BlendFactor::SrcAlpha, BlendFactor::OneMinusSrcAlpha);
    SDL_GL_SetSwapInterval(1);

    set_viewport({});

    init_shaders();
}
void Renderer::clean_memory()
{

}
void Renderer::make_context_current()
{
    SDL_GL_MakeCurrent(window, gl_context.get());
}
void Renderer::clear(const Color4f &color)
{
    glClearColor(color.r, color.g, color.b, color.a);
    glClear(GL_COLOR_BUFFER_BIT);
}
std::unique_ptr<ASCII_Atlas> Renderer::make_ascii_atlas(Font *font, int font_size)
{
    k_expects(font_size>=1 && font_size<=Font::MAX_FONT_SIZE);

    auto atlas = std::make_unique<ASCII_Atlas>();
    atlas->font = font;
    atlas->font_size = font_size;

    atlas->min_x1 = std::numeric_limits<float>::max();
    atlas->max_x2 = -std::numeric_limits<float>::max();
    atlas->min_y1 = std::numeric_limits<float>::max();
    atlas->max_y2 = -std::numeric_limits<float>::max();

    std::array<unique_ptr_sdl<SDL_Surface>, ASCII_Atlas::MAX_ASCII_CHAR+1> char_surfaces;

    int max_w = 0;
    int max_h = 0;

    std::array<std::vector<uint8_t>, ASCII_Atlas::MAX_ASCII_CHAR+1> bitmap_buffer;
    std::array<size_t, ASCII_Atlas::MAX_ASCII_CHAR+1> bitmap_pitch;

    if(FT_Set_Pixel_Sizes(font->ft_face, 0, font_size))
        log_error("FT_Set_Pixel_Sizes failed");
    for(int c=0; c<=ASCII_Atlas::MAX_ASCII_CHAR; c++) {
        auto glyph_idx = FT_Get_Char_Index(font->ft_face, c);
        if(glyph_idx == 0) {
            atlas->char_supported[c] = false;
            continue;
        } else
            atlas->char_supported[c] = true;

        if(FT_Load_Glyph(font->ft_face, glyph_idx, FT_LOAD_NO_BITMAP))
            log_error("FT_Load_Glyph failed for \'" + to_str(c) + "\'");
        if(FT_Render_Glyph(font->ft_face->glyph, FT_RENDER_MODE_NORMAL))
            log_error("FT_Render_Glyph failed for \'" + to_str(c) + "\'");

        auto &metrics = atlas->glyph_metrics[c];

        metrics.w = font->ft_face->glyph->bitmap.width;
        metrics.h = font->ft_face->glyph->bitmap.rows;
        metrics.left_offset =  font->ft_face->glyph->bitmap_left;
        metrics.top_offset  = -font->ft_face->glyph->bitmap_top;
        metrics.advance = font->ft_face->glyph->advance.x / 64.0;

        atlas->min_x1 = std::min(atlas->min_x1, metrics.left_offset);
        atlas->max_x2 = std::max(atlas->max_x2, metrics.left_offset + metrics.w);
        atlas->min_y1 = std::min(atlas->min_y1, metrics.top_offset);
        atlas->max_y2 = std::max(atlas->max_y2, metrics.top_offset + metrics.h);

        max_w = std::max(max_w, (int)metrics.w);
        max_h = std::max(max_h, (int)metrics.h);

        bitmap_pitch[c] = font->ft_face->glyph->bitmap.pitch;
        int buffer_size = bitmap_pitch[c] * metrics.h;
        bitmap_buffer[c] = std::vector<uint8_t>(buffer_size);
        auto buffer = font->ft_face->glyph->bitmap.buffer;
        std::copy_n((uint8_t*)buffer, buffer_size, bitmap_buffer[c].data());

    }

    atlas->max_w = max_w;
    atlas->max_h = max_h;

    auto bytes_per_slice = max_w * max_h;
    std::vector<uint8_t> pixels((ASCII_Atlas::MAX_ASCII_CHAR+1) * bytes_per_slice, 0);

    //The x range is normalized to [0, max_x - min_x] and y is normalized to [0, max_y - min_y]
    //GlyphMetrics has inaccurate values so I calculate them by scanning through the pixels myself.
    //e.g. if the global min_x=-10 and global max_x = 20, then x=7 is normalized to 17/30.
    //advance is normalized to max_w
    for(int c=1; c<=ASCII_Atlas::MAX_ASCII_CHAR; c++) {
        for(size_t row=0; row < atlas->glyph_metrics[c].h; row++) {
            for(size_t col=0; col < atlas->glyph_metrics[c].w; col++) {
                auto alpha = bitmap_buffer[c][row * bitmap_pitch[c] + col];
                pixels[c*bytes_per_slice + (max_h-row-1)*max_w + col] = alpha;
            }
        }
    }

    for(auto &k1: atlas->kerning) {
        static_assert(std::numeric_limits<std::remove_reference_t<decltype(*k1.begin())>>::has_signaling_NaN);
        std::fill(k1.begin(), k1.end(), std::numeric_limits<float>::signaling_NaN());
    }

    for(int a=1; a<=ASCII_Atlas::MAX_ASCII_CHAR; a++) {
        if(!atlas->char_supported[a])
            continue;
        for(int b=1; b<=ASCII_Atlas::MAX_ASCII_CHAR; b++) {
            if(!atlas->char_supported[b])
                continue;
            auto idx_a = FT_Get_Char_Index(font->ft_face, a);
            auto idx_b = FT_Get_Char_Index(font->ft_face, b);
            FT_Vector kerning;
            auto error = FT_Get_Kerning(font->ft_face, idx_a, idx_b, FT_KERNING_UNFITTED, &kerning);
            k_assert(error == 0);
            atlas->kerning[a][b] = kerning.x / 64.0;
        }
    }

    atlas->texture = make_texture_2d_array(max_w,
                                           max_h,
                                           ASCII_Atlas::MAX_ASCII_CHAR+1,
                                           Texture::Format::R8,
                                           false,
                                           1,
                                           pixels.data());
    atlas->texture->make_mipmaps();
    atlas->texture->set_min_filter(Texture::FilterAlgo::LinearMipmapLinear);
    std::array<GLint, 4> swizzle_mask{GL_ONE, GL_ONE, GL_ONE, GL_RED};
    glTextureParameteriv(atlas->texture->texture.id, GL_TEXTURE_SWIZZLE_RGBA, swizzle_mask.data());
    return atlas;
}
void Renderer::set_target(Texture *target)
{
    if(target == nullptr) {
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    } else {
        k_expects(target->framebuffer.id != 0);
        glBindFramebuffer(GL_FRAMEBUFFER, target->framebuffer.id);
    }
    render_target = target;
    set_viewport({});
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
        renderer_w = r->w;
        renderer_h = r->h;
        renderer_w_div2 = r->w / 2.0;
        renderer_h_div2 = r->h / 2.0;
    } else {
        int w;
        int h;
        if(render_target == nullptr) {
            SDL_GetWindowSize(window, &w, &h);
        } else {
            w = render_target->get_w();
            h = render_target->get_h();
        }
        glViewport(0, 0, w, h);
        renderer_w = w;
        renderer_h = h;
        renderer_w_div2 = w / 2.0;
        renderer_h_div2 = h / 2.0;
    }
}
int Renderer::get_render_w() const
{
    return renderer_w;
}
int Renderer::get_render_h() const
{
    return renderer_h;
}
std::unique_ptr<Texture> Renderer::make_texture_2d_array(int w,
                                                         int h,
                                                         int slices,
                                                         Texture::Format format,
                                                         bool is_srgb,
                                                         int samples,
                                                         void *pixels) const
{
    auto tex = std::unique_ptr<Texture>(new Texture(2, slices, w, h, format, is_srgb, samples, pixels));
    return tex;
}
std::unique_ptr<Texture> Renderer::make_texture_target(int w,
                                                       int h,
                                                       Texture::Format format,
                                                       bool is_srgb,
                                                       int samples,
                                                       void *pixels) const
{
    auto tex = std::unique_ptr<Texture>(new Texture(2, 1, w, h, format, is_srgb, samples, pixels));
    tex->make_targetable();
    return tex;
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

constexpr std::array<GLenum, 4> blend_factor_enum_convert {
    GL_ZERO,
    GL_ONE,
    GL_SRC_ALPHA,
    GL_ONE_MINUS_SRC_ALPHA
};
static_assert(blend_factor_enum_convert[(int)BlendFactor::Zero] == GL_ZERO);
static_assert(blend_factor_enum_convert[(int)BlendFactor::One] == GL_ONE);
static_assert(blend_factor_enum_convert[(int)BlendFactor::SrcAlpha] == GL_SRC_ALPHA);
static_assert(blend_factor_enum_convert[(int)BlendFactor::OneMinusSrcAlpha] == GL_ONE_MINUS_SRC_ALPHA);

void Renderer::set_blend_factors(BlendFactor src, BlendFactor dst)
{
    blend_factors = std::make_pair(src, dst);
    glBlendFunc(blend_factor_enum_convert[(int)src], blend_factor_enum_convert[(int)dst]);
}
void Renderer::set_blend_factors(const std::pair<BlendFactor, BlendFactor> &factors)
{
    blend_factors = factors;
    glBlendFunc(blend_factor_enum_convert[(int)factors.first],
                blend_factor_enum_convert[(int)factors.second]);
}
const std::pair<BlendFactor, BlendFactor>& Renderer::get_blend_factors() const
{
    return blend_factors;
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
constexpr std::array<GLenum, 3> draw_mode_enum_convert {
    GL_TRIANGLES,
    GL_TRIANGLE_FAN,
    GL_TRIANGLE_STRIP
};
static_assert(draw_mode_enum_convert[(int)DrawMode::Triangles] == GL_TRIANGLES);
static_assert(draw_mode_enum_convert[(int)DrawMode::TriangleFan] == GL_TRIANGLE_FAN);
static_assert(draw_mode_enum_convert[(int)DrawMode::TriangleStrip] == GL_TRIANGLE_STRIP);

void Renderer::draw_arrays(DrawMode mode, GLint first, GLsizei count)
{
    glDrawArrays(draw_mode_enum_convert[(int)mode], first, count);
}
void Renderer::draw_arrays_instanced(DrawMode mode, GLint first, GLsizei count, GLsizei instance_cnt)
{
    glDrawArraysInstanced(draw_mode_enum_convert[(int)mode], first, count, instance_cnt);
}
void Renderer::draw_elements(DrawMode mode, GLint first, GLsizei count)
{
    glDrawElements(draw_mode_enum_convert[(int)mode], count, GL_UNSIGNED_INT, (void*)(first*sizeof(GLuint)));
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
void Renderer::refresh()
{
    auto cur_time = Time::now();
    auto cutoff = cur_time - Time::Delta((int64_t)1, Time::Length::second);
    while(!frame_timestamps.empty() && frame_timestamps.front() < cutoff)
        frame_timestamps.pop();
    frame_timestamps.push(cur_time);

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
