#include "kx/sh/kshell.h"
#include "kx/sh/kterm_input_comm.h"
#include "kx/sh/kterm.h"
#include "kx/sh/program_db.h"
#include "kx/io.h"

#include <SDL2/SDL_clipboard.h>
#include <SDL2/SDL_version.h>
#include <cstdlib>
#include <cstdint>
#include <cstring>
#include <sstream>
#include <climits>
#include <cmath>
#include <map>
#include <mutex>

namespace kx { namespace sh {

namespace programs {

class history final: public KShellProgram
{
public:
    history(std::shared_ptr<KShell> shell_):
        KShellProgram(std::move(shell_))
    {}
    std::string manual_str() const override
    {
        return "Returns all commands that have been input\n";
    }
    void run(std::vector<std::string> args) override
    {
        if(args.size() > 1) {
            println("error: program takes at most 1 argument");
        } else if(args.size() == 1) {
            if(args[0]=="-c" || args[0]=="--clear") {
                //no need for a mutex because shell won't reading/writing
                //command_history while a child program is active
                shell->clear_command_history({});
                return;
            } else
                println("unrecognized argument: " + args[0]);
        } else {
            std::string output;
            auto command_history = shell->get_command_history({});
            //no need for a mutex because shell won't be reading/writing
            //command_history while a child program is active
            for(size_t i=0; i < command_history.size(); i++)
                output += to_str(i) + " " + command_history[i] + "\n";
            print(output);
        }
    }
};

class addertest final: public KShellProgram
{
public:
    addertest(std::shared_ptr<KShell> shell_):
        KShellProgram(std::move(shell_))
    {}
    std::string manual_str() const override
    {
        return "Returns the signed int32 result of adding two signed int32s\n";
    }
    void run(std::vector<std::string> args) override
    {
        if(args.size() > 0) {
            println("error: program does not take arguments");
            return;
        }
        print("A = ");
        int a = std::atoi(read_line().c_str());
        print("B = ");
        int b = std::atoi(read_line().c_str());
        println(a+b);
    }
};

class version final: public KShellProgram
{
public:
    version(std::shared_ptr<KShell> shell_):
        KShellProgram(std::move(shell_))
    {}
    std::string version_str(int major, int minor, int patch)
    {
        return to_str(major) + "." + to_str(minor) + "." + to_str(patch);
    }
    std::string manual_str() const override
    {
        return "Outputs version information about this program\n";
    }
    void run(std::vector<std::string> args) override
    {
        if(args.size() > 0) {
            println("error: program does not take arguments");
            return;
        }
        //Compiler info
        std::string compiler_info = "Compiler: ";
        #ifdef __MINGW32__
        compiler_info += "MinGW " + version_str(__GNUC__, __GNUC_MINOR__, __GNUC_PATCHLEVEL__) + " ";
        #elif defined(__GNUC__)
        compiler_info += "GCC " + version_str(__GNUC__, __GNUC_MINOR__, __GNUC_PATCHLEVEL__) + " ";
        #elif defined(_MSC_VER)
        compiler_info += "MSVC ";
        #elif defined(__clang__)
        compiler_info += "clang ";
        #else
        compiler_info += "Unknown ";
        #endif
        println(compiler_info);

        //Build date/time
        std::string build_time = __DATE__ + to_str(" ") + to_str(__TIME__);
        if(build_time[4] == ' ')
            build_time[4] = '0';
        println("Build time: " + build_time);

        //C++ version
        println("C++ version: " + to_str(__cplusplus));

        //OS info
        std::string os_info = "OS: ";
        #if defined(_WIN32) || defined(_WIN64)
        os_info += "Windows";
        #elif __linux__
        os_info += "Linux";
        #elif defined(__APPLE__) || defined(__MACH__)
        os_info += "Mac OS X";
        #else
        os_info += "Unknown";
        #endif

        os_info += " ";

        #if INTPTR_MAX==INT32_MAX
        os_info += "32 bit";
        #elif INTPTR_MAX==INT64_MAX
        os_info += "64 bit";
        #else
        os_info += "?? bit";
        #endif
        println(os_info);

        //SDL2 version
        SDL_version sdl_c, sdl_l;
        SDL_VERSION(&sdl_c);
        SDL_GetVersion(&sdl_l);
        println("SDL compiled version: " + version_str(sdl_c.major, sdl_c.minor, sdl_c.patch));
        println("SDL linked version: " + version_str(sdl_l.major, sdl_l.minor, sdl_l.patch));
    }
};

class clear final: public KShellProgram
{
public:
    clear(std::shared_ptr<KShell> shell_):
        KShellProgram(std::move(shell_))
    {}
    std::string manual_str() const override
    {
        return "Clears the terminal\n";
    }
    void run(std::vector<std::string> args) override
    {
        if(args.size() > 0) {
            println("error: program does not take arguments");
            return;
        }
        shell->clear_terminal_history({});
    }
};

class man final: public KShellProgram
{
public:
    man(std::shared_ptr<KShell> shell_):
        KShellProgram(std::move(shell_))
    {}
    std::string manual_str() const override
    {
        return "usage:\n-man: prints out the list of available programs\n-man \"program\":"
               "prints out the manual for a KShellProgram\n";
    }
    void run(std::vector<std::string> args) override
    {
        if(args.size() == 0) {
            println("--Available programs--");
            for(const auto &program_name: shell->get_program_db({})->get_program_list())
                println(program_name);
        } else if(args.size() == 1) {
            auto program = shell->get_program_db({})->construct_program(shell, args[0]);
            if(program != nullptr) {
                print(program->manual_str());
            } else
                println("\"" + args[0] + "\": program not found");
        } else
            print("error: program takes at most 1 argument\n");
    }
};

class bigprint1by1 final: public KShellProgram
{
public:
    bigprint1by1(std::shared_ptr<KShell> shell_):
        KShellProgram(std::move(shell_))
    {}
    std::string manual_str() const override
    {
        return "Prints a lot of uppercase letters (1e4 by default, but the amount"
               "can be customized by passing in an argument) one by one\n";
    }
    void run(std::vector<std::string> args) override
    {
        if(args.size() > 1) {
            println("error: program does not take arguments");
            return;
        }
        size_t target_length;
        if(args.size() == 0) {
            target_length = 1e4;
        } else {
            std::stringstream ss(args[0]);
            ss >> target_length;
        }
        if(target_length > 1e7) {
            println("error: printing " + to_str(target_length) +
                    " letters is too extreme (max 1e7). Skipping");
            return;
        }
        for(size_t i=0; i<target_length; i++)
            print((char)((i%16) + 'A'));
        println("");
    }
};

class bigprintbatch final: public KShellProgram
{
public:
    bigprintbatch(std::shared_ptr<KShell> shell_):
        KShellProgram(std::move(shell_))
    {}
    std::string manual_str() const override
    {
        return "Prints a lot of uppercase letters (1e5 by default, but the amount"
               "can be customized by passing in an argument) in one batch\n";
    }
    void run(std::vector<std::string> args) override
    {
        if(args.size() > 1) {
            println("error: program does not take arguments");
            return;
        }
        size_t target_length;
        if(args.size() == 0) {
            target_length = 1e5;
        } else {
            std::stringstream ss(args[0]);
            ss >> target_length;
        }
        if(target_length > 1e7) {
            println("error: printing " + to_str(target_length) +
                    " letters is too extreme (max 1e7). Skipping");
            return;
        }
        std::string to_print;
        for(size_t i=0; i<target_length; i++)
            print((char)((i%16) + 'A'));
        println(to_print);
    }
};

class time final: public KShellProgram {
public:
    time(std::shared_ptr<KShell> shell_):
        KShellProgram(std::move(shell_))
    {}
    std::string manual_str() const override
    {
        return "Returns the wall time taken for a program to run\nUsage: time program args";
    }
    void run(std::vector<std::string> args) override
    {
        if(args.size() == 0) {
            println("this command needs at least 1 argument");
            return;
        }

        auto start_time = Time::now();

        std::string program_name = std::move(args[0]);
        args.erase(args.begin());
        auto program = shell->get_program_db({})->construct_program(shell, program_name);
        if(program != nullptr) {
            program->run(std::move(args));
        } else
            println("\"" + program_name + "\": program not found\n");

        auto elapsed = (Time::now() - start_time).to_int64(Time::Length::millisecond);
        println("\nTook " + to_str(elapsed) + "ms");
    }
};

class date final: public KShellProgram
{
public:
    date(std::shared_ptr<KShell> shell_):
        KShellProgram(std::move(shell_))
    {}
    std::string manual_str() const override
    {
        return "Returns the date\nUsage: date";
    }
    void run(std::vector<std::string> args) override
    {
        if(args.size() > 0) {
            println("this command takes no arguments");
            return;
        }
        println(Time::now().to_str(Time::Format::YYYY_mm_DD_HH_MM_SS));
    }
};

class test_carriage_return final: public KShellProgram
{
    public:
    test_carriage_return(std::shared_ptr<KShell> shell_):
        KShellProgram(std::move(shell_))
    {}
    std::string manual_str() const override
    {
        return "Tests the functionality of carriage returns";
    }
    void run(std::vector<std::string> args) override
    {
        if(args.size() > 0) {
            println("this command takes no arguments");
            return;
        }
        static constexpr int n = 2000;
        for(int i=0; i<=n; i++) {
            print("\r" + to_str(i) + "/" + to_str(n));
            sleep_for(Time::Delta(10, Time::Length::ms));
        }
        println("");
    }
};

}

std::vector<std::string> KShell::parse_args(const std::string &str)
{
    std::vector<std::string> parts;
    bool in_quotes = false;
    for(size_t i=0, j=0; i<=str.size(); i++) {
        if(i<str.size() && str[i]=='\"') {
            in_quotes = !in_quotes;
            if(i != j)
                parts.push_back(str.substr(j, i-j));
            j = i+1;
        } else if(i==str.size() || (!in_quotes && str[i]==' ')) {
            if(i != j)
                parts.push_back(str.substr(j, i-j));
            j = i+1;
        }
    }
    return parts;
}

class KShellTerminateRequestException final: virtual public std::exception //virtual just in case
{
public:
    KShellTerminateRequestException(Passkey<KShell>) {}

