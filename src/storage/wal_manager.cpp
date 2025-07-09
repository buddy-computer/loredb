#include "wal_manager.h"
#include "../util/logger.h"
#include <fstream>
#include <chrono>
#include <cstring>
#include <unistd.h>
#include <cstdio>

namespace loredb::storage {

size_t WALRecord::get_serialized_size() const {
    size_t base_size = sizeof(LSN) + sizeof(transaction::TransactionId) + 
                       sizeof(WALRecordType) + sizeof(uint64_t) + sizeof(uint32_t);
    
    if (std::holds_alternative<std::pair<NodeId, std::vector<Property>>>(data)) {
        const auto& [node_id, props] = std::get<std::pair<NodeId, std::vector<Property>>>(data);
        base_size += sizeof(NodeId);
        for (const auto& prop : props) {
            base_size += prop.key.size() + sizeof(uint32_t); // key length + key
            // Simplified property value size calculation
            base_size += sizeof(uint32_t) + 64; // type + estimated value size
        }
    } else if (std::holds_alternative<std::tuple<EdgeId, NodeId, NodeId, std::string, std::vector<Property>>>(data)) {
        const auto& [edge_id, from, to, label, props] =
            std::get<std::tuple<EdgeId, NodeId, NodeId, std::string, std::vector<Property>>>(data);
        base_size += sizeof(EdgeId) + sizeof(NodeId) + sizeof(NodeId) + sizeof(uint32_t) + label.size();
        for (const auto& prop : props) {
            base_size += prop.key.size() + sizeof(uint32_t);
            base_size += sizeof(uint32_t) + 64;
        }
    } else if (std::holds_alternative<LSN>(data)) {
        base_size += sizeof(LSN);
    }
    
    return base_size;
}

WALManager::WALManager(const std::string& path) : path_(path) {
    // Try to open existing log file first
    log_file_.open(path_, std::ios::in | std::ios::out | std::ios::binary);
    
    if (!log_file_.is_open()) {
        // Create new log file
        log_file_.open(path_, std::ios::out | std::ios::binary);
        if (!log_file_.is_open()) {
            LOG_ERROR("Failed to create WAL file: {}", path_);
            return;
        }
        log_file_.close();
        log_file_.open(path_, std::ios::in | std::ios::out | std::ios::binary);
        
        // Write header for new file
        header_.creation_time = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();
        
        if (auto result = write_header(); !result.has_value()) {
            LOG_ERROR("Failed to write WAL header: {}", result.error().message);
        }
    } else {
        // Read existing header
        if (auto result = read_header(); !result.has_value()) {
            LOG_ERROR("Failed to read WAL header: {}", result.error().message);
        } else if (result.value()) {
            // Header exists, scan to find current LSN
            log_file_.seekg(0, std::ios::end);
            std::streampos file_size = log_file_.tellg();
            log_file_.seekg(sizeof(WALHeader), std::ios::beg);
            
            LSN max_lsn = 0;
            while (log_file_.tellg() < file_size) {
                auto record_result = read_record(log_file_);
                if (!record_result.has_value()) break;
                max_lsn = std::max(max_lsn, record_result.value().lsn);
            }
            current_lsn_ = max_lsn + 1;
            last_checkpoint_lsn_ = header_.last_checkpoint_lsn;
            
            // Position at end for writing
            log_file_.clear();
            log_file_.seekp(0, std::ios::end);
        }
    }
    
    LOG_INFO("WAL initialized: path={}, current_lsn={}, checkpoint_lsn={}", 
             path_, current_lsn_.load(), last_checkpoint_lsn_.load());
}

WALManager::~WALManager() {
    if (log_file_.is_open()) {
        force_sync();
        log_file_.close();
    }
}

util::expected<LSN, Error> WALManager::log_begin_transaction(transaction::TransactionId tx_id) {
    WALRecord record;
    record.lsn = current_lsn_.fetch_add(1);
    record.tx_id = tx_id;
    record.type = WALRecordType::BEGIN_TRANSACTION;
    record.timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    record.data_size = 0;
    record.data = std::monostate{};
    
    return write_record(record);
}

util::expected<LSN, Error> WALManager::log_commit_transaction(transaction::TransactionId tx_id) {
    WALRecord record;
    record.lsn = current_lsn_.fetch_add(1);
    record.tx_id = tx_id;
    record.type = WALRecordType::COMMIT_TRANSACTION;
    record.timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    record.data_size = 0;
    record.data = std::monostate{};
    
    auto result = write_record(record);
    if (result.has_value()) {
        force_sync(); // Ensure commit is durable
    }
    return result;
}

util::expected<LSN, Error> WALManager::log_abort_transaction(transaction::TransactionId tx_id) {
    WALRecord record;
    record.lsn = current_lsn_.fetch_add(1);
    record.tx_id = tx_id;
    record.type = WALRecordType::ABORT_TRANSACTION;
    record.timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    record.data_size = 0;
    record.data = std::monostate{};
    
    return write_record(record);
}

util::expected<LSN, Error> WALManager::log_create_node(transaction::TransactionId tx_id, 
                                                       NodeId node_id, 
                                                       const std::vector<Property>& properties) {
    WALRecord record;
    record.lsn = current_lsn_.fetch_add(1);
    record.tx_id = tx_id;
    record.type = WALRecordType::CREATE_NODE;
    record.timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    record.data = std::make_pair(node_id, properties);
    record.data_size = static_cast<uint32_t>(record.get_serialized_size() - 
                                            (sizeof(LSN) + sizeof(transaction::TransactionId) + 
                                             sizeof(WALRecordType) + sizeof(uint64_t) + sizeof(uint32_t)));
    
    return write_record(record);
}

util::expected<LSN, Error> WALManager::log_update_node(transaction::TransactionId tx_id,
                                                       NodeId node_id,
                                                       const std::vector<Property>& properties) {
    WALRecord record;
    record.lsn = current_lsn_.fetch_add(1);
    record.tx_id = tx_id;
    record.type = WALRecordType::UPDATE_NODE;
    record.timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    record.data = std::make_pair(node_id, properties);
    record.data_size = static_cast<uint32_t>(record.get_serialized_size() - 
                                            (sizeof(LSN) + sizeof(transaction::TransactionId) + 
                                             sizeof(WALRecordType) + sizeof(uint64_t) + sizeof(uint32_t)));
    
    return write_record(record);
}

util::expected<LSN, Error> WALManager::log_delete_node(transaction::TransactionId tx_id, NodeId node_id) {
    WALRecord record;
    record.lsn = current_lsn_.fetch_add(1);
    record.tx_id = tx_id;
    record.type = WALRecordType::DELETE_NODE;
    record.timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    record.data = std::make_pair(node_id, std::vector<Property>{});
    record.data_size = sizeof(NodeId);
    
    return write_record(record);
}

util::expected<LSN, Error> WALManager::log_create_edge(transaction::TransactionId tx_id,
                                                       EdgeId edge_id,
                                                       NodeId from_node,
                                                       NodeId to_node,
                                                       const std::string& label,
                                                       const std::vector<Property>& properties) {
    WALRecord record;
    record.lsn = current_lsn_.fetch_add(1);
    record.tx_id = tx_id;
    record.type = WALRecordType::CREATE_EDGE;
    record.timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    record.data = std::make_tuple(edge_id, from_node, to_node, label, properties);
    record.data_size = static_cast<uint32_t>(record.get_serialized_size() - 
                                            (sizeof(LSN) + sizeof(transaction::TransactionId) + 
                                             sizeof(WALRecordType) + sizeof(uint64_t) + sizeof(uint32_t)));
    
    return write_record(record);
}

util::expected<LSN, Error> WALManager::log_update_edge(transaction::TransactionId tx_id,
                                                       EdgeId edge_id,
                                                       const std::vector<Property>& properties) {
    WALRecord record;
    record.lsn = current_lsn_.fetch_add(1);
    record.tx_id = tx_id;
    record.type = WALRecordType::UPDATE_EDGE;
    record.timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    record.data = std::make_tuple(edge_id, NodeId{0}, NodeId{0}, std::string{}, properties);
    record.data_size = static_cast<uint32_t>(record.get_serialized_size() - 
                                            (sizeof(LSN) + sizeof(transaction::TransactionId) + 
                                             sizeof(WALRecordType) + sizeof(uint64_t) + sizeof(uint32_t)));
    
    return write_record(record);
}

util::expected<LSN, Error> WALManager::log_delete_edge(transaction::TransactionId tx_id, EdgeId edge_id) {
    WALRecord record;
    record.lsn = current_lsn_.fetch_add(1);
    record.tx_id = tx_id;
    record.type = WALRecordType::DELETE_EDGE;
    record.timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    record.data = std::make_tuple(edge_id, NodeId{0}, NodeId{0}, std::string{}, std::vector<Property>{});
    record.data_size = sizeof(EdgeId);
    
    return write_record(record);
}

util::expected<LSN, Error> WALManager::checkpoint() {
    LSN checkpoint_lsn = current_lsn_.fetch_add(1);
    
    WALRecord record;
    record.lsn = checkpoint_lsn;
    record.tx_id = 0; // No transaction
    record.type = WALRecordType::CHECKPOINT;
    record.timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    record.data = checkpoint_lsn;
    record.data_size = sizeof(LSN);
    
    auto result = write_record(record);
    if (result.has_value()) {
        {
            std::lock_guard<std::mutex> lock(mutex_);
            last_checkpoint_lsn_ = checkpoint_lsn;
            header_.last_checkpoint_lsn = checkpoint_lsn;
            
            // Update header on disk
            auto pos = log_file_.tellp();
            log_file_.seekp(0);
            write_header();
            log_file_.seekp(pos);
        }
        
        force_sync();
        LOG_INFO("WAL checkpoint completed: LSN={}", checkpoint_lsn);
    }
    
    return result;
}

util::expected<void, Error> WALManager::force_sync() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!log_file_.is_open()) {
        return util::unexpected(Error{ErrorCode::IO_ERROR, "Log file not open"});
    }
    
    log_file_.flush();
    if (!log_file_) {
        return util::unexpected(Error{ErrorCode::IO_ERROR, "Flush failed"});
    }
    
    // Force OS to write to disk - simplified for cross-platform compatibility
    // In a production system, you'd want proper file descriptor management
    log_file_.sync();
    
    return {};
}

