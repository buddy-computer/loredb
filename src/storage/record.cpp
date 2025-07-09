#include "record.h"
#include <cstring>

namespace loredb::storage {

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
    
    // Write node record header
    buffer.resize(sizeof(NodeRecord));
    std::memcpy(buffer.data(), &node, sizeof(NodeRecord));
    
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
    
    NodeRecord node;
    std::memcpy(&node, data.data(), sizeof(NodeRecord));
    
    auto prop_data = data.subspan(sizeof(NodeRecord));
    auto properties_result = deserialize_properties(prop_data);
    if (!properties_result.has_value()) {
        return util::unexpected(properties_result.error());
    }
    
    return std::make_pair(node, std::move(properties_result.value()));
}

std::vector<uint8_t> RecordSerializer::serialize_edge(const EdgeRecord& edge, const std::vector<Property>& properties) {
    std::vector<uint8_t> buffer;
    
    // Write edge record header
    buffer.resize(sizeof(EdgeRecord));
    std::memcpy(buffer.data(), &edge, sizeof(EdgeRecord));
    
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
    
    EdgeRecord edge;
    std::memcpy(&edge, data.data(), sizeof(EdgeRecord));
    
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
            buffer.resize(buffer.size() + sizeof(double));
            std::memcpy(buffer.data() + buffer.size() - sizeof(double), &v, sizeof(double));
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
            double value;
            std::memcpy(&value, data.data(), sizeof(double));
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