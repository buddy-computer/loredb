#include "file_page_store.h"
#include "../util/crc32.h"
#include <system_error>
#include <cstring>
#include <iostream>

namespace loredb::storage {

FilePageStore::FilePageStore(const std::string& path, bool sync_on_write)
    : file_path_(path), sync_on_write_(sync_on_write), is_closed_(false), next_page_id_(1), allocated_pages_(0),
      page_buffer_(std::make_unique<std::vector<uint8_t>>(PAGE_SIZE)),
      initial_size_(1024 * 1024), growth_factor_(2.0) {
    
    // Open file in binary mode for reliable positioning operations
    file_stream_.open(file_path_, std::ios::in | std::ios::out | std::ios::binary);
    
    if (!file_stream_.is_open()) {
        // If the file doesn't exist, create it
        file_stream_.clear();
        file_stream_.open(file_path_, std::ios::out | std::ios::binary);
        file_stream_.close();
        file_stream_.clear();
        file_stream_.open(file_path_, std::ios::in | std::ios::out | std::ios::binary);
        if (!file_stream_.is_open()) {
            throw std::runtime_error("Failed to create or open file: " + file_path_);
        }
    }
    
    // Clear any error flags
    file_stream_.clear();
}

FilePageStore::~FilePageStore() {
    close();
}

util::expected<PageId, Error> FilePageStore::allocate_page() {
    std::lock_guard<std::mutex> lock(file_mutex_);
    
    if (is_closed_) {
        return util::unexpected(Error{ErrorCode::IO_ERROR, "PageStore is closed"});
    }
    
    PageId page_id;
    
    // Try to reuse a freed page first
    if (!free_pages_.empty()) {
        page_id = *free_pages_.begin();
        free_pages_.erase(free_pages_.begin());
    } else {
        // Allocate a new page
        page_id = next_page_id_.fetch_add(1);
        // Extend file if needed to accommodate the full page
        ensure_file_size((page_id + 1) * PAGE_SIZE);
    }
    
    allocated_pages_.fetch_add(1);
    
    return page_id;
}

util::expected<void, Error> FilePageStore::deallocate_page(PageId page_id) {
    std::lock_guard<std::mutex> lock(file_mutex_);
    
    if (is_closed_) {
        return util::unexpected(Error{ErrorCode::IO_ERROR, "PageStore is closed"});
    }
    
    // Validate page_id
    if (page_id == INVALID_PAGE_ID || page_id >= next_page_id_.load()) {
        return util::unexpected(Error{ErrorCode::INVALID_ARGUMENT, "Invalid page ID"});
    }
    
    // Check if page is already deallocated
    if (free_pages_.find(page_id) != free_pages_.end()) {
        return util::unexpected(Error{ErrorCode::INVALID_ARGUMENT, "Page already deallocated"});
    }
    
    // Add to free pages set
    free_pages_.insert(page_id);
    
    // Decrement allocated pages counter
    allocated_pages_.fetch_sub(1);
    
    return {};
}

util::expected<std::span<uint8_t>, Error> FilePageStore::read_page(PageId page_id) {
    std::lock_guard<std::mutex> lock(file_mutex_);
    
    if (is_closed_) {
        return util::unexpected(Error{ErrorCode::IO_ERROR, "PageStore is closed"});
    }
    
    // Validate page_id
    if (page_id == INVALID_PAGE_ID || page_id >= next_page_id_.load()) {
        return util::unexpected(Error{ErrorCode::INVALID_ARGUMENT, "Invalid page ID"});
    }
    
    file_stream_.clear(); // Clear any error flags
    size_t file_offset = page_id * PAGE_SIZE;
    file_stream_.seekg(file_offset, std::ios::beg);
    file_stream_.clear(); // Clear flags after seek
    
    if (file_stream_.fail()) {
        return util::unexpected(Error{ErrorCode::IO_ERROR, "Failed to seek to page location"});
    }
    
    // Clear buffer first to ensure we don't have stale data
    std::fill(page_buffer_->begin(), page_buffer_->end(), 0);
    
    file_stream_.read(reinterpret_cast<char*>(page_buffer_->data()), PAGE_SIZE);
    file_stream_.clear(); // Clear flags after read
    
    if (file_stream_.bad()) {
        return util::unexpected(Error{ErrorCode::IO_ERROR, "Failed to read page"});
    }
    
    return std::span<uint8_t>{page_buffer_->data(), PAGE_SIZE};
}

util::expected<void, Error> FilePageStore::write_page(PageId page_id, std::span<const uint8_t> data) {
    if (data.size() != PAGE_SIZE) {
        return util::unexpected(Error{ErrorCode::INVALID_ARGUMENT, "Page data size must be PAGE_SIZE"});
    }
    
    std::lock_guard<std::mutex> lock(file_mutex_);
    
    if (is_closed_) {
        return util::unexpected(Error{ErrorCode::IO_ERROR, "PageStore is closed"});
    }
    
    // Validate page_id
    if (page_id == INVALID_PAGE_ID || page_id >= next_page_id_.load()) {
        return util::unexpected(Error{ErrorCode::INVALID_ARGUMENT, "Invalid page ID"});
    }
    
    file_stream_.clear(); // Clear any error flags
    size_t file_offset = page_id * PAGE_SIZE;
    file_stream_.seekp(file_offset, std::ios::beg);
    file_stream_.clear(); // Clear flags after seek
    
    if (file_stream_.fail()) {
        return util::unexpected(Error{ErrorCode::IO_ERROR, "Failed to seek to page location for write"});
    }
    
    // Modify the page header 
    std::vector<uint8_t> modified_data(data.begin(), data.end());
    PageHeader* header = reinterpret_cast<PageHeader*>(modified_data.data());
    header->magic = PageHeader::MAGIC;
    header->page_id = page_id;
    // page_type is preserved from the input data
    
    // Calculate checksum over the page data (excluding the checksum field itself)
    header->checksum = 0; // Clear checksum field first
    header->checksum = util::CRC32::calculate(std::span<const uint8_t>(modified_data.data(), PAGE_SIZE));
    
    file_stream_.write(reinterpret_cast<const char*>(modified_data.data()), PAGE_SIZE);
    file_stream_.clear(); // Clear flags after write
    
    if (file_stream_.fail()) {
        return util::unexpected(Error{ErrorCode::IO_ERROR, "Failed to write page"});
    }
    
    file_stream_.flush();
    
    if (sync_on_write_) {
        file_stream_.flush();
    }
    
    return {};
}

util::expected<void, Error> FilePageStore::sync() {
    std::lock_guard<std::mutex> lock(file_mutex_);
    
    if (is_closed_) {
        return util::unexpected(Error{ErrorCode::IO_ERROR, "PageStore is closed"});
    }
    
    file_stream_.flush();
    
    return {};
}

util::expected<void, Error> FilePageStore::close() {
    std::lock_guard<std::mutex> lock(file_mutex_);
    
    if (!is_closed_) {
        file_stream_.close();
        is_closed_ = true;
    }
    
    return {};
}

size_t FilePageStore::get_page_count() const {
    return next_page_id_.load() - 1; // next_page_id_ starts at 1
}

size_t FilePageStore::get_allocated_pages() const {
    return allocated_pages_.load();
}

void FilePageStore::ensure_file_size(size_t required_size) {
    if (is_closed_) {
        throw std::runtime_error("FilePageStore is closed");
    }
    
    // Save current position
    auto original_pos = file_stream_.tellp();
    
    file_stream_.seekp(0, std::ios::end);
    file_stream_.clear();
    size_t current_size = file_stream_.tellp();
    
    if (current_size < required_size) {
        // Extend file by writing zeros to fill the gap
        size_t bytes_to_write = required_size - current_size;
        std::vector<char> zeros(bytes_to_write, 0);
        file_stream_.write(zeros.data(), bytes_to_write);
        
        // Flush to ensure the file extension is written
        file_stream_.flush();
        file_stream_.clear();
    }
    
    // Restore original position if it was valid
    if (original_pos != std::streampos(-1)) {
        file_stream_.seekp(original_pos);
        file_stream_.clear();
    }
}

void FilePageStore::set_initial_size(size_t size) {
    initial_size_ = size;
}

void FilePageStore::set_growth_factor(double factor) {
    growth_factor_ = factor;
}

void FilePageStore::set_sync_on_write(bool sync) {
    sync_on_write_ = sync;
}

}  // namespace loredb::storage