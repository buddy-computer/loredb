/// \file varint.h
/// \brief Variable-length integer and zigzag encoding utilities.
/// \author wiki-graph contributors
/// \ingroup util
#pragma once

#include <cstdint>
#include <span>

namespace graphdb::util {

/**
 * @class VarInt
 * @brief Static utility for variable-length integer encoding/decoding.
 */
class VarInt {
public:
    /**
     * @brief Encode a uint64_t value to a byte buffer.
     * @param value Value to encode.
     * @param buffer Output buffer.
     * @return Number of bytes written.
     */
    static size_t encode(uint64_t value, std::span<uint8_t> buffer);
    /**
     * @brief Decode a uint64_t value from a byte buffer.
     * @param buffer Input buffer.
     * @param value Output value.
     * @return Number of bytes consumed, or 0 on error.
     */
    static size_t decode(std::span<const uint8_t> buffer, uint64_t& value);
    /**
     * @brief Calculate the encoded size of a value without actually encoding.
     * @param value Value to check.
     * @return Encoded size in bytes.
     */
    static size_t encoded_size(uint64_t value);
    /**
     * @brief Maximum possible encoded size for any uint64_t.
     */
    static constexpr size_t MAX_ENCODED_SIZE = 10;
};

/**
 * @class ZigZag
 * @brief Static utility for zigzag encoding/decoding of signed integers.
 */
class ZigZag {
public:
    /**
     * @brief Zigzag-encode a signed integer.
     * @param value Signed integer value.
     * @return Encoded unsigned integer.
     */
    static uint64_t encode(int64_t value);
    /**
     * @brief Decode a zigzag-encoded unsigned integer to signed integer.
     * @param value Encoded unsigned integer.
     * @return Decoded signed integer.
     */
    static int64_t decode(uint64_t value);
};

}  // namespace graphdb::util