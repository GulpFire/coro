#pragma once

#include <concepts/executor.h>

#include <atomic>
#include <coroutine>

namespace coro
{
enum class ResumeOrderPolicy
{
    LIFO,
    FIFO
};

class Event
{
    public:
        struct awaiter
        {
            awaiter(const Event& e) noexcept
                : event_(e)
            {

            }

            bool await_ready() const noexcept
            {
                return event_.is_set();
            }

            bool await_suspend(std::coroutine_handle<> awaiting_coroutine) noexcept;

            void await_resume() noexcept 
            {

            }

            const Event& event_;
            std::coroutine_handle<> awaiting_coroutine_;
            awaiter* next_{nullptr};
        };

        explicit Event(bool initially_set = false) noexcept;
        ~Event() = default;

        Event(const Event&) = delete;
        
        Event(Event&&) = delete;
        
        Event& operator=(const Event&) = delete;
        
        Event& operator=(Event&&) = delete;

        bool is_set() const noexcept
        {
            return state_.load(std::memory_order_acquire) == this;
        }

        void set(ResumeOrderPolicy policy = ResumeOrderPolicy::LIFO) noexcept;

        template <concepts::executor executor_type>
        void set(executor_type& e, ResumeOrderPolicy policy = ResumeOrderPolicy::LIFO) noexcept
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
                    e.resume(waiters->awaiting_coroutine_);
                    waiters = next;
                }
            }
        }

        awaiter operator co_await() const noexcept
        {
            return awaiter(*this);
        }

        void reset() noexcept;

    protected:
        friend struct awaiter;
        mutable std::atomic<void*> state_;
    private:
        auto reverse(awaiter* head) -> awaiter*;
};  

} // namespace coro
