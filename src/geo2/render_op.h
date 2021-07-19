#pragma once

#include "kx/gfx/renderer_types.h"
#include "kx/gfx/font.h"
#include "kx/fixed_size_array.h"

#include "kx/kx_span.h"

#include <vector>
#include <cstdint>
#include <memory>
#include <string>
#include <variant>

namespace kx { namespace gfx {
    class Renderer;
    class ShaderProgram;
    class KWindowRunning;
    struct ASCII_Atlas;
    using UBIndex = unsigned; //this is static asserted in the cpp file.
}}

namespace geo2 {

constexpr int MAX_RO_NUM_UBOS = 16;
constexpr int MAX_RO_UBO_SIZE = 1<<14;

class UBO_Allocator;

class IShader final
{
private:
    template<class T> using FSA = kx::FixedSizeArray<T>;
    //Each UB has some global data at the beginning, then up to
    //MAX_INSTANCES blocks of instance data
    struct UB
    {
        kx::gfx::UBIndex index;
        int global_data_size; //bytes
        int instance_data_size; //bytes
    };

    std::shared_ptr<kx::gfx::ShaderProgram> program;
    int max_instances;
    kx::gfx::DrawMode draw_mode;
    int count;
    std::vector<UB> UBs;
public:
    IShader(const std::shared_ptr<kx::gfx::ShaderProgram> &program_,
            int max_instances_,
            const kx::gfx::DrawMode draw_mode_,
            int count_);

    void add_UB(const std::string &name, int global_data_sz, int instance_data_sz);
    size_t get_num_UBs() const;
    int get_instance_uniform_size_bytes(size_t idx) const;
    int get_max_instances() const;
    void render(UBO_Allocator *ubo_allocator,
                kx::kx_span<const FSA<FSA<uint8_t>>*> instance_uniform_data,
                kx::gfx::Renderer *rdr,
                kx::Passkey<class RenderOpList>) const;

    ///nonmovable because things hold pointers to it
    IShader(IShader&&) = delete;
    IShader &operator = (IShader&&) = delete;
};

class RenderOp
{
public:
    virtual ~RenderOp() = default;
};

/** note we have a non-owning pointer here (IShader *shader); it's assumed that while
 *  a RenderOpShader is used, the pointed-to shader is valid; generally, the pointed
 *  to shader should be loaded upon startup and destroyed upon closing the game, so
 *  this should rarely be an issue.
 */
class RenderOpShader final: public RenderOp
{
    friend class RenderOpList;
    friend class RenderOpGroup;

    const IShader *shader;
    kx::FixedSizeArray<kx::FixedSizeArray<uint8_t>> instance_uniform_data;
public:
    RenderOpShader(const IShader &shader_);
    void set_instance_uniform(size_t uniform_id, kx::kx_span<uint8_t> data);
    kx::kx_span<uint8_t> map_instance_uniform(size_t uniform_id);
};

struct Font
{
    std::unique_ptr<kx::gfx::ASCII_Atlas> atlas;

    Font(kx::gfx::Renderer *rdr, kx::gfx::Font *font_);
};

class RenderOpText final: public RenderOp
{
    friend class RenderOpList;
public:
    enum class HorizontalAlign: uint8_t {
        Left, Center, Right
    };
    enum class VerticalAlign: uint8_t {
        Top, Center, Bottom
    };
private:
    std::string text;
    const Font *font;
    kx::gfx::LinearColor color;
    float font_size;
    float x;
    float w;
    float y;
    HorizontalAlign horizontal_align;
    VerticalAlign vertical_align;
public:
    RenderOpText();
    void set_text(const std::string &text_);
    void set_font(const Font *font_);
    void set_color(const kx::gfx::LinearColor &color_);
    void set_font_size(float size_);
    void set_x(float x_);
    void set_w(float w_);
    void set_y(float y_);
    void set_horizontal_align(HorizontalAlign align);
    void set_vertical_align(VerticalAlign align);
    void add_iu_data(std::vector<float> *iu_data,
                     const kx::gfx::Renderer *rdr,
                     kx::Passkey<RenderOpList>) const;
};

class RenderOpGroup final
{
    friend class RenderOpList;

    float priority;
    const IShader *op_cmp;
    std::vector<std::shared_ptr<RenderOp>> ops;
public:
    RenderOpGroup(float priority_);
    void add_op(const std::shared_ptr<RenderOp> &op);
    void clear();
    bool empty() const;
};

class RenderOpList
{
    kx::gfx::UBIndex text_ascii_characters_ub_index;
    int text_ascii_atlas_loc;
    std::unique_ptr<kx::gfx::ShaderProgram> text_ascii;

    kx::gfx::Renderer *cur_renderer;
    std::vector<std::shared_ptr<RenderOpGroup>> op_groups;
    std::unique_ptr<UBO_Allocator> ubo_allocator;

    void render_text(kx::gfx::Renderer *rdr, const Font *font, const std::vector<float> &text_iu_data);
protected:
    void render_internal(kx::gfx::KWindowRunning *kwin_r, int render_w, int render_h);
public:
    RenderOpList();
    virtual ~RenderOpList();
    void set_op_groups(std::vector<std::shared_ptr<RenderOpGroup>> &&op_groups_);
    void steal_op_groups_into(std::vector<std::shared_ptr<RenderOpGroup>> *op_groups_);
};

}
