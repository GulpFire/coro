#pragma once

#include <arpa/inet.h>

#include <array>
#include <cstring>
#include <span>
#include <stdexcept>
#include <string>

namespace coro::net
{
enum class Domain : int
{
    IPv4 = AF_INET,
    IPv6 = AF_INET6
};

const std::string& to_string(Domain domain);

class IPAddress
{
    public:
        static const constexpr size_t ipv4_len{4};
        static const constexpr size_t ipv6_len{16};

        IPAddress() = default;
        IPAddress(std::span<const uint8_t> binary_address, Domain domain = Domain::IPv4)
            : domain_(domain)
        {
            if (domain_ == Domain::IPv4 && binary_address.size() > ipv4_len)
            {
                throw std::runtime_error{"binary ip address too long"};
            }
            else if (binary_address.size() > ipv6_len)
            {
                throw std::runtime_error{"binary ip address too long"};
            }
            
            std::copy(binary_address.begin(), binary_address.end(), data_.begin());
        }

        IPAddress(const IPAddress&) = default;
        IPAddress(IPAddress&&) = default;
        IPAddress& operator=(const IPAddress&) = default;
        IPAddress& operator=(IPAddress&&) = default;
       
        ~IPAddress() = default;

        Domain domain() const
        {
            return domain_;
        }

        std::span<const uint8_t> data() const
        {
            if (domain_ == Domain::IPv4)
            {
                return std::span<const uint8_t>{data_.data(), ipv4_len};
            }
            else
            {
                return std::span<const uint8_t>{data_.data(), ipv6_len};
            }
        }

        static IPAddress from_string(const std::string& address, Domain domain = Domain::IPv4)
        {
            IPAddress addr{};
            addr.domain_ = domain;

            auto success = inet_pton(static_cast<int>(addr.domain_), address.data(), addr.data_.data());

            if (success != 1)
            {
                throw std::runtime_error{"failed to convert IPAddress from string"};
            }
            return addr;
        }

        std::string to_string() const
        {
            std::string output;
            if (domain_ == Domain::IPv4)
            {
                output.resize(INET_ADDRSTRLEN, '\0');
            }
            else
            {
                output.resize(INET6_ADDRSTRLEN, '\0');
            }

            auto success = inet_ntop(static_cast<int>(domain_), data_.data(),
                    output.data(), output.length());

            if (success != nullptr)
            {
                auto len = strnlen(success, output.length());
                output.resize(len);
            }
            else
            {
                throw std::runtime_error{"failed to convert to string"};
            }
            return output;
        }

        auto operator<=>(const IPAddress& other) const = default;

    private:
        Domain domain_{Domain::IPv4};
        std::array<uint8_t, ipv6_len> data_{};
};
} // namespace coro::net
