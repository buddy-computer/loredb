#include "src/storage/file_page_store.h"
#include "src/storage/page_store.h"
#include <iostream>
#include <iomanip>
#include <numeric>

using namespace loredb::storage;

void print_bytes(const std::vector<uint8_t>& data, const std::string& name, size_t max_bytes = 32) {
    std::cout << name << " (first " << max_bytes << " bytes): ";
    for (size_t i = 0; i < std::min(max_bytes, data.size()); ++i) {
        std::cout << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(data[i]) << " ";
    }
    std::cout << std::dec << std::endl;
}

void print_bytes(const std::span<uint8_t>& data, const std::string& name, size_t max_bytes = 32) {
    std::cout << name << " (first " << max_bytes << " bytes): ";
    for (size_t i = 0; i < std::min(max_bytes, data.size()); ++i) {
        std::cout << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(data[i]) << " ";
    }
    std::cout << std::dec << std::endl;
}

int main() {
    std::cout << "=== FilePageStore Debug Test ===" << std::endl;
    
    // Create a page store
    auto page_store = std::make_unique<FilePageStore>("debug_test.db", false);
    
    // Allocate a page
    auto alloc_result = page_store->allocate_page();
    if (!alloc_result.has_value()) {
        std::cout << "Failed to allocate page: " << alloc_result.error().message << std::endl;
        return 1;
    }
    PageId page_id = alloc_result.value();
    std::cout << "Allocated page ID: " << page_id << std::endl;
    
    // Create test data
    std::vector<uint8_t> test_data(PAGE_SIZE);
    std::iota(test_data.begin(), test_data.end(), 0);
    
    std::cout << "PageHeader size: " << sizeof(PageHeader) << std::endl;
    std::cout << "PAGE_SIZE: " << PAGE_SIZE << std::endl;
    
    print_bytes(test_data, "Original test data");
    
    // Write the page
    auto write_result = page_store->write_page(page_id, test_data);
    if (!write_result.has_value()) {
        std::cout << "Failed to write page: " << write_result.error().message << std::endl;
        return 1;
    }
    std::cout << "Page written successfully" << std::endl;
    
    // Read the page back
    auto read_result = page_store->read_page(page_id);
    if (!read_result.has_value()) {
        std::cout << "Failed to read page: " << read_result.error().message << std::endl;
        return 1;
    }
    
    auto read_data = read_result.value();
    std::cout << "Read data size: " << read_data.size() << std::endl;
    
    print_bytes(read_data, "Read data");
    
    // Compare header
    PageHeader* original_header = reinterpret_cast<PageHeader*>(test_data.data());
    PageHeader* read_header = reinterpret_cast<PageHeader*>(read_data.data());
    
    std::cout << "\n=== Header Comparison ===" << std::endl;
    std::cout << "Original header magic: " << original_header->magic << std::endl;
    std::cout << "Read header magic: " << read_header->magic << std::endl;
    std::cout << "Original header page_id: " << original_header->page_id << std::endl;
    std::cout << "Read header page_id: " << read_header->page_id << std::endl;
    std::cout << "Original header checksum: " << original_header->checksum << std::endl;
    std::cout << "Read header checksum: " << read_header->checksum << std::endl;
    
    // Compare data after header
    bool data_matches = std::equal(test_data.begin() + sizeof(PageHeader), test_data.end(), 
                                   read_data.begin() + sizeof(PageHeader));
    std::cout << "\n=== Data Comparison ===" << std::endl;
    std::cout << "Data after header matches: " << (data_matches ? "YES" : "NO") << std::endl;
    
    if (!data_matches) {
        std::cout << "First few bytes after header:" << std::endl;
        size_t header_size = sizeof(PageHeader);
        print_bytes(std::vector<uint8_t>(test_data.begin() + header_size, test_data.begin() + header_size + 32), 
                   "Original data after header", 32);
        print_bytes(std::vector<uint8_t>(read_data.begin() + header_size, read_data.begin() + header_size + 32), 
                   "Read data after header", 32);
    }
    
    return 0;
}
