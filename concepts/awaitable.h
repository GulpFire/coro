#pragma once

#include <concepts>
#include <coroutine>
#include <type_traits>
#include <utility>

/*
 * The Awaitable interface specifies methods that control the semantics of a 
 * co_await expression. When a value is co_awaited, the code is translated into
 * a series of calls to methods on the awaitable object that allow it to 
 * specify: whether to suspend the current coroutine, execute some logic after
 * it has suspended to schedule the coroutine for later resumption, and execute
 * some logic after the coroutine resumes to produce the result of the 
 * co_await expression.
 * 
 * A type that supports the co_await operator is called an Awaitable type.
 *
 * An Awaiter type is a type that implements the three special methods that 
 * are called as part of a co_await expression: await_ready, await_suspend 
 * and await_resume.
 */

namespace coro::concepts
{

template <typename type>
concept awaiter = requires(type t, std::coroutine_handle<> co)
{
    { t.await_ready() } ->std::same_as<bool>;
    requires std::same_as<decltype(t.await_suspend(c)), void>
        || std::same_as<decltype(t.await_suspend(c)), bool>
        || std::same_as<decltype(t.await_suspend(c)), std::coroutine_handle<>>;
    { t.await_resume() };
};

template <typename type>
concept awaitable = requires(type t)
{
    { t.operator co_await() } ->awaiter;
};

template <typename type>
concept awaiter_void = requires(type t, std::coroutine_handle<>)
{
    { t.await_ready() } -> std::same_as<bool>;
    requires std::same_as<decltype(t.await_suspend(c)), void>
        || std::same_as<decltype(t.await_suspend(c)), bool>
        || std::same_as<decltype(t.await_suspend(c)), std::coroutine_handle<>>;
    { t.await_resume() } -> std::same_as<void>;
};

template <typename type>
concept awaitable_void = requires(type t)
{
    { t.operator co_await() } -> awaiter_void;
};

template <awaitable awaitable>
struct awaitable_traits
{

};

template <awaitable awaitable>
static auto get_awaiter(awaitable&& vlaue)
{
    return std::forward<awaitable>(value).operator co_await();
}

template <awaitable awaitable>
struct awaitable_traits<awaitable>
{
    using awaiter_type = decltype(get_awaiter(std::declval<awaitable>()));
    using awaiter_return_type = 
        decltype(std::declval<awaiter_type>().await_resume());
};

} // namespace coro::concepts
