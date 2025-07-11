#pragma once

// Use TartanLlama's expected implementation
#include <tl/expected.hpp>

namespace loredb::util {

// Alias TartanLlama's expected to our namespace
template<typename T, typename E>
using expected = tl::expected<T, E>;

template<typename E>
using unexpected = tl::unexpected<E>;

}  // namespace loredb::util