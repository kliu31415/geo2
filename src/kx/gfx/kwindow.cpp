#include "kx/gfx/kwindow.h"
#include "kx/gfx/renderer.h"
#include "kx/util.h"
#include "kx/io.h"
#include "kx/log.h"
#include "kx/debug.h"

#include <SDL2/SDL_events.h>
#include <vector>
#include <algorithm>
#include <functional>

namespace kx { namespace gfx {

/** This is a wrapper to make sure KItem::owner is managed properly
 *  The ctor notifies the KItem it has a new owner.
 *  The dtor notifies the KItem that it has no owner anymore.
 */
struct KWindow::KItemOwnerPtr
{
    const std::shared_ptr<KItem> obj;
    bool is_exclusive;
    KItemOwnerPtr(std::shared_ptr<KWindow> kwindow, std::shared_ptr<KItem> item):
        obj(std::move(item)),
        is_exclusive(false)
    {
        if(obj->get_owner() != nullptr) {
            log_warning("assigning a KItem that is already owned by a KWindow a new owning KWindow");
        }
        obj->set_owner(std::move(kwindow), {});
    }
    ~KItemOwnerPtr()
    {
        obj->set_owner(nullptr, {});
    }

    KItemOwnerPtr(const KItemOwnerPtr&) = delete;
    KItemOwnerPtr &operator = (const KItemOwnerPtr&) = delete;

    //delete move ctor for simplicity
    KItemOwnerPtr(KItemOwnerPtr&&) = delete;
    KItemOwnerPtr &operator = (KItemOwnerPtr&&) = delete;
};


KWindow::KWindow(const std::string &title, int x, int y, int w, int h,
                 Uint32 window_flags, SRGB_Color background_color_):

    AbstractWindow(title, x, y, w, h, window_flags),
    input_handler(default_input_handler),
    freeze_toggle(false),
    background_color(background_color_)
{}

std::shared_ptr<KWindow> KWindow::create(const std::string &title, int x, int y, int w, int h,
                                         Uint32 window_flags, SRGB_Color background_color)
{
    //use C-style cast to work around private inheritance
    auto new_kwin = (KWindow*)new KWindowRunning(title, x, y, w, h, window_flags, background_color);
    std::shared_ptr<KWindow> window(new_kwin); //can't use make_shared because KWindow has a non-public constructor
    add_window_to_db(window);
    return window;
}
bool KWindow::default_input_handler(KWindowRunning *kwindow, const SDL_Event &input)
{
    if(input.type==SDL_WINDOWEVENT && input.window.event==SDL_WINDOWEVENT_CLOSE) {
        kwindow->close();
        return true;
    }
    return false;
}
void KWindow::set_input_handler(const std::function<bool(KWindowRunning*, const SDL_Event&)> &new_input_handler)
{
    k_expects(new_input_handler != nullptr);
    input_handler = new_input_handler;
}
KWindow::Status KWindow::run()
{
    if(is_closed())
        return Status::AlreadyFinishedRunning;

    auto kwindow_running = (KWindowRunning*)this; //C-style cast to work around private inheritance

    //even if the window is frozen, still check for input using the input handler
    //(this also allows us to close the window if the user Xes it out)
    if(freeze_toggle) {
        SDL_Event input0;
        while(input.poll(&input0))
            input_handler(kwindow_running, input0);
        if(is_closed())
            return Status::JustFinishedRunning;
        return Status::Running;
    }

    input_deque.clear();
    SDL_Event input0;
    while(input.poll(&input0)) {
        //if the input handler doesn't consume the input, allow KItems to see it
        if(!input_handler(kwindow_running, input0))
            input_deque.push_back(std::move(input0));
    }
    if(is_closed()) //the window got deleted (presumably by the input handler)
        return Status::JustFinishedRunning;

    k_expects(rdr() != nullptr);
    rdr()->make_context_current();
    k_expects(rdr()->get_target() == nullptr); //sanity check to ensure nobody else has changed the target

    std::vector<std::shared_ptr<Texture>> item_textures;
    //each KItem processes input and renders to a buffer texture
    for(const auto &item: items) {
        item_textures.push_back(item->obj->run(kwindow_running)); //generate the texture for this KItem
        erase_remove(&input_deque, std::nullopt); //remove all null SDL_Event*s (these have been "consumed")
        //I think using nullopt works but might as well make sure
        for(const auto &i: input_deque)
            k_assert(i.has_value());
    }

    rdr()->set_target(nullptr);
    rdr()->set_color(background_color);
    rdr()->clear();

    //render all the KItem buffer textures to the screen in opposite order
    for(int i=(int)item_textures.size()-1; i>=0; i--) {
        auto render_rect = items[i]->obj->get_render_rect();
        rdr()->draw_texture(*item_textures[i], render_rect);
    }
    rdr()->refresh();
    return Status::Running;
}
bool KWindow::set_exclusive_item(const KItem *item)
{
    if(item == nullptr) {
        if(items.size() == 1)
            items[0]->is_exclusive = false;
        return true;
    }
    if(items.size() != 1)
        return false;
    if(items[0]->obj.get() != item)
        return false;

    items[0]->is_exclusive = true;

    return true;
}
bool KWindow::is_exclusive_item(const KItem *item) const
{
    return items.size()==1 && items[0]->obj.get()==item && items[0]->is_exclusive;
}
bool KWindow::add_item_front(const std::shared_ptr<KItem> &item)
{
    //if we have an exclusive KItem we can't add more items
    if(items.size()==1 && items[0]->is_exclusive)
        return false;

    //could probably use static_cast here but I'm not confident I know C++ well enough
    auto this_as_kwindow = std::dynamic_pointer_cast<KWindow>(shared_from_this());
    k_assert(this_as_kwindow != nullptr);

    items.push_back(std::make_unique<KItemOwnerPtr>(this_as_kwindow, item));

    return true;
}
bool KWindow::remove_item(const KItem *item)
{
    for(size_t i=0; i<items.size(); i++) {
        if(items[i]->obj.get() == item) {
            items.erase(items.begin() + i);
            return true;
        }
    }
    return false;
}
void KWindow::freeze(bool toggle)
{
    freeze_toggle = toggle;
}

template<class ...Args> KWindowRunning::KWindowRunning(Args &&...args):
    KWindow(std::forward<Args>(args)...) {}

KItem::KItem(Rect render_rect_):
    render_rect(render_rect_)
{}
Rect KItem::get_render_rect() const
{
    return render_rect;
}
void KItem::set_owner(std::shared_ptr<KWindow> new_owner, Passkey<KWindow>)
{
    owner_weak_ptr = std::move(new_owner);
}
std::shared_ptr<KWindow> KItem::get_owner() const
{
    return owner_weak_ptr.lock();
}

}}
