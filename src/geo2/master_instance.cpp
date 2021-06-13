#include "geo2/master_instance.h"
#include "kx/gfx/renderer.h"

namespace geo2 {

void run()
{
    using namespace kx;

    auto instance = std::make_shared<MasterInstance>(gfx::Rect(0, 0, 1600, 900));

    auto window = gfx::KWindow::create("geo2",
                                       SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                                       1600, 900,
                                       SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);
    gfx::AbstractWindow::RendererAtny::show_fps(window.get(), true);
    gfx::AbstractWindow::RendererAtny::set_fps_color(window.get(), gfx::Color::CYAN);
    window->add_item_front(instance);

    while(true) {
        gfx::update_input();
        if(window->run() != gfx::KWindow::Status::Running)
            break;
        gfx::clean_memory();
    }
}

MasterInstance::MasterInstance(const kx::gfx::Rect &render_rect_):
    KItem(render_rect_),
    state(State::InGame),
    render_w(1600),
    render_h(900),
    game(std::make_unique<Game>())
{

}
std::shared_ptr<kx::gfx::Texture> MasterInstance::run(kx::gfx::KWindowRunning *kwin_r)
{
    using namespace kx;

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
        return_texture = game->run(kwin_r, render_w, render_h);
        break;
    case State::MainMenu:
        break;
    default:
        log_error("bad geo2::MasterInstance::State");
    }

    return return_texture;
}
}
