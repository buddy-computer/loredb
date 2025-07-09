#pragma once

#include "page_store.h"
#include <string>
#include <vector>

namespace loredb::storage {

/**
 * @class IndexManager
 * @brief Abstract interface for managing graph indexes (properties, adjacency).
 *
 * Defines the contract for creating, updating, and querying property indexes and adjacency lists.
 */
class IndexManager {
public:
    virtual ~IndexManager() = default;
    
    // Node property indexing
    virtual void index_node_property(NodeId node_id, const std::string& key, const std::string& value) = 0;
    virtual void remove_node_property_index(NodeId node_id, const std::string& key) = 0;
    virtual std::vector<NodeId> find_nodes_by_property(
        const std::string& key,
        const std::string& value
    ) const = 0;
    
    // Edge property indexing
    virtual void index_edge_property(EdgeId edge_id, const std::string& key, const std::string& value) = 0;
    virtual void remove_edge_property_index(EdgeId edge_id, const std::string& key) = 0;
    virtual std::vector<EdgeId> find_edges_by_property(
        const std::string& key,
        const std::string& value
    ) const = 0;
    
    // Adjacency list management
    virtual void add_edge_to_adjacency(NodeId from_node, NodeId to_node, EdgeId edge_id) = 0;
    virtual void remove_edge_from_adjacency(NodeId from_node, NodeId to_node, EdgeId edge_id) = 0;
    virtual std::vector<EdgeId> get_outgoing_edges(NodeId node_id) const = 0;
    virtual std::vector<EdgeId> get_incoming_edges(NodeId node_id) const = 0;
    virtual std::vector<NodeId> get_adjacent_nodes(NodeId node_id) const = 0;
    
    // Statistics
    virtual size_t get_node_property_index_size() const = 0;
    virtual size_t get_edge_property_index_size() const = 0;
    virtual size_t get_adjacency_list_count() const = 0;
    
    // Maintenance
    virtual void clear_all_indexes() = 0;
};

}  // namespace loredb::storage