/// \file file_page_store.h
/// \brief File-backed implementation of PageStore for persistent storage.
/// \author wiki-graph contributors
/// \ingroup storage
#pragma once

#include "page_store.h"
#include <atomic>
#include <memory>
#include <mutex>
#include <string>
#include <vector>
#include <unordered_set>

namespace graphdb::storage {

/**
 * @class FilePageStore
 * @brief File-backed implementation of PageStore using memory-mapped files.
 *
 * Manages allocation, deallocation, reading, and writing of fixed-size pages.
 * Supports configuration for initial file size, growth factor, and sync behavior.
 */
class FilePageStore : public PageStore {
public:
    // Rule-of-five: non-copyable, movable
    FilePageStore(const FilePageStore&) = delete;
    FilePageStore& operator=(const FilePageStore&) = delete;
    FilePageStore(FilePageStore&&) noexcept = default;
    FilePageStore& operator=(FilePageStore&&) noexcept = default;
    /**
     * @brief Construct a FilePageStore for the given file.
     * @param filename Path to the backing file.
     */
    explicit FilePageStore(const std::string& filename);
    /** Destructor. */
    ~FilePageStore();
    
    // PageStore interface
    util::expected<PageId, Error> allocate_page() override;
    util::expected<void, Error> deallocate_page(PageId page_id) override;
    util::expected<std::span<uint8_t>, Error> read_page(PageId page_id) override;
    util::expected<void, Error> write_page(PageId page_id, std::span<const uint8_t> data) override;
    util::expected<void, Error> sync() override;
    util::expected<void, Error> close() override;
    
    size_t get_page_count() const override;
    size_t get_allocated_pages() const override;
    
    // Configuration
    void set_initial_size(size_t size) { initial_size_ = size; }
    void set_growth_factor(double factor) { growth_factor_ = factor; }
    void set_sync_on_write(bool sync) { sync_on_write_ = sync; }

private:
    util::expected<void, Error> ensure_capacity(size_t required_pages);
    util::expected<void, Error> extend_file(size_t new_size);
    util::expected<void, Error> remap_file();
    PageId get_next_page_id();
    
    std::string filename_;
    int fd_;
    uint8_t* mapped_memory_;
    size_t file_size_;
    size_t mapped_size_;
    
    // Page management
    std::atomic<PageId> next_page_id_;
    mutable std::mutex free_pages_mutex_;
    std::unordered_set<PageId> free_pages_;
    
    // Configuration
    size_t initial_size_;
    double growth_factor_;
    bool sync_on_write_;
    
    // State
    bool is_open_;
    std::mutex file_mutex_;
};

}  // namespace graphdb::storage