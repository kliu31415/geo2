#pragma once

#include "kx/gfx/renderer_types.h"
#include "kx/fixed_size_array.h"

#include "span_lite.h"

#include <vector>
#include <cstdint>
#include <memory>
#include <string>
#include <variant>

namespace kx { namespace gfx {
    class Renderer;
    class ShaderProgram;
    class KWindowRunning;
    using UBIndex = unsigned; //this is static asserted in the cpp file.
}}

namespace geo2 {

constexpr int MAX_RO_NUM_UBOS = 16;
constexpr int MAX_RO_UBO_SIZE = 1<<14;

class UBO_Allocator;

class GShader final
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
    GShader(const std::shared_ptr<kx::gfx::ShaderProgram> &program_,
            int max_instances_,
            const kx::gfx::DrawMode draw_mode_,
            int count_);

    void add_UB(const std::string &name, int global_data_sz, int instance_data_sz);
    size_t get_num_UBs() const;
    int get_instance_uniform_size_bytes(size_t idx) const;
    int get_max_instances() const;
    void render(UBO_Allocator *ubo_allocator,
                nonstd::span<const FSA<FSA<uint8_t>>*> instance_uniform_data,
                kx::gfx::KWindowRunning *kwin_r,
                kx::Passkey<class RenderOpList>) const;

    ///nonmovable because things hold references to it
    GShader(GShader&&) = delete;
    GShader &operator = (GShader&&) = delete;
};

class RenderOp
{
public:
    virtual ~RenderOp() = default;
};

class RenderOpShader final: public RenderOp
{
    friend class RenderOpList;
    friend class RenderOpGroup;

    const GShader *shader;
    kx::FixedSizeArray<kx::FixedSizeArray<uint8_t>> instance_uniform_data;
public:
    static bool cmp_shader(const RenderOpShader &a, const RenderOpShader &b);

    RenderOpShader(const GShader &shader_);
    void set_instance_uniform(size_t uniform_id, nonstd::span<uint8_t> data);
    nonstd::span<uint8_t> map_instance_uniform(size_t uniform_id);
};

class RenderOpGroup final
{
    friend class RenderOpList;

    float priority;
    const GShader *op_cmp; ///used to speed up sorting by op
    std::vector<std::shared_ptr<RenderOp>> ops;
public:
    static bool cmp_priority(const std::shared_ptr<RenderOpGroup> &a,
                             const std::shared_ptr<RenderOpGroup> &b);
    static bool cmp_op(const std::shared_ptr<RenderOpGroup> &a,
                       const std::shared_ptr<RenderOpGroup> &b);

    RenderOpGroup(float priority_);
    void add_op(const std::shared_ptr<RenderOp> &op);
    bool empty() const;
};

class RenderOpList
{
    kx::gfx::Renderer *cur_renderer;
    std::vector<std::shared_ptr<RenderOpGroup>> op_groups;
    std::unique_ptr<UBO_Allocator> ubo_allocator;
protected:
    void render_internal(kx::gfx::KWindowRunning *kwin_r, int render_w, int render_h);
public:
    RenderOpList();
    virtual ~RenderOpList();
    void add_op_group(const std::shared_ptr<RenderOpGroup> &op_group);
    void add_op_groups(const std::vector<std::shared_ptr<RenderOpGroup>> &op_groups_);
    void clear();
};

}
