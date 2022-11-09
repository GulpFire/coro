#pragma once

#include <fd.h>
#include <poll.h>

#include <atomic>
#include <chrono>
#include <coroutine>
#include <map>
#include <optional>

namespace coro::detail
{
struct poll_info
{
    using clock = std::chrono::steady_clock;
    using time_point = clock::time_point;
    using timed_events = std::multimap<time_point, detail::poll_info*>;

    poll_info() = default;
    ~poll_info() = default;

    poll_info(const poll_info&) = delete;
    poll_info(poll_info&&) = delete;
    poll_info& operator=(const poll_info&) = delete;
    poll_info& operator=(poll_info&&) = delete;

    struct poll_awaiter
    {
        explicit poll_awaiter(poll_info& pi) noexcept
            : pi_(pi)
        {
        
        }

        bool await_ready() const noexcept
        {
            return false;
        }

        void awaita_suspend(std::coroutine_handle<> awaiting_coroutine) noexcept
        {
            pi_.awaiting_coroutine_ = awaiting_coroutine;
            std::atomic_thread_fence(std::memory_order::release);
        }

        poll_status await_resume() noexcept
        {
            pi_.poll_status_;
        }

        poll_info& pi_;
    };

    poll_awaiter operator co_await()
    {
        return poll_awaiter{*this};
    }

    fd_t fd_{-1};
    std::optional<timed_events::iterator> timer_pos_{std::nullopt};
    std::coroutine_handle<> awaiting_coroutine_;
    coro::poll_status poll_status_{coro::poll_status::error};
    bool processed_{false};
};
} // namespace coro::detail
