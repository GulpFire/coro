#pragma once

#include <net/ip_address.h>
#include <poll.h>

#include <arpa/inet.h>
#include <fcntl.h>
#include <span>
#include <unistd.h>
#include <utility>

#include <iostream>

namespace coro::net
{
class Socket
{
    public:
        enum class Kind
        {
            UDP,
            TCP
        };

        enum class Blocking
        {
            YES,
            NO
        };

        struct options
        {
            domain_t domain;
            Kind kind;
            Blocking blocking;
        };

        static int kind_to_os(Kind kind);

        Socket() = default;
        
        explicit Socket(int fd)
            : fd_(fd)
        {
        
        }

        Socket(const Socket&) = delete;
        Socket(Socket&& other)
            : fd_(std::exchange(other.fd_, -1))
        {
        }

        Socket& operator=(const Socket&) = delete;
        Socket& operator=(Socket&& other) noexcept;

        ~sock() 
        {
            close();
        }

        bool is_valid()
        {
            return fd_ != -1;
        }

        bool blocking(blocking_t block);

        bool shutdown(poll_op how = poll_op::read_write);

        void close();

        int native_handle() const
        {
            return fd_;
        }
    private:
        int fd_{-1};
};

Socket make_socket(const Socket::options& opts);

Socket make_accept_socket(const socket::options& opts,
        const net::ip_address& address, uint16_t port, int32_t backlog = 128);
} // namespace coro::net
