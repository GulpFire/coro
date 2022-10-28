#pragma once

#include <sys/epoll.h>

namespace coro
{
enum class PollOption : uint64_t
{
    READ = EPOLLIN,
    WRITE = EPOLLOUT,
    READ_WRITE = EPOLLIN | EPOLLOUT
};

inline bool poll_op_readable(PollOption op)
{
    return (static_cast<uint64_t>(op) & PollOption::READ);
}

inline bool poll_op_writeable(PollOption op)
{
    return (static_cast<uint64_t>(op) & PollOption::WRITE);
}

enum class PollStatus
{
    EVENT,
    TIMEOUT,
    ERROR,
    CLOSED
};

} // namespace coro
