#pragma once

#include <cstdint>
#include <errno.h>
#include <string>

namespace coro::net
{
enum class ReceiveStatus : int64_t
{
    OK = 0,
    CLOSED = -1,
    UDP_NOT_BOUND = -2,
    TRY_AGAIN = EAGAIN,
    WOULD_BLOCK = EWOULDBLOCK,
    BAD_FILE_DESCRIPTOR = EBADF,
    CONNECTION_REFUSED = ECONNREFUSED,
    MEMORY_FAULT = EFAULT,
    INTERRUPTED = EINTR,
    INVALID_ARGUMENT = EINVAL,
    NO_MEMORY = ENOMEM,
    NOT_CONNECTED = ENOTCONN,
    NOT_A_SOCKET = ENOTSOCK,

    SSL_ERROR = -3
};

const std::string& to_string(ReceiveStatus status);

} // namespace coro::net
