#include <gtest/gtest.h>
#include "../../src/storage/file_page_store.h"
#include <unistd.h>
#include <cstring>
#include <numeric>

using namespace graphdb::storage;

class FilePageStoreTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create a temporary file for testing
        db_filename_ = "/tmp/test_graphdb_" + std::to_string(getpid()) + ".db";
        page_store_ = std::make_unique<FilePageStore>(db_filename_);
    }
    
    void TearDown() override {
        page_store_.reset();
        unlink(db_filename_.c_str());
    }
    
    std::string db_filename_;
    std::unique_ptr<FilePageStore> page_store_;
};

TEST_F(FilePageStoreTest, AllocatePage) {
    auto result = page_store_->allocate_page();
    ASSERT_TRUE(result.has_value()) << "Failed to allocate page: " << result.error().message;
    
    PageId page_id = result.value();
    ASSERT_NE(page_id, INVALID_PAGE_ID);
    ASSERT_EQ(page_store_->get_allocated_pages(), 1);
}

TEST_F(FilePageStoreTest, AllocateMultiplePages) {
    std::vector<PageId> page_ids;
    const size_t num_pages = 10;
    
    for (size_t i = 0; i < num_pages; ++i) {
        auto result = page_store_->allocate_page();
        ASSERT_TRUE(result.has_value()) << "Failed to allocate page " << i;
        page_ids.push_back(result.value());
    }
    
    ASSERT_EQ(page_store_->get_allocated_pages(), num_pages);
    
    // Verify all page IDs are unique
    std::sort(page_ids.begin(), page_ids.end());
    auto last = std::unique(page_ids.begin(), page_ids.end());
    ASSERT_EQ(std::distance(page_ids.begin(), last), num_pages);
}

TEST_F(FilePageStoreTest, ReadWritePage) {
    auto alloc_result = page_store_->allocate_page();
    ASSERT_TRUE(alloc_result.has_value());
    PageId page_id = alloc_result.value();
    
    // Create test data
    std::vector<uint8_t> test_data(PAGE_SIZE);
    std::iota(test_data.begin(), test_data.end(), 0);
    
    // Write the page
    auto write_result = page_store_->write_page(page_id, test_data);
    ASSERT_TRUE(write_result.has_value()) << "Failed to write page: " << write_result.error().message;
    
    // Read the page back
    auto read_result = page_store_->read_page(page_id);
    ASSERT_TRUE(read_result.has_value()) << "Failed to read page: " << read_result.error().message;
    
    auto read_data = read_result.value();
    ASSERT_EQ(read_data.size(), PAGE_SIZE);
    
    // Compare the data (skip header for checksum)
    bool data_matches = std::equal(test_data.begin() + sizeof(PageHeader), test_data.end(), 
                                   read_data.begin() + sizeof(PageHeader));
    ASSERT_TRUE(data_matches) << "Read data doesn't match written data";
}

TEST_F(FilePageStoreTest, WritePageTooLarge) {
    auto alloc_result = page_store_->allocate_page();
    ASSERT_TRUE(alloc_result.has_value());
    PageId page_id = alloc_result.value();
    
    // Try to write data larger than a page
    std::vector<uint8_t> large_data(PAGE_SIZE + 1, 0xAA);
    auto write_result = page_store_->write_page(page_id, large_data);
    ASSERT_FALSE(write_result.has_value());
    ASSERT_EQ(write_result.error().code, ErrorCode::INVALID_ARGUMENT);
}

TEST_F(FilePageStoreTest, ReadInvalidPage) {
    PageId invalid_page_id = 999999;
    auto read_result = page_store_->read_page(invalid_page_id);
    ASSERT_FALSE(read_result.has_value());
    ASSERT_EQ(read_result.error().code, ErrorCode::INVALID_ARGUMENT);
}

TEST_F(FilePageStoreTest, WriteInvalidPage) {
    PageId invalid_page_id = 999999;
    std::vector<uint8_t> test_data(PAGE_SIZE, 0xFF);
    auto write_result = page_store_->write_page(invalid_page_id, test_data);
    ASSERT_FALSE(write_result.has_value());
    ASSERT_EQ(write_result.error().code, ErrorCode::INVALID_ARGUMENT);
}

TEST_F(FilePageStoreTest, DeallocatePage) {
    auto alloc_result = page_store_->allocate_page();
    ASSERT_TRUE(alloc_result.has_value());
    PageId page_id = alloc_result.value();
    
    size_t initial_allocated = page_store_->get_allocated_pages();
    
    auto dealloc_result = page_store_->deallocate_page(page_id);
    ASSERT_TRUE(dealloc_result.has_value()) << "Failed to deallocate page: " << dealloc_result.error().message;
    
    ASSERT_EQ(page_store_->get_allocated_pages(), initial_allocated - 1);
}

