#pragma once

// Use C++23 std::expected implementation
#include <expected>

namespace loredb::util {

// Alias std::expected to our namespace
template<typename T, typename E>
using expected = std::expected<T, E>;

template<typename E>
using unexpected = std::unexpected<E>;

}  // namespace loredb::util