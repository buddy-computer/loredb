/// \file mvcc_manager.h
/// \brief Multi-version concurrency control (MVCC) manager for versioned storage.
/// \author LoreDB contributors
/// \ingroup transaction
#pragma once

#include "mvcc.h"
#include "../storage/page_store.h"  // For NodeRecord / EdgeRecord
#include "../storage/record.h"      // For Property
#include <unordered_map>
#include <variant>
#include <vector>
#include <shared_mutex>
#include "lock_manager.h"

namespace loredb::transaction {

// Error codes for MVCC operations
enum class MVCCErrorCode {
    OK = 0,
    NOT_FOUND,
    CONFLICT,
    UNKNOWN
};

struct MVCCError {
    MVCCErrorCode code;
    std::string message;
};

struct Version {
    TransactionId created_tx_id{0};
    TransactionId deleted_tx_id{0}; // 0 means not deleted / still live
    std::variant<storage::NodeRecord, storage::EdgeRecord> data;
    std::vector<storage::Property> properties; // Property versioning support
};

/**
 * @class MVCCManager
 * @brief In-memory MVCC manager for versioned node/edge records.
 *
 * Tracks versions for each logical key (node/edge ID), supports snapshot isolation and garbage collection.
 */
class MVCCManager {
public:
    /**
     * @brief Construct an MVCCManager with a TransactionManager.
     * @param txn_manager Shared pointer to TransactionManager.
     */
    explicit MVCCManager(std::shared_ptr<TransactionManager> txn_manager);

    // Read the visible version for a transaction.
    util::expected<Version, MVCCError> read_version(uint64_t key, TransactionId tx_id) const;

    // Write a new version. The supplied version.created_tx_id must be set by caller.
    // On success, returns {}. On conflict, returns error.
    util::expected<void, MVCCError> write_version(uint64_t key, Version version);

    // Garbage collect versions that are older than min_active_tx_id (i.e., no TX can see them)
    void garbage_collect(TransactionId min_active_tx_id);

    // Get the lock manager
    LockManager& get_lock_manager() {
        return *lock_manager_;
    }

private:
    // Check if a version is visible to a transaction
    bool is_version_visible(const Version& version, TransactionId tx_id) const;
    
    std::shared_ptr<TransactionManager> txn_manager_;
    std::unique_ptr<LockManager> lock_manager_;
    mutable std::shared_mutex mutex_;
    std::unordered_map<uint64_t, std::vector<Version>> versions_;
};

} // namespace loredb::transaction 