#pragma once

namespace coro
{
class asio_scheduler
{
    using clock = detail::poll_info::clock;
    using time_point = detail::poll_info::time_point;
    using timed_events = detail::poll_info::timed_events;

};

} // namespace coro
