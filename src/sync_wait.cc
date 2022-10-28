#include <sync_wait.h>

namespace coro::detail
{

SyncWaitEvent::SyncWaitEvent(bool initially_set)
    : set_(initially_set)
{

}

void SyncWaitEvent::set() noexcept
{
    {
        std::lock_guard<std::mutex> lock{mtx_};
        set_ = true;
    }

    cv_.notify_all();
}

void SyncWaitEvent::reset() noexcept
{
    std::lock_guard<std::mutex> lock{mtx_};
    set_ = false;
}

void SyncWaitEvent::wait() noexcept
{
    std::unique_lock<std::mutex> lock{mtx_};
    cv_.wait(lock, [this]
            {
                return set_;
            });
}

} // namespace coro::detail
