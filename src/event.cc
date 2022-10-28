#include <event.h>
#include <thread_pool.h>

namespace coro
{

Event::Event(bool initially_set) noexcept
    : state_((initially_set) ? static_cast<void*>(this) : nullptr)
{

}

void Event::set(ResumeOrderPolicy policy) noexcept
{
    void* old_value = state_.exchange(this, std::memory_order::acq_rel);
    if (old_value != this)
    {
        if (policy == ResumeOrderPolicy::FIFO)
        {
            old_value = reverse(static_cast<awaiter*>(old_value));
        }

        auto* waiters = static_cast<awaiter*>(old_value);

        while (waiters != nullptr)
        {
            auto* next = waiters->next_;
            waiters->awaiting_coroutine_.resume();
            waiters = next;
        }
    }
}

auto Event::reverse(awaiter* current) -> awaiter*
{
    if (nullptr == current || nullptr == current->next_)
    {
        return current;
    }

    awaiter* prev = nullptr;
    awaiter* next = nullptr;

    while (current != nullptr)
    {
        next = current->next_;
        current->next_ = prev;
        prev = current;
        current = next;
    }

    return prev;
}

bool Event::awaiter::await_suspend(std::coroutine_handle<> awaiting_coroutine) noexcept
{
    const void* const set_state = &event_;

    awaiting_coroutine_ = awaiting_coroutine;

    void* old_value = event_.state_.load(std::memory_order::acquire);

    do 
    {
        if (old_value == set_state)
        {
            return false;
        }

        next_ = static_cast<awaiter*>(old_value);
    } 
    while (!event_.state_.compare_exchange_weak(
                old_value, this, std::memory_order::release, std::memory_order::acquire));

    return true;
}

void Event::reset() noexcept
{
    void* old_value = this;
    state_.compare_exchange_strong(old_value, nullptr, std::memory_order::acquire);
}

} // namespace coro
