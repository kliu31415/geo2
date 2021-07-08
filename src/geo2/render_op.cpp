#include "geo2/render_op.h"
#include "geo2/timer.h"

#include "kx/gfx/renderer.h"
#include "kx/gfx/kwindow.h"

#include <execution>

namespace geo2 {

static_assert(std::is_same<kx::gfx::UBIndex, unsigned>::value);

class UBO_Allocator
{
    std::vector<std::unique_ptr<kx::gfx::UBO>> UBOs;
    size_t idx;
public:
    void reserve_UBOs(kx::gfx::Renderer *rdr)
    {
        idx = 0;
        UBOs.clear();
        for(int i=0; i<MAX_RO_NUM_UBOS; i++) {
            auto ubo = rdr->make_UBO();
            rdr->bind_UBO(*ubo);
            ubo->buffer_data(nullptr, MAX_RO_UBO_SIZE);
            UBOs.emplace_back(std::move(ubo));
        }
    }
    kx::gfx::UBO *get_UBO()
    {
        idx++;
        if(idx == UBOs.size())
            idx = 0;

        return UBOs[idx].get();
    }
};

IShader::IShader(const std::shared_ptr<kx::gfx::ShaderProgram> &program_,
                 int max_instances_,
                 const kx::gfx::DrawMode draw_mode_,
                 int count_):
    program(program_),
    max_instances(max_instances_),
    draw_mode(draw_mode_),
    count(count_)
{}
std::unique_ptr<IShader> IShader::get_empty_IShader()
{
    return std::unique_ptr<IShader>(new IShader());
}
void IShader::add_UB(const std::string &name, int global_data_sz, int instance_data_sz)
{
    k_expects(global_data_sz + instance_data_sz*max_instances <= MAX_RO_UBO_SIZE);
    k_expects(UBs.size() < MAX_RO_NUM_UBOS);

    UB ub;
    ub.index = program->get_UB_index(name.c_str());
    ub.global_data_size = global_data_sz;
    ub.instance_data_size = instance_data_sz;
    UBs.push_back(std::move(ub));
}
size_t IShader::get_num_UBs() const
{
    return UBs.size();
}
int IShader::get_instance_uniform_size_bytes(size_t idx) const
{
    return UBs[idx].instance_data_size;
}
int IShader::get_max_instances() const
{
    return max_instances;
}

using ByteFSA2D = kx::FixedSizeArray<kx::FixedSizeArray<uint8_t>>;

template<class T> nonstd::span<T> to_span(const kx::FixedSizeArray<uint8_t> &data)
{
    return {(T*)data.begin(), (T*)data.end()};
}
void IShader::render(UBO_Allocator *ubo_allocator,
                     nonstd::span<const ByteFSA2D*> instance_uniform_data,
                     kx::gfx::KWindowRunning *kwin_r,
                     kx::Passkey<class RenderOpList>) const
{
    auto rdr = kwin_r->rdr();
    rdr->prepare_for_custom_shader();
    rdr->use_shader_program(*program);

    size_t num_instances = instance_uniform_data.size();
    size_t num_instance_uniforms = UBs.size();

    k_expects(num_instance_uniforms <= MAX_RO_NUM_UBOS);

    for(size_t i=0; i<num_instance_uniforms; i++) {
        auto usize = get_instance_uniform_size_bytes(i);
        auto ubo = ubo_allocator->get_UBO();
        rdr->bind_UBO(*ubo);
        ubo->invalidate();

        kx::FixedSizeArray<uint8_t> data(usize*num_instances);
        for(size_t j=0; j<instance_uniform_data.size(); j++) {
            k_expects((*instance_uniform_data[j])[i].size() == (size_t)usize);
            std::copy_n((*instance_uniform_data[j])[i].begin(), usize, data.begin() + j*usize);
        }

        ubo->buffer_sub_data(UBs[i].global_data_size, data.begin(), usize*num_instances);
        rdr->bind_UB_base(i, *ubo);
        program->bind_UB(UBs[i].index, i);
    }

    rdr->draw_arrays_instanced(draw_mode, 0, count, num_instances);

}

bool RenderOpShader::cmp_shader(const RenderOpShader &a, const RenderOpShader &b)
{
    return a.shader < b.shader;
}
RenderOpShader::RenderOpShader(const IShader &shader_):
    shader(&shader_),
    instance_uniform_data(shader->get_num_UBs())
{
    for(size_t i=0; i<instance_uniform_data.size(); i++) {
        auto len = shader->get_instance_uniform_size_bytes(i);
        instance_uniform_data[i] = kx::FixedSizeArray<uint8_t>(len);
    }
}
void RenderOpShader::set_instance_uniform(size_t uniform_id, nonstd::span<uint8_t> data)
{
    k_expects(data.size() == instance_uniform_data[uniform_id].size());
    std::copy(data.begin(), data.end(), instance_uniform_data[uniform_id].begin());
}
nonstd::span<uint8_t> RenderOpShader::map_instance_uniform(size_t uniform_id)
{
    return to_span<uint8_t>(instance_uniform_data[uniform_id]);
}
bool RenderOpGroup::cmp_priority(const std::shared_ptr<RenderOpGroup> &a,
                                 const std::shared_ptr<RenderOpGroup> &b)
{
    return a->priority < b->priority;
}
bool RenderOpGroup::cmp_op(const std::shared_ptr<RenderOpGroup> &a,
                           const std::shared_ptr<RenderOpGroup> &b)
{
    return a->op_cmp < b->op_cmp;
}
RenderOpGroup::RenderOpGroup(float priority_):
    priority(priority_)
{}
void RenderOpGroup::add_op(const std::shared_ptr<RenderOp> &op)
{
    if(empty()) {
        if(auto shader_op = dynamic_cast<RenderOpShader*>(op.get()))
            op_cmp = shader_op->shader;
        else op_cmp = nullptr;
    }
    ops.push_back(op);
}
void RenderOpGroup::clear()
{
    ops.clear();
}
bool RenderOpGroup::empty() const
{
    return ops.empty();
}

RenderOpList::RenderOpList():
    cur_renderer(nullptr)
{}
RenderOpList::~RenderOpList()
{}
void RenderOpList::set_op_groups(std::vector<std::shared_ptr<RenderOpGroup>> &&op_groups_)
{
    op_groups = std::move(op_groups_);
}
void RenderOpList::render_internal(kx::gfx::KWindowRunning *kwin_r,
                                   [[maybe_unused]] int render_w,
                                   [[maybe_unused]] int render_h)
{
    auto rdr = kwin_r->rdr();

    if(ubo_allocator == nullptr) {
        ubo_allocator = std::make_unique<UBO_Allocator>();
        ubo_allocator->reserve_UBOs(rdr);
    }

    if(cur_renderer != rdr) {
        cur_renderer = rdr;
        ubo_allocator->reserve_UBOs(rdr);
    }

    //sort groups by priority
    std::sort(op_groups.begin(),
              op_groups.end(),
              RenderOpGroup::cmp_priority);

    std::vector<const ByteFSA2D*> instance_uniform_data;
    const IShader *cur_shader = nullptr;
    size_t prev_pos = 0;

    for(size_t i=1; i<=op_groups.size(); i++) {
        if(i==op_groups.size() || op_groups[i]->priority != op_groups[i-1]->priority) {
            //within a priority level, sort by op to take advantage of batching
            std::sort(op_groups.begin() + prev_pos,
                      op_groups.begin() + i,
                      RenderOpGroup::cmp_op);

            //render all ops at this priority level
            for(size_t j=prev_pos; j<i; j++) {
                for(const auto &op: op_groups[j]->ops) {
                    if(auto shader = dynamic_cast<RenderOpShader*>(op.get())) {
                        if(cur_shader!=nullptr &&
                           (instance_uniform_data.size()==(size_t)shader->shader->get_max_instances() ||
                            cur_shader!=shader->shader))
                        {
                            cur_shader->render(ubo_allocator.get(), instance_uniform_data, kwin_r, {});
                            instance_uniform_data.clear();
                        }
                        cur_shader = shader->shader;
                        instance_uniform_data.push_back(&shader->instance_uniform_data);
                    } else {
                        kx::log_error("unknown RenderOp type");
                    }
                }
            }
            prev_pos = i;
        }
    }

    //render any buffered data
    if(cur_shader != nullptr)
        cur_shader->render(ubo_allocator.get(), instance_uniform_data, kwin_r, {});
}
void RenderOpList::steal_op_groups_into(std::vector<std::shared_ptr<RenderOpGroup>> *op_groups_)
{
    *op_groups_ = std::move(op_groups);
}

}
