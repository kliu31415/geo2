#pragma once

#include "kx/multithread/spinlock.h"
#include "kx/atomic/queue.h"

#include <vector>
#include <thread>
#include <future>
#include <condition_variable>
#include <mutex>
#include <optional>
#include <queue>

namespace geo2 {

class ThreadPool
{
    struct Task
    {
        std::promise<void> ret;
        std::function<void()> task_fn;

        Task(std::promise<void> &&ret_, const std::function<void()> &task_fn_):
            ret(std::move(ret_)),
            task_fn(task_fn_)
        {}
    };
    std::vector<std::thread> threads;
    kx::AtomicQueue<Task> tasks;

    void run_thread()
    {
        while(true) {
            ///a null function is the quit signal
            auto task = tasks.poll_blocking();

            if(task.task_fn) {
                task.task_fn();
                task.ret.set_value();
            } else
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
        for(size_t i=0; i<threads.size(); i++) {
            tasks.push({{}, {}});
        }

        for(size_t i=0; i<threads.size(); i++)
            threads[i].join();
    }
    ///can't be copied
    ThreadPool(const ThreadPool&) = delete;
    ThreadPool &operator = (const ThreadPool&) = delete;

    ///too lazy to add move support rn
    ThreadPool(ThreadPool&&) = delete;
    ThreadPool &operator = (ThreadPool&&) = delete;

    [[nodiscard]] std::future<void> add_task(const std::function<void()> &to_add)
    {
        std::promise<void> ret;
        auto fut = ret.get_future();
        ///don't add null functions; they serve as a special signal
        if(to_add)
            tasks.push(Task(std::move(ret), to_add));
        else
            ret.set_value(); ///fill the promise ourselves (because no worker thread will)
        return fut;
    }
    size_t size() const
    {
        return threads.size();
    }
};

/*
class SpinThreadPool
{
    struct Task
    {
        std::promise<void> ret;
        std::function<void()> task_fn;

        Task()
        {}
        Task(std::promise<void> &&ret_, const std::function<void()> &task_fn_):
            ret(std::move(ret_)),
            task_fn(task_fn_)
        {}
    };
    std::vector<std::thread> threads;

    std::mutex mtx;
    std::condition_variable cv;

    std::atomic<bool> should_spin;
    std::atomic<bool> should_quit;
    std::atomic<int> num_tasks;

    kx::Spinlock<kx::SpinlockWaitStrategy::mm_pause> task_lock;
    std::queue<Task> tasks;

    void run_thread()
    {
        while(true) {
            ///wait until we're told to spin or quit
            {
                std::unique_lock lk(mtx);
                cv.wait(lk, [this]{return should_spin.load(std::memory_order_relaxed) ||
                                          should_quit.load(std::memory_order_relaxed);});
            }

            if(should_quit.load(std::memory_order_relaxed))
                break;

            while(true) {
                std::optional<Task> task;
                ///keep spinning until there's a task in the queue OR we're told
                ///to stop spinning
                while(num_tasks.load(std::memory_order_relaxed)==0 &&
                      should_spin.load(std::memory_order_relaxed))
                {
                    _mm_pause();
                }

                {
                    auto lg = task_lock.get_lock_guard();
                    if(tasks.size() > 0) {
                        task = std::move(tasks.front());
                        tasks.pop();
                        num_tasks.fetch_sub(1, std::memory_order_relaxed);
                    }
                }

                if(task) {
                    task->task_fn();
                    task->ret.set_value();
                }

                if(!should_spin.load(std::memory_order_relaxed))
                    break;
            }
        }
    }
public:
    SpinThreadPool(int n):
        should_spin(false),
        should_quit(false),
        num_tasks(0)
    {
        if(n < 1) {

            kx::log_warning("SpinThreadPool with <1 thread constructed. Upping thread count to 1."
                            "Note: This machine has " +
                            kx::to_str(std::thread::hardware_concurrency()) + " threads");
            n = 1;
        }
        threads.resize(n);
        for(int i=0; i<n; i++)
            threads[i] = std::thread([this]{run_thread();});
    }
    ~SpinThreadPool()
    {
        should_spin.store(false, std::memory_order_relaxed);
        should_quit.store(true, std::memory_order_relaxed);
        cv.notify_all();

        for(size_t i=0; i<threads.size(); i++)
            threads[i].join();
    }
    ///can't be copied
    SpinThreadPool(const SpinThreadPool&) = delete;
    SpinThreadPool &operator = (const SpinThreadPool&) = delete;

    ///too lazy to add move support rn
    SpinThreadPool(SpinThreadPool&&) = delete;
    SpinThreadPool &operator = (SpinThreadPool&&) = delete;

    void set_spin(bool val)
    {
        should_spin.store(val, std::memory_order_relaxed);
        cv.notify_all();
    }
    [[nodiscard]] std::future<void> add_task(const std::function<void()> &to_add)
    {
        std::promise<void> ret;
        auto fut = ret.get_future();
        auto lg = task_lock.get_lock_guard();
        tasks.push(Task(std::move(ret), to_add));
        num_tasks.fetch_add(1, std::memory_order_relaxed);
        return fut;
    }
    size_t size() const
    {
        return threads.size();
    }
};
*/

}
