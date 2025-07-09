#include "file_page_store.h"
#include "../util/crc32.h"
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#include <algorithm>
#include <cstring>

namespace graphdb::storage {

FilePageStore::FilePageStore(const std::string& filename)
    : filename_(filename), fd_(-1), mapped_memory_(nullptr), 
      file_size_(0), mapped_size_(0), next_page_id_(1),
      initial_size_(1024 * 1024), growth_factor_(2.0), 
      sync_on_write_(false), is_open_(false) {
    
    // Open or create the file
    fd_ = open(filename_.c_str(), O_RDWR | O_CREAT, 0644);
    if (fd_ == -1) {
        return;
    }
    
    // Get existing file size
    struct stat st;
    if (fstat(fd_, &st) == 0) {
        file_size_ = st.st_size;
    }
    
    // Ensure minimum size
    if (file_size_ < initial_size_) {
        if (extend_file(initial_size_).has_value()) {
            file_size_ = initial_size_;
        }
    }
    
    // Map the file
    if (remap_file().has_value()) {
        is_open_ = true;
        
        // If this is a new file, initialize the first page as metadata
        if (file_size_ == initial_size_) {
            PageHeader header;
            header.page_id = 0;
            header.page_type = static_cast<uint32_t>(PageType::METADATA);
            header.next_page_id = 1;
            
            std::memcpy(mapped_memory_, &header, sizeof(header));
            next_page_id_.store(1);
        } else {
            // Read metadata page to get next page ID
            PageHeader* header = reinterpret_cast<PageHeader*>(mapped_memory_);
            if (header->magic == PageHeader::MAGIC) {
                next_page_id_.store(header->next_page_id);
            }
        }
    }
}

FilePageStore::~FilePageStore() {
    close();
}

util::expected<PageId, Error> FilePageStore::allocate_page() {
    if (!is_open_) {
        return util::unexpected(Error{ErrorCode::IO_ERROR, "File not open"});
    }
    
    // Check for free pages first
    {
        std::lock_guard<std::mutex> lock(free_pages_mutex_);
        if (!free_pages_.empty()) {
            PageId page_id = *free_pages_.begin();
            free_pages_.erase(free_pages_.begin());
            return page_id;
        }
    }
    
    // Allocate new page
    PageId page_id = get_next_page_id();
    
    // Ensure we have capacity
    size_t required_pages = page_id + 1;
    if (auto result = ensure_capacity(required_pages); !result.has_value()) {
        return util::unexpected(result.error());
    }
    
    // Initialize page header
    PageHeader header;
    header.page_id = page_id;
    header.page_type = static_cast<uint32_t>(PageType::FREE);
    
    size_t offset = page_id * PAGE_SIZE;
    std::memcpy(mapped_memory_ + offset, &header, sizeof(header));
    
    // Update metadata page with next page ID
    PageHeader* metadata = reinterpret_cast<PageHeader*>(mapped_memory_);
    metadata->next_page_id = next_page_id_.load();
    
    if (sync_on_write_) {
        sync();
    }
    
    return page_id;
}

util::expected<void, Error> FilePageStore::deallocate_page(PageId page_id) {
    if (!is_open_) {
        return util::unexpected(Error{ErrorCode::IO_ERROR, "File not open"});
    }
    
    if (page_id == 0 || page_id >= get_page_count()) {
        return util::unexpected(Error{ErrorCode::INVALID_ARGUMENT, "Invalid page ID"});
    }
    
    // Mark page as free
    size_t offset = page_id * PAGE_SIZE;
    PageHeader* header = reinterpret_cast<PageHeader*>(mapped_memory_ + offset);
    header->page_type = static_cast<uint32_t>(PageType::FREE);
    header->record_count = 0;
    header->next_free_offset = sizeof(PageHeader);
    
    // Add to free list
    {
        std::lock_guard<std::mutex> lock(free_pages_mutex_);
        free_pages_.insert(page_id);
    }
    
    if (sync_on_write_) {
        sync();
    }
    
    return {};
}

util::expected<std::span<uint8_t>, Error> FilePageStore::read_page(PageId page_id) {
    if (!is_open_) {
        return util::unexpected(Error{ErrorCode::IO_ERROR, "File not open"});
    }
    
    if (page_id >= get_page_count()) {
        return util::unexpected(Error{ErrorCode::INVALID_ARGUMENT, "Invalid page ID"});
    }
    
    size_t offset = page_id * PAGE_SIZE;
    return std::span<uint8_t>(mapped_memory_ + offset, PAGE_SIZE);
}

util::expected<void, Error> FilePageStore::write_page(PageId page_id, std::span<const uint8_t> data) {
    if (!is_open_) {
        return util::unexpected(Error{ErrorCode::IO_ERROR, "File not open"});
    }
    
    if (page_id >= get_page_count()) {
        return util::unexpected(Error{ErrorCode::INVALID_ARGUMENT, "Invalid page ID"});
    }
    
    if (data.size() > PAGE_SIZE) {
        return util::unexpected(Error{ErrorCode::INVALID_ARGUMENT, "Data too large for page"});
    }
    
    size_t offset = page_id * PAGE_SIZE;
    std::memcpy(mapped_memory_ + offset, data.data(), data.size());
    
    // Update checksum in page header
    PageHeader* header = reinterpret_cast<PageHeader*>(mapped_memory_ + offset);
    header->checksum = util::CRC32::calculate(data.subspan(sizeof(PageHeader)));
    
    if (sync_on_write_) {
        sync();
    }
    
    return {};
}

util::expected<void, Error> FilePageStore::sync() {
    if (!is_open_) {
        return util::unexpected(Error{ErrorCode::IO_ERROR, "File not open"});
    }
    
    if (msync(mapped_memory_, mapped_size_, MS_SYNC) != 0) {
        return util::unexpected(Error{ErrorCode::IO_ERROR, "Failed to sync memory map"});
    }
    
    if (fsync(fd_) != 0) {
        return util::unexpected(Error{ErrorCode::IO_ERROR, "Failed to sync file"});
    }
    
    return {};
}

util::expected<void, Error> FilePageStore::close() {
    if (!is_open_) {
        return {};
    }
    
    // Sync before closing
    sync();
    
    // Unmap memory
    if (mapped_memory_ != nullptr) {
        munmap(mapped_memory_, mapped_size_);
        mapped_memory_ = nullptr;
    }
    
    // Close file
    if (fd_ != -1) {
        ::close(fd_);
        fd_ = -1;
    }
    
    is_open_ = false;
    return {};
}

size_t FilePageStore::get_page_count() const {
    return file_size_ / PAGE_SIZE;
}

size_t FilePageStore::get_allocated_pages() const {
    // Pages actually allocated correspond to the range \[1, next_page_id_) minus any pages
    // returned to the free list. Page 0 is reserved for metadata.
    const size_t allocated_so_far = next_page_id_.load();
    std::lock_guard<std::mutex> lock(free_pages_mutex_);
    // Subtract 1 to exclude metadata page (page 0) from the allocation count
    return (allocated_so_far > 0 ? allocated_so_far - 1 : 0) - free_pages_.size();
}

util::expected<void, Error> FilePageStore::ensure_capacity(size_t required_pages) {
    size_t current_pages = get_page_count();
    
    if (required_pages <= current_pages) {
        return {};
    }
    
    // Calculate new size with growth factor
    size_t new_size = file_size_;
    while (new_size / PAGE_SIZE < required_pages) {
        new_size = static_cast<size_t>(new_size * growth_factor_);
    }
    
    return extend_file(new_size);
}

util::expected<void, Error> FilePageStore::extend_file(size_t new_size) {
    std::lock_guard<std::mutex> lock(file_mutex_);
    
    // Extend the file
    if (ftruncate(fd_, new_size) != 0) {
        return util::unexpected(Error{ErrorCode::IO_ERROR, "Failed to extend file"});
    }
    
    file_size_ = new_size;
    
    // Remap the file
    return remap_file();
}

util::expected<void, Error> FilePageStore::remap_file() {
    // Unmap existing mapping
    if (mapped_memory_ != nullptr) {
        munmap(mapped_memory_, mapped_size_);
    }
    
    // Map the file
    mapped_memory_ = static_cast<uint8_t*>(
        mmap(nullptr, file_size_, PROT_READ | PROT_WRITE, MAP_SHARED, fd_, 0)
    );
    
    if (mapped_memory_ == MAP_FAILED) {
        mapped_memory_ = nullptr;
        return util::unexpected(Error{ErrorCode::IO_ERROR, "Failed to map file"});
    }
    
    mapped_size_ = file_size_;
    
    // Advise kernel about access pattern
    madvise(mapped_memory_, mapped_size_, MADV_RANDOM);
    
    return {};
}

PageId FilePageStore::get_next_page_id() {
    return next_page_id_.fetch_add(1);
}

}  // namespace graphdb::storage