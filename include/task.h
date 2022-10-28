#pragma once

#include <concepts/promise.h>

#include <coroutine>
#include <exception>
#include <utility>

namespace coro
{
template <typename return_type = void>
class Task;

namespace detail
{

struct promise_base
{
    friend struct final_awaitable;
    struct final_awaitable
    {
        auto await_ready() const noexcept -> bool { return false; }

        template <typename promise_type>
        auto await_suspend(std::coroutine_handle<promise_type> coroutine) noexcept -> std::coroutine_handle<>
        {
            auto& promise = coroutine.promise();
            if (promise.continuation_ != nullptr)
            {
                return promise.continuation_;
            }
            else
            {
                return std::noop_coroutine();
            }
        }

        void await_resume() noexcept
        {
        
        }
    };

    promise_base() noexcept = default;
    ~promise_base() = default;

    auto initial_suspend() { return std::suspend_always{}; }

    auto final_suspend() noexcept(true) { return final_awaitable{}; }
    
    void unhandled_exception() { p_exception_ = std::current_exception(); }

    void continuation(std::coroutine_handle<> continuation) noexcept { continuation_ = continuation; }

    protected:
        std::coroutine_handle<> continuation_{nullptr};
        std::exception_ptr p_exception_{};
};

template <typename return_type>
struct promise final : public promise_base
{
    using task_type = Task<return_type>;
    using coroutine_handle = std::coroutine_handle<promise<return_type>>;

    promise() noexcept = default;
    ~promise() = default;

    auto get_return_object() noexcept -> task_type;
    
    void return_value(return_type value) { return_value_ = std::move(value); }

    auto result() const& -> const return_type&
    {
        if (p_exception_)
        {
            std::rethrow_exception(p_exception_);
        }

        return return_value_;
    }

    auto result() && -> return_type&&
    {
        if (p_exception_)
        {
            std::rethrow_exception(p_exception_);
        }
        return std::move(return_value_);
    }

    private:
        return_type return_value_;
};

template <>
struct promise<void> : public promise_base
{
    using task_type = Task<void>;
    using coroutine_handle = std::coroutine_handle<promise<void>>;

    promise() noexcept = default;
    ~promise() = default;

    auto get_return_object() noexcept -> task_type;

    auto return_void() noexcept -> void {}

    auto result() -> void
    {
        if (p_exception_)
        {
            std::rethrow_exception(p_exception_);
        }
    }
};

} // namespace detail

template <typename return_type>
class [[nodiscard]] Task
{
    public:
        using task_type = Task<return_type>;
        using promise_type = detail::promise<return_type>;
        using coroutine_handle = std::coroutine_handle<promise_type>;

        struct awaitable_base
        {
            awaitable_base(coroutine_handle coroutine) noexcept : coroutine_(coroutine) {}

            auto await_ready() const noexcept -> bool { return !coroutine_ || coroutine_.done(); }

            auto await_suspend(std::coroutine_handle<> awaiting_coroutine) noexcept -> std::coroutine_handle<>
            {
                coroutine_.promise().continuation(awaiting_coroutine);
                return coroutine_;
            }

            std::coroutine_handle<promise_type> coroutine_{nullptr};
        };

        Task() noexcept : coroutine_(nullptr) {}
        
        explicit Task(coroutine_handle handle) : coroutine_(handle) {}

        Task(const Task&) = delete;

        Task(Task&& other) noexcept : coroutine_(std::exchange(other.coroutine_, nullptr)) { }

        ~Task()
        {
            if (coroutine_ != nullptr)
            {
                coroutine_.destroy();
            }
        }

        auto operator=(const Task&) -> Task& = delete;

        auto operator=(Task&& other) noexcept -> Task&
        {
            if (std::addressof(other) != this)
            {
                if (coroutine_ != nullptr)
                {
                    coroutine_.destory();
                }

                coroutine_ = std::exchange(other.coroutine_, nullptr);
            }
            return *this;
        }

        auto is_ready() const noexcept -> bool { return coroutine_ == nullptr || coroutine_.done(); }

        auto resume() -> bool
        {
            if (!coroutine_.done())
            {
                coroutine_.resume();
            }
            return !coroutine_.done();
        }

        auto destory() -> bool
        {
            if (coroutine_ != nullptr)
            {
                coroutine_.destory();
                coroutine_ = nullptr;
                return true;
            }

            return false;
        }

        auto operator co_await() const& noexcept
        {
            struct awaitable : public awaitable_base
            {
                auto await_resume() -> decltype(auto)
                {
                    if constexpr (std::is_same_v<void, return_type>)
                    {
                        this->coroutine_.promise().result();
                        return;
                    }
                    else
                    {
                        return this->coroutine_.promise().result();
                    }
                }
            };
            return awaitable(coroutine_);
        }

        auto operator co_await() const&& noexcept
        {
            struct awaitable : public awaitable_base
            {
                auto await_resume() -> decltype(auto)
                {
                    if constexpr (std::is_same_v<void, return_type>)
                    {
                        this->coroutine_.promise().result();
                        return;
                    }
                    else
                    {
                        return std::move(this->coroutine_.promise()).result();
                    }
                }
            };

            return awaitable(coroutine_);
        }

        auto promise()& -> promise_type& { return coroutine_.promise(); }

        auto promise() const& -> const promise_type& { return coroutine_.promise(); }

        auto promise()&& -> promise_type&& { return std::move(coroutine_.promise()); }

        auto handle() -> coroutine_handle { return coroutine_; }
        
    private:
        coroutine_handle coroutine_{nullptr};
};

namespace detail
{
template <typename return_type>
inline auto promise<return_type>::get_return_object() noexcept -> Task<return_type>
{
    return Task<return_type>{coroutine_handle::from_promise(*this)};
}

inline auto promise<void>::get_return_object() noexcept -> Task<>
{
    return Task<>{coroutine_handle::from_promise(*this)};
}
} // namespace detail

} // namespace coro
