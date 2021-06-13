#pragma once

#include "kx/debug.h"

#include <string>
#include <mutex>

namespace kx {

///This class isn't well tested
class AtomicString final
{
    std::string str;
    std::mutex mtx;
public:
    AtomicString() {}
    AtomicString(const std::string &s)
    {
        std::lock_guard<std::mutex> lg(mtx);
        str = s;
    }

    AtomicString(const AtomicString&) = delete;
    AtomicString &operator = (const AtomicString&) = delete;

    ///moving probably isn't thread safe until I write move functions, so I'm deleting it for now
    AtomicString(const AtomicString&&) = delete;
    AtomicString &operator = (const AtomicString&&) = delete;

    std::string copy_str()
    {
        std::lock_guard<std::mutex> lg(mtx);
        return str;
    }
    void clear()
    {
        std::lock_guard<std::mutex> lg(mtx);
        str.clear();
    }
    void operator += (const std::string &other)
    {
        std::lock_guard<std::mutex> lg(mtx);
        str += other;
    }
};

}
