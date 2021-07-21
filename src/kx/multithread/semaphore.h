#pragma once

#include "kx/util.h"
#include "kx/debug.h"

#include <cstdint>
#include <mutex>
#include <condition_variable>

namespace kx {

/** This is a semaphore class that can handle up to INT64_MAX available permits.
 *  release() can be called without acquire(), but doing so is odd, so it is
 *  discouraged. Note that calling release() like this can result in more permits
 *  available than when the semaphore was constructed.
 *
 *  All blocked threads are notified whenever permits are released. This may
 *  cause performance issues in some scenarios (i.e. if there are many blocked
 *  threads and permits are released one at a time).
 *
 *  This is not well tested.
 */
class CountingSemaphore final
{
    int64_t permits;
    std::condition_variable cv;
    std::mutex mtx;
public:
    ///constructs a semaphore with a given number of permits
    CountingSemaphore(int64_t permits_):
        permits(permits_)
    {
        k_expects(permits_ > 0);
    }

    void acquire(int64_t num_permits)
    {
        k_expects(num_permits > 0);
        std::unique_lock<std::mutex> lk(mtx);
        cv.wait(lk, [=]{return num_permits <= permits;});
        k_assert(lk.owns_lock()); //checking to make sure I understand cv.wait()'s postcondition
        permits -= num_permits;
    }
    void acquire()
    {
        acquire(1);
    }

    void release(int64_t num_permits)
    {
        k_expects(num_permits > 0);

        std::unique_lock<std::mutex> lk(mtx);
        permits += num_permits;
        lk.unlock();

        cv.notify_all();
    }
    void release()
    {
        release(1);
    }
};

}
