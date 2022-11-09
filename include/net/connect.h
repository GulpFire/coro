#pragma once

#include <string>

namespace coro::net
{
enum class ConnectStatus
{
    CONNECTED,
    INVALID_IP_ADDRESS,
    TIMEOUT,
    ERROR
};

auto to_string(const ConnectStatus& status) -> const std::string&;

} // namespace coro::net
