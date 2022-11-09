#pragma once

#include <string>

namespace coro::net
{
class HostName
{
    public:
        HostName() = default;
        explicit HostName(std::string host_name)
            : host_name_(std::move(host_name))
        {
        
        }

        HostName(const HostName&) = default;
        HostName(HostName&&) = default;
        HostName& operator=(const HostName&) noexcept = default;
        HostName& operator=(HostName&&) noexcept = default;

        ~HostName() = default;

        const std::string& data() const
        {
            return host_name_;
        }

        auto operator<=>(const HostName& ohter) const
        {
            return host_name_ <=> other.host_name_;
        }

    private:
        std::string host_name_;
};
} // namespace coro::net
