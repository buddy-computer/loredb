#pragma once

#include "page_store.h"
#include <vector>
#include <mutex>
#include <shared_mutex>
#include <tbb/concurrent_hash_map.h>
#include <tbb/concurrent_vector.h>

namespace loredb::storage {

// Simplified index manager using lock-free structures for property indexing
class SimpleIndexManager {
public:
    SimpleIndexManager() = default;
    ~SimpleIndexManager() = default;
    
    // Node property indexing
    void index_node_property(NodeId node_id, const std::string& key, const std::string& value);
    void remove_node_property_index(NodeId node_id, const std::string& key, const std::string& value);
    std::vector<NodeId> find_nodes_by_property(const std::string& key, const std::string& value) const;
    
    // Edge property indexing
    void index_edge_property(EdgeId edge_id, const std::string& key, const std::string& value);
    void remove_edge_property_index(EdgeId edge_id, const std::string& key, const std::string& value);
    std::vector<EdgeId> find_edges_by_property(const std::string& key, const std::string& value) const;
    
    // Adjacency list management
    void add_edge_to_adjacency(NodeId from_node, NodeId to_node, EdgeId edge_id);
    void remove_edge_from_adjacency(NodeId from_node, NodeId to_node, EdgeId edge_id);
    std::vector<EdgeId> get_outgoing_edges(NodeId node_id) const;
    std::vector<EdgeId> get_incoming_edges(NodeId node_id) const;
    std::vector<NodeId> get_adjacent_nodes(NodeId node_id) const;
    
    // Statistics
    size_t get_node_property_index_size() const;
    size_t get_edge_property_index_size() const;
    size_t get_adjacency_list_count() const;
    
    // Maintenance
    void clear_all_indexes();

private:
    struct PropertyKey {
        std::string key;
        std::string value;
        
        bool operator==(const PropertyKey& other) const {
            return key == other.key && value == other.value;
        }
    };
    
    struct PropertyKeyHash {
        static size_t hash(const PropertyKey& pk) {
            return std::hash<std::string>{}(pk.key) ^ (std::hash<std::string>{}(pk.value) << 1);
        }
        
        static bool equal(const PropertyKey& a, const PropertyKey& b) {
            return a.key == b.key && a.value == b.value;
        }
    };
    
    // Node property indexes
    tbb::concurrent_hash_map<PropertyKey, tbb::concurrent_vector<NodeId>, PropertyKeyHash> node_property_index_;
    
    // Edge property indexes
    tbb::concurrent_hash_map<PropertyKey, tbb::concurrent_vector<EdgeId>, PropertyKeyHash> edge_property_index_;
    
    // Adjacency lists (still using mutexes for now)
    mutable std::shared_mutex adjacency_mutex_;
    std::unordered_map<NodeId, std::vector<EdgeId>> outgoing_adjacency_;
    std::unordered_map<NodeId, std::vector<EdgeId>> incoming_adjacency_;
    std::unordered_map<EdgeId, std::pair<NodeId, NodeId>> edge_endpoints_;
};

}