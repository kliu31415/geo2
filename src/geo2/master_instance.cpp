#include "geo2/master_instance.h"
#include "geo2/texture_utils.h"

#include "kx/gfx/renderer.h"

#include <SDL2/SDL_events.h>

namespace geo2 {

constexpr int SCREEN_W = 1920;
constexpr int SCREEN_H = 1080;

void run(const LibraryPointers &libraries)
{

    using namespace kx;

    auto instance = std::make_shared<MasterInstance>(libraries, gfx::Rect(0, 0, SCREEN_W, SCREEN_H));

    auto window = gfx::KWindow::make(libraries.gfx_library,
                                     "geo2",
                                     SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                                     SCREEN_W, SCREEN_H,
                                     SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE | SDL_WINDOW_FULLSCREEN);

    auto mono_default = libraries.font_library->get_font(kx::gfx::FontLibrary::FONT_MONO_DEFAULT);
    gfx::AbstractWindow::RendererAtny::set_fps_font(window.get(), mono_default);
    gfx::AbstractWindow::RendererAtny::show_fps(window.get(), true);
    gfx::AbstractWindow::RendererAtny::set_fps_color(window.get(), gfx::Color::CYAN);
    window->add_item_front(instance);

    uint64_t iteration = 0;
    constexpr uint64_t CLEAN_MEM_EVERY_N_ITERATIONS = 16;
    while(true) {
        libraries.gfx_library->update_input();
        if(window->run() != gfx::KWindow::Status::Running)
            break;
        if((iteration++) % CLEAN_MEM_EVERY_N_ITERATIONS == 0)
            libraries.gfx_library->clean_memory();
    }
}

class gfx_impl
{
    std::unique_ptr<kx::gfx::ShaderProgram> hdr;
    std::unique_ptr<kx::gfx::VAO> hdr_vao;
    std::shared_ptr<kx::gfx::VBO> full_target_vbo;
    std::array<float, 16> full_target;
    PersistentTextureTarget hdr_func_tex;
public:
    kx::gfx::Renderer *rdr;

    int render_w;
    int render_h;

    gfx_impl(kx::gfx::Renderer *rdr_):
        rdr(rdr_),
        render_w(SCREEN_W),
        render_h(SCREEN_H)
    {

        auto hdr_vert = rdr->make_vert_shader("geo2_data/shaders/hdr1.vert");
        auto hdr_frag = rdr->make_frag_shader("geo2_data/shaders/hdr1.frag");
        hdr = rdr->make_shader_program(*hdr_vert, *hdr_frag);
        hdr_vao = rdr->make_VAO();
        full_target_vbo = rdr->make_VBO();
        rdr->bind_VAO(*hdr_vao);
        rdr->bind_VBO(*full_target_vbo);
        full_target_vbo->buffer_data(nullptr, full_target.size() * sizeof(full_target[0]));
        hdr_vao->add_VBO(full_target_vbo);
        hdr_vao->vertex_attrib_pointer_f(0, 2, 4*sizeof(float), 0*sizeof(float)); //dst loc
        hdr_vao->vertex_attrib_pointer_f(1, 2, 4*sizeof(float), 2*sizeof(float)); //src loc
        hdr_vao->enable_vertex_attrib_array(0);
        hdr_vao->enable_vertex_attrib_array(1);
    }
    void apply_hdr(kx::gfx::Texture *texture)
    {
        using namespace kx::gfx;

        k_expects(!texture->is_srgb());
        auto original_target = rdr->get_target();
        auto original_blend_factors = rdr->get_blend_factors();
        kx::ScopeGuard sg([=]() -> void {
                                            rdr->set_target(original_target);
                                            rdr->set_blend_factors(original_blend_factors);
                                        });

        int w = texture->get_w();
        int h = texture->get_h();

        set_to_full_target(&full_target, rdr, w, h);
        rdr->bind_VBO(*full_target_vbo);
        full_target_vbo->buffer_data(&full_target[0], full_target.size() * sizeof(full_target[0]));

        auto tex = hdr_func_tex.get(rdr, w, h, Texture::Format::RGB16F, false);

        //copy the existing texture
        rdr->set_target(tex.get());
        rdr->set_blend_factors(BlendFactor::One, BlendFactor::Zero);
        rdr->draw_texture_nc(*texture, Rect(0.0, 0.0, 1.0, 1.0));

        //draw back to the passed in texture
        rdr->prepare_for_custom_shader();
        rdr->set_active_texture(0);
        rdr->set_target(texture);
        rdr->bind_texture(*tex);
        rdr->use_shader_program(*hdr);
        hdr->set_uniform1i(hdr->get_uniform_loc("texture_in"), 0);
        rdr->bind_VAO(*hdr_vao);
        rdr->draw_arrays(DrawMode::TriangleStrip, 0, 4);
    }
};

MasterInstance::MasterInstance(const LibraryPointers &libraries_, const kx::gfx::Rect &render_rect_):
    KItem(render_rect_),
    state(State::InGame),
    libraries(libraries_),
    game(new Game({}))
{

}
std::shared_ptr<kx::gfx::Texture> MasterInstance::run(kx::gfx::KWindowRunning *kwin_r)
{
    using namespace kx;

    if(gfx==nullptr || gfx->rdr!=kwin_r->rdr()) {
        gfx = std::make_unique<gfx_impl>(kwin_r->rdr());
    }

    for(const auto &input: kwin_r->input_deque) {
        switch(input->type) {
        case SDL_WINDOWEVENT:
            if(input->window.event == SDL_WINDOWEVENT_RESIZED) {
                kwin_r->rdr()->set_viewport({});
            }
            break;
        default:
            break;
        }
    }

    std::shared_ptr<gfx::Texture> return_texture;
    switch(state) {
    case State::InGame:
        return_texture = game->run(libraries, kwin_r, gfx->render_w, gfx->render_h);
        break;
    case State::MainMenu:
        break;
    default:
        log_error("bad geo2::MasterInstance::State");
    }

    k_expects(return_texture != nullptr);

    gfx->apply_hdr(return_texture.get());

    return return_texture;
}
}
