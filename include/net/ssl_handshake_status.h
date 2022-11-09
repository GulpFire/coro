#pragma once

namespace coro::net
{
enum class SSL_HANDSHAKE_STATUS
{
    OK,
    NOT_CONNECTED,
    SSL_CONTEXT_REQUIRED,
    SSL_RESOURCE_ALLOCATION_FAILED,
    SSL_SET_FD_FAILURE,
    HANDSHAKE_FAILED,
    TIMEOUT,
    POLL_ERROR,
    UNEXPECTED_CLOSE
};
}; // namespace coro::net
