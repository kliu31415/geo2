#pragma once

#include <string>
#include <sstream>

namespace kx {

/** All functions are thread-safe. toggle_log_timestamps may take a while to
 *  propagate to other threads. However, since we use a mutex when we print and
 *  store logs, and mutexes have acquire/release semantics, it shouldn't ever
 *  occur that timestamps appear inconsistently, i.e. if thread 1 calls
 *  toggle_log_timestamps(...) and logs something, and thread 2 later logs something
 *  else, and no thread called toggle_log_timestamps(...) in between, then thread 2's
 *  log message will have a timestamp iff thread 1's does.
 */
struct LogMessage final
{
    std::string message;
    enum class Type {Info, Warning, Error, Fatal} type;
    LogMessage(std::string_view msg_, Type type_);
};

void toggle_log_timestamps(bool toggle);
void log(std::string_view message, LogMessage::Type type);

namespace impl {
    template<class T> void string_from_args(std::stringstream *ss, T&& arg1)
    {
        *ss << std::forward<T>(arg1);
    }
    template<class T, class... Args> void string_from_args(std::stringstream *ss, T&& arg1, Args&& ...others)
    {
        *ss << std::forward<T>(arg1);
        string_from_args(ss, std::forward<Args>(others)...);
    }
}

#define KX_DECLARE_LOG_FUNC(func_name, type) \
    template<class ...Args> void func_name(Args&& ...args) \
    { \
        std::stringstream ss; \
        impl::string_from_args(&ss, std::forward<Args>(args)...); \
        log(ss.str(), type); \
    }

KX_DECLARE_LOG_FUNC(log_info, LogMessage::Type::Info)
KX_DECLARE_LOG_FUNC(log_warning, LogMessage::Type::Warning)
KX_DECLARE_LOG_FUNC(log_error, LogMessage::Type::Error)
KX_DECLARE_LOG_FUNC(log_fatal, LogMessage::Type::Fatal)

}
