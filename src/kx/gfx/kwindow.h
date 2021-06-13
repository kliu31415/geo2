#pragma once

#include "kx/gfx/gfx.h"

#include <cstdint>
#include <deque>
#include <memory>
#include <functional>
#include <optional>

namespace kx { namespace gfx {

class KItem;
class KWindowRunning;

/** This is an automatically managed window. Add/remove KItems, which automatically
 *  render every frame when you call KWindow.run(). KItems have a defined order in which
 *  they appear on the window (you can control this by moving KItems closer to the front
 *  or the back), but the order in which they are run should be considered to be
 *  undefined, i.e. don't write code that assumes KItem_x.run() will be called before
 *  KItem_y.run().
 *
 *  All created KWindows are actually KWindowRunnings. The user only has access to a
 *  KWindow, which can't be rendered to. KWindow.run() internally casts itself to a
 *  KWindowRunning, which can be rendered to, and which it passes to its KItems. It does
 *  this via a C-style cast (which is a hack that circumvents private inheritance). The user
 *  should NOT attempt to convert a KWindow to a KWindowRunning, because the user shouldn't
 *  be directly drawing to a KWindow.
 */
class KWindow: public AbstractWindow
{
    /** KItemOwnerPtr is a small RAII wrapper that ensures that a KItem's owner field gets
     *  properly updated when it's added/removed from a KWindow.
     */
    struct KItemOwnerPtr;
    std::deque<std::unique_ptr<KItemOwnerPtr>> items;

    ///returns true iff the input is "consumed"
    std::function<bool(KWindowRunning*, const SDL_Event&)> input_handler;

    bool freeze_toggle;
    SRGB_Color background_color;
protected:
    std::deque<std::optional<SDL_Event>> input_deque;

    KWindow(const std::string &title,
            int x, int y, int w, int h,
            uint32_t window_flags,
            SRGB_Color background_color);
public:
    KWindow(const KWindow&) = delete;
    KWindow &operator = (const KWindow&) = delete;

    KWindow(KWindow&&) = delete;
    KWindow &operator = (KWindow&&) = delete;

    enum class Status{Running, AlreadyFinishedRunning, JustFinishedRunning};

    static std::shared_ptr<KWindow> create(const std::string &title, int x, int y, int w, int h,
                                           uint32_t window_flags = SDL_WINDOW_SHOWN,
                                           SRGB_Color background_color_ = Color::WHITE);

    static bool default_input_handler(KWindowRunning *kwindow, const SDL_Event &input);
    void set_input_handler(const std::function<bool(KWindowRunning*, const SDL_Event&)> &new_input_handler);

    Status run(); ///~KWindow is guaranteed not to be called during a run() call

    ///this always works (returns true) unless there's currently an exclusive item on the window
    bool add_item_front(const std::shared_ptr<KItem> &item);

    /** An exclusive item has knowledge that it is the sole item on a KWindow. However, the converse
      * is not true, i.e. if an item is the only item on a window, it is not necessarily exclusive.
      * This is useful because if a KItem knows it is exclusive, it may decide to use the whole window
      * for rendering and automatically resize itself whenever the window is resized.
      */
    bool set_exclusive_item(const KItem *item); ///returns true iff successful
    bool is_exclusive_item(const KItem *item) const;

    bool remove_item(const KItem *item); ///returns true on success (i.e. a KItem was removed)

    void freeze(bool toggle);
};

class KWindowRunning final: private KWindow
{
    friend KWindow;
    template<class ...Args> KWindowRunning(Args &&...args); ///forwards args to KWindow
public:
    KWindowRunning(const KWindowRunning&) = delete;
    KWindowRunning &operator = (const KWindowRunning&) = delete;

    KWindowRunning(KWindowRunning&&) = delete;
    KWindowRunning &operator = (KWindowRunning&&) = delete;

    using AbstractWindow::rdr;

    using AbstractWindow::get_window_w;
    using AbstractWindow::get_window_h;
    using AbstractWindow::close;
    using AbstractWindow::is_closed;
    using AbstractWindow::is_focused;

    using KWindow::input_deque;
};

class KItem ///anything that renders onto a KWindow (e.g. buttons, graphs, text boxes)
{
    std::weak_ptr<KWindow> owner_weak_ptr; //use weak_ptr prevent pointer cycles
protected:
    Rect render_rect;
    KItem(Rect render_rect_);
public:
    virtual ~KItem() = default;

    /** I suppose you could copy a KItem without causing errors, but it feels
      * weird and it's generally something that you wouldn't want to do, so I'm
      * deleting the copy operations for now.
      */
    KItem(const KItem&) = delete;
    KItem &operator = (const KItem&) = delete;

    ///You could probably move a KItem without errors, but ban it for simplicity
    KItem(KItem&&) = delete;
    KItem &operator = (KItem&&) = delete;

    /** Preconditions: kwin_r->input_deque must not contain any nullptrs
     */
    virtual std::shared_ptr<class Texture> run(KWindowRunning *kwin_r) = 0;

    Rect get_render_rect() const;

    void set_owner(std::shared_ptr<KWindow> new_owner, Passkey<KWindow>);
    std::shared_ptr<KWindow> get_owner() const;
};

}}
