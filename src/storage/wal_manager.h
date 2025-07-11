/// \file wal_manager.h
/// \brief Write-ahead logging for ensuring database durability.
/// \author LoreDB contributors
/// \ingroup storage
#pragma once

#include "../util/expected.h"
#include "../transaction/mvcc.h"
#include "record.h"
#include <fstream>
#include <mutex>
#include <string>
#include <functional>
#include <atomic>
#include <variant>
#include <vector>
#include <chrono>
#include "page_store.h"

namespace loredb::storage {

using LSN = uint64_t; // Log Sequence Number

// Backward compatibility for the simple interface
struct OperationLog {
    std::string json_line;
};

enum class WALRecordType : uint8_t {
    BEGIN_TRANSACTION = 1,
    COMMIT_TRANSACTION = 2,
    ABORT_TRANSACTION = 3,
    CREATE_NODE = 4,
    UPDATE_NODE = 5,
    DELETE_NODE = 6,
    CREATE_EDGE = 7,
    UPDATE_EDGE = 8,
    DELETE_EDGE = 9,
    CHECKPOINT = 10
};

struct WALHeader {
    uint32_t magic = 0xDEADBEEF;
    uint32_t version = 1;
    uint64_t creation_time;
    LSN last_checkpoint_lsn = 0;
};

struct WALRecord {
    LSN lsn;
    transaction::TransactionId tx_id;
    WALRecordType type;
    uint64_t timestamp;
    uint32_t data_size;
    
    // Operation-specific data
    std::variant<
        std::monostate,  // For simple records like BEGIN/COMMIT/ABORT
        std::pair<NodeId, std::vector<Property>>,  // For node operations
        std::tuple<EdgeId, NodeId, NodeId, std::string, std::vector<Property>>,  // For edge operations
        LSN  // For checkpoint records
    > data;
    
    // Calculate total record size for serialization
    size_t get_serialized_size() const;
};

/**
 * @class WALManager
 * @brief Manages write-ahead logging for transactions and operations.
 *
 * Supports transaction lifecycle logging, operation logging, checkpointing, and recovery.
 * Provides APIs for log replay and backward compatibility with legacy log formats.
 */
class WALManager {
public:
    // Rule-of-five: non-copyable, movable
    WALManager(const WALManager&) = delete;
    WALManager& operator=(const WALManager&) = delete;
    WALManager(WALManager&&) noexcept = delete;
    WALManager& operator=(WALManager&&) noexcept = delete;
    /**
     * @brief Construct a WALManager for the given log file path.
     * @param path Path to the WAL file.
     */
    explicit WALManager(const std::string& path);
    /** Destructor. */
    ~WALManager();

    // Transaction lifecycle logging
    util::expected<LSN, Error> log_begin_transaction(transaction::TransactionId tx_id);
    util::expected<LSN, Error> log_commit_transaction(transaction::TransactionId tx_id);
    util::expected<LSN, Error> log_abort_transaction(transaction::TransactionId tx_id);
    
    // Operation logging
    util::expected<LSN, Error> log_create_node(transaction::TransactionId tx_id, 
                                               NodeId node_id, 
                                               const std::vector<Property>& properties);
    util::expected<LSN, Error> log_update_node(transaction::TransactionId tx_id,
                                               NodeId node_id,
                                               const std::vector<Property>& properties);
    util::expected<LSN, Error> log_delete_node(transaction::TransactionId tx_id, NodeId node_id);
    
    util::expected<LSN, Error> log_create_edge(transaction::TransactionId tx_id,
                                               EdgeId edge_id,
                                               NodeId from_node,
                                               NodeId to_node,
                                               const std::string& label,
                                               const std::vector<Property>& properties);
    util::expected<LSN, Error> log_update_edge(transaction::TransactionId tx_id,
                                               EdgeId edge_id,
                                               const std::vector<Property>& properties);
    util::expected<LSN, Error> log_delete_edge(transaction::TransactionId tx_id, EdgeId edge_id);
    
    // Checkpointing and recovery
    util::expected<LSN, Error> checkpoint();
    util::expected<void, Error> force_sync(); // fsync for durability
    util::expected<void, Error> recover_from_log(const std::function<util::expected<void, Error>(const WALRecord&)>& apply_fn);
    
    // State management
    LSN get_current_lsn() const { return current_lsn_.load(); }
    LSN get_last_checkpoint_lsn() const { return last_checkpoint_lsn_.load(); }
    
    // Backward compatibility - deprecated, use structured logging methods above
    util::expected<void, Error> log_operation(const OperationLog& op);

private:
    util::expected<LSN, Error> write_record(const WALRecord& record);
    util::expected<WALRecord, Error> read_record(std::istream& file);
    util::expected<void, Error> write_header();
    util::expected<bool, Error> read_header();
    
    std::string path_;
    std::fstream log_file_;
    std::mutex mutex_;
    
    std::atomic<LSN> current_lsn_{1};
    std::atomic<LSN> last_checkpoint_lsn_{0};
    WALHeader header_;
};

} // namespace loredb::storage 