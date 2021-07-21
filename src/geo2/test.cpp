#include "geo2/test.h"
#include "kx/io.h"
#include "kx/gfx/gfx.h"
#include "kx/gfx/renderer.h"

#include <cmath>

namespace geo2
{

void test1(kx::gfx::GfxLibrary *library)
{
    using namespace kx;
    auto dwindow = gfx::DWindow::make(library,
                                      "geo2 test1",
                                      SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                                      800, 800,
                                      SDL_WINDOW_SHOWN);
    dwindow->rdr()->show_fps(true);
    dwindow->rdr()->set_fps_color(gfx::Color::CYAN);
    gfx::SRGB_Color c(0.0, 0.0, 0.0, 1.0);
    while(true) {
        library->update_input();

        SDL_Event input;
        while(dwindow->poll_input(&input)) {
            if(input.type==SDL_WINDOWEVENT && input.window.event==SDL_WINDOWEVENT_CLOSE)
                goto end_loop;
        }

        auto renderer = dwindow->rdr();
        renderer->set_color(gfx::SRGB_Color(0.5f, 0.5f, 0.5f, 1.0f));
        renderer->clear();

        c.r = std::fmod(c.r + 0.05, 1);
        c.g = std::fmod(c.g + 0.056, 1);
        c.b = std::fmod(c.b + 0.061, 1);
        renderer->set_color(c);

        for(size_t i=0; i<1e5; i++) {
            renderer->fill_rect(gfx::Rect(10.0, 10.0, 8.0, 12.0));
        }

        renderer->refresh();

        //io::print("                             \r" + to_str(renderer->get_fps()));

        library->clean_memory();
    }
    end_loop: (void)0;
}
}
