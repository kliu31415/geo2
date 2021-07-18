#include "geo2/render_op.h"
#include "geo2/timer.h"
#include "geo2/texture_utils.h"

#include "kx/gfx/renderer.h"
#include "kx/gfx/kwindow.h"

#include <cmath>

namespace geo2 {

static_assert(std::is_same<kx::gfx::UBIndex, unsigned>::value);

class UBO_Allocator
{
    std::vector<std::unique_ptr<kx::gfx::UBO>> UBOs;
    size_t idx;
public:
    UBO_Allocator(kx::gfx::Renderer *rdr)
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
                     kx::gfx::Renderer *rdr,
                     kx::Passkey<class RenderOpList>) const
{
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

Font::Font(kx::gfx::Renderer *rdr, const kx::gfx::Font *font):
    atlas(rdr->make_ascii_atlas(font, kx::gfx::Font::MAX_FONT_SIZE))
{}

RenderOpText::RenderOpText():
    font(nullptr),
    color(kx::gfx::Color::BLACK),
    font_size(std::numeric_limits<decltype(font_size)>::quiet_NaN()),
    x(std::numeric_limits<decltype(x)>::quiet_NaN()),
    w(std::numeric_limits<decltype(w)>::max()),
    y(std::numeric_limits<decltype(y)>::quiet_NaN()),
    horizontal_align(HorizontalAlign::Left),
    vertical_align(VerticalAlign::Top)
{}
void RenderOpText::set_text(const std::string &text_)
{
    text = text_;
}
void RenderOpText::set_font(const Font *font_)
{
    font = font_;
}
void RenderOpText::set_color(const kx::gfx::LinearColor &color_)
{
    color = color_;
}
void RenderOpText::set_font_size(float size_)
{
    font_size = size_;
}
void RenderOpText::set_x(float x_)
{
    x = x_;
}
void RenderOpText::set_w(float w_)
{
    w = w_;
}
void RenderOpText::set_y(float y_)
{
    y = y_;
}
void RenderOpText::set_horizontal_align(HorizontalAlign align)
{
    horizontal_align = align;
}
void RenderOpText::set_vertical_align(VerticalAlign align)
{
    vertical_align = align;
}
void RenderOpText::add_iu_data(std::vector<float> *iu_data,
                               const kx::gfx::Renderer *rdr,
                               kx::Passkey<RenderOpList>) const
{
    k_expects(font != nullptr);

    k_expects(!std::isnan(font_size));
    k_expects(!std::isnan(x));
    k_expects(!std::isnan(y));

    //no support for other alignments yet
    k_expects(horizontal_align == HorizontalAlign::Left);
    k_expects(vertical_align == VerticalAlign::Top);

    double cur_x = x;
    double cur_y = y + font_size * font->atlas->max_ascent_per_size;
    double cur_w = 0;
    for(const auto &c: text) {
        k_assert(c != '\0');
        const auto &metrics_per_size = font->atlas->glyph_metrics_per_size[c];
        std::remove_const_t<std::remove_reference_t<decltype(metrics_per_size)>> metrics;
        metrics.w = font_size * metrics_per_size.w;
        metrics.h = font_size * metrics_per_size.h;
        metrics.min_x = font_size * metrics_per_size.min_x;
        metrics.max_x = font_size * metrics_per_size.max_x;
        metrics.min_y = font_size * metrics_per_size.min_y;
        metrics.max_y = font_size * metrics_per_size.max_y;
        metrics.advance = font_size * metrics_per_size.advance;

        //if the assert fails, then our width is too small for the font size;
        //we can't even fit one character in the alloted width!
        k_assert(metrics.advance <= w);

        auto next_w = cur_w + metrics.advance;
        if(next_w > w) {
            cur_w = 0;
            cur_x = x;
            cur_y += font->atlas->font->get_recommended_line_skip(font->atlas->font_size);
        }

        auto x1 = rdr->x_to_ndc(cur_x + metrics.min_x);
        auto x2 = rdr->x_to_ndc(cur_x + metrics.max_x);
        auto y1 = rdr->y_to_ndc(cur_y + metrics.min_y);
        auto y2 = rdr->y_to_ndc(cur_y + metrics.max_y);

        iu_data->push_back(x1);
        iu_data->push_back(y2);
        iu_data->push_back(x2 - x1);
        iu_data->push_back(y1 - y2);

        iu_data->push_back(font_size);
        iu_data->push_back(int_bits_to_float(c));
        iu_data->push_back(metrics.w / (double)font->atlas->max_w);
        iu_data->push_back(metrics.h / (double)font->atlas->max_h);

        iu_data->push_back(color.r);
        iu_data->push_back(color.g);
        iu_data->push_back(color.b);
        iu_data->push_back(color.a);

        cur_x += metrics.advance;
        cur_w += metrics.advance;
    }
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

//these values should correspond to uniform block's properties in the text shader
constexpr size_t TEXT_IU_FLOATS_PER_INSTANCE = 3*4;
constexpr size_t TEXT_MAX_INSTANCES = 341;
constexpr auto TEXT_MAX_FLOATS_PER_BATCH = TEXT_IU_FLOATS_PER_INSTANCE * TEXT_MAX_INSTANCES;

void RenderOpList::render_text(kx::gfx::Renderer *rdr, const Font *font, const std::vector<float> &text_iu_data)
{
    k_expects(font != nullptr);

    rdr->prepare_for_custom_shader();
    rdr->use_shader_program(*text_ascii);
    rdr->set_active_texture(0);
    rdr->bind_texture(*font->atlas->texture);
    text_ascii->set_uniform1i(text_ascii_atlas_loc, 0);

    for(size_t start_idx=0; start_idx < text_iu_data.size(); start_idx += TEXT_MAX_FLOATS_PER_BATCH) {
        auto end_idx = std::min(text_iu_data.size(), start_idx + TEXT_MAX_FLOATS_PER_BATCH);
        auto num_instances = end_idx / TEXT_IU_FLOATS_PER_INSTANCE;

        auto ubo = ubo_allocator->get_UBO();
        rdr->bind_UBO(*ubo);
        ubo->invalidate();
        ubo->buffer_sub_data(0, text_iu_data.data() + start_idx, (end_idx - start_idx) * sizeof(float));
        rdr->bind_UB_base(0, *ubo);
        text_ascii->bind_UB(text_ascii_characters_ub_index, 0);
        rdr->draw_arrays_instanced(kx::gfx::DrawMode::TriangleStrip, 0, 4, num_instances);
    }
}
void RenderOpList::render_internal(kx::gfx::KWindowRunning *kwin_r,
                                   [[maybe_unused]] int render_w,
                                   [[maybe_unused]] int render_h)
{
    auto rdr = kwin_r->rdr();

    if(cur_renderer != rdr) {
        cur_renderer = rdr;
        ubo_allocator = std::make_unique<UBO_Allocator>(rdr);

        auto vert = rdr->make_vert_shader("geo2_data/shaders/text_ascii_1.vert");
        auto frag = rdr->make_frag_shader("geo2_data/shaders/text_ascii_1.frag");
        text_ascii = rdr->make_shader_program(*vert, *frag);
        text_ascii_characters_ub_index = text_ascii->get_UB_index("characters");
        text_ascii_atlas_loc = text_ascii->get_uniform_loc("atlas");
    }

    auto cmp_op_group = [](const std::shared_ptr<RenderOpGroup> &a,
                           const std::shared_ptr<RenderOpGroup> &b) ->
                                bool
                           {
                               //sort by priority, then by op/
                               //this could be further optimized by alternating
                               //between using "<" and ">" for op; this requires us
                               //to know the priority rank beforehand.
                               if(a->priority != b->priority)
                                    return a->priority < b->priority;
                               return a->op_cmp < b->op_cmp;
                           };
    //sort groups by priority
    std::sort(op_groups.begin(), op_groups.end(), cmp_op_group);

    const IShader *cur_shader = nullptr;
    std::vector<const ByteFSA2D*> shader_iu_data;

    const Font *cur_font = nullptr;
    std::vector<float> text_iu_data;

    for(size_t i=0; i<op_groups.size(); i++) {
        for(const auto &op: op_groups[i]->ops) {
            if(auto shader_op = dynamic_cast<RenderOpShader*>(op.get())) {
                if(cur_shader!=nullptr) {
                   if(shader_iu_data.size()==(size_t)shader_op->shader->get_max_instances() ||
                    cur_shader!=shader_op->shader)
                    {
                        cur_shader->render(ubo_allocator.get(), shader_iu_data, rdr, {});
                        shader_iu_data.clear();
                    }
                } else if(!text_iu_data.empty()) {
                    render_text(rdr, cur_font, text_iu_data);
                    text_iu_data.clear();
                    cur_font = nullptr;
                }
                cur_shader = shader_op->shader;
                shader_iu_data.push_back(&shader_op->instance_uniform_data);
            } else if(auto text_op = dynamic_cast<RenderOpText*>(op.get())) {
                if(cur_shader != nullptr) {
                    cur_shader->render(ubo_allocator.get(), shader_iu_data, rdr, {});
                    cur_shader = nullptr;
                    shader_iu_data.clear();
                }
                if(!text_iu_data.empty() && cur_font!=text_op->font) {
                    render_text(rdr, cur_font, text_iu_data);
                    text_iu_data.clear();
                }
                cur_font = text_op->font;
                text_op->add_iu_data(&text_iu_data, rdr, {});
            } else {
                kx::log_error("unknown RenderOp type");
            }
        }
    }

    //render any buffered data
    if(cur_shader != nullptr)
        cur_shader->render(ubo_allocator.get(), shader_iu_data, rdr, {});
    else if(!text_iu_data.empty()) {
        render_text(rdr, cur_font, text_iu_data);
    }
}
void RenderOpList::steal_op_groups_into(std::vector<std::shared_ptr<RenderOpGroup>> *op_groups_)
{
    *op_groups_ = std::move(op_groups);
}

}
