#pragma once

#include <concepts/awaitable.h>
#include <when_all.h>

#include <condition_variable>
#include <mutex>

namespace coro {
namespace detail {

class SyncWaitEvent
{
    public:
        SyncWaitEvent(bool initially_set = false);
        SyncWaitEvent(const SyncWaitEvent&) = delete;
        SyncWaitEvent(SyncWaitEvent&&) = delete;
        auto operator=(const SyncWaitEvent&) -> SyncWaitEvent& = delete;
        auto operator=(SyncWaitEvent&) -> SyncWaitEvent& = delete;
        ~SyncWaitEvent() = default;

        void set() noexcept;
        void reset() noexcept;
        void wait() noexcept;

    private:
        std::mutex mtx_;
        std::condition_variable cv_;
        bool set_{false};
};

class sync_wait_task_promise_base
{
    public:
        sync_wait_task_promise_base() noexcept = default;
        virtual ~sync_wait_task_promise_base() = default;

        auto initial_suspend() noexcept -> std::suspend_always
        {
            return {};
        }

        auto unhandled_exception() -> void
        {
            exception_ = std::current_exception();
        }

    protected:
        SyncWaitEvent* event_{nullptr};
        std::exception_ptr exception_;
};

template <typename return_type>
class sync_wait_task_promise : public sync_wait_task_promise_base
{
    public:
        using coroutine_type = std::coroutine_handle<sync_wait_task_promise<return_type>>;
        sync_wait_task_promise() noexcept = default;
        ~sync_wait_task_promise() override = default;

        auto start(SyncWaitEvent& event)
        {
            event_ = &event;
            coroutine_type::from_promise(*this).resume();
        }

        auto get_return_object() noexcept 
        {
            return coroutine_type::from_promise(*this);
        }

        auto yield_value(return_type&& value) noexcept
        {
            return_value_ = std::addressof(value);
            return final_suspend();
        }

        auto final_suspend() noexcept
        {
            struct completion_notifier
            {
                auto await_ready() const noexcept
                {
                    return false;
                }

                auto await_suspend(coroutine_type coroutine) const noexcept
                {
                    coroutine.promise().event_->set();
                }

                auto await_resume() noexcept
                {
                
                }
            };

            return completion_notifier{};
        }

        auto result() -> return_type&&
        {
            if (exception_)
            {
                std::rethrow_exception(exception_);
            }

            return static_cast<return_type&&>(*return_value_);
        }
    private:
        std::remove_reference_t<return_type>* return_value_;
};

template <>
class sync_wait_task_promise<void> : public sync_wait_task_promise_base
{
    using coroutine_type = std::coroutine_handle<sync_wait_task_promise<void>>;
    public:
        sync_wait_task_promise() noexcept = default;
        ~sync_wait_task_promise() override = default;

        auto start(SyncWaitEvent& event)
        {
            event_ = &event;
            coroutine_type::from_promise(*this).resume();
        }

        auto get_return_object() noexcept
        {
            return coroutine_type::from_promise(*this);
        }

        auto final_suspend() noexcept
        {
            struct completion_notifier
            {
                auto await_ready() const noexcept 
                {
                    return false;
                }
                
                auto await_suspend(coroutine_type coroutine) const noexcept
                {
                    coroutine.promise().event_->set();
                }

                auto await_resume() noexcept {}
            };

            return completion_notifier{};
        }

        auto return_void() -> void
        {
        
        }

        auto result() -> void
        {
            if (exception_)
            {
                std::rethrow_exception(exception_);
            }
        }
};

template <typename return_type>
class SyncWaitTask
{
    public:
        using promise_type = sync_wait_task_promise<return_type>;
        using coroutine_type = std::coroutine_handle<promise_type>;

        SyncWaitTask(coroutine_type coroutine) noexcept
            : coroutine_(coroutine)
        {
        
        }

        SyncWaitTask(const SyncWaitTask&) = delete;

        SyncWaitTask(SyncWaitTask&& other) noexcept
            : coroutine_(std::exchange(other.coroutine_, coroutine_type{}))
        {

        }

        auto operator=(const SyncWaitTask&) -> SyncWaitTask& = delete;
        
        auto operator=(SyncWaitTask&& other) -> SyncWaitTask&
        {
            if (std::addressof(other) != this)
            {
                coroutine_ = std::exchange(other.coroutine_, coroutine_type{});
            }
            return *this;
        }

        ~SyncWaitTask()
        {
            if (coroutine_)
            {
                coroutine_.destroy();
            }
        }

        auto start(SyncWaitEvent& event) noexcept
        {
            coroutine_.promise().start(event);
        }

        auto return_value() -> decltype(auto)
        {
            if constexpr (std::is_same_v<void, return_type>)
            {
                coroutine_.promise().result();
                return;
            }
            else
            {
                return coroutine_.promise().result();
            }
        }
    private:
        coroutine_type coroutine_;
};

template <
    concepts::awaitable awaitable_type,
    typename return_type = concepts::awaitable_traits<awaitable_type>::awaiter_return_type>
static auto make_sync_wait_task(awaitable_type&& a) -> SyncWaitTask<return_type> __attribute__((used));

template <concepts::awaitable awaitable_type, typename return_type>
static auto make_sync_wait_task(awaitable_type&& a) -> SyncWaitTask<return_type>
{
    if constexpr (std::is_void_v<return_type>)
    {
        co_await std::forward<awaitable_type>(a);
        co_return;
    }
    else
    {
        co_yield co_await std::forward<awaitable_type>(a);
    }
}

} // namespace detail

template <concepts::awaitable awaitable_type>
auto sync_wait(awaitable_type&& a) -> decltype(auto)
{
    detail::SyncWaitEvent e{};
    auto task = detail::make_sync_wait_task(std::forward<awaitable_type>(a));
    task.start(e);
    e.wait();

    return task.return_value();
}

} // namespace coro
