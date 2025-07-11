#include <gtest/gtest.h>
#include "../../src/storage/page_store.h"

using namespace loredb::storage;

class PageStoreTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Basic test setup
    }
    
    void TearDown() override {
        // Cleanup
    }
};

TEST_F(PageStoreTest, ErrorCodeValues) {
    // Test error code enum values
    ASSERT_EQ(static_cast<int>(ErrorCode::OK), 0);
    ASSERT_NE(static_cast<int>(ErrorCode::IO_ERROR), 0);
    ASSERT_NE(static_cast<int>(ErrorCode::CORRUPTION), 0);
    ASSERT_NE(static_cast<int>(ErrorCode::OUT_OF_MEMORY), 0);
    ASSERT_NE(static_cast<int>(ErrorCode::INVALID_ARGUMENT), 0);
    ASSERT_NE(static_cast<int>(ErrorCode::NOT_FOUND), 0);
    ASSERT_NE(static_cast<int>(ErrorCode::ALREADY_EXISTS), 0);
}

TEST_F(PageStoreTest, ErrorConstruction) {
    Error error(ErrorCode::IO_ERROR, "Test error message");
    ASSERT_EQ(error.code, ErrorCode::IO_ERROR);
    ASSERT_EQ(error.message, "Test error message");
}

TEST_F(PageStoreTest, PageHeaderInitialization) {
    PageHeader header;
    
    ASSERT_EQ(header.magic, PageHeader::MAGIC);
    ASSERT_EQ(header.version, 1);
    ASSERT_EQ(header.page_type, 0);
    ASSERT_EQ(header.checksum, 0);
    ASSERT_EQ(header.next_free_offset, sizeof(PageHeader));
    ASSERT_EQ(header.record_count, 0);
    ASSERT_EQ(header.page_id, INVALID_PAGE_ID);
    ASSERT_EQ(header.next_page_id, INVALID_PAGE_ID);
}

TEST_F(PageStoreTest, PageTypeEnum) {
    ASSERT_EQ(static_cast<uint32_t>(PageType::FREE), 0);
    ASSERT_EQ(static_cast<uint32_t>(PageType::NODE), 1);
    ASSERT_EQ(static_cast<uint32_t>(PageType::EDGE), 2);
    ASSERT_EQ(static_cast<uint32_t>(PageType::PROPERTY), 3);
    ASSERT_EQ(static_cast<uint32_t>(PageType::INDEX), 4);
    ASSERT_EQ(static_cast<uint32_t>(PageType::METADATA), 5);
}

TEST_F(PageStoreTest, NodeRecordInitialization) {
    NodeRecord node;
    
    ASSERT_EQ(node.id, 0);
    ASSERT_EQ(node.label_count, 0);
    ASSERT_EQ(node.property_count, 0);
    ASSERT_EQ(node.in_degree, 0);
    ASSERT_EQ(node.out_degree, 0);
    ASSERT_EQ(node.property_offset, 0);
    ASSERT_EQ(node.in_edges_offset, 0);
    ASSERT_EQ(node.out_edges_offset, 0);
}

TEST_F(PageStoreTest, EdgeRecordInitialization) {
    EdgeRecord edge;
    
    ASSERT_EQ(edge.id, 0);
    ASSERT_EQ(edge.from_node, 0);
    ASSERT_EQ(edge.to_node, 0);
    ASSERT_EQ(edge.label_id, 0);
    ASSERT_EQ(edge.property_count, 0);
    ASSERT_EQ(edge.property_offset, 0);
    ASSERT_EQ(edge.timestamp, 0);
}

TEST_F(PageStoreTest, PropertyRecordInitialization) {
    PropertyRecord prop;
    
    ASSERT_EQ(prop.id, 0);
    ASSERT_EQ(prop.key_len, 0);
    ASSERT_EQ(prop.value_len, 0);
    ASSERT_EQ(prop.value_type, 0);
    ASSERT_EQ(prop.flags, 0);
    ASSERT_EQ(prop.reserved, 0);
}

TEST_F(PageStoreTest, PropertyTypeEnum) {
    ASSERT_EQ(static_cast<uint8_t>(PropertyType::STRING), 0);
    ASSERT_EQ(static_cast<uint8_t>(PropertyType::INTEGER), 1);
    ASSERT_EQ(static_cast<uint8_t>(PropertyType::FLOAT), 2);
    ASSERT_EQ(static_cast<uint8_t>(PropertyType::BOOLEAN), 3);
    ASSERT_EQ(static_cast<uint8_t>(PropertyType::BYTES), 4);
    ASSERT_EQ(static_cast<uint8_t>(PropertyType::ARRAY), 5);
    ASSERT_EQ(static_cast<uint8_t>(PropertyType::OBJECT), 6);
}

TEST_F(PageStoreTest, Constants) {
    ASSERT_EQ(PAGE_SIZE, 4096);
    ASSERT_EQ(INVALID_PAGE_ID, 0);
    ASSERT_EQ(sizeof(PageHeader), 48); // Verify header size is as expected
}