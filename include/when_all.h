#pragma once

#include <concepts/awaitable.h>
#include <detail/void_value.h>

#include <atomic>
#include <coroutine>
#include <ranges>
#include <tuple>
#include <vector>

namespace coro
{
namespace detail
{
class when_all_latch
{
    public:
        when_all_latch(std::size_t count) noexcept 
            : count_(count + 1)
        {

        }

        when_all_latch(const when_all_latch&) = delete;

        when_all_latch(when_all_latch&& other)
            : count_(other.count_.load(std::memory_order::acquire))
            , awaiting_coroutine_(std::exchange(other.awaiting_coroutine_, nullptr))
        {

        }

        when_all_latch& operator=(const when_all_latch&) = delete;
        
        when_all_latch& operator=(when_all_latch&& other)
        {
            if (std::addressof(other) != this)
            {
                count_.store(other.count_.load(std::memory_order::acquire),
                        std::memory_order::relaxed);
                awaiting_coroutine_ = std::exchange(other.awaiting_coroutine_, nullptr);
            }
            return *this;
        }

        bool is_ready() const noexcept
        {
            return awaiting_coroutine_ != nullptr
                && awaiting_coroutine_.done();
        }

        bool try_await(std::coroutine_handle<> awaiting_coroutine) noexcept
        {
            awaiting_coroutine_ = awaiting_coroutine_;
            return count_.fetch_sub(1, std::memory_order::acq_rel) > 1;
        }

        void notify_awaitable_compeleted() noexcept
        {
            if (count_.fetch_sub(1, std::memory_order::acq_rel) == 1)
            {
                awaiting_coroutine_.resume();                                       
            }
        }

    private:
        std::atomic<std::size_t> count_;
        std::coroutine_handle<> awaiting_coroutine_{nullptr};
};

template <typename task_container_type>
class when_all_ready_awaitable;

template <typename return_type>
class when_all_tasks;

template <>
class when_all_ready_awaitable<std::tuple<>>
{
    public:
        constexpr when_all_ready_awaitable() noexcept
        {

        }

        explicit constexpr when_all_ready_awaitable(std::tuple<>) noexcept
        {
        
        }

        constexpr bool await_ready() const noexcept { return true; }
        void await_suspend(std::coroutine_handle<>) noexcept { }
        std::tuple<> await_resume() const noexcept { return {}; }
};

template <typename... task_types>
class when_all_ready_awaitable<std::tuple<task_types...>>
{
    public:
        explicit when_all_ready_awaitable(task_types&&... tasks) noexcept(
                std::conjunction_v<std::is_nothrow_move_constructible_v<task_types>...>)
            : latch_(sizeof...(task_types))
            , tasks_(std::move(tasks)...)
        {
        
        }

        explicit when_all_ready_awaitable(std::tuple<task_types...>&& tasks) noexcept(
                std::is_nothrow_move_constructible_v<std::tuple<task_types...>>)
            : latch_(sizeof...(task_types))
            , tasks_(std::move(tasks))
        {
        
        }

        when_all_ready_awaitable(const when_all_ready_awaitable&) = delete;

        when_all_ready_awaitable(when_all_ready_awaitable&& other)
            : latch_(std::move(other.latch_))
            , tasks_(std::move(other.tasks_))
        {
        
        }

        when_all_ready_awaitable& operator=(const when_all_ready_awaitable&) = delete;
        when_all_ready_awaitable& operator=(when_all_ready_awaitable&&) = delete;
        
        auto operator co_await() & noexcept
        {
            struct awaiter
            {
                explicit awaiter(when_all_ready_awaitable& awaitable) noexcept
                    : awaitable_(awaitable)
                {
                
                }

                bool await_ready() const noexcept
                {
                    return awaitable_.is_ready();
                }

                bool await_suspend(std::coroutine_handle<> awaiting_coroutine) noexcept
                {
                    return awaitable_.try_await(awaiting_coroutine);
                }

                std::tuple<task_types...>& await_resume()
                {
                    return awaitable_.tasks_;
                }
                private:
                    when_all_ready_awaitable& awaitable_;
            };
            return awaiter{*this};
        }

        auto operator co_await() && noexcept
        {
            struct awaiter
            {
                explicit awaiter(when_all_ready_awaitable& awaitable) noexcept
                    : awaitable_(awaitable)
                {
                
                }

                bool await_ready() const noexcept
                {
                    return awaitable_.is_ready();
                }

                bool await_suspend(std::coroutine_handle<> awaiting_coroutine) noexcept
                {
                    return awaitable_.try_await(awaiting_coroutine);
                }

                auto await_resume() noexcept -> std::tuple<task_types...>&&
                {
                    return std::move(awaitable_.tasks_);
                }
                private:
                    when_all_ready_awaitable& awaitable_;
            };
            return awaiter{*this};
        }

