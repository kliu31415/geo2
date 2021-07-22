#pragma once

#include "kx/util.h"
#include "kx/sdl_deleter.h"
#include "kx/gfx/renderer_types.h"

#include <memory>
#include <string>
#include <queue>
#include <vector>
#include <map>

union SDL_Event;

namespace kx { namespace gfx {

/** gfx is NOT thread-safe. This is a largely limitation of SDL2. Any function here, including
 *  update_input() and clean_memory(), should ONLY be called from the main thread. Rendering
 *  functions should also ONLY be called on the main thread.
 */

class Renderer;
class Font;

using mouse_state_t = uint32_t;
using window_flags_t = uint32_t;
using keyboard_state_t = const uint8_t*;

//this is static asserted for correctness in the .cpp file
struct SDL_Event_Placeholder
{
    uint8_t reserve_space[56];
};

class GfxLibrary final
{
    keyboard_state_t keyboard_state;
    mouse_state_t mouse_state;
    int mouse_x;
    int mouse_y;
    ///events not associated with any SDL_Window (currently unused)
    std::queue<SDL_Event_Placeholder> global_events;

    using window_id_t = uint32_t;
    std::map<window_id_t, std::weak_ptr<class AbstractWindow> > ID_to_window;
    shared_ptr_sdl<SDL_Surface> default_window_icon;
    std::unique_ptr<class WindowPool> window_pool;
public:
    GfxLibrary();
    ~GfxLibrary();

    GfxLibrary(const GfxLibrary&) = delete;
    GfxLibrary& operator = (const GfxLibrary&) = delete;

    GfxLibrary(GfxLibrary&&) = delete;
    GfxLibrary& operator = (GfxLibrary&&) = delete;

    keyboard_state_t get_keyboard_state() const;
    mouse_state_t get_mouse_state() const;
    int get_mouse_x() const;
    int get_mouse_y() const;

    void update_input(); ///push input events to their corresponding queues
    void clean_memory(); ///removes old/unused stuff from memory. Ideally call this at least once per second.

    shared_ptr_sdl<SDL_Surface> get_default_window_icon(Passkey<AbstractWindow>);
    WindowPool *get_window_pool(Passkey<AbstractWindow>);
    void add_window_to_db(std::shared_ptr<class AbstractWindow> window);
    std::vector<int> get_active_window_IDs() const;
};

static constexpr window_flags_t DEFAULT_WINDOW_FLAGS = 0x00000004;
static constexpr window_flags_t WINDOW_POS_CENTERED = 0x2FFF0000u;

/** All windows created should shared_ptrs (otherwise stuff will bug out)
 *  non-copyable and non-movable
 */
class AbstractWindow: public std::enable_shared_from_this<AbstractWindow>
{
    friend void update_input();

    class InputQueue
    {
        std::queue<SDL_Event_Placeholder> input_queue;
    public:
        bool poll(SDL_Event *input_arg); ///returns true and fills input_arg iff there is more input
        void push(const SDL_Event &input_arg);
        size_t size() const;
        void clear();
    };

    shared_ptr_sdl<SDL_Window> sdl_window;
    std::unique_ptr<Renderer> renderer_;
    GfxLibrary *library;
protected:
    InputQueue input;

    Renderer *rdr();

    AbstractWindow(GfxLibrary *library,
                   std::string_view title,
                   int x, int y, int w, int h,
                   window_flags_t window_flags);
public:
    ///weak_ptrs to dead windows will be automatically removed in gfx::clean_memory, so =default is fine
    virtual ~AbstractWindow();

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
    InputQueue *get_input_queue(Passkey<GfxLibrary>);

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
    DWindow(GfxLibrary *library,
            std::string_view title,
            int x, int y, int w, int h,
            window_flags_t window_flags);
public:
    static std::shared_ptr<DWindow> make(GfxLibrary *library,
                                         std::string_view title,
                                         int x, int y, int w, int h,
                                         window_flags_t window_flags = DEFAULT_WINDOW_FLAGS);

    virtual ~DWindow() = default;

    DWindow(const DWindow&) = delete;
    DWindow &operator = (const DWindow&) = delete;

    DWindow(DWindow&&) = delete;
    DWindow &operator = (DWindow&&) = delete;

    using AbstractWindow::rdr;

    bool poll_input(SDL_Event *input_arg);
};

}}
