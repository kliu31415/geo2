#pragma once

#include "kx/util.h"
#include "kx/debug.h"
#include "kx/multithread/shared_spinlock.h"

#include <mutex>
#include <shared_mutex>
#include <memory>
#include <map>
#include <string>

namespace kx {

template<class T, class SetLock = std::shared_mutex, class GlobalLock = std::mutex>
class SharedLock_Set final
{
    /** We could make global_mutex a shared_mutex and only use the exclusive lock when
     *  we modify the locks map. This would likely result in a considerable speedup
     *  when there is high activity. However, to implement this easily, we need a way to
     *  upgrade a shared_mutex from shared to exclusive locking, which there isn't a
     *  method for.
     *
     *  The objects in the set are referred to as "tickers" because this was originally
     *  developed to be used for stock ticker strings.
     */
    GlobalLock global_lock;

    std::map<T, SetLock> locks;
public:
    SharedLock_Set() = default;
    ~SharedLock_Set()
    {
        /** Good program design should result in no busy tickers when this dtor is called
         *  because we shouldn't be loading or downloading anything at that time. If KX_DEBUG
         *  is defined, the dtor attempts to lock every ticker to make sure none are being
         *  used. This might falsely fail if threads take a longer time than usual to sync
         *  data (i.e. the mutex has just been unlocked, but the current thread doesn't see
         *  it yet).
         *
         *  The dtor currently isn't thread-safe and assumes there won't be any new lock(...)
         *  requests. If we want to support the dtor being thread safe, then we would need to
         *  keep a flag that indicates whether more busy tickers can be acquired and set it
         *  to false in the dtor. The dtor would then block until busy.empty()==true.
         *
         *  The lock(ticker) function would need to check if this flag is true or not
         *  (failing if it's false) before making a ticker busy.
         */
        #ifdef KX_DEBUG
        std::lock_guard<GlobalLock> lg(global_lock);
        for(auto &ticker_mutex: locks) {
            bool try_lock_result = ticker_mutex.second.try_lock();
            ScopeGuard sg([=, &ticker_mutex]{if(try_lock_result) {ticker_mutex.second.unlock();}});
            if(!try_lock_result)
                log_error("failed to lock ticker mutex in SharedLock_Set dtor"
                          " (this could be a spurious failure, which is uncommon but not a"
                          " cause for concern)");
        }
        #endif
    }

    ///noncopyable because it's a synchronization type and you can't copy those
    SharedLock_Set(const SharedLock_Set&) = delete;
    SharedLock_Set &operator = (const SharedLock_Set&) = delete;

    ///nonmovable because we keep mutex addresses and we return ScopeGuards referencing "this"
    SharedLock_Set(SharedLock_Set&&) = delete;
    SharedLock_Set &operator = (SharedLock_Set&&) = delete;

    void lock(const T &ticker)
    {
        ///we immediately lock the ticker's mutex if the ticker doesn't exist yet
        std::unique_lock<GlobalLock> global_mutex_lock(global_lock);
        if(!locks.count(ticker)) {
            //create an entry and lock it
            locks[ticker].lock();
            return;
        }
        auto ticker_mutex = &locks[ticker];
        global_mutex_lock.unlock();

        /** otherwise we wait until we can lock it. This won't be a dangling pointer
         *  because we never erase from the map. Even if the map's contents are moved
         *  around, because the ticker_mutex is stored as a pointer, it'll stay in the
         *  same place.
         */
        ticker_mutex->lock();
    }
    void unlock(const T &ticker)
    {
        std::lock_guard<GlobalLock> lg(global_lock);

        k_expects(locks.count(ticker)); //somehow the ticker disappeared from the set?
        locks[ticker].unlock();
    }
    [[nodiscard]] ScopeGuard get_lock_guard(const T &ticker)
    {
        lock(ticker);
        return ScopeGuard([&, ticker]{unlock(ticker);});
    }

    void lock_shared(const T &ticker)
    {
        ///we immediately lock the ticker's mutex if the ticker doesn't exist yet
        std::unique_lock<GlobalLock> global_mutex_lock(global_lock);
        if(!locks.count(ticker)) {
            //create an entry and lock it in shared mode
            locks[ticker].lock_shared();
            return;
        }
        auto ticker_mutex = &locks[ticker];
        global_mutex_lock.unlock();

        /** otherwise we wait until we can lock it. This won't be a dangling pointer
         *  because we never erase from the map. Even if the map's contents are moved
         *  around, because the ticker_mutex is stored as a pointer, it'll stay in the
         *  same place.
         */
        ticker_mutex->lock_shared();
    }
    void unlock_shared(const T &ticker)
    {
        std::lock_guard<GlobalLock> lg(global_lock);

        k_expects(locks.count(ticker)); //somehow the ticker disappeared from the set?
        locks[ticker].unlock_shared();
    }
    [[nodiscard]] ScopeGuard get_lock_shared_guard(const T &ticker)
    {
        lock_shared(ticker);
        return ScopeGuard([&, ticker]{unlock_shared(ticker);});
    }
    /** There can't be a remove_ticker_mutex(...) function without changing the class a
     *  bit because we currently use temporary pointers to the locks in the locking
     *  functions, which would become dangling if the corresponding locks were erased.
     */
};

}
