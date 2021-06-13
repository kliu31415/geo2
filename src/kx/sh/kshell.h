#pragma once

#include "kx/atomic/queue.h"
#include "kx/util.h"
#include "kx/debug.h"

#include <memory>
#include <string>
#include <vector>
#include <future>
#include <atomic>

namespace kx { namespace sh {

namespace programs
{
    class history;
    class addertest;
    class version;
    class clear;
    class man;
    class bigprint1by1;
    class bigprintbatch;
    class time;
    class date;
    class test_carriage_return;
}

class KTerm;
struct KTermInputComm;
class KShellProgram;
class KShProgramDb;

/** KShells should be created with make_shared (not doing so won't cause a
 *  bug, but you can't attach it to a terminal without a shared_ptr, meaning
 *  you'll be unable to interact with it if it's not a shared_ptr)
 */
class KShell: public std::enable_shared_from_this<KShell>
{
protected:
    std::vector<std::string> command_history;

    /** Each shell has some programs it can run, stored in a db. This is a shared_ptr because
     *  KShellPrograms might request a copy of the program_db pointer.
     */
    std::shared_ptr<KShProgramDb> program_db;

    std::atomic<bool> terminate_request_flag;

    AtomicQueue<std::string> input_q;
    AtomicQueue<std::string> output_q;

    std::future<void> shell_program_return;

    std::weak_ptr<KTerm> terminal_weak_ptr; ///make it weak_ptr to prevent smart pointer cycles

    size_t command_history_selected_idx;

    std::string cur_input_str;
    size_t cur_input_str_pos;

    std::atomic<int> prevent_exit_count;

    static std::vector<std::string> parse_args(const std::string &str);

    /** There are two versions of KShell::run(...). They do the same thing. One of them is
     *  protected, and the other requires a Passkey<KTerm>. Derived classes can override the
     *  one that requires Passkey<KTerm> (which is the one that the KTerm will call, and
     *  internally calls the protected run()), but they shouldn't override the protected
     *  run() because KTerm doesn't call the protected run().
     *  This is virtual and final just to prevent accidental overrides.
     */
    virtual void run() final; //prevent accidental override with final

    void request_shell_program_termination(); ///does nothing if no program is running
    void reset_cursor(const KTermInputComm &comm);
public:
    KShell();
    virtual ~KShell();

    ///You can't copy or move a shell because KTerm and KShellProgram have pointers to it
    KShell(const KShell&) = delete;
    KShell &operator = (const KShell&) = delete;

    KShell(KShell&&) = delete;
    KShell &operator = (KShell&&) = delete;

    void add_default_programs();

    virtual std::string name() const = 0;
    bool is_program_running() const;

    void process_input(const KTermInputComm &comm);
    virtual void run(Passkey<KTerm>);
    void set_owning_terminal(std::shared_ptr<KTerm> new_owner, Passkey<KTerm>);
    size_t get_cur_input_str_pos(Passkey<KTerm>) const;
    std::string get_cur_input_str(Passkey<KTerm>) const;

    bool is_program_termination_request_active() const; ///thread-safe

    void clear_terminal_history(Passkey<programs::clear>);
    void clear_command_history(Passkey<programs::history>);
    const std::vector<std::string> &get_command_history(Passkey<programs::history>) const;

    const std::shared_ptr<KShProgramDb> &get_program_db(Passkey<programs::man, programs::time>) const;

    /** Be careful about poll_input_q(...). If there's a program termination request active, it'll
     *  throw an exception to stop the program. If the KShellProgram isn't using more than one
     *  thread at the time, the exception will be caught and handled by the KShell. However, if
     *  there are multiple threads (e.g. OpenMP), this could lead to an issue. The reason this has
     *  to throw is that if the KShell is requesting program termination, there's a good chance
     *  there won't be any more input, so this might stall forever waiting for input if it doesn't
     *  throw.
     *
     *  push_output_q(...) doesn't throw because multithreaded KShellPrograms (e.g. ts1::iq's
     *  downloader) may want to print stuff, and they can print fine even if a terminate request
     *  is active.
     */
    std::string poll_input_q(Passkey<KShellProgram>);
    void push_output_q(std::string text, Passkey<KShellProgram>);
};

class KShellSample final: public KShell
{
public:
    KShellSample();
    std::string name() const override;
};

class KShellProgram
{
protected:
    /** We keep a shared_ptr to the owning shell. The KShellProgram does NOT own the shell and
     *  the shell should never go out of scope while a KShellProgram is running because the
     *  shell terminates all KShellPrograms when the shell is detached from a terminal. However,
     *  using a shared_ptr rather than a raw pointer, even though we aren't really an owner,
     *  results in more flexible and easier to debug code. This won't result in a memory leak
     *  from pointer cycles because KShell only runs programs while attached to a terminal, so
     *  it is always possible to reach the KShellProgram's memory location via the terminal.
     */
    std::shared_ptr<KShell> shell;

    KShellProgram(std::shared_ptr<KShell> shell_);

    std::string read_line(); ///like getline (blocks until input is read)
    template<class T> void print(T &&val); ///like cout
    template<class T> void println(T &&val); ///like cout with endl
    void println(); ///just prints a newline
public:
    virtual ~KShellProgram() = default;

    ///noncopyable and nonmovable because KShell holds a pointer to KShellProgram
    KShellProgram(const KShellProgram&) = delete;
    KShellProgram &operator = (const KShellProgram&) = delete;

    KShellProgram(KShellProgram&&) = delete;
    KShellProgram &operator = (KShellProgram&&) = delete;

    virtual std::string manual_str() const;
    virtual bool run_on_separate_thread() const; /** defaults to "true", i.e. a new thread will be
                                                  *  created for this KShellProgram. All programs
                                                  *  reading input need their own thread.
                                                  */
    virtual void run(std::vector<std::string> args) = 0;
};






///---------.tpp----------
template<class T> void KShellProgram::print(T &&val)
{
    k_expects(shell != nullptr);
    shell->push_output_q(to_str(std::forward<T>(val)), {});
}
template<class T> void KShellProgram::println(T &&val)
{
    k_expects(shell != nullptr);
    shell->push_output_q(to_str(std::forward<T>(val)) + "\n", {});
}

}}