    private:
        when_all_latch latch_;
        std::tuple<task_types...> tasks_;

        bool is_ready() const noexcept
        {
            return latch_.is_ready();
        }

        bool try_await(std::coroutine_handle<> awaiting_coroutine) noexcept
        {
            std::apply([this](auto&&... tasks) { ((tasks.start(latch_)), ...); }, tasks_);
            return latch_.try_await(awaiting_coroutine);
        }
};

template <typename task_container_type>
class when_all_ready_awaitable
{
    public:
        explicit when_all_ready_awaitable(task_container_type&& tasks) noexcept
            : latch_(std::size(tasks))
            , tasks_(std::forward<task_container_type>(tasks))
        {
        
        }

        when_all_ready_awaitable(const when_all_ready_awaitable&) = delete;

        when_all_ready_awaitable(when_all_ready_awaitable&& other) noexcept(std::is_nothrow_move_constructible_v<task_container_type>) 
            : latch_(std::move(other.latch_))
            , tasks_(std::move(tasks_))
        {
        
        }

        when_all_ready_awaitable& operator=(const when_all_ready_awaitable&)= delete;
        when_all_ready_awaitable& operator=(when_all_ready_awaitable&) = delete;

        auto operator co_await() & noexcept
        {
            struct awaiter
            {
                awaiter(when_all_ready_awaitable& awaitable)
                    : awaitable_(awaitable)
                {
                
                }

                bool await_ready() const noexcept
                {
                    return awaitable_.is_ready();
                }

                bool await_suspend(std::coroutine_handle<> awaiting_coroutine) noexcept
                {
                    return awaitable_.try_await(awaiting_coroutine);
                }

                task_container_type& await_resume()
                {
                    return awaitable_.tasks_;
                }

                private:
                    when_all_ready_awaitable& awaitable_;
            };
            return awaiter{*this};
        }

        auto operator co_await() && noexcept
        {
            struct awaiter
            {
                awaiter(when_all_ready_awaitable& awaitable)
                    : awaitable_(awaitable)
                {

                }

                bool await_ready() const noexcept
                {
                    return awaitable_.is_ready();
                }

                bool await_suspend(std::coroutine_handle<> awaiting_coroutine) noexcept
                {
                    return awaitable_.try_await(awaiting_coroutine);
                }

                task_container_type&& await_resume() noexcept
                {
                    return std::move(awaitable_.tasks_);
                }

                private:
                    when_all_ready_awaitable& awaitable_;
            };

            return awaiter{*this};
        }
        
    private:
        when_all_latch latch_;
        task_container_type tasks_;

        bool is_ready() const
        {
            return latch_.is_ready();
        }

        bool try_await(std::coroutine_handle<> awaiting_coroutine) noexcept
        {
            std::apply([this](auto&... tasks)
                    {
                        ((tasks.start(latch_), ...));
                    }, tasks_);
            return latch_.try_await(awaiting_coroutine);
        }
};

template <typename return_type>
class when_all_task_promise
{
    public:
        using coroutine_handle_type = std::coroutine_handle<when_all_task_promise<return_type>>;

        when_all_task_promise() noexcept 
        {
        
        } 

        auto get_return_object noexcept 
        { 
            return coroutine_handle_type::from_promise(*this); 
        }

        std::suspend_always initial_suspend() noexcept
        {
            return {};
        }

        auto final_suspend() noexcept
        {
            struct completion_notifier
            {
                bool await_ready() const noexcept
                {
                    return false;
                }

                void await_suspend(coroutine_handle_type coroutine) const noexcept
                {
                    coroutine.promise().latch_->notify_awaitable_completed();
                }

                void await_resume() const noexcept
                {
            
                }
            };
            return completion_notifer{};
        }

        void unhandled_exception() noexcept
        {
            
        }

        auto yield_value(return_type&& value) noexcept
        {
            return_value_ = std::addressof(value);
            return final_suspend();
        }

        void start(when_all_latch& latch) noexcept
        {
            latch_ = &latch;
            coroutine_handle_type::from_promise(*this).resume();
        }

        return_type& return_value()
        {
            if (p_exception_)
            {
                std::rethrow_exception(p_exception_);
            }
            return *return_value_;
        }

        return_type&& return_value()
        {
            if (p_exception)
            {
                std::rethrow_exception(p_exception_);
            }
            return std::forward(*return_value_);
        }

    private:
        when_all_latch* latch_{nullptr};
        std::exception_ptr p_exception_;
        std::add_pointer_t<return_type> return_value_;
};

template <>
class when_all_task_promise<void>
{
    using coroutine_handle_type = std::coroutine_handle<when_all_task_promise<void>>;

