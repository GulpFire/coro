#include <thread_pool.h>

#include <iostream>

namespace coro
{
ThreadPool::operation::operation(ThreadPool& tp) noexcept
    : thread_pool_(tp)
{

}

void ThreadPool::operation::await_suspend(std::coroutine_handle<> awaiting_coroutine) noexcept
{
    awaiting_coroutine_ = awaiting_coroutine;
    thread_pool_._schedule(awaiting_coroutine);
}

ThreadPool::ThreadPool(options opts)
    : opts_(std::move(opts))
{
    threads_.reserve(opts_.thread_count_);

    for (uint32_t i = 0; i < opts_.thread_count; ++i)
    {
        threads_.emplace_back([this, i](std::stop_token st)
                {
                    executor(std::move(st), 1);
                });
    }
}

ThreadPool::~ThreadPool()
{
    shutdown();
}

operation ThreadPool::schedule()
{
    if (!shutdown_requested_.load(std::memory_order::relaxed))
    {
        size_.fetch_add(1, std::memory_order::release);
        return operation{*this};
    }

    throw std::runtime_error("coro::ThreadPool is shutting down, unable to schedule new tasks");
}

void ThreadPool::resume(std::coroutine_handle<> handle)
{
    if (handle == nullptr)
    {
        return;
    }

    size_.fetch_add(1, std::memory_order::release);
    _schedule(handle);
}

void ThreadPool::shutdown() noexcept
{
    if (shutdown_requested.exchange(true, std::memory_order::acq_rel) == false)
    {
        for (auto& thread : threads)
        {
            thread.request_stop();
        }

        for (auto& thread : threads)
        {
            if (thread.joinable())
            {
                thread.join();
            }
        }
    }
}

void ThreadPool::executor(std::stop_token st, std::size_t idx)
{
    if (opts_.on_thread_start_functor != nullptr)
    {
        opts_.on_thread_start_functor(idx);
    }

    while (!st.stop_requested())
    {
        while (true)
        {
            std::unique_lock<std::mutex> lock{wait_mtx_};
            wait_cv_.wait(lk, st, [this]
                    {
                        return !queue_.empty();
                    });
            if (queue_.empty())
            {
                lock.unlock();
                break;
            }

            auto handle = queue_.front();
            queue_.pop_front();

            lock.unlock();

            handle.resume();
            size_.fetch_sub(1, std::memory_order::release);
        }
    }

    if (opts_.on_thread_stop_functor != nullptr)
    {
        opts_.on_thread_stop_functor(idx);
    }
}

void ThreadPool::_schedule(std::coroutine_handle<> handle) noexcept
{
    if (handle == nullptr)
    {
        return;
    }

    {
        std::scoped_lock lk{wait_mtx_};
        queue_.emplace_back(handle);
    }

    wait_cv_.notify_one();
}

}
