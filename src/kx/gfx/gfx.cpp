#include "kx/gfx/gfx.h"
#include "kx/gfx/renderer.h"
#include "kx/util.h"
#include "kx/debug.h"

#include <SDL2/SDL_image.h>
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL.h>
#include <map>
#include <memory>
#include <vector>
#include <cmath>
#include <utility>
#include <atomic>

namespace kx { namespace gfx {

/** This exists for two purposes:
 *  1) It makes creating windows faster
 *  2) There are bugs when using SDL2's OpenGL renderer that occur when a window is
 *  closed. By hiding a window instead of closing it, we prevent these bugs.
 *
 *  This never deletes windows, so if we end up creating and destroying lots of
 *  windows, we'll end up with a memory leak.
 *
 */


class WindowPool
{
    //window_flags to window
    std::map<Uint32, std::vector<shared_ptr_sdl<SDL_Window>>> windows;
public:
    shared_ptr_sdl<SDL_Window> get_window(const std::string &title,
                                          int x, int y, int w, int h,
                                          Uint32 window_flags)
    {
        //search the pool for a unused window with the right flags first
        for(auto &window: windows[window_flags]) {
            if(window.use_count() == 1) {
                SDL_SetWindowTitle(window.get(), title.c_str());
                SDL_SetWindowPosition(window.get(), x, y);
                SDL_SetWindowSize(window.get(), w, h);
                SDL_ShowWindow(window.get());
                return window;
            }
        }

        //if none exist, create one
        auto sdl_window = SDL_CreateWindow(title.c_str(), x, y, w, h, window_flags);
        auto window = shared_ptr_sdl<SDL_Window>(sdl_window);
        windows[window_flags].push_back(window);
        return window;
    }
};
static std::unique_ptr<WindowPool> window_pool;

bool AbstractWindow::InputQueue::poll(SDL_Event *input_arg)
{
    if(input_queue.empty())
        return false;
    *input_arg = std::move(input_queue.front());
    input_queue.pop();
    return true;
}
void AbstractWindow::InputQueue::push(SDL_Event input_arg)
{
    input_queue.push(input_arg);

    //Check if unprocessed input is taking up a lot of memory
    //(in which case we might want to clean it up)
    k_ensures(input_queue.size() != 1e4);
    k_ensures(input_queue.size() != 1e5);
    k_ensures(input_queue.size() != 1e6);
}
size_t AbstractWindow::InputQueue::size() const
{
    return input_queue.size();
}
void AbstractWindow::InputQueue::clear()
{
    input_queue = std::queue<SDL_Event>();
}
Renderer *AbstractWindow::rdr()
{
    return renderer_.get();
}
#ifdef KX_RENDERER_SDL2
static constexpr Uint32 RENDERER_FLAGS = SDL_RENDERER_PRESENTVSYNC |
                                         SDL_RENDERER_ACCELERATED;
#else
static constexpr Uint32 RENDERER_FLAGS = 0;
#endif
AbstractWindow::AbstractWindow(Library *library_,
                               const std::string &title,
                               int x, int y, int w, int h,
                               Uint32 window_flags):
    library(library_)
{
    #ifdef KX_RENDERER_GL
    window_flags |= SDL_WINDOW_OPENGL;
    #elif defined KX_RENDERER_VK
    window_flags |= SDL_WINDOW_VULKAN;
    #endif

    sdl_window = window_pool->get_window(title, x, y, w, h, window_flags);
    //the SDL_GetError() return here might be out of order because if renderer creation
    //also fails then SDL_GetError() might return the renderer error
    if(sdl_window == nullptr) {
        log_error((std::string)"window creation failed: " + SDL_GetError());
    } else {
        auto icon = library->get_default_window_icon({});
        if(icon != nullptr)
            SDL_SetWindowIcon(sdl_window.get(), icon.get());
    }

    renderer_ = std::make_unique<Renderer>(sdl_window.get(), RENDERER_FLAGS);
}
void AbstractWindow::close()
{
    //Sometimes, sdl_window is nullptr already. I'm not sure why, but my guess is that
    //SDL2 sometimes sends multiple quit signals to the same window.
    if(sdl_window == nullptr) {
        //log_warning("AbstractWindow::close() called on window that is already closed");
        return;
    }

    //Using SDL_HideWindow isn't very clean because it requires knowing about WindowPool's
    //internals. However, it's very simple. We can refactor it later if needed.
    SDL_HideWindow(sdl_window.get());

    sdl_window = nullptr;
    renderer_ = nullptr;

    //we could do more to try to free more memory, but usually the window will
    //go out of scope soon if close() is called
}
bool AbstractWindow::is_closed() const
{
    return sdl_window == nullptr;
}
int AbstractWindow::get_window_w() const
{
    k_expects(sdl_window != nullptr);
    int w;
    SDL_GetWindowSize(sdl_window.get(), &w, nullptr);
    return w;
}
int AbstractWindow::get_window_h() const
{
    k_expects(sdl_window != nullptr);
    int h;
    SDL_GetWindowSize(sdl_window.get(), nullptr, &h);
    return h;
}
int AbstractWindow::get_sdl_window_id() const
{
    k_expects(sdl_window != nullptr);
    int window_id = SDL_GetWindowID(sdl_window.get());
    if(window_id == 0)
        log_error("SDL_GetWindowID error: " + (std::string)SDL_GetError());
    return window_id;
}
bool AbstractWindow::is_focused() const
{
    return SDL_GetKeyboardFocus() == sdl_window.get();
}
void AbstractWindow::clean_memory()
{
    if(rdr() != nullptr)
        rdr()->clean_memory();
    else
        log_warning("renderer is nullptr in AbstractWindow.clean_memory()");
}
void AbstractWindow::RendererAtny::show_fps(AbstractWindow *window, bool toggle)
{
    k_expects(window->rdr() != nullptr);
    window->rdr()->show_fps(toggle);
}
int AbstractWindow::RendererAtny::set_fps_font(AbstractWindow *window, std::shared_ptr<const Font> font)
{
    k_expects(window->rdr() != nullptr);
    return window->rdr()->set_fps_font(std::move(font));
}
void AbstractWindow::RendererAtny::set_fps_color(AbstractWindow *window, SRGB_Color color)
{
    k_expects(window->rdr() != nullptr);
    window->rdr()->set_fps_color(color);
}
Library *AbstractWindow::get_library()
{
    return library;
}
AbstractWindow::InputQueue *AbstractWindow::get_input_queue(Passkey<Library>)
{
    return &input;
}

DWindow::DWindow(Library *library_,
                 const std::string &title,
                 int x, int y, int w, int h,
                 Uint32 window_flags):
    AbstractWindow(library_, title, x, y, w, h, window_flags)
{}

std::shared_ptr<DWindow> DWindow::make(Library *library,
                                       const std::string &title,
                                       int x, int y, int w, int h,
                                       Uint32 window_flags)
{
    //can't use make_shared due to non-public constructor
    std::shared_ptr<DWindow> window(new DWindow(library, title, x, y, w, h, window_flags));
    library->add_window_to_db(window);
    return window;
}
bool DWindow::poll_input(SDL_Event *input_arg)
{
    return input.poll(input_arg);
}

std::atomic<int> init_count(0);
Library::Library()
{
    //we can't really have more than one gfx library
    k_expects(init_count.fetch_add(1) == 0);

    if(SDL_Init(SDL_INIT_VIDEO) < 0) {
        log_error((std::string)"SDL_Init error: " + SDL_GetError());
    }
    if(IMG_Init(IMG_INIT_PNG | IMG_INIT_JPG) < 0) {
        log_error((std::string)"IMG_Init error: " + IMG_GetError());
    }

    #ifdef KX_RENDERER_GL
    SDL_GL_SetAttribute(SDL_GL_ACCELERATED_VISUAL, 1);
    SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, NUM_SAMPLES_DEFAULT);
    if(SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE) < 0)
        log_error(SDL_GetError());
    if(SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4) < 0)
        log_error(SDL_GetError());
    if(SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 5) < 0)
        log_error(SDL_GetError());
    #endif

    default_window_icon = IMG_Load("kx_data/default_window_icon.png");
    if(default_window_icon == nullptr) {
        log_error((std::string)"Failed to load default window icon: " + IMG_GetError());
    } else {
        SDL_SetColorKey(default_window_icon.get(), SDL_TRUE, SDL_MapRGB(default_window_icon->format, 255, 255, 255));
    }

    SDL_StartTextInput(); //may or may not be necessary

    Font::init({});

    window_pool = std::make_unique<WindowPool>();
}
Library::~Library()
{
    window_pool = nullptr;

    Font::quit({});

    default_window_icon = nullptr;

    for(auto &window: ID_to_window) {
        if(window.second.use_count() > 1) {
            log_warning("Exiting program, but window with ID " + to_str(window.first) + " still has " +
                        to_str(window.second.use_count()-1) + " remaining owners");
        }
    }
    ID_to_window.clear();

    global_events = std::queue<SDL_Event>();

    SDL_Quit();
    IMG_Quit();
}
const Uint8 *Library::get_keyboard_state() const
{
    return keyboard_state;
}
kx::gfx::mouse_state_t Library::get_mouse_state() const
{
    return mouse_state;
}
int Library::get_mouse_x() const
{
    return mouse_x;
}
int Library::get_mouse_y() const
{
    return mouse_y;
}
void Library::update_input()
{
    //update input
    SDL_Event input;
    while(SDL_PollEvent(&input)) {
        auto window = ID_to_window.find(input.window.windowID);
        //note: depending on the SDL2 version, it seems like if one window is open when SDL_QUIT is
        //received, SDL_WINDOWEVENT_CLOSE may or may not be received. We comment out a block of code
        //because in the current version of SDL (2.0.12+), it appears that SDL_WINDOWEVENT_CLOSE is
        //pushed to the window already, so we don't want to push it again (it could cause bugs if the
        //the window close is processed multiple times).
        if(window == ID_to_window.end()) { //this event isn't associated with a window
            //for simplicity, if SDL_QUIT is received, push SDL_WINDOWEVENT_CLOSE to the corresponding window
            if(input.type == SDL_QUIT) {
                int num_windows_open = 0;
                for(const auto &w: ID_to_window) {
                    auto win = w.second.lock();
                    if(win != nullptr) {
                        num_windows_open++;
                        SDL_Event close_window;
                        SDL_zero(close_window);
                        close_window.type = SDL_WINDOWEVENT;
                        close_window.window.timestamp = input.quit.timestamp;
                        close_window.window.windowID = w.first;
                        close_window.window.event = SDL_WINDOWEVENT_CLOSE;
                        win->get_input_queue({})->push(close_window);
                    }
                }
                k_expects(num_windows_open == 1); //afaik, only 1 window can be open if SDL_QUIT is received
            }

            global_events.push(input);

            if(global_events.size() == 1e5)
                log_warning("global_events.size()==1e5. Maybe clear it.");
        } else {
            auto window_shared_ptr = window->second.lock(); //this should always return non nullptr
                                                            //unless the window has just been closed
            if(window_shared_ptr != nullptr)
                window_shared_ptr->get_input_queue({})->push(input);
        }
    }
    keyboard_state = SDL_GetKeyboardState(nullptr);
    mouse_state = SDL_GetMouseState(&mouse_x, &mouse_y);
}
void Library::clean_memory()
{
    //clean up dead windows and text textures
    for(auto i = ID_to_window.begin(); i != ID_to_window.end(); ) {
        auto window = i->second.lock();
        if(window == nullptr) { //this window doesn't exist anymore, so delete it
            i = ID_to_window.erase(i);
        } else {
            window->clean_memory();
            i++;
        }
    }
}
shared_ptr_sdl<SDL_Surface> Library::get_default_window_icon(Passkey<AbstractWindow>)
{
    return default_window_icon;
}
void Library::add_window_to_db(std::shared_ptr<AbstractWindow> window)
{
    ID_to_window[window->get_sdl_window_id()] = std::move(window);
}
std::vector<int> Library::get_active_window_IDs() const
{
    std::vector<int> res;
    for(auto &window: ID_to_window)
        res.push_back(window.first);
    return res;
}

}}
