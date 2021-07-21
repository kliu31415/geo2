#pragma once

#include "kx/multithread/spinlock_wait_strategy.h"
#include "kx/debug.h"
#include "kx/util.h"

#include <cstdint>
#include <xmmintrin.h>
#include <thread>
#include <atomic>

namespace kx {

/** This is a spinlock with shared and exclusive ownership.
 *  Locking has acquire semantics and unlocking has release semantics.
 *  Trying to lock may spuriously fail and has acquire semantics only if it succeeds.
 *
 *  This lock deal with up to 2**63-1 threads locking it in shared mode. No checking is
 *  done for overflows; it's assumed that this limit will never be exceeded.
 *
 *  THIS IS NOT WELL TESTED!
 */
template<SpinlockWaitStrategy wait_strategy = SpinlockWaitStrategy::mm_pause>
class SharedSpinlock
{
    using counter_t = int64_t;
    std::atomic<counter_t> counter;

    ///EXCLUSIVE_VAL can be any negative value, but further from 0 = easier to debug.
    static constexpr counter_t EXCLUSIVE_VAL = std::numeric_limits<counter_t>::min();

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
    SharedSpinlock():
        counter(0)
    {}

    ~SharedSpinlock()
    {
        [[maybe_unused]] auto counter_val = counter.load(std::memory_order_relaxed);
        k_expects(counter_val==0, counter_val);
    }

    ///locks can't be copied
    SharedSpinlock(const SharedSpinlock&) = delete;
    SharedSpinlock &operator = (const SharedSpinlock&) = delete;

    ///nonmovable because we return ScopeGuards referencing "this"
    SharedSpinlock(SharedSpinlock&&) = delete;
    SharedSpinlock &operator = (SharedSpinlock&&) = delete;

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
    void lock_shared()
    {
        counter_t expected = std::max((counter_t)0, counter.load(std::memory_order_relaxed));
        counter_t desired = expected + 1;
        while(!comp_exch_weak(&expected, desired))
        {
            expected = std::max((counter_t)0, expected);
            desired = expected + 1;
            spinlock_wait();
        }
    }
    bool try_lock_shared()
    {
        counter_t expected = std::max((counter_t)0, counter.load(std::memory_order_relaxed));
        counter_t desired = expected + 1;
        return comp_exch_weak(&expected, desired);
    }
    void unlock_shared()
    {
        k_expects(counter.load(std::memory_order_relaxed) > 0);
        counter.fetch_sub(1, std::memory_order_release);
    }
    [[nodiscard]] ScopeGuard get_lock_guard()
    {
        lock();
        return ScopeGuard([this]{unlock();});
    }
    [[nodiscard]] ScopeGuard get_lock_shared_guard()
    {
        lock_shared();
        return ScopeGuard([this]{unlock_shared();});
    }
};

}
