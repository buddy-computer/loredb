/// \file varint.h
/// \brief Variable-length integer encoding and decoding.
/// \author LoreDB contributors
/// \ingroup util
#pragma once

#include <cstdint>
#include <span>
#include "../util/expected.h"
#include "../storage/page_store.h"

namespace loredb::util {

/**
 * @class VarInt
 * @brief Provides static methods for variable-length integer encoding and decoding.
 */
class VarInt {
public:
    static size_t encode(uint64_t value, std::span<uint8_t> buffer);
    static util::expected<uint64_t, storage::Error> decode(std::span<const uint8_t>& data);
    static size_t encoded_size(uint64_t value);
    static constexpr size_t MAX_ENCODED_SIZE = 10;
};

/**
 * @class ZigZag
 * @brief Provides static methods for ZigZag encoding and decoding of signed integers.
 */
class ZigZag {
public:
    static uint64_t encode(int64_t value);
    static int64_t decode(uint64_t value);
};

}  // namespace loredb::util