    [[maybe_unused]] const char *what() const throw() override
    {
        return "KShell requested for KShellProgram to be terminated";
    }
};

void KShell::run()
{
    auto terminal = terminal_weak_ptr.lock();
    k_expects(terminal != nullptr);

    //check if the program is finished
    if(is_program_running() &&
       shell_program_return.wait_for(std::chrono::seconds(0))==std::future_status::ready)
    {
        shell_program_return.get();
        output_q.push("\n");
    }

    //process output first to avoid weird graphical bugs when a set of fast commands
    //is copy pasted (e.g. "history\nhistory\nhistory\n" results in all outputs being
    //shifted one down relative to the "ShellName$" part because the first "history"
    //program would be executed, the output queue would be polled and appear empty,
    //and then in the next loop "history" would have finished, so the second "history"
    //would be executed, and THEN the output of the first "history" would appear.
    std::string output;
    while(output_q.poll_if_nonempty(&output))
        terminal->add_text(std::move(output), {});

    //if a program is running the shell doesn't want to compete with it for input
    if(!is_program_running()) {
        std::string input;
        //keep polling input until we find a valid command (in which case we break) or
        //the input queue is empty
        while(input_q.poll_if_nonempty(&input)) {
            terminal->add_command_text(input + "\n", {});
            command_history.push_back(input);
            command_history_selected_idx = command_history.size();

            if(!input.empty()) {
                if(input[0]=='#') { //'#' as a first character indicates a comment...
                    if(input.size()>1 && input[1]=='!') { //except if '#' is followed by '!'
                        std::string expected = input.substr(2);
                        std::string got = name();
                        if(expected != got) {
                            std::string msg = "this script is intended for \"" + expected + "\", "
                                              "but we're using \"" + got + "\". There may be "
                                              "(possibly silent) compatibility issues.\n";
                            log_warning(msg);
                            output_q.push(msg);
                            break; //this is effective a mini-program because it prints stuff,
                                   //so break like an actual program to not reorder terminal output
                        }
                    }
                    continue;
                }
                auto args = parse_args(input);
                std::string program_name = std::move(args[0]);
                args.erase(args.begin());
                auto program = program_db->construct_program(shared_from_this(), program_name);
                if(program != nullptr) {
                    if(!program->run_on_separate_thread()) {
                        program->run(std::move(args));
                        output_q.push("\n");
                    } else {
                        //"program" and "args" are stack variables, so we move them in async and
                        //wait until we receive confirmation that they've been moved
                        std::atomic<bool> finished_moving(false);
                        shell_program_return = std::async(std::launch::async,
                            [&]
                            {
                                auto args_ = std::move(args);
                                auto program_ = std::move(program);
                                finished_moving.store(true, std::memory_order_release);
                                program_->run(std::move(args_));
                            });
                        while(!finished_moving.load(std::memory_order_acquire)) {}
                    }
                    break;
                } else
                    output_q.push("\"" + program_name + "\": program not found\n\n");
            }
        }
    }
}
void KShell::request_shell_program_termination()
{
    if(is_program_running()) {
        log_warning("attempting to terminate a running KShellProgram");
        terminate_request_flag.store(true, std::memory_order_relaxed);
        //a KShellProgram might be waiting on input. Note that only 1 string is
        //pushed because it's assumed only 1 thread is connected to input at a time.
        input_q.push("");
        //The exception won't necessarily be caught if we're using OpenMP in the KShellProgram
        try {
            shell_program_return.get();
        } catch(const KShellTerminateRequestException &e) {
            log_info("successfully terminated KShellProgram");
        }
        terminate_request_flag.store(false, std::memory_order_relaxed);
    }
}
void KShell::reset_cursor(const KTermInputComm &comm)
{
    //if the current output is below us, we'll automatically scroll down until we can see it.
    //This is because add_text causes the terminal to recompute the cursor position.
    comm.terminal->add_text("", {});
    *comm.last_keydown_time = Time::now();
}

KShell::KShell():
    program_db(std::make_shared<KShProgramDb>()),
    terminate_request_flag(false),
    prevent_exit_count(0)
{}
KShell::~KShell()
{
    request_shell_program_termination();
}
void KShell::add_default_programs()
{
    program_db->add_program_nonatomic<programs::history>("history");
    program_db->add_program_nonatomic<programs::addertest>("addertest");
    program_db->add_program_nonatomic<programs::version>("version");
    program_db->add_program_nonatomic<programs::clear>("clear");
    program_db->add_program_nonatomic<programs::man>("man");
    program_db->add_program_nonatomic<programs::bigprint1by1>("bigprint1by1");
    program_db->add_program_nonatomic<programs::bigprintbatch>("bigprintbatch");
    program_db->add_program_nonatomic<programs::time>("time");
    program_db->add_program_nonatomic<programs::date>("date");
    program_db->add_program_nonatomic<programs::test_carriage_return>("test_carriage_return");
}
bool KShell::is_program_running() const
{
    return shell_program_return.valid();
}
void KShell::process_input(const KTermInputComm &comm)
{
    //we use comm.terminal because it's faster than calling terminal_weak_ptr.lock()
    switch(comm.input->type) {
    case SDL_TEXTINPUT:
        //TEXTINPUT is effectively a keydown input (this is
        //useful in case there's an onscreen keyboard or something)
        *comm.last_keydown_time = Time::now();

        cur_input_str.insert(cur_input_str_pos, comm.input->text.text);
        cur_input_str_pos += std::strlen(comm.input->text.text);
        break;
    case SDL_KEYDOWN: {
        switch(comm.input->key.keysym.sym) {
        case SDLK_BACKSPACE:
            reset_cursor(comm);
            if(cur_input_str_pos > 0) {
                cur_input_str.erase(cur_input_str.begin() + cur_input_str_pos - 1);
                cur_input_str_pos--;
            }
            break;
        case SDLK_LEFT:
            reset_cursor(comm);
            if(cur_input_str_pos > 0)
                cur_input_str_pos--;
            break;
        case SDLK_RIGHT:
            reset_cursor(comm);
            if(cur_input_str_pos < cur_input_str.size())
                cur_input_str_pos++;
            break;
        case SDLK_UP:
            reset_cursor(comm);
            if(!is_program_running()) {
                if(command_history_selected_idx > 0)
                    command_history_selected_idx--;
                if(command_history.size() > 0) {
                    cur_input_str = command_history[command_history_selected_idx];
                    cur_input_str_pos = cur_input_str.size();
                }
            }
            break;
        case SDLK_DOWN:
            reset_cursor(comm);
            if(!is_program_running() && command_history_selected_idx+1 < command_history.size()) {
                command_history_selected_idx++;
                cur_input_str = command_history[command_history_selected_idx];
                cur_input_str_pos = cur_input_str.size();
            }
            break;
        case SDLK_RETURN:
        case SDLK_RETURN2:
            reset_cursor(comm);
            input_q.push(cur_input_str);
            cur_input_str_pos = 0;
            cur_input_str.clear();
            break;
        case SDLK_c:
            if(comm.keymod & KMOD_CTRL) {
                reset_cursor(comm);

                if(is_program_running()) {
                    request_shell_program_termination();
                    comm.terminal->add_text(cur_input_str + "\n", {});
                } else {
                    comm.terminal->add_command_text(cur_input_str + "\n", {});
                }

                cur_input_str_pos = 0;
                cur_input_str.clear();
            }
            break;
        case SDLK_l:
            if(comm.keymod & KMOD_CTRL) {
                reset_cursor(comm);
                *comm.scroll_line = comm.num_lines_drawn - 1;
            }
            break;
        case SDLK_a:
            if(comm.keymod & KMOD_CTRL) {
                reset_cursor(comm);
                cur_input_str_pos = 0;
            }
            break;
        case SDLK_e:
            if(comm.keymod & KMOD_CTRL) {
                reset_cursor(comm);
                cur_input_str_pos = cur_input_str.size();
            }
            break;
        case SDLK_v:
            if(comm.keymod & KMOD_CTRL) {
                reset_cursor(comm);

                char *clipboard_text = SDL_GetClipboardText();
                for(auto i=clipboard_text; *i!='\0'; i++) {
                    if(*i == '\r') //windows might have these
                        continue;
                    if(*i == '\n') {
                        input_q.push(cur_input_str);
                        cur_input_str_pos = 0;
                        cur_input_str.clear();
                    } else {
                        cur_input_str.insert(cur_input_str.begin() + cur_input_str_pos, *i);
                        cur_input_str_pos++;
                    }
                }
                SDL_free(clipboard_text);
            }
            break;
        case SDLK_PAGEUP:
            *comm.scroll_line = std::clamp(*comm.scroll_line - 15,
                                           comm.min_scroll_line, comm.max_scroll_line);
            break;
        case SDLK_PAGEDOWN:
            *comm.scroll_line = std::clamp(*comm.scroll_line + 15,
                                           comm.min_scroll_line, comm.max_scroll_line);
            break;
        default:
            break;
        }
        break;}
    case SDL_MOUSEWHEEL:
        if((comm.keymod & KMOD_LCTRL) || (comm.keymod & KMOD_RCTRL)) {
            *comm.font_size = std::clamp(*comm.font_size * std::pow(1.05, comm.input->wheel.y),
                                         comm.min_font_size, comm.max_font_size);
        } else
            *comm.scroll_line = std::clamp(*comm.scroll_line - comm.input->wheel.y*3,
                                           comm.min_scroll_line, comm.max_scroll_line);
        break;
    default:
        break;
    }
}
void KShell::run(Passkey<KTerm>)
{
    run();
}
void KShell::set_owning_terminal(std::shared_ptr<KTerm> new_owner, Passkey<KTerm>)
{
    auto shell_term = terminal_weak_ptr.lock();
    if(shell_term!=nullptr && shell_term.get()!=new_owner.get())
        log_warning("Giving a new terminal ownership of a shell that "
                    "is already owned by another terminal");

    if(new_owner != nullptr) { //we have an owner
        terminate_request_flag.store(false, std::memory_order_relaxed);
        command_history_selected_idx = 0;
        cur_input_str_pos = 0;
        cur_input_str = "";
    } else {//we don't have an owner
        auto prevent_exit_count_ = prevent_exit_count.load(std::memory_order_acquire);
        if(prevent_exit_count_ > 0)
        {
            log_info("Waiting for prevent_exit_count to reach 0 (currently " +
                     to_str(prevent_exit_count_) + ") before finishing KShell dtor");
            while(prevent_exit_count.load(std::memory_order_acquire) > 0)
                std::this_thread::yield();
        }
        request_shell_program_termination();
    }
    terminal_weak_ptr = std::move(new_owner);
}
std::string KShell::get_cur_input_str(Passkey<KTerm>) const
{
    return cur_input_str;
}
size_t KShell::get_cur_input_str_pos(Passkey<KTerm>) const
{
    return cur_input_str_pos;
}
bool KShell::is_program_termination_request_active() const
{
    return terminate_request_flag.load(std::memory_order_relaxed);
}
void KShell::clear_terminal_history(Passkey<programs::clear>)
{
    auto terminal = terminal_weak_ptr.lock();
    k_expects(terminal != nullptr);
    terminal->clear_history();
}
void KShell::clear_command_history(Passkey<programs::history>)
{
    command_history.clear();
}
const std::vector<std::string> &KShell::get_command_history(Passkey<programs::history>) const
{
    return command_history;
}
const std::shared_ptr<KShProgramDb> &KShell::get_program_db(Passkey<programs::man, programs::time>) const
{
    return program_db;
}

std::string KShell::poll_input_q(Passkey<KShellProgram>)
{
    auto input = input_q.poll_blocking();
    output_q.push(input + "\n");
    if(terminate_request_flag.load(std::memory_order_relaxed))
        throw KShellTerminateRequestException({});
    return input;
}
void KShell::push_output_q(std::string text, Passkey<KShellProgram>)
{
    output_q.push(std::move(text));
    //don't check for a termination request here here because if the program is using multiple
    //threads (e.g. OpenMP) while printing, that could cause an issue. Throwing while reading
    //from input is OK because generally only one thread will be active if input is being read.
}
KShellSample::KShellSample():
    KShell()
{
    add_default_programs();
}
std::string KShellSample::name() const
{
    return "KShSample";
}

KShellProgram::KShellProgram(std::shared_ptr<KShell> shell_):
    shell(std::move(shell_))
{}
bool KShellProgram::run_on_separate_thread() const
{
    return true;
}
std::string KShellProgram::read_line()
{
    //we must be running on a different thread to read input
    k_expects(run_on_separate_thread());

    return shell->poll_input_q({});
}
void KShellProgram::println()
{
    k_expects(shell != nullptr);
    shell->push_output_q("", {}); ///push the q so we can check for TerminateRequest
}
std::string KShellProgram::manual_str() const
{
    return "This command does not have a manual\n";
}
}}