TEST_F(FilePageStoreTest, DeallocateInvalidPage) {
    PageId invalid_page_id = 999999;
    auto dealloc_result = page_store_->deallocate_page(invalid_page_id);
    ASSERT_FALSE(dealloc_result.has_value());
    ASSERT_EQ(dealloc_result.error().code, ErrorCode::INVALID_ARGUMENT);
}

TEST_F(FilePageStoreTest, ReallocateDeallocatedPage) {
    // Allocate a page
    auto alloc_result1 = page_store_->allocate_page();
    ASSERT_TRUE(alloc_result1.has_value());
    PageId page_id1 = alloc_result1.value();
    
    // Deallocate it
    auto dealloc_result = page_store_->deallocate_page(page_id1);
    ASSERT_TRUE(dealloc_result.has_value());
    
    // Allocate another page (should reuse the deallocated one)
    auto alloc_result2 = page_store_->allocate_page();
    ASSERT_TRUE(alloc_result2.has_value());
    PageId page_id2 = alloc_result2.value();
    
    ASSERT_EQ(page_id1, page_id2);
}

TEST_F(FilePageStoreTest, Sync) {
    auto alloc_result = page_store_->allocate_page();
    ASSERT_TRUE(alloc_result.has_value());
    PageId page_id = alloc_result.value();
    
    std::vector<uint8_t> test_data(PAGE_SIZE, 0xCC);
    auto write_result = page_store_->write_page(page_id, test_data);
    ASSERT_TRUE(write_result.has_value());
    
    auto sync_result = page_store_->sync();
    ASSERT_TRUE(sync_result.has_value()) << "Failed to sync: " << sync_result.error().message;
}

TEST_F(FilePageStoreTest, Close) {
    auto alloc_result = page_store_->allocate_page();
    ASSERT_TRUE(alloc_result.has_value());
    
    auto close_result = page_store_->close();
    ASSERT_TRUE(close_result.has_value()) << "Failed to close: " << close_result.error().message;
    
    // Operations after close should fail
    auto alloc_result2 = page_store_->allocate_page();
    ASSERT_FALSE(alloc_result2.has_value());
    ASSERT_EQ(alloc_result2.error().code, ErrorCode::IO_ERROR);
}

TEST_F(FilePageStoreTest, PageHeaderChecksum) {
    auto alloc_result = page_store_->allocate_page();
    ASSERT_TRUE(alloc_result.has_value());
    PageId page_id = alloc_result.value();
    
    // Write some data
    std::vector<uint8_t> test_data(PAGE_SIZE, 0);
    PageHeader* header = reinterpret_cast<PageHeader*>(test_data.data());
    header->magic = PageHeader::MAGIC;
    header->page_id = page_id;
    header->page_type = static_cast<uint32_t>(PageType::NODE);
    
    auto write_result = page_store_->write_page(page_id, test_data);
    ASSERT_TRUE(write_result.has_value());
    
    // Read it back
    auto read_result = page_store_->read_page(page_id);
    ASSERT_TRUE(read_result.has_value());
    
    auto read_data = read_result.value();
    PageHeader* read_header = reinterpret_cast<PageHeader*>(read_data.data());
    
    ASSERT_EQ(read_header->magic, PageHeader::MAGIC);
    ASSERT_EQ(read_header->page_id, page_id);
    ASSERT_EQ(read_header->page_type, static_cast<uint32_t>(PageType::NODE));
    ASSERT_NE(read_header->checksum, 0); // Checksum should be calculated
}

TEST_F(FilePageStoreTest, Configuration) {
    // Test configuration methods
    page_store_->set_initial_size(2 * 1024 * 1024);  // 2MB
    page_store_->set_growth_factor(1.5);
    page_store_->set_sync_on_write(true);
    
    // These should not cause any errors
    auto alloc_result = page_store_->allocate_page();
    ASSERT_TRUE(alloc_result.has_value());
}

TEST_F(FilePageStoreTest, FileGrowth) {
    // Allocate many pages to trigger file growth
    std::vector<PageId> page_ids;
    const size_t num_pages = 1000;
    
    for (size_t i = 0; i < num_pages; ++i) {
        auto result = page_store_->allocate_page();
        ASSERT_TRUE(result.has_value()) << "Failed to allocate page " << i;
        page_ids.push_back(result.value());
    }
    
    ASSERT_EQ(page_store_->get_allocated_pages(), num_pages);
    ASSERT_GE(page_store_->get_page_count(), num_pages);
}