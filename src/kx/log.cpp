#include "kx/log.h"
#include "kx/io.h"
#include "kx/time.h"
#include "kx/util.h"

#include <string>
#include <deque>
#include <mutex>
#include <atomic>

namespace kx {

LogMessage::LogMessage(const std::string &msg_, Type type_):
    message(msg_), type(type_)
{}
static std::deque<LogMessage> log_records;
static std::mutex log_mutex;
static std::atomic<bool> log_timestamps(true);

void toggle_log_timestamps(bool toggle)
{
    log_timestamps.store(toggle, std::memory_order_relaxed);
}
void log(const std::string &msg, LogMessage::Type type)
{
    //acquire the mutex here so we can load log_timestamps
    std::lock_guard<std::mutex> lg(log_mutex);

    std::string part1;
    if(log_timestamps.load(std::memory_order_relaxed)) {
        part1 += "[" + Time::now().to_str(Time::Format::HH_MM_SS) + "] ";
    }
    switch(type)
    {
    case LogMessage::Type::Info:
        part1 += "I: ";
        break;
    case LogMessage::Type::Warning:
        part1 += "W: ";
        break;
    case LogMessage::Type::Error:
        part1 += "E: ";
        break;
    case LogMessage::Type::Fatal:
        part1 += "Fatal Error: ";
        break;
    default:
        break;
    }
    std::string indented_msg;
    for(size_t i=0; i<msg.size(); i++) {
        indented_msg += msg[i];
        if(msg[i] == '\n') {
            for(size_t j=0; j<part1.size(); j++)
                indented_msg += ' ';
        }
    }

    auto final_msg = part1 + indented_msg;
    io::println(final_msg);

    log_records.emplace_back(final_msg, type);
    constexpr size_t MAX_LOG_RECORDS_SIZE = 1e4;
    if(log_records.size() > MAX_LOG_RECORDS_SIZE)
        log_records.pop_front();
}
void log_info(const std::string &msg)
{
    log(msg, LogMessage::Type::Info);
}
void log_warning(const std::string &msg)
{
    log(msg, LogMessage::Type::Warning);
}
void log_error(const std::string &msg)
{
    log(msg, LogMessage::Type::Error);
}
void log_fatal(const std::string &msg)
{
    log(msg, LogMessage::Type::Fatal);
}
void clear_log()
{
    std::lock_guard<std::mutex> lg(log_mutex);
    log_records.clear();
}

}
