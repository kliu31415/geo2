#pragma once

#include "kx/util.h"
#include "kx/debug.h"

#include <condition_variable>
#include <queue>
#include <mutex>
#include <utility>

namespace kx {

///moving probably isn't thread safe until I write move functions, so I'm deleting it for now
template<class T> class AtomicQueue final
{
    std::queue<T> q;
    mutable std::mutex mtx;
    std::condition_variable cv;
public:
    AtomicQueue() = default;

    AtomicQueue(const AtomicQueue&) = delete;
    AtomicQueue &operator = (const AtomicQueue&) = delete;
    AtomicQueue(const AtomicQueue&&) = delete;
    AtomicQueue &operator = (const AtomicQueue&&) = delete;

    void push(const T &val)
    {
        {
            std::lock_guard<std::mutex> lg(mtx);
            q.push(val);
        }
        cv.notify_one();
    }
    void push(T &&val)
    {
        {
            std::lock_guard<std::mutex> lg(mtx);
            q.push(std::forward<T>(val));
        }
        cv.notify_one();
    }
    T poll_blocking()
    {
        std::unique_lock<std::mutex> mlock(mtx);
        if(q.empty()) {
            //don't use this->empty() because that causes deadlock
            //by acquiring the same mutex twice in the same thread
            cv.wait(mlock, [&]{return !q.empty();});
        }
        T ret = std::move(q.front());
        q.pop();
        return ret;
    }
    bool poll_if_nonempty(T *return_holder)
    {
        std::lock_guard<std::mutex> lg(mtx);
        if(!q.empty()) {
            *return_holder = std::move(q.front());
            q.pop();
            return true;
        }
        return false;
    }
    bool empty() const
    {
        std::lock_guard<std::mutex> lg(mtx);
        return q.empty();
    }
};

}
