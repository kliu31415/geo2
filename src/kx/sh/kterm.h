#pragma once

#include "kx/gfx/kwindow.h"
#include "kx/gfx/renderer_types.h"
#include "kx/time.h"

#include <memory>
#include <string>
#include <atomic>
#include <mutex>
#include <deque>

namespace kx { namespace gfx {struct MonofontAtlas;}}

namespace kx { namespace sh {

class KShell;
class KShellProgram;

class KTerm final: public gfx::KItem, public std::enable_shared_from_this<KTerm>
{
    std::shared_ptr<KShell> shell;
    std::unique_ptr<std::deque<class ColorfulString>> history; ///pImpl
    size_t cur_history_char_pos;
    double font_size;
    std::unique_ptr<gfx::MonofontAtlas> monofont_atlas;
    double scroll_bar_w;
    int scroll_line;
    bool text_added;
    std::atomic<bool> should_clear_history;
    Time last_keydown_time;
    gfx::SRGB_Color output_text_color;
    int num_lines_drawn;

    std::shared_ptr<gfx::Texture> render(gfx::KWindowRunning *kwin_r);
    void process_input(gfx::KWindowRunning *kwin_r);
public:
    KTerm(const gfx::Rect &render_rect_);
    ~KTerm() override;

    ///noncopyable and nonmovable because KShell keeps a pointer to KTerm
    KTerm(const KTerm&) = delete;
    KTerm &operator = (const KTerm&) = delete;
    KTerm(KTerm&&) = delete;
    KTerm &operator = (KTerm&&) = delete;

    void set_shell(std::shared_ptr<KShell> new_shell); ///creates an owning pointer to new_shell

    ///currently set_output_text_color has very limited uses because KShellProgram has no way to call it
    void set_output_text_color(const gfx::SRGB_Color &color, Passkey<KTerm, KShell>); ///not thread-safe
    void add_text(const std::string &text, Passkey<KShell, KTerm>); ///not thread-safe
    void add_command_text(const std::string &text, Passkey<KShell>); ///not thread-safe

    void clear_history(); ///thread-safe
    std::shared_ptr<gfx::Texture> run(gfx::KWindowRunning *kwin_r) override;
};

}}
