#pragma once

#include <cstdint>
#include <span>
#include <string>
#include <system_error>
#include "../util/expected.h"

namespace loredb::storage {

using PageId = uint64_t;
using NodeId = uint64_t;
using EdgeId = uint64_t;
using PropertyId = uint64_t;

constexpr size_t PAGE_SIZE = 4096;
constexpr PageId INVALID_PAGE_ID = 0;

enum class ErrorCode {
    OK = 0,
    IO_ERROR,
    CORRUPTION,
    OUT_OF_MEMORY,
    INVALID_ARGUMENT,
    NOT_FOUND,
    ALREADY_EXISTS
};

struct Error {
    ErrorCode code;
    std::string message;
    
    Error() : code(ErrorCode::OK), message("") {}
    Error(ErrorCode c, std::string msg) : code(c), message(std::move(msg)) {}
};

class PageStore {
public:
    virtual ~PageStore() = default;
    
    virtual util::expected<PageId, Error> allocate_page() = 0;
    virtual util::expected<void, Error> deallocate_page(PageId page_id) = 0;
    virtual util::expected<std::span<uint8_t>, Error> read_page(PageId page_id) = 0;
    virtual util::expected<void, Error> write_page(PageId page_id, std::span<const uint8_t> data) = 0;
    virtual util::expected<void, Error> sync() = 0;
    virtual util::expected<void, Error> close() = 0;
    
    virtual size_t get_page_count() const = 0;
    virtual size_t get_allocated_pages() const = 0;
};

struct PageHeader {
    static constexpr uint32_t MAGIC = 0x47524148; // 'GRAH'
    
    uint32_t magic;
    uint32_t version;
    uint32_t page_type;
    uint32_t checksum;
    uint32_t next_free_offset;
    uint32_t record_count;
    uint64_t page_id;
    uint64_t next_page_id;
    uint64_t reserved;  // Reserved for future use / alignment
    
    PageHeader() : magic(MAGIC), version(1), page_type(0), checksum(0), 
                   next_free_offset(sizeof(PageHeader)), record_count(0), 
                   page_id(INVALID_PAGE_ID), next_page_id(INVALID_PAGE_ID), reserved(0) {}
};

enum class PageType : uint32_t {
    FREE = 0,
    NODE = 1,
    EDGE = 2,
    PROPERTY = 3,
    INDEX = 4,
    METADATA = 5
};

struct NodeRecord {
    NodeId id;
    uint32_t label_count;
    uint32_t property_count;
    uint32_t in_degree;
    uint32_t out_degree;
    uint64_t property_offset;
    uint64_t in_edges_offset;
    uint64_t out_edges_offset;
    
    NodeRecord() : id(0), label_count(0), property_count(0), 
                   in_degree(0), out_degree(0), property_offset(0), 
                   in_edges_offset(0), out_edges_offset(0) {}
};

struct EdgeRecord {
    EdgeId id;
    NodeId from_node;
    NodeId to_node;
    uint32_t label_id;
    uint32_t property_count;
    uint64_t property_offset;
    uint64_t timestamp;
    
    EdgeRecord() : id(0), from_node(0), to_node(0), label_id(0), 
                   property_count(0), property_offset(0), timestamp(0) {}
};

struct PropertyRecord {
    PropertyId id;
    uint32_t key_len;
    uint32_t value_len;
    uint8_t value_type;
    uint8_t flags;
    uint16_t reserved;
    
    PropertyRecord() : id(0), key_len(0), value_len(0), value_type(0), 
                       flags(0), reserved(0) {}
};

enum class PropertyType : uint8_t {
    STRING = 0,
    INTEGER = 1,
    FLOAT = 2,
    BOOLEAN = 3,
    BYTES = 4,
    ARRAY = 5,
    OBJECT = 6
};

}  // namespace loredb::storage