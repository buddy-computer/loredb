#include <gtest/gtest.h>
#include "../../src/util/varint.h"
#include <span>

using namespace loredb;

TEST(VarIntTest, EncodeDecode) {
    uint64_t value;
    uint8_t buffer[util::VarInt::MAX_ENCODED_SIZE];
    
    size_t encoded_size = util::VarInt::encode(12345, buffer);
    std::span<const uint8_t> span(buffer, encoded_size);
    auto decoded_result = util::VarInt::decode(span);
    
    ASSERT_TRUE(decoded_result.has_value());
    EXPECT_EQ(decoded_result.value(), 12345);
}

TEST(VarIntTest, EncodedSize) {
    ASSERT_EQ(util::VarInt::encoded_size(0), 1);
    ASSERT_EQ(util::VarInt::encoded_size(127), 1);
    ASSERT_EQ(util::VarInt::encoded_size(128), 2);
    ASSERT_EQ(util::VarInt::encoded_size(16383), 2);
    ASSERT_EQ(util::VarInt::encoded_size(16384), 3);
    ASSERT_EQ(util::VarInt::encoded_size(2097151), 3);
    ASSERT_EQ(util::VarInt::encoded_size(2097152), 4);
    ASSERT_EQ(util::VarInt::encoded_size(268435455), 4);
    ASSERT_EQ(util::VarInt::encoded_size(268435456), 5);
}

TEST(VarIntTest, DecodeInvalidData) {
    uint8_t buffer[util::VarInt::MAX_ENCODED_SIZE];
    std::fill(buffer, buffer + util::VarInt::MAX_ENCODED_SIZE, 0xFF);
    
    std::span<const uint8_t> span(buffer, util::VarInt::MAX_ENCODED_SIZE);
    auto result = util::VarInt::decode(span);
    EXPECT_FALSE(result.has_value());
}

TEST(VarIntTest, DecodeEmptyBuffer) {
    std::span<const uint8_t> empty_span;
    auto result = util::VarInt::decode(empty_span);
    EXPECT_FALSE(result.has_value());
}

TEST(VarIntTest, EncodeInsufficientBuffer) {
    uint8_t buffer[1];
    size_t encoded_size = util::VarInt::encode(16384, buffer); // Requires 3 bytes
    EXPECT_EQ(encoded_size, 0);
}