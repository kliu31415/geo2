#pragma once

#include "kx/atomic/queue.h"

#include <vector>
#include <thread>

namespace geo2 {

class ThreadPool
{
    std::vector<std::thread> threads;
    kx::AtomicQueue<std::function<void()>> tasks;

    void run_thread()
    {
        while(true) {
            ///a null function is the quit signal
            auto task_fn = tasks.poll_blocking();
            if(task_fn)
                task_fn();
            else
                break;
        }
    }
public:
    ThreadPool(int n)
    {
        if(n < 1) {

            kx::log_warning("ThreadPool with <1 thread constructed. Upping thread count to 1."
                            "Note: This machine has " +
                            kx::to_str(std::thread::hardware_concurrency()) + " threads");
            n = 1;
        }
        threads.resize(n);
        for(int i=0; i<n; i++)
            threads[i] = std::thread([this]{run_thread();});
    }
    ~ThreadPool()
    {
        ///add enough null functions for all threads to get one
        for(size_t i=0; i<threads.size(); i++)
            tasks.push(std::function<void()>());

        for(size_t i=0; i<threads.size(); i++)
            threads[i].join();
    }
    ///can't be copied
    ThreadPool(const ThreadPool&) = delete;
    ThreadPool &operator = (const ThreadPool&) = delete;

    ///too lazy to add move support rn
    ThreadPool(ThreadPool&&) = delete;
    ThreadPool &operator = (ThreadPool&&) = delete;

    void add_task(const std::function<void()> &to_add)
    {
        ///don't add null functions; they serve as a special signal
        if(to_add)
            tasks.push(to_add);
    }
    void add_tasks(const std::vector<std::function<void()>> &to_add)
    {
        for(const auto &f: to_add) {
            ///don't add null functions; they serve as a special signal
            if(f)
                tasks.push(f);
        }
    }
    size_t size() const
    {
        return threads.size();
    }
};

}
