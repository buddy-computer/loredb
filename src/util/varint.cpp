#include "varint.h"

namespace graphdb::util {

size_t VarInt::encode(uint64_t value, std::span<uint8_t> buffer) {
    size_t pos = 0;
    
    while (value >= 0x80 && pos < buffer.size()) {
        buffer[pos++] = static_cast<uint8_t>(value | 0x80);
        value >>= 7;
    }
    
    if (pos < buffer.size()) {
        buffer[pos++] = static_cast<uint8_t>(value);
        return pos;
    }
    
    return 0; // Buffer too small
}

size_t VarInt::decode(std::span<const uint8_t> buffer, uint64_t& value) {
    value = 0;
    size_t pos = 0;
    int shift = 0;
    
    while (pos < buffer.size() && shift < 64) {
        uint8_t byte = buffer[pos++];
        value |= static_cast<uint64_t>(byte & 0x7F) << shift;
        
        if ((byte & 0x80) == 0) {
            return pos;
        }
        
        shift += 7;
    }
    
    return 0; // Invalid encoding
}

size_t VarInt::encoded_size(uint64_t value) {
    size_t size = 1;
    while (value >= 0x80) {
        size++;
        value >>= 7;
    }
    return size;
}

uint64_t ZigZag::encode(int64_t value) {
    return (static_cast<uint64_t>(value) << 1) ^ (static_cast<uint64_t>(value >> 63));
}

int64_t ZigZag::decode(uint64_t value) {
    return static_cast<int64_t>((value >> 1) ^ (-(value & 1)));
}

}  // namespace graphdb::util