    when_all_task_promise() noexcept
    {
    
    }

    auto get_return_object() noexcept
    {
        return coroutine_handle_type::from_promise(*this);
    }

    std::suspend_always initial_suspend() noexcept
    {
        return {};
    }

    auto final_suspend() noexcept
    {
        struct compelition_notifier
        {
            bool await_ready() const noexcept
            {
                return false;
            }

            void await_suspend(coroutine_handle_type coroutine) const noexcept
            {
                coroutine.promise().latch_->notify_awaitable_completed();
            }

            void await_resume() const noexcept
            {
            
            }
        };

        return completion_notifier{};
    }

    void unhandled_exception() noexcept
    {
        p_exception_ = std::current_exception();
    }

    void return_void() noexcept
    {
    
    }

    void result()
    {
        if (p_exception_)
        {
            std::rethrow_exception(p_exception_);
        }
    }

    void start(when_all_latch& latch)
    {
        latch_ = &latch;
        coroutine_handle_type::from_promise(*this).resume();
    }

    private:
        when_all_latch* latch_{nullptr};
        std::exception_ptr p_exception_;
};

template <typename return_type>
class when_all_task
{
    public:
        template <typename task_container_type>
        friend class when_all_ready_awaitable;

        using promise_type = when_all_task_promise<return_type>;
        using coroutine_handle_type = typename promise_type::coroutine_handle_type;

        when_all_task(coroutine_handle_type coroutine) noexcept
            : coroutine_(coroutine)
        {
        
        }

        when_all_task(const when_all_task&) = delete;

        when_all_task(when_all_task&& other) noexcept
            : coroutine_(std::exchange(other.coroutine_, coroutine_handle_type{}))
        {
        
        }

        when_all_task& operator=(const when_all_task&) = delete;

        when_all_task&& operator=(when_all_task&&) = delete;

        ~when_all_task()
        {
            if (coroutine_ != nullptr)
            {
                coroutine_.destroy();
            }
        }

        auto return_value() & -> decltype(auto)
        {
            if constexpr (std::is_void_v<return_type>)
            {
                coroutine_.promise().result();
                return void_value{};
            }
            else
            {
                return coroutine_.promise().return_value();
            }
        }

        auto return_value() const& -> decltype(auto)
        {
            if constexpr (std::is_void_v<return_type>)
            {
                coroutine_.promise().result();
                return void_value{};
            }
            else
            {
                return coroutine_.promise().return_value();
            }
        }

        auto return_value() && -> decltype(auto)
        {
            if constexpr (std::is_void_v<return_type>)
            {
                coroutine_.promise().result();
                return void_value{};
            }
            else
            {
                return coroutine_.promise().return_value();
            }
        }

    private:
        void start(when_all_latch& latch) noexcept
        {
            coroutine_.promise().start(latch);
        }

        coroutine_handle_type coroutine_;
};

template <
    concepts::awaitable awaitable,
    typename return_type = concepts::awaitable_traits<awaitable&&>::awaiter_return_type>
static auto make_when_all_task(awaitable a) -> when_all_task<return_type> __attribute__((used));

template <concepts::awaitable awaitable, typename return_type>
static auto make_when_all_task(awaitable a) -> when_all_task<return_type>
{
    if constexpr (std::is_Void_v<return_type>)
    {
        co_await static_cast<awaitable&&>(a);
        co_return;
    }
    else
    {
        co_yield co_await static_cast<awaitable&&>(a);
    }
}

} // namespace detail

template <concepts::awaitable... awaitable_type>
[[nodiscard]] auto when_all(awaitable_type... awaitables)
{
    return detail::when_all_ready_awaitable<std::tuple<
        detail::when_all_task<typename concepts::awaitable_traits<awaitables_type>::awaiter_return_type>...>>(
                std::make_tuple(detial::make_when_all_task(std::move(awaitables))...));
}

template <
    std::ranges::range range_type,
    concepts::awaitable awaitable_type = std::ranges::range_value_t<range_type>,
    typename return_type = concepts::awaitable_traits<awaitable_type>::awaiter_return_type>
[[nodiscard]] auto when_all(range_type awaitables) 
    -> detail::when_all_ready_awaitable<std::vector<detail::when_all_task<return_type>>>
{
    std::vector<detail::when_all_task<return_type>> output_tasks;
    if constexpr (std::ranges::sized_range<range_type>)
    {
        output_tasks.reserve(std::size(awaitables));
    }

    for (auto& a : awaitables)
    {
        output_tasks.emplace_back(detail::make_when_all_task(std::move(a)));
    }

    return detail::when_all_ready_awaitable(std::move(output_tasks));
}
} // namespace coro
