#pragma once

#include "page_store.h"
#include "../util/varint.h"
#include "../util/expected.h"
#include <string>
#include <vector>
#include <span>
#include <variant>

namespace graphdb::storage {

// Property value types
using PropertyValue = std::variant<
    std::string,
    int64_t,
    double,
    bool,
    std::vector<uint8_t>
>;

struct Property {
    std::string key;
    PropertyValue value;
    
    Property(std::string k, PropertyValue v) : key(std::move(k)), value(std::move(v)) {}
};

class RecordSerializer {
public:
    // Serialize a list of properties into a byte buffer
    static std::vector<uint8_t> serialize_properties(const std::vector<Property>& properties);
    
    // Deserialize properties from a byte buffer
    static util::expected<std::vector<Property>, Error> deserialize_properties(std::span<const uint8_t> data);
    
    // Serialize a node record
    static std::vector<uint8_t> serialize_node(const NodeRecord& node, const std::vector<Property>& properties);
    
    // Deserialize a node record
    static util::expected<std::pair<NodeRecord, std::vector<Property>>, Error> 
    deserialize_node(std::span<const uint8_t> data);
    
    // Serialize an edge record
    static std::vector<uint8_t> serialize_edge(const EdgeRecord& edge, const std::vector<Property>& properties);
    
    // Deserialize an edge record
    static util::expected<std::pair<EdgeRecord, std::vector<Property>>, Error> 
    deserialize_edge(std::span<const uint8_t> data);

private:
    static void write_varint(std::vector<uint8_t>& buffer, uint64_t value);
    static util::expected<uint64_t, Error> read_varint(std::span<const uint8_t>& data);
    static void write_string(std::vector<uint8_t>& buffer, const std::string& str);
    static util::expected<std::string, Error> read_string(std::span<const uint8_t>& data);
    static void write_property_value(std::vector<uint8_t>& buffer, const PropertyValue& value);
    static util::expected<PropertyValue, Error> read_property_value(std::span<const uint8_t>& data, PropertyType type);
};

}  // namespace graphdb::storage