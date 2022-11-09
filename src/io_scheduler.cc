#include <io_scheduler.h>

#include <atomic>
#include <cstring>
#include <optional>

#include <sys/epoll.h>
#include <sys/eventfd.h>
#include <sys/socket.h>
#include <sys/timerfd.h>
#include <sys/types.h>
#include <unistd.h>

using namespace std::chrono_literals;

namespace coro
{
IOScheduler::IOScheduler(options opts)
    : opts_(std::move(opts))
    , epoll_fd_(epoll_create1(EPOLL_CLOEXEC))
    , shutdown_fd_(eventfd(0, EFD_CLOEXEC | EFD_NONBLOCK))
    , timer_fd_(timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK | TFD_CLOEXEC))
    , schedule_fd_(eventfd(0, EFD_CLOEXEC | EFD_NONBLOCK))
    , owned_tasks_(new coro::task_container<coro::io_scheduler>(*this))
{
    if (opts.execution_strategy == ExecutionStrategy::PROCESS_TASKS_ON_THREAD_POOL)
    {
        thread_pool_ = std::make_unique<ThreadPool>(std::move(opts_.pool));
    }

    epoll_event e{};
    
    e.events = EPOLLIN;
    e.data.ptr = const_cast<void*>(shutdown_ptr_);
    epoll_ctl(epoll_fd_, EPOLL_CTL_ADD, shutdown_fd_, &e);

    e.data.ptr = const_cast<void>(timer_ptr_);
    epoll_ctl(epoll_fd_, EPOLL_CTL_ADD, timer_fd_, &e);

    e.data.ptr = const_cast<void*>(schedule_ptr_);
    epoll_ctl(epoll_fd_, EPOLL_CTL_ADD, schedule_fd_, &e);

    if (opts_.thread_strategy == ThreadStrategy::SPAWN)
    {
        io_thread_ = std::thread([this]()
                {
                    process_events_dedicated_thread();
                });
    }
}

IOScheduler::~IOScheduler()
{
    shutdown();

    if (io_thread_.joinable())
    {
        io_thread_.join();
    }

    if (epoll_fd_ != -1)
    {
        close(epoll_fd_);
        epoll_fd_ = -1;
    }
     
    if (timer_fd_ != -1)
    {
        close(timer_fd_);
        timer_fd_ = -1;
    }
     
    if (schedule_fd_ != -1)
    {
        close(schedule_fd_);
        schedule_fd_ = -1;
    }
  
    if (owned_tasks != nullptr)
    {
        delete static_cast<coro::task_container<coro::IOScheduler>*>(owned_tasks_);
        owned_tasks_ = nullptr;
    }
}

std::size_t IOScheduler::process_events(std::chrono::milliseconds timeout)
{
    process_events_manual(timeout);
    return size();
}

auto IOScheduler::schedule_after(std::chrono::milliseconds amound) -> coro::task<void>
{
    return yield_for(amount);
}

auto IOScheduler::schedule_at(time_point time) -> coro::task<void>
{
    return yield_until(time);
}

auto IOScheduler::yield_for()
}
