/// \file executor.h
/// \brief Query execution engine for property graph queries.
/// \author LoreDB contributors
/// \ingroup query
#pragma once

#include "../storage/graph_store.h"
#include "../storage/simple_index_manager.h"
#include "../transaction/mvcc.h"
#include "../util/expected.h"
#include "query_types.h"
#include <atomic>
#include <memory>
#include <vector>
#include <string>
#include <functional>

namespace loredb::query {

/**
 * @class QueryExecutor
 * @brief Executes property graph queries (node, edge, traversal, aggregate, and document-specific).
 *
 * Provides APIs for direct node/edge queries, traversals, path finding, and document-centric operations.
 * Supports batch operations and integration with MVCC.
 */
class QueryExecutor {
public:
    /**
     * @brief Construct a QueryExecutor with a GraphStore and SimpleIndexManager.
     * @param graph_store Shared pointer to GraphStore.
     * @param index_manager Shared pointer to SimpleIndexManager.
     */
    QueryExecutor(std::shared_ptr<storage::GraphStore> graph_store,
                  std::shared_ptr<storage::SimpleIndexManager> index_manager);
    /** Destructor. */
    ~QueryExecutor();
    
    // Node queries
    util::expected<QueryResult, storage::Error> get_node_by_id(storage::NodeId node_id);
    util::expected<QueryResult, storage::Error> get_nodes_by_property(const std::string& key, const std::string& value);
    util::expected<QueryResult, storage::Error> get_all_nodes(size_t limit = 1000);
    
    // Edge queries
    util::expected<QueryResult, storage::Error> get_edge_by_id(storage::EdgeId edge_id);
    util::expected<QueryResult, storage::Error> get_edges_by_property(const std::string& key, const std::string& value);
    util::expected<QueryResult, storage::Error> get_all_edges(size_t limit = 1000);
    
    // Graph traversal queries
    util::expected<QueryResult, storage::Error> get_adjacent_nodes(storage::NodeId node_id);
    util::expected<QueryResult, storage::Error> get_outgoing_edges(storage::NodeId node_id);
    util::expected<QueryResult, storage::Error> get_incoming_edges(storage::NodeId node_id);
    
    // Path queries
    util::expected<QueryResult, storage::Error> find_shortest_path(storage::NodeId from_node, storage::NodeId to_node);
    util::expected<QueryResult, storage::Error> find_paths_with_length(storage::NodeId from_node, storage::NodeId to_node, size_t max_length);
    
    // Aggregate queries
    util::expected<QueryResult, storage::Error> count_nodes();
    util::expected<QueryResult, storage::Error> count_edges();
    util::expected<QueryResult, storage::Error> get_node_degree_stats();
    
    // Batch operations
    util::expected<QueryResult, storage::Error> batch_get_nodes(const std::vector<storage::NodeId>& node_ids);
    util::expected<QueryResult, storage::Error> batch_get_edges(const std::vector<storage::EdgeId>& edge_ids);
    
    // Document-specific queries
    util::expected<QueryResult, storage::Error> get_document_backlinks(storage::NodeId document_id);
    util::expected<QueryResult, storage::Error> get_document_outlinks(storage::NodeId document_id);
    util::expected<QueryResult, storage::Error> find_related_documents(storage::NodeId document_id, size_t max_results = 10);
    util::expected<QueryResult, storage::Error> suggest_links_for_document(storage::NodeId document_id, const std::string& content_snippet);

private:
    std::shared_ptr<storage::GraphStore> graph_store_;
    std::shared_ptr<storage::SimpleIndexManager> index_manager_;

    // Transaction context for MVCC-enabled graphs
    transaction::TransactionManager txn_manager_;
    transaction::TransactionId tx_id_;
    
    // Helper methods
    std::string property_value_to_string(const storage::PropertyValue& value);
    QueryResult node_to_result(const storage::NodeRecord& node, const std::vector<storage::Property>& properties);
    QueryResult edge_to_result(const storage::EdgeRecord& edge, const std::vector<storage::Property>& properties);
    
    // BFS for path finding
    std::vector<storage::NodeId> bfs_shortest_path(storage::NodeId from_node, storage::NodeId to_node);
    std::vector<std::vector<storage::NodeId>> bfs_paths_with_length(storage::NodeId from_node, storage::NodeId to_node, size_t max_length);
};

// Simplified streaming interfaces (removing C++20 generator dependency)
// These can be implemented later with proper coroutine support

}  // namespace loredb::query