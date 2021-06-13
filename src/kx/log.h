#pragma once

#include <string>

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
    LogMessage(const std::string &msg_, Type type_);
};

void toggle_log_timestamps(bool toggle);
void log(const std::string &msg, LogMessage::Type type);
void log_info(const std::string &msg);
void log_warning(const std::string &msg);
void log_error(const std::string &msg);
void log_fatal(const std::string &msg);
void clear_log();

}