util::expected<void, Error> WALManager::recover_from_log(const std::function<util::expected<void, Error>(const WALRecord&)>& apply_fn) {
    std::ifstream recovery_file(path_, std::ios::binary);
    if (!recovery_file.is_open()) {
        return util::unexpected(Error{ErrorCode::IO_ERROR, "Cannot open log for recovery"});
    }
    
    // Skip header
    recovery_file.seekg(sizeof(WALHeader));
    
    size_t records_applied = 0;
    LSN max_lsn = 0;
    
    while (recovery_file.good()) {
        auto record_result = read_record(recovery_file);
        if (!record_result.has_value()) {
            if (recovery_file.eof()) break;
            LOG_WARN("Failed to read WAL record during recovery: {}", record_result.error().message);
            continue;
        }
        
        const auto& record = record_result.value();
        max_lsn = std::max(max_lsn, record.lsn);
        
        auto apply_result = apply_fn(record);
        if (!apply_result.has_value()) {
            LOG_ERROR("Failed to apply WAL record LSN={}: {}", record.lsn, apply_result.error().message);
            return apply_result;
        }
        
        records_applied++;
    }
    
    current_lsn_ = max_lsn + 1;
    LOG_INFO("WAL recovery completed: {} records applied, max_lsn={}", records_applied, max_lsn);
    
    return {};
}

