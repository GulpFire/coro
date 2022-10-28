#pragma once

#include <concepts/range_of.h>
#include <event.h>
#include <task.h>

#include <atomic>
#include <condition_variable>
#include <coroutine>
#include <deque>
#include <functional>
#include <mutex>
#include <optional>
#include <ranges>
#include <thread>
#include <variant>
#include <vector>

namespace coro
{

class ThreadPool
{
    public:
        struct operation
        {
            bool await_ready() noexcept 
            {
                return false;
            }

            void await_suspend(std::coroutine_handle<> awaiting_coroutine) noexcept;

            void await_resume() noexcept
            {
            
            }

            private:
                friend class ThreadPool;
                ThreadPool& thread_pool_;
                std::coroutine_handle<> awaiting_coroutine{nullptr};

                explicit operation(ThreadPool& tp) noexcept;
        };

        struct options
        {
            uint32_t thread_count = std::thread::hardware_concurrency();
            std::function<void(std::size_t)> on_thread_start_functor = nullptr;
            std::function<void(std::size_t)> on_thread_stop_functor = nullptr;
        };

        explicit ThreadPool(
                options opts = options{
                    .thread_count = std::thread::hardware_concurrency(),
                    .on_thread_start_functor = nullptr,
                    .on_thread_stop_functor = nullptr});

        ThreadPool(const ThreadPool&) = delete;
        
        ThreadPool(ThreadPool&&) = delete;
        
        ThreadPool& operator=(const ThreadPool&) = delete;
        
        ThreadPool& operator=(ThreadPool&&) = delete;

        virtual ~ThreadPool();

        uint32_t thread_count() const noexcept
        {
            return threads_.size();
        }

        [[nodiscard]] operation schedule();

        template <typename Func, typename... Args>
        [[nodiscard]] auto schedule(Func& f, Args... args) -> Task<decltype(f(std::forward<Args>(args)...))>
        {
            co_await schedule();

            if constexpr (std::is_same_v<void, decltype(f(std::forward<Args>(args)...))>)
            {
                f(std::forward<Args>(args)...);
                co_return;
            }
            else
            {
                co_return f(std::forward<Args>(args)...);
            }
        }

        void resume(std::coroutine_handle<> handle) noexcept;

        template <coro::concepts::range_of<std::coroutine_handle<>> range_type>
        void resume(const range_type& handles) noexcept
        {
            size_.fetch_add(std::size(handles), std::memory_order::release);
            size_t null_handles{0};

            {
                std::scoped_lock lock{wait_mtx_};
                for (const auto& handle : handles)
                {
                    if (handle != nullptr) [[likely]]
                    {
                        queue_.emplace_back(handle);
                    }
                    else
                    {
                        ++null_handles;
                    }
                }
            }

            if (null_handles > 0)
            {
                size_.fetch_sub(null_handles, std::memory_order::release);
            }

            wait_cv_.notify_one();
        }

        [[nodiscard]] operation yield()
        {
            return schedule();
        }

        void shutdown() noexcept;

        std::size_t size() const noexcept
        {
            return size_.load(std::memory_order::acquire);
        }

        bool empty() const noexcept
        {
            return 0 == size_;
        }

        std::size_t queue_size() const noexcept
        {
            std::atomic_thread_fence(std::memory_order::acquire);
            return queue_.size();
        }

        bool queue_empty() const noexcept
        {
            return 0 == queue_.size();
        }

    private:
        options opts_;
        std::vector<std::jthread> threads_;
        std::mutex wait_mtx_;
        std::condition_variable_any wait_cv_;
        std::deque<std::coroutine_handle<>> queue_;

        void executor(std::stop_token, std::size_t idx);

        void _schedule(std::coroutine_handle<> handle) noexcept;

        std::atomic<std::size_t> size_{0};
        std::atomic<bool> shutdown_requested_{false};
};

} // namespace coro
