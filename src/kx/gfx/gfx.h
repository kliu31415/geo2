#pragma once

#include "kx/util.h"
#include "kx/sdl_deleter.h"
#include "kx/gfx/renderer_types.h"

#include <SDL2/SDL_events.h>
#include <memory>
#include <string>
#include <queue>
#include <vector>

namespace kx { namespace gfx {

/** gfx is NOT thread-safe. This is a largely limitation of SDL2. Any function here, including
 *  update_input() and clean_memory(), should ONLY be called from the main thread. Rendering
 *  functions should also ONLY be called on the main thread.
 */

class Renderer;
class Font;

/** All windows created should shared_ptrs (otherwise stuff will bug out)
 *  non-copyable and non-movable
 */
class AbstractWindow: public std::enable_shared_from_this<AbstractWindow>
{
    friend void update_input();

    class InputQueue
    {
        std::queue<SDL_Event> input_queue;
    public:
        bool poll(SDL_Event *input_arg); ///returns true and fills input_arg iff there is more input
        void push(SDL_Event input_arg);
        size_t size() const;
        void clear();
    };

    shared_ptr_sdl<SDL_Window> sdl_window;
    std::unique_ptr<Renderer> renderer_;
protected:
    static void add_window_to_db(std::shared_ptr<AbstractWindow> window);

    InputQueue input;

    Renderer *rdr();

    AbstractWindow(const std::string &title, int x, int y, int w, int h, Uint32 window_flags);
public:
    ///weak_ptrs to dead windows will be automatically removed in gfx::clean_memory, so =default is fine
    virtual ~AbstractWindow() = default;

    ///noncopyable because you can't copy a window
    AbstractWindow(const AbstractWindow&) = delete;
    AbstractWindow &operator = (const AbstractWindow&) = delete;

    ///nonmovable because an internal db holds a pointer to it
    AbstractWindow(AbstractWindow&&) = delete;
    AbstractWindow &operator = (AbstractWindow&&) = delete;

    void close();
    bool is_closed() const;
    int get_window_w() const;
    int get_window_h() const;
    int get_sdl_window_id() const;
    bool is_focused() const;
    void clean_memory();

    friend class RendererAtny;
    struct RendererAtny ///Attorney-Client idiom
    {
        static void show_fps(AbstractWindow *window, bool toggle);
        static int set_fps_font(AbstractWindow *window, std::shared_ptr<const Font> font);
        static void set_fps_color(AbstractWindow *window, SRGB_Color color);
    };
};

/** D = "Directly Drawable". This is a basic window class. It is destroyed as soon as it goes out of scope.
 *  Every frame, the user should draw to it, process input, and call refresh().
 */
class DWindow final: public AbstractWindow
{
    DWindow(const std::string &title, int x, int y, int w, int h, Uint32 window_flags);
public:
    static std::shared_ptr<DWindow> create(const std::string &title, int x, int y, int w, int h,
                                           Uint32 window_flags = SDL_WINDOW_SHOWN);

    virtual ~DWindow() = default;

    DWindow(const DWindow&) = delete;
    DWindow &operator = (const DWindow&) = delete;

    DWindow(DWindow&&) = delete;
    DWindow &operator = (DWindow&&) = delete;

    using AbstractWindow::rdr;

    bool poll_input(SDL_Event *input_arg);
};

const Uint8 *get_keyboard_state();
Uint32 get_mouse_state();
int get_mouse_x();
int get_mouse_y();

void update_input(); ///push input events to their corresponding queues
void clean_memory(); ///removes old/unused stuff from memory. Ideally call this at least once per second.

std::vector<int> get_active_window_IDs();

int init(); ///call at beginning of program on the main thread

/** call "quit()" at the end of the program on the main thread.
 *  crashes might happen on program close if we don't free up everything before calling quit()
 *  (because then the program attempts to call dtors on our global SDL2 objects after SDL_Quit())
 */
void quit(); ///call at end of program

}}
