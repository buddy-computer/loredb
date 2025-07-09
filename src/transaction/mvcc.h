#pragma once

#include <cstdint>
#include <atomic>
#include <memory>
#include <unordered_map>
#include <shared_mutex>

namespace graphdb::transaction {

using TransactionId = uint64_t;
using Timestamp = uint64_t;

enum class TransactionState {
    ACTIVE,
    COMMITTED,
    ABORTED
};

struct Transaction {
    TransactionId id;
    Timestamp start_timestamp;
    Timestamp commit_timestamp;
    TransactionState state;
    
    Transaction(TransactionId tid) 
        : id(tid), start_timestamp(0), commit_timestamp(0), state(TransactionState::ACTIVE) {}
};

class TransactionManager {
public:
    TransactionManager();
    ~TransactionManager();
    
    std::shared_ptr<Transaction> begin_transaction();
    bool commit_transaction(std::shared_ptr<Transaction> txn);
    bool abort_transaction(std::shared_ptr<Transaction> txn);
    
    // Check if a transaction is committed
    bool is_transaction_committed(TransactionId tx_id) const;
    
    Timestamp get_current_timestamp();
    bool is_visible(Timestamp created_at, Timestamp deleted_at, Timestamp read_timestamp);

private:
    std::atomic<TransactionId> next_transaction_id_;
    std::atomic<Timestamp> current_timestamp_;
    
    // Track transaction states
    mutable std::shared_mutex transactions_mutex_;
    std::unordered_map<TransactionId, std::shared_ptr<Transaction>> active_transactions_;
    std::unordered_map<TransactionId, TransactionState> completed_transactions_;
};

}  // namespace graphdb::transaction