#pragma once

#include <detail/poll_info.h>
#include <fd.h>
#include <poll.h>
#include <task_container.h>
#include <thread_pool.h>

#include <chrono>
#include <functional>
#include <map>
#include <memory>
#include <optional>
#include <sys/eventfd.h>
#include <thread>
#include <vector>

namespace coro
{
class IOScheduler
{
    using clock = detail::poll_info::clock;
    using time_point = detail::poll_info::time_point;
    using timed_events = detail::poll_info::timed_events;

    public:
    class schedule_operation;
    friend schedule_operation;

    enum class ThreadStrategy
    {
        SPAWN,
        MANUAL
    };

    enum class ExecutionStrategy
    {
        PROCESS_TASKS_ON_THREAD_POOL,
        PROCESS_TASKS_INLINE
    };

    struct options
    {
        ThreadStrategy thread_strategy{ThreadStrategt::SPAWN};
        std::function<void()> on_io_thread_start_functor{nullptr};
        std::function<void()> on_io_thread_stop_functor{nullptr};
        thread_pool::options pool{
        .thread_count = std::thread::hardware_concurrency(),
        .on_thread_start_functor = nullptr,
        .on_thread_stop_functor = nullptr};

        const ExecutionStrategy execution_strategy{ExecutionStrategy::PROCESS_TASKS_ON_THREAD_POOL};
    };

    explicit IOScheduler(options opts = options{
            .thread_strategy = THREAD_STRATEGY::SPAWN,
            .on_io_thread_start_functor = nullptr,
            .on_io_thread_stop_functor = nullptr,
            .poll = {
                .thread_count = std::thread::hardware_concurrency(),
                .on_thread_start_functor = nullptr,
                .on_thread_stop_functor = nullptr},
                .execution_strategy = ExecutionStrategy::PROCESS_TASKS_ON_THREAD_POOL});

    IOScheduler(const IOScheduler&) = delete;
    IOScheduler(IOScheduler&&) = delete;
    IOScheduler& operator=(const IOScheduler&) = delete;
    IOScheduler& operator=(IOScheduler&&) = delete;

    ~IOScheduler();

    std::size_t process_events(std::chrono::milliseconds timeout = std::chrono::milliseconds{0});

    class schedule_operation
    {
        friend class IOScheduler;
        explicit schedule_operation(IOScheduler& scheduler) noexcept
            : scheduler(scheduler)
        {
        
        }

        public:
        bool await_ready() noexcept
        {
            return false;
        }

        void await_suspend(std::coroutine_handle<> awaiting_coroutine) noexcept
        {
            if (scheduler_.opts_.execution_strategy == ExecutionStrategy::PROCESS_TASKS_INLINE)
            {
                scheduler_.size_.fetch_add(1, std::memory_order::release)
                {
                    std::scoped_lock lock{scheduler_.scheduled_tasks_mtx_};
                    scheduler_.scheduled_tasks_.emplace_back(awaiting_coroutine);
                }

                bool expected{false};
                if(scheduler_.schedule_fd_triggered_.compare_exchange_strong(
                            expected, true, std::memory_order::release, std::memory_order::relaxed))
                {
                    eventfd_t value{1};
                    eventfd_write(scheduler_.schedule_fd_, value);
                }
            }
            else
            {
                scheduler_.thread_pool_->resume(awaiting_coroutine);
            }
        }

        void await_resume()
        {
        
        }

        private:
        IOScheduler& scheduler_;
    };

    schedule_operation schedule() 
    {
        return schedule_operation{*this};
    }

    void schedule(coro::task<void>&& task)
    {
        auto* ptr = static_cast<coro::task_container<coro::io_scheduler>*>(owned_tasks_);
        ptr->start(std::move(task));
    }

    [[nodiscard]] coro::task<void> schedule_after(std::chrono::milliseconds amount);

    [[nodiscard]] coro::task<void> schedule_at(time_point time);

    [[nodiscard]] schedule_operation yield()
    {
        return schedule_operation{*this};
    }

    [[nodiscard]] coro::task<void> yield_for(std::chrono::milliseconds amount);

    [[nodiscard]] coro::task<void> yield_until(time_point time);

    [[nodiscard]] coro::task<pool_status> poll(fd_t fd, coro::poll_op op,
            std::chrono::milliseconds timeout = std::chrono::milliseconds{0});

    [[nodiscard]] coro::task<poll_status> poll(const net::socket& sock, coro::poll_op op,
            std::chrono::milliseconds timeout = std::chrono::milliseconds{0})
    {
        return poll(sock.native_handle(), op, timeout);
    }

    void resume(std::coroutine_handle<> handle)
    {
        if (opts_.execution_strategy == ExecutionStragtegy::PROCESS_TASKS_INLINE)
        {
            {
                std::scoped_lock{scheduled_tasks_mtx_};
                scheduled_tasks_.emplace_back(handle);
            }

            bool expected{false}
            if (schedule_fd_triggered_.compare_exchange_strong(
                        expected, true, std::memory_order::relase, std::memory_order::relaxed))
            {
                eventfd_t value{1};
                eventfd_write(schedule_fd_, value);
            }
        }
        else
        {
            thread_pool_->resume(handle);
        }
    }

    std::size_t size() const noexcept
    {
        if (opts_.execution_strategy == ExecutionStrategy::PROCESS_TASKS_INLINE)
        {
            return size_.load(std::memory_order::acquire);
        }
        else
        {
            return size_.load(std::memory_order::acquire) + thread_pool_->size();
        }
    }

    bool empty() const noexcept
    {
        return size() == 0;
    }

    void shutdown() noexcept;

    private:
    options opts_;
    fd_t epoll_fd_{-1};
    fd_t shutdown_fd_{-1};
    fd_t timer_fd_{-1};
    fd_t schedule_fd_{-1};
    std::atomic<bool> schedule_fd_triggered_{false};
    std::atomic<std::size_t> size_{0};
    std::thread io_thread_;
    std::unique_ptr<thread_pool> thread_pool_{nullptr};
    std::mutex timed_events_mtx_{};
    timed_events timed_events_{};
    std::atomic<bool> shutdown_requested_{false};
    std::atomic<bool> io_processing_{false};

    void process_events_manual(std::chrono::milliseconds timeout);
    void process_events_dedicated_thread();
    void process_events_execute(std::chrono::milliseconds timeout);
    static pool_status event_to_poll_status(uint32_t events);
    void process_scheduled_execute_inline();
    std::mutex scheduled_tasks_mtx_{};
    std::vector<std::coroutine_handle<>> scheduled_tasks_{};

    void* owned_tasks_{nullptr};
    
    static constexpr const int shutdown_object_{0};
    static constexpr const void* shutdown_ptr = &shutdown_object_;
    static constexpr const int timer_object_{0};
    static constexpr const void* timer_ptr = &timer_object_;
    static constexpr const int schedule_object_{0};
    static constexpr const void* schedule_ptr = &schedule_object_;

    static const constexpr std::chrono::milliseconds default_timeout_{1000};
    static const constexpr std::chrono::milliseconds no_timeout_{0};
    static const constexpr std::size_t max_events_ = 16;
    std::array<struct epoll_event, max_events_> events_;
    std::vector<std::coroutine_handle<>> handles_to_resume_{};

    void process_event_execute(detail::poll_info* pi, poll_status status);
    void process_timeout_execute();
    timed_events::iterator add_timer_token(time_point tp, detail::poll_info& pi);
    void remove_timer_token(timed_events::iterator pos);
    void update_timeout(time_point now);

};
} // namespace coro
