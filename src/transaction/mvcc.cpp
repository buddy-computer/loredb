#include "mvcc.h"

namespace loredb::transaction {

TransactionManager::TransactionManager() : next_transaction_id_(1), current_timestamp_(1) {
}

TransactionManager::~TransactionManager() = default;

std::shared_ptr<Transaction> TransactionManager::begin_transaction() {
    TransactionId tid = next_transaction_id_.fetch_add(1);
    auto txn = std::make_shared<Transaction>(tid);
    txn->start_timestamp = get_current_timestamp();
    
    // Track active transaction
    {
        std::unique_lock<std::shared_mutex> lock(transactions_mutex_);
        active_transactions_[tid] = txn;
    }
    
    return txn;
}

bool TransactionManager::commit_transaction(std::shared_ptr<Transaction> txn) {
    if (txn->state != TransactionState::ACTIVE) {
        return false;
    }
    
    txn->commit_timestamp = get_current_timestamp();
    txn->state = TransactionState::COMMITTED;
    
    // Move from active to completed
    {
        std::unique_lock<std::shared_mutex> lock(transactions_mutex_);
        active_transactions_.erase(txn->id);
        completed_transactions_[txn->id] = TransactionState::COMMITTED;
    }
    
    return true;
}

bool TransactionManager::abort_transaction(std::shared_ptr<Transaction> txn) {
    if (txn->state != TransactionState::ACTIVE) {
        return false;
    }
    
    txn->state = TransactionState::ABORTED;
    
    // Move from active to completed
    {
        std::unique_lock<std::shared_mutex> lock(transactions_mutex_);
        active_transactions_.erase(txn->id);
        completed_transactions_[txn->id] = TransactionState::ABORTED;
    }
    
    return true;
}

Timestamp TransactionManager::get_current_timestamp() {
    return current_timestamp_.fetch_add(1);
}

bool TransactionManager::is_visible(Timestamp created_at, Timestamp deleted_at, Timestamp read_timestamp) {
    if (created_at > read_timestamp) {
        return false; // Created after read timestamp
    }
    
    if (deleted_at > 0 && deleted_at <= read_timestamp) {
        return false; // Deleted before or at read timestamp
    }
    
    return true;
}

bool TransactionManager::is_transaction_committed(TransactionId tx_id) const {
    std::shared_lock<std::shared_mutex> lock(transactions_mutex_);
    
    // Check if transaction is still active
    if (active_transactions_.find(tx_id) != active_transactions_.end()) {
        return false;
    }
    
    // Check completed transactions
    auto it = completed_transactions_.find(tx_id);
    if (it != completed_transactions_.end()) {
        return it->second == TransactionState::COMMITTED;
    }
    
    // Transaction not found, assume it's old and committed
    // This handles the case where we've garbage collected old transaction records
    return true;
}

}  // namespace loredb::transaction