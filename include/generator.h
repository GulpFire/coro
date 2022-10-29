#pragma once

#include <coroutine>
#include <type_traits>
#include <exception>
#include <memory>

namespace coro
{

template <typename T>
class Generator;

namespace detail
{

template <typename T>
class generator_promise
{
    public:
        using value_type = std::remove_reference_t<T>;
        using reference_type = std::conditional_t<std::is_reference_v<T>, T, T&>;
        using pointer_type = value_type*;

        generator_promise() = default;

        Generator<T> get_return_object() noexcept;

        auto initial_suspend() const 
        {
            return std::suspend_always{};
        }

        auto final_suspend() const noexcept(true)
        {
            return std::suspend_always{};
        }

        template <typename U = T, std::enable_if_t<!std::is_rvalue_reference<U>::value, int> = 0>
        auto yield_value(std::remove_reference_t<T>& value) noexcept
        {
            value_ = std::addressof(value);
            return std::suspend_always{};
        }

        auto yield_value(std::remove_reference_t<T>&& value) noexcept
        {
            value_ = std::addressof(value);
            return std::suspend_always{};
        }

        void unhandled_exception()
        {
            exception_ = std::current_exception();
        }

        void return_void() noexcept
        {
        
        }

        reference_type value() const noexcept
        {
            return static_cast<reference_type>(*value_);
        }

        template <typename U>
        std::suspend_never await_transform(U&& value) = delete;

        void rethrow_if_exception()
        {
            if (exception_)
            {
                std::rethrow_exception(exception_);
            }
        }

    private:
        pointer_type value_{nullptr};
        std::exception_ptr exception_;
};

struct generator_sentinel
{

};

template <typename T>
class generator_iterator
{
    using coroutine_handle = std::coroutine_handle<generator_promise<T>>;

    public:
        using iterator_category = std::input_iterator_tag;
        using difference_type = std::ptrdiff_t;
        using value_type = typename generator_promise<T>::value_type;
        using reference = typename generator_promise<T>::reference_type;
        using pointer = typename generator_promise<T>::pointer_type;

        generator_iterator() noexcept
        {
        
        }

        explicit generator_iterator(coroutine_handle coroutine) noexcept
            : coroutine_(coroutine)
        {
            
        }

        friend bool operator==(const generator_iterator& it, generator_sentinel s) noexcept
        {
            return it.coroutine_ == nullptr
                || it.coroutine_.done();
        }

        friend bool operator!=(const generator_iterator& it, generator_sentinel s) noexcept
        {
            return !(it == s);
        }

        friend bool operator==(generator_sentinel s, const generator_iterator& it) noexcept
        {
            return (it == s);
        }

        friend bool operator!=(generator_sentinel s, const generator_iterator& it) noexcept
        {
            return (it != s);
        }

        generator_iterator& operator++()
        {
            coroutine_.resume();
            if (coroutine_.done())
            {
                coroutine_.promise().rethrow_if_exception();
            }
            return *this;
        }

        void operator++(int)
        {
            (void)operator++();
        }

        reference operator*() const noexcept
        {
            return coroutine_.promise().value();
        }

        pointer operator->() const noexcept
        {
            return std::addressof(operator*());
        }

    private:
        coroutine_handle coroutine_{nullptr};
};

} // namespace detail

template <typename T>
class Generator
{
    public:
        using promise_type = detail::generator_promise<T>;
        using iterator = detail::generator_iterator<T>;
        using sentinel = detail::generator_sentinel;

        Generator() noexcept
            : coroutine_(nullptr)
        {
        
        }

        Generator(const Generator&) = delete;

        Generator(Generator&& other) noexcept
            : coroutine_(other.coroutine_)
        {
            other.coroutine_ = nullptr;
        }

        Generator& operator=(const Generator&) = delete;

        Generator& operator=(Generator&& other) noexcept
        {
            coroutine_ = other.coroutine_;
            other.coroutine_ = nullptr;
            
            return *this;
        }

        ~Generator()
        {
            if (coroutine_)
            {
                coroutine_.destroy();
            }
        }

        iterator begin()
        {
            if (coroutine_ != nullptr)
            {
                coroutine_.resume();
                if (coroutine_.done())
                {
                    coroutine_.promise().rethrow_if_exception();
                }
            }
            return iterator{coroutine_};
        }

        sentinel end() noexcept
        {
            return sentinel{};
        }

    private:
        friend class detail::generator_promise<T>;

        explicit Generator(std::coroutine_handle<promise_type> coroutine) noexcept
            : coroutine_(coroutine)
        {

        }

        std::coroutine_handle<promise_type> coroutine_;
};

namespace detail
{

template <typename T>
Generator<T> generator_promise<T>::get_return_object() noexcept
{
    return Generator<T>{std::coroutine_handle<generator_promise<T>>::from_promise(*this)};
}

} // namespace detail
} // namespace coro
