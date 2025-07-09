#include "mvcc_manager.h"
#include <algorithm>

namespace loredb::transaction {

MVCCManager::MVCCManager(std::shared_ptr<TransactionManager> txn_manager) 
    : txn_manager_(std::move(txn_manager)), lock_manager_(std::make_unique<LockManager>()) {
}

util::expected<Version, MVCCError> MVCCManager::read_version(uint64_t key, TransactionId tx_id) const {
    std::shared_lock<std::shared_mutex> lock(mutex_);
    auto it = versions_.find(key);
    if (it == versions_.end()) {
        return util::unexpected(MVCCError{MVCCErrorCode::NOT_FOUND, "Key not found"});
    }

    // Traverse versions in reverse (newest first)
    const auto & vec = it->second;
    for (auto rit = vec.rbegin(); rit != vec.rend(); ++rit) {
        const Version & ver = *rit;
        if (is_version_visible(ver, tx_id)) {
            return ver;
        }
    }
    return util::unexpected(MVCCError{MVCCErrorCode::NOT_FOUND, "No visible version for tx"});
}

util::expected<void, MVCCError> MVCCManager::write_version(uint64_t key, Version version) {
    if (!lock_manager_->lock(version.created_tx_id, key, LockMode::EXCLUSIVE)) {
        return util::unexpected(MVCCError{MVCCErrorCode::CONFLICT, "Deadlock detected"});
    }

    std::unique_lock<std::shared_mutex> lock(mutex_);
    auto & vec = versions_[key];
    // Check the latest version for conflicts (write-write)
    if (!vec.empty()) {
        Version & latest = vec.back();
        if (latest.deleted_tx_id == 0) {
            // Mark latest as deleted by this tx
            latest.deleted_tx_id = version.created_tx_id;
        }
    }
    vec.push_back(std::move(version));
    return {};
}

void MVCCManager::garbage_collect(TransactionId min_active_tx_id) {
    std::unique_lock<std::shared_mutex> lock(mutex_);
    for (auto it = versions_.begin(); it != versions_.end(); ) {
        auto & vec = it->second;
        // Remove versions that are deleted and visibility ended before min_active_tx_id
        vec.erase(std::remove_if(vec.begin(), vec.end(), [min_active_tx_id](const Version &v) {
            return v.deleted_tx_id != 0 && v.deleted_tx_id < min_active_tx_id;
        }), vec.end());

        if (vec.empty()) {
            it = versions_.erase(it);
        } else {
            ++it;
        }
    }
}

bool MVCCManager::is_version_visible(const Version& version, TransactionId tx_id) const {
    // A version is visible if:
    // 1. It was created by a committed transaction that finished before the reader started
    // 2. OR it was created by the same transaction (dirty read)
    // 3. AND it hasn't been deleted by a committed transaction that finished before the reader started
    
    // Same transaction can see its own writes
    if (version.created_tx_id == tx_id) {
        return version.deleted_tx_id == 0 || version.deleted_tx_id != tx_id;
    }
    
    // Check if creating transaction is committed
    if (!txn_manager_->is_transaction_committed(version.created_tx_id)) {
        return false;
    }
    
    // Check if reader started after creator committed
    if (version.created_tx_id > tx_id) {
        return false;
    }
    
    // Check deletion visibility
    if (version.deleted_tx_id != 0) {
        // If deleted by same transaction, not visible
        if (version.deleted_tx_id == tx_id) {
            return false;
        }
        
        // If deleted by committed transaction before reader started, not visible
        if (txn_manager_->is_transaction_committed(version.deleted_tx_id) && 
            version.deleted_tx_id <= tx_id) {
            return false;
        }
    }
    
    return true;
}

} // namespace loredb::transaction 