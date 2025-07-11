#include "record.h"
#include <cstring>
#include <bit>

namespace loredb::storage {

// C++23 std::byteswap-based endianness helpers for consistent serialization
namespace {
    constexpr bool is_little_endian() noexcept {
        return std::endian::native == std::endian::little;
    }
    
    // Convert host byte order to little-endian (for consistent serialization)
    template<typename T>
    constexpr T host_to_le(T value) noexcept {
        if constexpr (is_little_endian()) {
            return value;
        } else {
            return std::byteswap(value);
        }
    }
    
    // Convert little-endian to host byte order (for deserialization)
    template<typename T>
    constexpr T le_to_host(T value) noexcept {
        if constexpr (is_little_endian()) {
            return value;
        } else {
            return std::byteswap(value);
        }
    }
}

std::vector<uint8_t> RecordSerializer::serialize_properties(const std::vector<Property>& properties) {
    std::vector<uint8_t> buffer;
    
    // Write property count
    write_varint(buffer, properties.size());
    
    for (const auto& prop : properties) {
        // Write key
        write_string(buffer, prop.key);
        
        // Write value with type
        write_property_value(buffer, prop.value);
    }
    
    return buffer;
}

util::expected<std::vector<Property>, Error> RecordSerializer::deserialize_properties(std::span<const uint8_t> data) {
    std::vector<Property> properties;
    
    // Read property count
    auto count_result = read_varint(data);
    if (!count_result.has_value()) {
        return util::unexpected(count_result.error());
    }
    
    size_t count = count_result.value();
    properties.reserve(count);
    
    for (size_t i = 0; i < count; ++i) {
        // Read key
        auto key_result = read_string(data);
        if (!key_result.has_value()) {
            return util::unexpected(key_result.error());
        }
        
        // Read value type
        if (data.empty()) {
            return util::unexpected(Error{ErrorCode::CORRUPTION, "Unexpected end of data"});
        }
        
        PropertyType type = static_cast<PropertyType>(data[0]);
        data = data.subspan(1);
        
        // Read value
        auto value_result = read_property_value(data, type);
        if (!value_result.has_value()) {
            return util::unexpected(value_result.error());
        }
        
        properties.emplace_back(std::move(key_result.value()), std::move(value_result.value()));
    }
    
    return properties;
}

std::vector<uint8_t> RecordSerializer::serialize_node(const NodeRecord& node, const std::vector<Property>& properties) {
    std::vector<uint8_t> buffer;
    
    // Write node record header with endianness conversion
    buffer.resize(sizeof(NodeRecord));
    
    // Use C++23 std::byteswap for consistent serialization
    NodeRecord le_node;
    le_node.id = host_to_le(node.id);
    le_node.label_count = host_to_le(node.label_count);
    le_node.property_count = host_to_le(node.property_count);
    le_node.in_degree = host_to_le(node.in_degree);
    le_node.out_degree = host_to_le(node.out_degree);
    le_node.property_offset = host_to_le(node.property_offset);
    le_node.in_edges_offset = host_to_le(node.in_edges_offset);
    le_node.out_edges_offset = host_to_le(node.out_edges_offset);
    
    std::memcpy(buffer.data(), &le_node, sizeof(NodeRecord));
    
    // Write properties
    auto prop_data = serialize_properties(properties);
    buffer.insert(buffer.end(), prop_data.begin(), prop_data.end());
    
    return buffer;
}

util::expected<std::pair<NodeRecord, std::vector<Property>>, Error> 
RecordSerializer::deserialize_node(std::span<const uint8_t> data) {
    if (data.size() < sizeof(NodeRecord)) {
        return util::unexpected(Error{ErrorCode::CORRUPTION, "Insufficient data for node record"});
    }
    
    NodeRecord le_node;
    std::memcpy(&le_node, data.data(), sizeof(NodeRecord));
    
    // Use C++23 std::byteswap for consistent deserialization
    NodeRecord node;
    node.id = le_to_host(le_node.id);
    node.label_count = le_to_host(le_node.label_count);
    node.property_count = le_to_host(le_node.property_count);
    node.in_degree = le_to_host(le_node.in_degree);
    node.out_degree = le_to_host(le_node.out_degree);
    node.property_offset = le_to_host(le_node.property_offset);
    node.in_edges_offset = le_to_host(le_node.in_edges_offset);
    node.out_edges_offset = le_to_host(le_node.out_edges_offset);
    
    auto prop_data = data.subspan(sizeof(NodeRecord));
    auto properties_result = deserialize_properties(prop_data);
    if (!properties_result.has_value()) {
        return util::unexpected(properties_result.error());
    }
    
    return std::make_pair(node, std::move(properties_result.value()));
}

std::vector<uint8_t> RecordSerializer::serialize_edge(const EdgeRecord& edge, const std::vector<Property>& properties) {
    std::vector<uint8_t> buffer;
    
    // Write edge record header with endianness conversion
    buffer.resize(sizeof(EdgeRecord));
    
    // Use C++23 std::byteswap for consistent serialization
    EdgeRecord le_edge;
    le_edge.id = host_to_le(edge.id);
    le_edge.from_node = host_to_le(edge.from_node);
    le_edge.to_node = host_to_le(edge.to_node);
    le_edge.label_id = host_to_le(edge.label_id);
    le_edge.property_count = host_to_le(edge.property_count);
    le_edge.property_offset = host_to_le(edge.property_offset);
    le_edge.timestamp = host_to_le(edge.timestamp);
    
    std::memcpy(buffer.data(), &le_edge, sizeof(EdgeRecord));
    
    // Write properties
    auto prop_data = serialize_properties(properties);
    buffer.insert(buffer.end(), prop_data.begin(), prop_data.end());
    
    return buffer;
}

util::expected<std::pair<EdgeRecord, std::vector<Property>>, Error> 
RecordSerializer::deserialize_edge(std::span<const uint8_t> data) {
    if (data.size() < sizeof(EdgeRecord)) {
        return util::unexpected(Error{ErrorCode::CORRUPTION, "Insufficient data for edge record"});
    }
    
    EdgeRecord le_edge;
    std::memcpy(&le_edge, data.data(), sizeof(EdgeRecord));
    
    // Use C++23 std::byteswap for consistent deserialization
    EdgeRecord edge;
    edge.id = le_to_host(le_edge.id);
    edge.from_node = le_to_host(le_edge.from_node);
    edge.to_node = le_to_host(le_edge.to_node);
    edge.label_id = le_to_host(le_edge.label_id);
    edge.property_count = le_to_host(le_edge.property_count);
    edge.property_offset = le_to_host(le_edge.property_offset);
    edge.timestamp = le_to_host(le_edge.timestamp);
    
    auto prop_data = data.subspan(sizeof(EdgeRecord));
    auto properties_result = deserialize_properties(prop_data);
    if (!properties_result.has_value()) {
        return util::unexpected(properties_result.error());
    }
    
    return std::make_pair(edge, std::move(properties_result.value()));
}

void RecordSerializer::write_varint(std::vector<uint8_t>& buffer, uint64_t value) {
    uint8_t temp[10];
    size_t len = util::VarInt::encode(value, temp);
    buffer.insert(buffer.end(), temp, temp + len);
}

util::expected<uint64_t, Error> RecordSerializer::read_varint(std::span<const uint8_t>& data) {
    auto result = util::VarInt::decode(data);
    if (!result.has_value()) {
        return util::unexpected<storage::Error>(result.error());
    }
    return result.value();
}

void RecordSerializer::write_string(std::vector<uint8_t>& buffer, const std::string& str) {
    write_varint(buffer, str.size());
    buffer.insert(buffer.end(), str.begin(), str.end());
}

util::expected<std::string, Error> RecordSerializer::read_string(std::span<const uint8_t>& data) {
    auto len_result = read_varint(data);
    if (!len_result.has_value()) {
        return util::unexpected(len_result.error());
    }
    
    size_t len = len_result.value();
    if (data.size() < len) {
        return util::unexpected(Error{ErrorCode::CORRUPTION, "Insufficient data for string"});
    }
    
    std::string result(reinterpret_cast<const char*>(data.data()), len);
    data = data.subspan(len);
    return result;
}

void RecordSerializer::write_property_value(std::vector<uint8_t>& buffer, const PropertyValue& value) {
    std::visit([&buffer](const auto& v) {
        using T = std::decay_t<decltype(v)>;
        
        if constexpr (std::is_same_v<T, std::string>) {
            buffer.push_back(static_cast<uint8_t>(PropertyType::STRING));
            write_string(buffer, v);
        } else if constexpr (std::is_same_v<T, int64_t>) {
            buffer.push_back(static_cast<uint8_t>(PropertyType::INTEGER));
            write_varint(buffer, util::ZigZag::encode(v));
        } else if constexpr (std::is_same_v<T, double>) {
            buffer.push_back(static_cast<uint8_t>(PropertyType::FLOAT));
            // Use C++23 std::byteswap for consistent endianness
            uint64_t bits;
            std::memcpy(&bits, &v, sizeof(double));
            uint64_t le_bits = host_to_le(bits);
            buffer.resize(buffer.size() + sizeof(double));
            std::memcpy(buffer.data() + buffer.size() - sizeof(double), &le_bits, sizeof(double));
        } else if constexpr (std::is_same_v<T, bool>) {
            buffer.push_back(static_cast<uint8_t>(PropertyType::BOOLEAN));
            buffer.push_back(v ? 1 : 0);
        } else if constexpr (std::is_same_v<T, std::vector<uint8_t>>) {
            buffer.push_back(static_cast<uint8_t>(PropertyType::BYTES));
            write_varint(buffer, v.size());
            buffer.insert(buffer.end(), v.begin(), v.end());
        }
    }, value);
}

util::expected<PropertyValue, Error> RecordSerializer::read_property_value(std::span<const uint8_t>& data, PropertyType type) {
    switch (type) {
        case PropertyType::STRING: {
            auto str_result = read_string(data);
            if (!str_result.has_value()) {
                return util::unexpected<storage::Error>(str_result.error());
            }
            return PropertyValue{std::move(str_result.value())};
        }
        
        case PropertyType::INTEGER: {
            auto varint_result = read_varint(data);
            if (!varint_result.has_value()) {
                return util::unexpected<storage::Error>(varint_result.error());
            }
            return PropertyValue{util::ZigZag::decode(varint_result.value())};
        }
        
        case PropertyType::FLOAT: {
            if (data.size() < sizeof(double)) {
                return util::unexpected<storage::Error>(Error{ErrorCode::CORRUPTION, "Insufficient data for float"});
            }
            // Use C++23 std::byteswap for consistent endianness
            uint64_t le_bits;
            std::memcpy(&le_bits, data.data(), sizeof(double));
            uint64_t host_bits = le_to_host(le_bits);
            double value;
            std::memcpy(&value, &host_bits, sizeof(double));
            data = data.subspan(sizeof(double));
            return PropertyValue{value};
        }
        
        case PropertyType::BOOLEAN: {
            if (data.empty()) {
                return util::unexpected<storage::Error>(Error{ErrorCode::CORRUPTION, "Insufficient data for boolean"});
            }
            bool value = data[0] != 0;
            data = data.subspan(1);
            return PropertyValue{value};
        }
        
        case PropertyType::BYTES: {
            auto len_result = read_varint(data);
            if (!len_result.has_value()) {
                return util::unexpected<storage::Error>(len_result.error());
            }
            
            size_t len = len_result.value();
            if (data.size() < len) {
                return util::unexpected<storage::Error>(Error{ErrorCode::CORRUPTION, "Insufficient data for bytes"});
            }
            
            std::vector<uint8_t> bytes(data.begin(), data.begin() + len);
            data = data.subspan(len);
            return PropertyValue{std::move(bytes)};
        }
        
        default:
            return util::unexpected<storage::Error>(Error{ErrorCode::CORRUPTION, "Unknown property type"});
    }
}

}  // namespace loredb::storage