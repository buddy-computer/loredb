#pragma once

#include "mvcc.h"
#include <unordered_map>
#include <vector>
#include <mutex>
#include <condition_variable>
#include <memory>

namespace loredb::transaction {

using ResourceId = uint64_t;

enum class LockMode {
    SHARED,
    EXCLUSIVE
};

class LockManager {
public:
    LockManager() = default;

    // Attempts to acquire a lock. Returns true on success, false on failure (e.g. timeout)
    bool lock(TransactionId tx_id, ResourceId resource_id, LockMode mode);

    // Releases a lock
    void unlock(TransactionId tx_id, ResourceId resource_id);

    // Releases all locks held by a transaction
    void unlock_all(TransactionId tx_id);

private:
    struct LockRequest {
        TransactionId tx_id;
        LockMode mode;
        bool granted = false;
    };

    std::mutex mutex_;
    std::condition_variable cv_;
    std::unordered_map<ResourceId, std::vector<std::shared_ptr<LockRequest>>> lock_table_;

    // For deadlock detection
    std::unordered_map<TransactionId, std::vector<TransactionId>> waits_for_graph_;
    bool detect_deadlock(TransactionId start_tx);
    bool has_cycle(TransactionId u, std::unordered_map<TransactionId, bool>& visited, std::unordered_map<TransactionId, bool>& recursion_stack);
};

} // namespace loredb::transaction 