util::expected<LSN, Error> WALManager::write_record(const WALRecord& record) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (!log_file_.is_open()) {
        return util::unexpected(Error{ErrorCode::IO_ERROR, "Log file not open"});
    }
    
    // Write fixed-size header
    log_file_.write(reinterpret_cast<const char*>(&record.lsn), sizeof(record.lsn));
    log_file_.write(reinterpret_cast<const char*>(&record.tx_id), sizeof(record.tx_id));
    log_file_.write(reinterpret_cast<const char*>(&record.type), sizeof(record.type));
    log_file_.write(reinterpret_cast<const char*>(&record.timestamp), sizeof(record.timestamp));
    log_file_.write(reinterpret_cast<const char*>(&record.data_size), sizeof(record.data_size));
    
    // Write variable-size data based on type
    if (std::holds_alternative<std::pair<NodeId, std::vector<Property>>>(record.data)) {
        const auto& [node_id, props] = std::get<std::pair<NodeId, std::vector<Property>>>(record.data);
        log_file_.write(reinterpret_cast<const char*>(&node_id), sizeof(node_id));
        
        uint32_t prop_count = static_cast<uint32_t>(props.size());
        log_file_.write(reinterpret_cast<const char*>(&prop_count), sizeof(prop_count));
        
        for (const auto& prop : props) {
            uint32_t key_len = static_cast<uint32_t>(prop.key.size());
            log_file_.write(reinterpret_cast<const char*>(&key_len), sizeof(key_len));
            log_file_.write(prop.key.data(), key_len);
            
            // Simplified property value serialization - just write the variant index for now
            uint32_t value_type = static_cast<uint32_t>(prop.value.index());
            log_file_.write(reinterpret_cast<const char*>(&value_type), sizeof(value_type));
        }
    } else if (std::holds_alternative<std::tuple<EdgeId, NodeId, NodeId, std::string, std::vector<Property>>>(record.data)) {
        const auto& [edge_id, from, to, label, props] = std::get<std::tuple<EdgeId, NodeId, NodeId, std::string, std::vector<Property>>>(record.data);
        
        log_file_.write(reinterpret_cast<const char*>(&edge_id), sizeof(edge_id));
        log_file_.write(reinterpret_cast<const char*>(&from), sizeof(from));
        log_file_.write(reinterpret_cast<const char*>(&to), sizeof(to));
        
        uint32_t label_len = static_cast<uint32_t>(label.size());
        log_file_.write(reinterpret_cast<const char*>(&label_len), sizeof(label_len));
        log_file_.write(label.data(), label_len);
        
        uint32_t prop_count = static_cast<uint32_t>(props.size());
        log_file_.write(reinterpret_cast<const char*>(&prop_count), sizeof(prop_count));
        
        for (const auto& prop : props) {
            uint32_t key_len = static_cast<uint32_t>(prop.key.size());
            log_file_.write(reinterpret_cast<const char*>(&key_len), sizeof(key_len));
            log_file_.write(prop.key.data(), key_len);
            
            uint32_t value_type = static_cast<uint32_t>(prop.value.index());
            log_file_.write(reinterpret_cast<const char*>(&value_type), sizeof(value_type));
        }
    } else if (std::holds_alternative<LSN>(record.data)) {
        LSN checkpoint_lsn = std::get<LSN>(record.data);
        log_file_.write(reinterpret_cast<const char*>(&checkpoint_lsn), sizeof(checkpoint_lsn));
    }
    
    if (!log_file_) {
        return util::unexpected(Error{ErrorCode::IO_ERROR, "Write failed"});
    }
    
    return record.lsn;
}

