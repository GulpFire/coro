#pragma once

#include <concepts>
#include <ranges>

namespace coro::concepts
{
template <typename T, typename V>
concept range_of = std::ranges::range<T>&& std::is_same_v<V, std::ranges::range_value_t<T>>;

template <typename T, typename V>
concept sized_range_of = std::ranges::sized_range<T>&& std::is_same_v<V, std::ranges::range_value_t<T>>;
}
