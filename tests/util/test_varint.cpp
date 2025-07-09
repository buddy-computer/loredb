#include <gtest/gtest.h>
#include "../../src/util/varint.h"

using namespace graphdb::util;

class VarIntTest : public ::testing::Test {
protected:
    uint8_t buffer[VarInt::MAX_ENCODED_SIZE];
};

TEST_F(VarIntTest, EncodeDecode) {
    std::vector<uint64_t> test_values = {
        0, 1, 127, 128, 255, 256, 
        16383, 16384, 65535, 65536,
        2097151, 2097152, 16777215, 16777216,
        UINT64_MAX
    };
    
    for (uint64_t value : test_values) {
        size_t encoded_size = VarInt::encode(value, buffer);
        ASSERT_GT(encoded_size, 0) << "Failed to encode value: " << value;
        ASSERT_LE(encoded_size, VarInt::MAX_ENCODED_SIZE);
        
        uint64_t decoded_value;
        size_t decoded_size = VarInt::decode(std::span<const uint8_t>(buffer, encoded_size), decoded_value);
        ASSERT_EQ(decoded_size, encoded_size) << "Decoded size mismatch for value: " << value;
        ASSERT_EQ(decoded_value, value) << "Decoded value mismatch for value: " << value;
    }
}

TEST_F(VarIntTest, EncodedSize) {
    std::vector<std::pair<uint64_t, size_t>> test_cases = {
        {0, 1}, {127, 1}, {128, 2}, {16383, 2}, {16384, 3},
        {2097151, 3}, {2097152, 4}, {268435455, 4}, {268435456, 5},
        {UINT64_MAX, 10}
    };
    
    for (auto [value, expected_size] : test_cases) {
        size_t calculated_size = VarInt::encoded_size(value);
        ASSERT_EQ(calculated_size, expected_size) << "Size mismatch for value: " << value;
        
        size_t actual_size = VarInt::encode(value, buffer);
        ASSERT_EQ(actual_size, expected_size) << "Actual encode size mismatch for value: " << value;
    }
}

TEST_F(VarIntTest, DecodeInvalidData) {
    // Test with buffer that has all continuation bits set
    std::fill(buffer, buffer + VarInt::MAX_ENCODED_SIZE, 0xFF);
    
    uint64_t value;
    size_t decoded_size = VarInt::decode(std::span<const uint8_t>(buffer, VarInt::MAX_ENCODED_SIZE), value);
    ASSERT_EQ(decoded_size, 0) << "Should fail to decode invalid data";
}

TEST_F(VarIntTest, DecodeEmptyBuffer) {
    uint64_t value;
    size_t decoded_size = VarInt::decode(std::span<const uint8_t>(buffer, 0), value);
    ASSERT_EQ(decoded_size, 0) << "Should fail to decode empty buffer";
}

TEST_F(VarIntTest, EncodeInsufficientBuffer) {
    uint64_t large_value = UINT64_MAX;
    size_t required_size = VarInt::encoded_size(large_value);
    
    // Try to encode with insufficient buffer
    size_t encoded_size = VarInt::encode(large_value, std::span<uint8_t>(buffer, required_size - 1));
    ASSERT_EQ(encoded_size, 0) << "Should fail to encode with insufficient buffer";
}

class ZigZagTest : public ::testing::Test {};

TEST_F(ZigZagTest, EncodeDecodePositive) {
    std::vector<int64_t> test_values = {0, 1, 2, 127, 128, 255, 256, INT64_MAX};
    
    for (int64_t value : test_values) {
        uint64_t encoded = ZigZag::encode(value);
        int64_t decoded = ZigZag::decode(encoded);
        ASSERT_EQ(decoded, value) << "ZigZag mismatch for value: " << value;
    }
}

TEST_F(ZigZagTest, EncodeDecodeNegative) {
    std::vector<int64_t> test_values = {-1, -2, -127, -128, -255, -256, INT64_MIN};
    
    for (int64_t value : test_values) {
        uint64_t encoded = ZigZag::encode(value);
        int64_t decoded = ZigZag::decode(encoded);
        ASSERT_EQ(decoded, value) << "ZigZag mismatch for value: " << value;
    }
}

TEST_F(ZigZagTest, ZigZagProperties) {
    // Test that positive values encode to even numbers
    ASSERT_EQ(ZigZag::encode(0), 0);
    ASSERT_EQ(ZigZag::encode(1), 2);
    ASSERT_EQ(ZigZag::encode(2), 4);
    
    // Test that negative values encode to odd numbers
    ASSERT_EQ(ZigZag::encode(-1), 1);
    ASSERT_EQ(ZigZag::encode(-2), 3);
    ASSERT_EQ(ZigZag::encode(-3), 5);
}