util::expected<WALRecord, Error> WALManager::read_record(std::istream& file) {
    WALRecord record;
    
    // Read fixed-size header
    file.read(reinterpret_cast<char*>(&record.lsn), sizeof(record.lsn));
    file.read(reinterpret_cast<char*>(&record.tx_id), sizeof(record.tx_id));
    file.read(reinterpret_cast<char*>(&record.type), sizeof(record.type));
    file.read(reinterpret_cast<char*>(&record.timestamp), sizeof(record.timestamp));
    file.read(reinterpret_cast<char*>(&record.data_size), sizeof(record.data_size));
    
    if (!file) {
        return util::unexpected(Error{ErrorCode::IO_ERROR, "Failed to read record header"});
    }
    
    // Read variable-size data based on type
    switch (record.type) {
        case WALRecordType::BEGIN_TRANSACTION:
        case WALRecordType::COMMIT_TRANSACTION:
        case WALRecordType::ABORT_TRANSACTION:
            record.data = std::monostate{};
            break;
            
        case WALRecordType::CREATE_NODE:
        case WALRecordType::UPDATE_NODE:
        case WALRecordType::DELETE_NODE: {
            NodeId node_id;
            file.read(reinterpret_cast<char*>(&node_id), sizeof(node_id));
            
            std::vector<Property> props;
            if (record.type != WALRecordType::DELETE_NODE) {
                uint32_t prop_count;
                file.read(reinterpret_cast<char*>(&prop_count), sizeof(prop_count));
                
                for (uint32_t i = 0; i < prop_count; ++i) {
                    uint32_t key_len;
                    file.read(reinterpret_cast<char*>(&key_len), sizeof(key_len));
                    
                    std::string key(key_len, '\0');
                    file.read(key.data(), key_len);
                    
                    uint32_t value_type;
                    file.read(reinterpret_cast<char*>(&value_type), sizeof(value_type));
                    
                    // For now, just create a placeholder property
                    props.emplace_back(std::move(key), std::string("placeholder"));
                }
            }
            
            record.data = std::make_pair(node_id, std::move(props));
            break;
        }
        
        case WALRecordType::CHECKPOINT: {
            LSN checkpoint_lsn;
            file.read(reinterpret_cast<char*>(&checkpoint_lsn), sizeof(checkpoint_lsn));
            record.data = checkpoint_lsn;
            break;
        }
        
        default:
            return util::unexpected(Error{ErrorCode::CORRUPTION, "Unknown WAL record type"});
    }
    
    if (!file) {
        return util::unexpected(Error{ErrorCode::IO_ERROR, "Failed to read record data"});
    }
    
    return record;
}

