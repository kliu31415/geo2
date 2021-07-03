#include "geo2/render_op.h"

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
            UBOs.push_back(rdr->make_UBO());
            rdr->bind_UBO(*UBOs.back());
            UBOs.back()->buffer_data(nullptr, MAX_RO_UBO_SIZE);
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

GShader::GShader(const std::shared_ptr<kx::gfx::ShaderProgram> &program_,
                 int max_instances_,
                 const kx::gfx::DrawMode draw_mode_,
                 int count_):
    program(program_),
    max_instances(max_instances_),
    draw_mode(draw_mode_),
    count(count_)
{}
std::unique_ptr<GShader> GShader::get_empty_gshader()
{
    return std::unique_ptr<GShader>(new GShader());
}
void GShader::add_UB(const std::string &name, int global_data_sz, int instance_data_sz)
{
    k_expects(global_data_sz + instance_data_sz*max_instances <= MAX_RO_UBO_SIZE);
    k_expects(UBs.size() < MAX_RO_NUM_UBOS);

    UB ub;
    ub.index = program->get_UB_index(name.c_str());
    ub.global_data_size = global_data_sz;
    ub.instance_data_size = instance_data_sz;
    UBs.push_back(std::move(ub));
}
size_t GShader::get_num_UBs() const
{
    return UBs.size();
}
int GShader::get_instance_uniform_size_bytes(size_t idx) const
{
    return UBs[idx].instance_data_size;
}
int GShader::get_max_instances() const
{
    return max_instances;
}

using ByteFSA2D = kx::FixedSizeArray<kx::FixedSizeArray<uint8_t>>;

template<class T> nonstd::span<T> to_span(const kx::FixedSizeArray<uint8_t> &data)
{
    return {(T*)data.begin(), (T*)data.end()};
}
void GShader::render(UBO_Allocator *ubo_allocator,
                     nonstd::span<const ByteFSA2D*> instance_uniform_data,
                     kx::gfx::KWindowRunning *kwin_r,
                     kx::Passkey<class RenderOpList>) const
{
    size_t num_instances = instance_uniform_data.size();
    size_t num_instance_uniforms = UBs.size();
    ByteFSA2D grouped_iu_data(num_instance_uniforms);
    for(size_t i=0; i<num_instance_uniforms; i++) {
        auto usize = get_instance_uniform_size_bytes(i);
        grouped_iu_data[i] = kx::FixedSizeArray<uint8_t>(usize * num_instances);
    }
    size_t idx = 0;
    for(const auto &single_iu_data: instance_uniform_data) {
        for(size_t i=0; i<num_instance_uniforms; i++) {
            auto usize = get_instance_uniform_size_bytes(i);
            k_expects((*single_iu_data)[i].size() == (size_t)usize);
            std::copy_n((*single_iu_data)[i].begin(), usize, grouped_iu_data[i].begin() + idx*usize);
        }
        idx++;
    }

    auto rdr = kwin_r->rdr();
    rdr->prepare_for_custom_shader();
    rdr->use_shader_program(*program);
    for(size_t i=0; i<UBs.size(); i++) {
        auto usize = get_instance_uniform_size_bytes(i);
        auto ubo = ubo_allocator->get_UBO();
        rdr->bind_UBO(*ubo);
        ubo->buffer_sub_data(UBs[i].global_data_size, grouped_iu_data[i].begin(), usize*num_instances);
        rdr->bind_UB_base(i, *ubo);
        program->bind_UB(UBs[i].index, i);
    }
    rdr->draw_arrays_instanced(draw_mode, 0, count, num_instances);
}

bool RenderOpShader::cmp_shader(const RenderOpShader &a, const RenderOpShader &b)
{
    return a.shader < b.shader;
}
RenderOpShader::RenderOpShader(const GShader &shader_):
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
void RenderOpList::add_op_group(const std::shared_ptr<RenderOpGroup> &op_group)
{
    op_groups.push_back(op_group);
}
void RenderOpList::add_op_groups(const std::vector<std::shared_ptr<RenderOpGroup>> &op_groups_)
{
    op_groups.insert(op_groups.end(), op_groups_.begin(), op_groups_.end());
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

    //Remove all empty op groups. Sort groups by priority, and within each priority level,
    //sort groups by shader if the group's first element is a shader. All groups with
    //non-shaders as initial elements are put at the beginning.
    op_groups.erase(std::remove_if(std::execution::par_unseq,
                    op_groups.begin(),
                    op_groups.end(),
                    [](const std::shared_ptr<RenderOpGroup> &g) -> bool {return g->empty();}),
                    op_groups.end());
    std::sort(std::execution::par_unseq,
              op_groups.begin(),
              op_groups.end(),
              RenderOpGroup::cmp_priority);
    size_t prev_pos = 0;
    std::vector<RenderOp*> ops_flattened;
    for(size_t i=1; i<=op_groups.size(); i++) {
        if(i==op_groups.size() || op_groups[i]->priority != op_groups[i-1]->priority) {
            std::sort(std::execution::par_unseq,
                      op_groups.begin() + prev_pos,
                      op_groups.begin() + i,
                      RenderOpGroup::cmp_op);
            for(size_t j=prev_pos; j<i; j++) {
                for(const auto &op: op_groups[j]->ops) {
                    ops_flattened.push_back(op.get());
                }
            }
            prev_pos = i;
        }
    }
    //render everything
    std::vector<const ByteFSA2D*> instance_uniform_data;
    const GShader *cur_shader = nullptr;
    for(const auto &op: ops_flattened) {
        if(auto shader = dynamic_cast<RenderOpShader*>(op)) {
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
            kx::log_error("unknown RenderOp value");
        }
    }
    if(cur_shader!=nullptr)
        cur_shader->render(ubo_allocator.get(), instance_uniform_data, kwin_r, {});
}
void RenderOpList::clear()
{
    op_groups.clear();
}

}
