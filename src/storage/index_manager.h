#pragma once

#include "page_store.h"
#include <atomic>
#include <memory>
#include <vector>
#include <mutex>
#include <shared_mutex>

namespace graphdb::storage {

// Lock-free hash table for property indexing
template<typename K, typename V>
class LockFreeHashTable {
public:
    // Rule-of-five: non-copyable, movable
    LockFreeHashTable(const LockFreeHashTable&) = delete;
    LockFreeHashTable& operator=(const LockFreeHashTable&) = delete;
    LockFreeHashTable(LockFreeHashTable&&) noexcept = default;
    LockFreeHashTable& operator=(LockFreeHashTable&&) noexcept = default;

    struct Entry {
        std::atomic<K> key;
        std::atomic<V> value;
        std::atomic<Entry*> next;
        
        Entry() : key(K{}), value(V{}), next(nullptr) {}
    };
    
    explicit LockFreeHashTable(size_t initial_capacity = 1024);
    ~LockFreeHashTable();
    
    bool insert(const K& key, const V& value);
    bool find(const K& key, V& value) const;
    bool remove(const K& key);
    
    void clear();
    size_t size() const;
    
private:
    std::unique_ptr<std::atomic<Entry*>[]> buckets_;
    size_t capacity_;
    std::atomic<size_t> size_;
    
    size_t hash(const K& key) const;
    Entry* allocate_entry();
    void deallocate_entry(Entry* entry);
    
    // Memory management
    std::vector<std::unique_ptr<Entry[]>> entry_pools_;
    std::atomic<size_t> next_entry_index_;
    std::mutex pool_mutex_;
};

// Lock-free adjacency list for graph traversal
class LockFreeAdjacencyList {
public:
    // Rule-of-five: non-copyable, movable
    LockFreeAdjacencyList(const LockFreeAdjacencyList&) = delete;
    LockFreeAdjacencyList& operator=(const LockFreeAdjacencyList&) = delete;
    LockFreeAdjacencyList(LockFreeAdjacencyList&&) noexcept = default;
    LockFreeAdjacencyList& operator=(LockFreeAdjacencyList&&) noexcept = default;

    struct EdgeEntry {
        std::atomic<EdgeId> edge_id;
        std::atomic<NodeId> target_node;
        std::atomic<EdgeEntry*> next;
        
        EdgeEntry() : edge_id(0), target_node(0), next(nullptr) {}
    };
    
    LockFreeAdjacencyList();
    ~LockFreeAdjacencyList();
    
    void add_edge(EdgeId edge_id, NodeId target_node);
    void remove_edge(EdgeId edge_id);
    std::vector<EdgeId> get_edges() const;
    std::vector<NodeId> get_adjacent_nodes() const;
    
    size_t size() const;
    void clear();

private:
    std::atomic<EdgeEntry*> head_;
    std::atomic<size_t> size_;
    
    EdgeEntry* allocate_edge_entry();
    void deallocate_edge_entry(EdgeEntry* entry);
    
    // Memory management
    std::vector<std::unique_ptr<EdgeEntry[]>> entry_pools_;
    std::atomic<size_t> next_entry_index_;
    std::mutex pool_mutex_;
};

// Index manager for coordinating all indexes
class IndexManager {
public:
    explicit IndexManager(size_t initial_capacity = 1024);
    ~IndexManager();
    
    // Node property indexing
    void index_node_property(NodeId node_id, const std::string& key, const std::string& value);
    void remove_node_property_index(NodeId node_id, const std::string& key);
    std::vector<NodeId> find_nodes_by_property(
        const std::string& key,
        const std::string& value
    ) const;
    
    // Edge property indexing
    void index_edge_property(EdgeId edge_id, const std::string& key, const std::string& value);
    void remove_edge_property_index(EdgeId edge_id, const std::string& key);
    std::vector<EdgeId> find_edges_by_property(
        const std::string& key,
        const std::string& value
    ) const;
    
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
    void compact_indexes();
    void clear_all_indexes();

private:
    struct PropertyKey {
        std::string key;
        std::string value;
        
        bool operator==(const PropertyKey& other) const {
            return key == other.key && value == other.value;
        }
        
        size_t hash() const {
            return std::hash<std::string>{}(key) ^ (std::hash<std::string>{}(value) << 1);
        }
    };
    
    struct PropertyKeyHash {
        size_t operator()(const PropertyKey& pk) const {
            return pk.hash();
        }
    };
    
    // Node property indexes
    std::unique_ptr<LockFreeHashTable<PropertyKey, std::vector<NodeId>>> node_property_index_;
    mutable std::shared_mutex node_property_mutex_;
    
    // Edge property indexes
    std::unique_ptr<LockFreeHashTable<PropertyKey, std::vector<EdgeId>>> edge_property_index_;
    mutable std::shared_mutex edge_property_mutex_;
    
    // Adjacency lists
    std::unique_ptr<LockFreeHashTable<NodeId, std::unique_ptr<LockFreeAdjacencyList>>>
        outgoing_adjacency_;
    std::unique_ptr<LockFreeHashTable<NodeId, std::unique_ptr<LockFreeAdjacencyList>>>
        incoming_adjacency_;
    mutable std::shared_mutex adjacency_mutex_;
    
    // Helper methods
    LockFreeAdjacencyList* get_or_create_outgoing_list(NodeId node_id);
    LockFreeAdjacencyList* get_or_create_incoming_list(NodeId node_id);
};

}  // namespace graphdb::storage