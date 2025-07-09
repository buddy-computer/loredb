#include "varint.h"

namespace loredb::util {

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

util::expected<uint64_t, storage::Error> VarInt::decode(std::span<const uint8_t>& data) {
    uint64_t result = 0;
    uint8_t shift = 0;
    
    for (size_t i = 0; i < data.size(); ++i) {
        uint8_t byte = data[i];
        
        if (shift >= 64) {
            return util::unexpected<storage::Error>(storage::Error{storage::ErrorCode::CORRUPTION, "VarInt too long"});
        }
        
        result |= static_cast<uint64_t>(byte & 0x7F) << shift;
        
        if ((byte & 0x80) == 0) {
            data = data.subspan(i + 1);
            return result;
        }
        
        shift += 7;
    }
    
    return util::unexpected<storage::Error>(storage::Error{storage::ErrorCode::CORRUPTION, "Incomplete VarInt"});
}

uint64_t ZigZag::encode(int64_t value) {
    return (static_cast<uint64_t>(value) << 1) ^ (static_cast<uint64_t>(value >> 63));
}

int64_t ZigZag::decode(uint64_t value) {
    return static_cast<int64_t>((value >> 1) ^ (-(value & 1)));
}

size_t VarInt::encoded_size(uint64_t value) {
    size_t size = 1;
    while (value >= 0x80) {
        size++;
        value >>= 7;
    }
    return size;
}

}  // namespace loredb::util