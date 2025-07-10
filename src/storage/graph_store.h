/// \file graph_store.h
/// \brief Graph storage engine supporting MVCC, WAL, and property graph operations.
/// \author LoreDB contributors
/// \ingroup storage
#pragma once

#include "page_store.h"
#include "record.h"
#include "../transaction/mvcc_manager.h"
#include "../transaction/mvcc.h"
#include "wal_manager.h"
#include "../util/expected.h"
#include <memory>
#include <mutex>
#include <unordered_map>
#include <vector>

namespace loredb::storage {

/**
 * @class GraphStore
 * @brief Main storage engine for nodes and edges, supporting MVCC and WAL.
 *
 * Provides APIs for node/edge CRUD, batch operations, graph traversal, and statistics.
 * Can be constructed with or without MVCC and WAL support.
 */
class GraphStore {
public:
    /**
     * @brief Construct a GraphStore with legacy (non-MVCC) behavior.
     * @param page_store Unique pointer to a PageStore implementation.
     */
    explicit GraphStore(std::unique_ptr<PageStore> page_store);

    /**
     * @brief Construct a GraphStore with MVCC and optional WAL support.
     * @param page_store Unique pointer to a PageStore implementation.
     * @param mvcc_manager Shared pointer to MVCCManager for versioning.
     * @param wal_manager Shared pointer to WALManager for write-ahead logging (optional).
     */
    GraphStore(std::unique_ptr<PageStore> page_store,
               std::shared_ptr<transaction::MVCCManager> mvcc_manager,
               std::shared_ptr<WALManager> wal_manager = nullptr);
    /** Destructor. */
    ~GraphStore();
    
    /**
     * @brief Create a node in a transaction (MVCC-aware).
     * @param tx_id Transaction ID.
     * @param properties Node properties.
     * @return NodeId or Error.
     */
    util::expected<NodeId, Error> create_node(transaction::TransactionId tx_id,
                                             const std::vector<Property>& properties);
    /**
     * @brief Get a node in a transaction (MVCC-aware).
     * @param tx_id Transaction ID.
     * @param node_id Node ID.
     * @return NodeRecord and properties, or Error.
     */
    util::expected<std::pair<NodeRecord, std::vector<Property>>, Error> get_node(
        transaction::TransactionId tx_id,
        NodeId node_id);
    /**
     * @brief Get an edge in a transaction (MVCC-aware).
     * @param tx_id Transaction ID.
     * @param edge_id Edge ID.
     * @return EdgeRecord and properties, or Error.
     */
    util::expected<std::pair<EdgeRecord, std::vector<Property>>, Error> get_edge(
        transaction::TransactionId tx_id,
        EdgeId edge_id);
    /**
     * @brief Check if MVCC is enabled.
     * @return True if MVCC is enabled.
     */
    bool has_mvcc() const { return mvcc_manager_ != nullptr; }

    util::expected<NodeId, Error> create_node(const std::vector<Property>& properties);
    util::expected<void, Error> update_node(
        NodeId node_id,
        const std::vector<Property>& properties);
    /**
     * @brief Delete a node immediately (legacy, non-MVCC version).
     * 
     * This method physically removes the node from storage immediately
     * and is used only when MVCC is disabled. Unlike the MVCC version,
     * this does not create a tombstone record and the deletion is
     * irreversible within the same transaction context.
     * 
     * @param node_id The ID of the node to delete.
     * @return Success or Error.
     */
    util::expected<void, Error> delete_node(NodeId node_id);

    // MVCC-aware variants
    util::expected<void, Error> update_node(
        transaction::TransactionId tx_id,
        NodeId node_id,
        const std::vector<Property>& properties);
    util::expected<void, Error> delete_node(
        transaction::TransactionId tx_id,
        NodeId node_id);
    util::expected<std::pair<NodeRecord, std::vector<Property>>, Error> get_node(NodeId node_id);
    
    // Edge operations
    // MVCC-aware versions
    util::expected<EdgeId, Error> create_edge(
        transaction::TransactionId tx_id,
        NodeId from_node,
        NodeId to_node,
        const std::string& label,
        const std::vector<Property>& properties);
    util::expected<void, Error> update_edge(
        transaction::TransactionId tx_id,
        EdgeId edge_id,
        const std::vector<Property>& properties);
    util::expected<void, Error> delete_edge(
        transaction::TransactionId tx_id,
        EdgeId edge_id);

    util::expected<EdgeId, Error> create_edge(NodeId from_node, NodeId to_node, 
                                           const std::string& label,
                                           const std::vector<Property>& properties);
    util::expected<void, Error> update_edge(EdgeId edge_id, const std::vector<Property>& properties);
    util::expected<void, Error> delete_edge(EdgeId edge_id);
    util::expected<std::pair<EdgeRecord, std::vector<Property>>, Error> get_edge(EdgeId edge_id);
    
    // Graph traversal
    util::expected<std::vector<EdgeId>, Error> get_outgoing_edges(NodeId node_id);
    util::expected<std::vector<EdgeId>, Error> get_incoming_edges(NodeId node_id);
    util::expected<std::vector<NodeId>, Error> get_adjacent_nodes(NodeId node_id);
    
    // Batch operations
    util::expected<void, Error> batch_create_nodes(
        const std::vector<std::vector<Property>>& node_properties,
        std::vector<NodeId>& created_node_ids);
    util::expected<void, Error> batch_create_edges(
        const std::vector<std::tuple<NodeId, NodeId, std::string, std::vector<Property>>>& edges,
        std::vector<EdgeId>& created_edge_ids);
    
    // Statistics
    size_t get_node_count() const;
    size_t get_edge_count() const;
    
    // Maintenance
    util::expected<void, Error> sync();
    util::expected<void, Error> compact();

private:
    util::expected<PageId, Error> allocate_node_page();
    util::expected<PageId, Error> allocate_edge_page();
    util::expected<void, Error> store_node_record(NodeId node_id, const NodeRecord& node, 
                                                const std::vector<Property>& properties);
    util::expected<void, Error> store_edge_record(EdgeId edge_id, const EdgeRecord& edge, 
                                                const std::vector<Property>& properties);
    util::expected<void, Error> update_adjacency_lists(
        NodeId from_node,
        NodeId to_node,
        EdgeId edge_id,
        bool add);
    
    NodeId get_next_node_id();
    EdgeId get_next_edge_id();
    
    std::unique_ptr<PageStore> page_store_;
    
    // ID generators
    std::atomic<NodeId> next_node_id_;
    std::atomic<EdgeId> next_edge_id_;
    
    // Node and edge location tracking
    std::mutex node_index_mutex_;
    std::unordered_map<NodeId, PageId> node_page_index_;
    
    std::mutex edge_index_mutex_;
    std::unordered_map<EdgeId, PageId> edge_page_index_;
    
    // Adjacency lists (simplified for now)
    std::mutex adjacency_mutex_;
    std::unordered_map<NodeId, std::vector<EdgeId>> outgoing_edges_;
    std::unordered_map<NodeId, std::vector<EdgeId>> incoming_edges_;
    
    // Statistics
    std::atomic<size_t> node_count_;
    std::atomic<size_t> edge_count_;
    
    // Page allocation
    std::mutex page_alloc_mutex_;
    std::vector<PageId> node_pages_;
    std::vector<PageId> edge_pages_;

    // Optional MVCC manager
    std::shared_ptr<transaction::MVCCManager> mvcc_manager_;
    std::shared_ptr<WALManager> wal_manager_;
};

}  // namespace loredb::storage