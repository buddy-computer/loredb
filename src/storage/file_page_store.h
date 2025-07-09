/// \file file_page_store.h
/// \brief File-based page store implementation.
/// \author LoreDB contributors
/// \ingroup storage
#pragma once

#include "page_store.h"
#include <atomic>
#include <fstream>
#include <memory>
#include <mutex>
#include <string>
#include <vector>
#include <unordered_set>

namespace loredb::storage {

/**
 * @class FilePageStore
 * @brief File-backed implementation of PageStore for persistent storage.
 *
 * Supports configuration for initial file size, growth factor, and sync behavior.
 */
class FilePageStore : public PageStore {
public:
    // Rule-of-five: non-copyable, movable
    FilePageStore(const FilePageStore&) = delete;
    FilePageStore& operator=(const FilePageStore&) = delete;
    FilePageStore(FilePageStore&&) noexcept = delete;
    FilePageStore& operator=(FilePageStore&&) noexcept = delete;

    /**
     * @brief Construct a FilePageStore with a given file path.
     * @param path File path for the database.
     * @param sync_on_write If true, sync after every write operation.
     */
    explicit FilePageStore(const std::string& path, bool sync_on_write = false);
    /** Destructor. */
    ~FilePageStore() override;
    
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
    void set_initial_size(size_t size);
    void set_growth_factor(double factor);
    void set_sync_on_write(bool sync);

private:
    void ensure_file_size(size_t required_size);
    
    std::string file_path_;
    std::fstream file_stream_;
    bool sync_on_write_;
    bool is_closed_;
    
    // Page management
    std::mutex file_mutex_;
    std::atomic<PageId> next_page_id_;
    std::atomic<size_t> allocated_pages_;
    std::unique_ptr<std::vector<uint8_t>> page_buffer_; // for reads
    std::unordered_set<PageId> free_pages_;
    
    // Configuration
    size_t initial_size_;
    double growth_factor_;
};

}  // namespace loredb::storage