util::expected<void, Error> WALManager::write_header() {
    log_file_.seekp(0);
    log_file_.write(reinterpret_cast<const char*>(&header_), sizeof(header_));
    if (!log_file_) {
        return util::unexpected(Error{ErrorCode::IO_ERROR, "Failed to write header"});
    }
    return {};
}

util::expected<bool, Error> WALManager::read_header() {
    log_file_.seekg(0);
    log_file_.read(reinterpret_cast<char*>(&header_), sizeof(header_));
    
    if (!log_file_) {
        if (log_file_.eof()) {
            return false; // No header yet
        }
        return util::unexpected(Error{ErrorCode::IO_ERROR, "Failed to read header"});
    }
    
    if (header_.magic != 0xDEADBEEF) {
        return util::unexpected(Error{ErrorCode::CORRUPTION, "Invalid WAL magic number"});
    }
    
    if (header_.version != 1) {
        return util::unexpected(Error{ErrorCode::CORRUPTION, "Unsupported WAL version"});
    }
    
    return true;
}

util::expected<void, Error> WALManager::log_operation(const OperationLog& op) {
    // Backward compatibility - convert JSON string to a simple log record
    std::lock_guard<std::mutex> lock(mutex_);
    if (!log_file_.is_open()) {
        return util::unexpected(Error{ErrorCode::IO_ERROR, "Log file not open"});
    }
    log_file_ << op.json_line << "\n";
    if (!log_file_) {
        return util::unexpected(Error{ErrorCode::IO_ERROR, "Write failed"});
    }
    return {};
}

} // namespace loredb::storage