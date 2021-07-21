#pragma once

#include "kx/multithread/spinlock_wait_strategy.h"
#include "kx/debug.h"
#include "kx/util.h"

#include <cstdint>
#include <xmmintrin.h>
#include <thread>
#include <atomic>

namespace kx {

/** This is a spinlock with exclusive ownership.
 *  Locking has acquire semantics and unlocking has release semantics.
 *  Trying to lock may spuriously fail and has acquire semantics only if it succeeds.
 *
 *  THIS IS NOT WELL TESTED!
 */
template<SpinlockWaitStrategy wait_strategy = SpinlockWaitStrategy::mm_pause>
class Spinlock
{
    using counter_t = int_fast8_t;
    std::atomic<counter_t> counter;

    static constexpr counter_t EXCLUSIVE_VAL = 1;

    inline void spinlock_wait()
    {
        //mm_pause() hints to the CPU that we're in a spinlock loop
        if constexpr(wait_strategy == SpinlockWaitStrategy::mm_pause)
            _mm_pause();

        //yield() gives up the current timeslice
        if constexpr(wait_strategy == SpinlockWaitStrategy::Yield)
            std::this_thread::yield();

        //don't wait at all
        if constexpr(wait_strategy == SpinlockWaitStrategy::DontWait)
            ((void)0);
    }
    inline bool comp_exch_weak(counter_t *expected, counter_t desired)
    {
        return counter.compare_exchange_weak(*expected,
                                             desired,
                                             std::memory_order_acquire,
                                             std::memory_order_relaxed);
    }
public:
    Spinlock():
        counter(0)
    {}

    ~Spinlock()
    {
        [[maybe_unused]] auto counter_val = counter.load(std::memory_order_relaxed);
        k_expects(counter_val==0, counter_val);
    }

    ///locks can't be copied
    Spinlock(const Spinlock&) = delete;
    Spinlock &operator = (const Spinlock&) = delete;

    ///nonmovable because we return ScopeGuards referencing "this"
    Spinlock(Spinlock&&) = delete;
    Spinlock &operator = (Spinlock&&) = delete;

    void lock()
    {
        counter_t expected = 0;
        counter_t desired = EXCLUSIVE_VAL;
        while(!comp_exch_weak(&expected, desired))
        {
            expected = 0;
            spinlock_wait();
        }
    }
    bool try_lock()
    {
        counter_t expected = 0;
        counter_t desired = EXCLUSIVE_VAL;
        return comp_exch_weak(&expected, desired);
    }
    void unlock()
    {
        k_expects(counter.load(std::memory_order_relaxed) == EXCLUSIVE_VAL);
        counter.store(0, std::memory_order_release);
    }
    [[nodiscard]] ScopeGuard get_lock_guard()
    {
        lock();
        return ScopeGuard([this]{unlock();});
    }
};

}
