#include "kx/sh/kterm.h"
#include "kx/sh/kshell.h"
#include "kx/sh/kterm_input_comm.h"
#include "kx/gfx/renderer.h"

#include <cstring>
#include <climits>

namespace kx { namespace sh {

struct ColorfulChar
{
    char chr;
    gfx::SRGB_Color color;

    ColorfulChar() = default;
    ColorfulChar(char chr_, gfx::SRGB_Color color_):
        chr(chr_), color(color_)
    {}
};

class ColorfulString
{
    std::basic_string<ColorfulChar> str;
public:
    ColorfulString() = default;
    ColorfulString(const std::basic_string<ColorfulChar> &str_):
        str(str_)
    {}
    ColorfulString(const std::string &str_, gfx::SRGB_Color color)
    {
        str.resize(str_.size());
        for(size_t i=0; i<str_.size(); i++)
            str[i] = ColorfulChar(str_[i], color);
    }

    void clear()
    {
        str.clear();
    }
    bool empty()
    {
        return str.empty();
    }
    size_t size()
    {
        return str.size();
    }
    void pop_back()
    {
        str.pop_back();
    }

    ColorfulString operator + (const ColorfulString &other)
    {
        return ColorfulString(str + other.str);
    }
    void operator += (const ColorfulString &other)
    {
        str += other.str;
    }
    void operator += (const ColorfulChar &chr)
    {
        str += chr;
    }
    ColorfulChar &operator [] (size_t idx)
    {
        return str[idx];
    }
};

static const std::string SH_NAME_SUFFIX = "$ ";
static const gfx::SRGB_Color DEFAULT_TEXT_COLOR = gfx::Color::WHITE;
static const gfx::SRGB_Color SHELL_NAME_TEXT_COLOR = gfx::Color::GREEN;
static const gfx::SRGB_Color SH_NAME_SUFFIX_TEXT_COLOR = gfx::Color::MAGENTA;

std::shared_ptr<gfx::Texture> KTerm::render(gfx::KWindowRunning *kwin_r)
{
    auto return_texture = kwin_r->rdr()->make_texture_target(render_rect.w, render_rect.h);
    kwin_r->rdr()->set_target(return_texture.get());

    kwin_r->rdr()->set_color(gfx::Color::BLACK);
    kwin_r->rdr()->clear();

    if(monofont_atlas==nullptr || monofont_atlas->font_size!=(int)font_size) {
        monofont_atlas = kwin_r->rdr()->create_monofont_atlas(
            *gfx::Font::ROBOTO_MONO_REGULAR, font_size, gfx::Color::WHITE);
    }

    ColorfulString history_and_input;

    for(const auto &line: *history) {
        history_and_input += line;
        history_and_input += ColorfulChar('\n', DEFAULT_TEXT_COLOR);
    }
    history_and_input.pop_back(); //remove the last newline

    size_t display_input_str_position = history_and_input.size() + shell->get_cur_input_str_pos({});
    if(!shell->is_program_running()) {
        history_and_input += ColorfulString(shell->name(), SHELL_NAME_TEXT_COLOR);
        history_and_input += ColorfulString(SH_NAME_SUFFIX, SH_NAME_SUFFIX_TEXT_COLOR);
        display_input_str_position += shell->name().size() + SH_NAME_SUFFIX.size();
    }
    history_and_input += ColorfulString(shell->get_cur_input_str({}), gfx::Color::WHITE);

    float char_w = (float)monofont_atlas->char_w;
    float char_h = (float)monofont_atlas->char_h;

    if(char_h >= INT_MAX*0.09)
        log_warning("char_h is very big (>=INT_MAX*0.09). "
                    "This could cause overflow even if few lines are drawn");

    num_lines_drawn = 0;
    int cur_x = 0;
    int cur_y = -scroll_line * char_h;
    int max_x = render_rect.w - (int)scroll_bar_w;

    bool time_mod_condition = (Time::now() - last_keydown_time).to_int64(Time::Length::millisecond)%1200 < 600;

    for(size_t i=0; i<=history_and_input.size(); i++) {
        bool drew_char = false;
        if(i < history_and_input.size()) {
            char character = history_and_input[i].chr;
            if(character == '\n') {
                cur_x = 0;
                cur_y += char_h;
                num_lines_drawn++;
                if(cur_y >= INT_MAX*0.9)
                    log_warning("cur_y >= INT_MAX*0.9. Overflow could happen if this increases more.");
            } else {
                bool char_in_range = true;
                char_in_range &= (character >= gfx::MonofontAtlas::MIN_CHAR);
                #pragma GCC diagnostic ignored "-Wtype-limits" //if we change MAX_CHAR we'll want the check
                char_in_range &= (character <= gfx::MonofontAtlas::MAX_CHAR);
                if(cur_y > -char_h && cur_y < render_rect.h && char_in_range)
                {
                    gfx::Rect dst{(float)cur_x, (float)cur_y, char_w, char_h};
                    auto &texture = monofont_atlas->characters[(int)character];
                    float mod_r = history_and_input[i].color.r;
                    float mod_g = history_and_input[i].color.g;
                    float mod_b = history_and_input[i].color.b;
                    texture->set_color_mod(mod_r, mod_g, mod_b);
                    kwin_r->rdr()->draw_texture(*texture, dst);
                }

                drew_char = true;
            }
        }

        //show cursor
        if(display_input_str_position==i && time_mod_condition) {
            kwin_r->rdr()->set_color(gfx::SRGB_Color(1.0f, 1.0f, 1.0f, 0.4f));
            kwin_r->rdr()->invert_rect_colors(gfx::Rect(cur_x, cur_y, char_w, char_h));
        }

        if(drew_char) {
            cur_x += char_w;
            if(cur_x + char_w > max_x) {
                cur_x = 0;
                cur_y += char_h;
                num_lines_drawn++;
                if(cur_y >= INT_MAX*0.9)
                    log_warning("cur_y >= INT_MAX*0.9. Overflow could happen if this increases more.");
            }
        }
    }
    if(cur_x != 0) //count the last line if characters were drawn
        num_lines_drawn++;

    double lines_per_screen = render_rect.h / (double)char_h;
    int max_scroll_y = num_lines_drawn-1 + std::max(2*(int)lines_per_screen + 1, 20);
    scroll_line = std::min(scroll_line, max_scroll_y); //forbid scrolling too far down

    //fill the rect that hosts the scroll bar (this will be likely be bigger than the scroll bar itself)
    float scroll_bar_x = max_x;
    kwin_r->rdr()->set_color(gfx::Color::LIGHT_GRAY);
    kwin_r->rdr()->fill_rect(gfx::Rect(scroll_bar_x, 0, scroll_bar_w, render_rect.h));
    kwin_r->rdr()->set_color(gfx::Color::MEDIUM_GRAY);

    //use 1 + actual_height to avoid aliasing (otherwise the scroll bar might be unable to cover
    //the bottom pixel of the screen)
    float scroll_bar_h = 1 + render_rect.h * lines_per_screen / (max_scroll_y + lines_per_screen);
    auto scroll_bar_y = scroll_line / (double)(max_scroll_y + lines_per_screen) * render_rect.h;
    kwin_r->rdr()->fill_rect(gfx::Rect(max_x, scroll_bar_y, scroll_bar_w, scroll_bar_h + 1));

    //if text was added and there's new text past the bottom, automatically scroll down.
    //Note that this scrolls down 1 frame late, but it shouldn't matter that much.
    if(text_added) {
        text_added = false;
        if(cur_y + char_h >= render_rect.h) {
            scroll_line = (num_lines_drawn+1) - render_rect.h/(double)char_h;
        }
    }
    return return_texture;
}
void KTerm::process_input(gfx::KWindowRunning *kwin_r)
{
    static constexpr double MIN_FONT_SIZE = 2.0;
    static constexpr double MAX_FONT_SIZE = 200.0;
    static constexpr int MIN_SCROLL_LINE = 0;
    static constexpr int MAX_SCROLL_LINE = (1<<22);

    static_assert(MIN_FONT_SIZE >= 1.0);
    static_assert(MAX_FONT_SIZE <= 1000.0);
    static_assert(MIN_SCROLL_LINE == 0);
    static_assert(MAX_SCROLL_LINE < (1<<23)); //1<<23 is around the max int a float can represent

    KTermInputComm comm({});

    comm.terminal = this;
    comm.keymod = SDL_GetModState();

    comm.font_size = &font_size;
    comm.min_font_size = MIN_FONT_SIZE;
    comm.max_font_size = MAX_FONT_SIZE;

    comm.last_keydown_time = &last_keydown_time;

    comm.scroll_line = &scroll_line;
    comm.min_scroll_line = MIN_SCROLL_LINE;
    comm.max_scroll_line = MAX_SCROLL_LINE;

    comm.num_lines_drawn = num_lines_drawn; //on the first frame this is set to 1 (which should
                                            //be correct)

    auto owner = get_owner();
    k_expects(owner != nullptr); //how does this happen? We shouldn't be able to run a ownerless KItem
    for(auto &inp: kwin_r->input_deque) {
        switch(inp->type)
        {
        case SDL_WINDOWEVENT:
            switch(inp->window.event)
            {
            case SDL_WINDOWEVENT_RESIZED:
                if(owner->is_exclusive_item(this)) {
                    render_rect.w = inp->window.data1;
                    render_rect.h = inp->window.data2;
                }
                break;
            default:
                break;
            }
            inp.reset();
            break;
        case SDL_TEXTINPUT:
        case SDL_KEYDOWN:
        case SDL_KEYUP:
        case SDL_MOUSEWHEEL:
        case SDL_MOUSEBUTTONDOWN:
        case SDL_MOUSEBUTTONUP:
            comm.input = &*inp;
            shell->process_input(comm);
            inp.reset();
            break;
        default:
            break;
        }
    }
}

KTerm::KTerm(const gfx::Rect &render_rect_):
    KItem(render_rect_),
    history(std::make_unique<std::deque<ColorfulString>>(1)),
    cur_history_char_pos(0),
    font_size(16),
    scroll_bar_w(17),
    scroll_line(0),
    text_added(false),
    should_clear_history(false),
    last_keydown_time(Time::now()),
    output_text_color(DEFAULT_TEXT_COLOR),
    num_lines_drawn(1)
{}

KTerm::~KTerm()
{
    set_shell(nullptr);
}

void KTerm::set_shell(std::shared_ptr<KShell> new_shell)
{
    if(shell != nullptr)
        shell->set_owning_terminal(nullptr, {});
    shell = std::move(new_shell);
    if(shell != nullptr)
        shell->set_owning_terminal(shared_from_this(), {});
}

void KTerm::set_output_text_color(const gfx::SRGB_Color &color, Passkey<KTerm, KShell>)
{
    //I don't think I need a mutex for this
    output_text_color = color;
}
void KTerm::add_text(const std::string &text, Passkey<KShell, KTerm>)
{
    for(const auto &chr: text) {
        if(chr == '\n') {
            cur_history_char_pos = 0;
            history->emplace_back();
        } else if(chr == '\r') {
            cur_history_char_pos = 0;
        } else {
            if(cur_history_char_pos == history->back().size())
                history->back() += ColorfulChar(chr, output_text_color);
            else
                history->back()[cur_history_char_pos] = ColorfulChar(chr, output_text_color);
            cur_history_char_pos++;
        }
    }
    text_added = true;
}
void KTerm::add_command_text(const std::string &text, Passkey<KShell>)
{
    //this resets the text color
    set_output_text_color(SHELL_NAME_TEXT_COLOR, {});
    add_text(shell->name(), {});
    set_output_text_color(SH_NAME_SUFFIX_TEXT_COLOR, {});
    add_text(SH_NAME_SUFFIX, {});
    set_output_text_color(DEFAULT_TEXT_COLOR, {});
    add_text(text, {});
}
void KTerm::clear_history()
{
    //scroll_line isn't atomic, so don't modify history/scroll_line here
    should_clear_history.store(true, std::memory_order_relaxed);
}
std::shared_ptr<gfx::Texture> KTerm::run(gfx::KWindowRunning *kwin_r)
{
    process_input(kwin_r);

    if(shell != nullptr)
        shell->run({});

    if(should_clear_history.load(std::memory_order_relaxed)) {
        should_clear_history.store(false, std::memory_order_relaxed);
        scroll_line = 0;
        history->clear();
        history->resize(1);
    }

    //monofont_atlas should be non nullptr for all but the first time run() is called. It
    //should be OK to ignore the first time because the history should be empty anyway.
    //TODO: very big lines of history still hog tons of memory and CPU. Having 10 lines, each with
    //1e6 characters is bad, is bad. How do we deal with this?
    static constexpr size_t MAX_HISTORY_LINES = 300;
    if(monofont_atlas != nullptr) {
        if(history->size() > MAX_HISTORY_LINES) {
            k_expects(monofont_atlas->char_w > 0);
            while(history->size() > MAX_HISTORY_LINES) {
                scroll_line -= 1 + history->front().size() / (int)(render_rect.w / monofont_atlas->char_w);
                history->pop_front();
            }
        }
    }
    //note: render(...) actually does a few calculations that we need. It doesn't just render.
    //In particular, those calculations may be used by process_input(...) on the next iteration.
    auto render_result = render(kwin_r);

    return render_result;

}

}}
