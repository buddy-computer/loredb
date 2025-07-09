#include "index_manager.h"
#include <algorithm>
#include <functional>

namespace graphdb::storage {

// LockFreeHashTable implementation
template<typename K, typename V>
LockFreeHashTable<K, V>::LockFreeHashTable(size_t initial_capacity) 
    : capacity_(initial_capacity), size_(0), next_entry_index_(0) {
    buckets_ = std::make_unique<std::atomic<Entry*>[]>(capacity_);
    for (size_t i = 0; i < capacity_; ++i) {
        buckets_[i].store(nullptr);
    }
    
    // Pre-allocate entry pool
    entry_pools_.emplace_back(std::make_unique<Entry[]>(1024));
}

template<typename K, typename V>
LockFreeHashTable<K, V>::~LockFreeHashTable() {
    clear();
}

template<typename K, typename V>
bool LockFreeHashTable<K, V>::insert(const K& key, const V& value) {
    size_t bucket_index = hash(key) % capacity_;
    
    Entry* new_entry = allocate_entry();
    if (!new_entry) {
        return false;
    }
    
    new_entry->key.store(key);
    new_entry->value.store(value);
    
    // Insert at head of bucket chain
    Entry* expected = buckets_[bucket_index].load();
    do {
        new_entry->next.store(expected);
    } while (!buckets_[bucket_index].compare_exchange_weak(expected, new_entry));
    
    size_.fetch_add(1);
    return true;
}

template<typename K, typename V>
bool LockFreeHashTable<K, V>::find(const K& key, V& value) const {
    size_t bucket_index = hash(key) % capacity_;
    Entry* current = buckets_[bucket_index].load();
    
    while (current != nullptr) {
        if (current->key.load() == key) {
            value = current->value.load();
            return true;
        }
        current = current->next.load();
    }
    
    return false;
}

template<typename K, typename V>
bool LockFreeHashTable<K, V>::remove(const K& key) {
    size_t bucket_index = hash(key) % capacity_;
    
    Entry* prev = nullptr;
    Entry* current = buckets_[bucket_index].load();
    
    while (current != nullptr) {
        if (current->key.load() == key) {
            Entry* next = current->next.load();
            
            if (prev == nullptr) {
                // Removing head
                if (buckets_[bucket_index].compare_exchange_strong(current, next)) {
                    deallocate_entry(current);
                    size_.fetch_sub(1);
                    return true;
                }
            } else {
                // Removing from middle/end
                if (prev->next.compare_exchange_strong(current, next)) {
                    deallocate_entry(current);
                    size_.fetch_sub(1);
                    return true;
                }
            }
            
            return false; // Concurrent modification
        }
        
        prev = current;
        current = current->next.load();
    }
    
    return false;
}

template<typename K, typename V>
void LockFreeHashTable<K, V>::clear() {
    for (size_t i = 0; i < capacity_; ++i) {
        Entry* current = buckets_[i].load();
        while (current != nullptr) {
            Entry* next = current->next.load();
            deallocate_entry(current);
            current = next;
        }
        buckets_[i].store(nullptr);
    }
    size_.store(0);
}

template<typename K, typename V>
size_t LockFreeHashTable<K, V>::size() const {
    return size_.load();
}

template<typename K, typename V>
size_t LockFreeHashTable<K, V>::hash(const K& key) const {
    return std::hash<K>{}(key);
}

template<typename K, typename V>
typename LockFreeHashTable<K, V>::Entry* LockFreeHashTable<K, V>::allocate_entry() {
    size_t index = next_entry_index_.fetch_add(1);
    size_t pool_index = index / 1024;
    size_t entry_index = index % 1024;
    
    std::lock_guard<std::mutex> lock(pool_mutex_);
    
    // Allocate new pool if needed
    while (pool_index >= entry_pools_.size()) {
        entry_pools_.emplace_back(std::make_unique<Entry[]>(1024));
    }
    
    return &entry_pools_[pool_index][entry_index];
}

template<typename K, typename V>
void LockFreeHashTable<K, V>::deallocate_entry(Entry* entry) {
    // For simplicity, we don't actually deallocate - just mark as unused
    // In a production system, you'd want proper memory management
    entry->key.store(K{});
    entry->value.store(V{});
    entry->next.store(nullptr);
}

// LockFreeAdjacencyList implementation
LockFreeAdjacencyList::LockFreeAdjacencyList() : head_(nullptr), size_(0), next_entry_index_(0) {
    // Pre-allocate entry pool
    entry_pools_.emplace_back(std::make_unique<EdgeEntry[]>(1024));
}

LockFreeAdjacencyList::~LockFreeAdjacencyList() {
    clear();
}

void LockFreeAdjacencyList::add_edge(EdgeId edge_id, NodeId target_node) {
    EdgeEntry* new_entry = allocate_edge_entry();
    if (!new_entry) {
        return;
    }
    
    new_entry->edge_id.store(edge_id);
    new_entry->target_node.store(target_node);
    
    // Insert at head
    EdgeEntry* expected = head_.load();
    do {
        new_entry->next.store(expected);
    } while (!head_.compare_exchange_weak(expected, new_entry));
    
    size_.fetch_add(1);
}

void LockFreeAdjacencyList::remove_edge(EdgeId edge_id) {
    EdgeEntry* prev = nullptr;
    EdgeEntry* current = head_.load();
    
    while (current != nullptr) {
        if (current->edge_id.load() == edge_id) {
            EdgeEntry* next = current->next.load();
            
            if (prev == nullptr) {
                // Removing head
                if (head_.compare_exchange_strong(current, next)) {
                    deallocate_edge_entry(current);
                    size_.fetch_sub(1);
                    return;
                }
            } else {
                // Removing from middle/end
                if (prev->next.compare_exchange_strong(current, next)) {
                    deallocate_edge_entry(current);
                    size_.fetch_sub(1);
                    return;
                }
            }
            
            return; // Concurrent modification
        }
        
        prev = current;
        current = current->next.load();
    }
}

std::vector<EdgeId> LockFreeAdjacencyList::get_edges() const {
    std::vector<EdgeId> edges;
    EdgeEntry* current = head_.load();
    
    while (current != nullptr) {
        edges.push_back(current->edge_id.load());
        current = current->next.load();
    }
    
    return edges;
}

std::vector<NodeId> LockFreeAdjacencyList::get_adjacent_nodes() const {
    std::vector<NodeId> nodes;
    EdgeEntry* current = head_.load();
    
    while (current != nullptr) {
        nodes.push_back(current->target_node.load());
        current = current->next.load();
    }
    
    return nodes;
}

size_t LockFreeAdjacencyList::size() const {
    return size_.load();
}

void LockFreeAdjacencyList::clear() {
    EdgeEntry* current = head_.load();
    while (current != nullptr) {
        EdgeEntry* next = current->next.load();
        deallocate_edge_entry(current);
        current = next;
    }
    head_.store(nullptr);
    size_.store(0);
}

LockFreeAdjacencyList::EdgeEntry* LockFreeAdjacencyList::allocate_edge_entry() {
    size_t index = next_entry_index_.fetch_add(1);
    size_t pool_index = index / 1024;
    size_t entry_index = index % 1024;
    
    std::lock_guard<std::mutex> lock(pool_mutex_);
    
    // Allocate new pool if needed
    while (pool_index >= entry_pools_.size()) {
        entry_pools_.emplace_back(std::make_unique<EdgeEntry[]>(1024));
    }
    
    return &entry_pools_[pool_index][entry_index];
}

void LockFreeAdjacencyList::deallocate_edge_entry(EdgeEntry* entry) {
    // For simplicity, we don't actually deallocate - just mark as unused
    entry->edge_id.store(0);
    entry->target_node.store(0);
    entry->next.store(nullptr);
}

// IndexManager implementation
IndexManager::IndexManager(size_t initial_capacity) {
    node_property_index_ = std::make_unique<LockFreeHashTable<PropertyKey, std::vector<NodeId>>>(initial_capacity);
    edge_property_index_ = std::make_unique<LockFreeHashTable<PropertyKey, std::vector<EdgeId>>>(initial_capacity);
    outgoing_adjacency_ = std::make_unique<LockFreeHashTable<NodeId, std::unique_ptr<LockFreeAdjacencyList>>>(initial_capacity);
    incoming_adjacency_ = std::make_unique<LockFreeHashTable<NodeId, std::unique_ptr<LockFreeAdjacencyList>>>(initial_capacity);
}

IndexManager::~IndexManager() = default;

void IndexManager::index_node_property(NodeId node_id, const std::string& key, const std::string& value) {
    PropertyKey prop_key{key, value};
    
    std::shared_lock<std::shared_mutex> lock(node_property_mutex_);
    
    std::vector<NodeId> nodes;
    if (node_property_index_->find(prop_key, nodes)) {
        // Add to existing list
        if (std::find(nodes.begin(), nodes.end(), node_id) == nodes.end()) {
            nodes.push_back(node_id);
            node_property_index_->remove(prop_key);
            node_property_index_->insert(prop_key, nodes);
        }
    } else {
        // Create new list
        nodes.push_back(node_id);
        node_property_index_->insert(prop_key, nodes);
    }
}

void IndexManager::remove_node_property_index(NodeId node_id, const std::string& key) {
    // For simplicity, we don't implement full removal here
    // In a production system, you'd iterate through all values for the key
}

std::vector<NodeId> IndexManager::find_nodes_by_property(const std::string& key, const std::string& value) const {
    PropertyKey prop_key{key, value};
    
    std::shared_lock<std::shared_mutex> lock(node_property_mutex_);
    
    std::vector<NodeId> nodes;
    if (node_property_index_->find(prop_key, nodes)) {
        return nodes;
    }
    
    return {};
}

void IndexManager::index_edge_property(EdgeId edge_id, const std::string& key, const std::string& value) {
    PropertyKey prop_key{key, value};
    
    std::shared_lock<std::shared_mutex> lock(edge_property_mutex_);
    
    std::vector<EdgeId> edges;
    if (edge_property_index_->find(prop_key, edges)) {
        // Add to existing list
        if (std::find(edges.begin(), edges.end(), edge_id) == edges.end()) {
            edges.push_back(edge_id);
            edge_property_index_->remove(prop_key);
            edge_property_index_->insert(prop_key, edges);
        }
    } else {
        // Create new list
        edges.push_back(edge_id);
        edge_property_index_->insert(prop_key, edges);
    }
}

void IndexManager::remove_edge_property_index(EdgeId edge_id, const std::string& key) {
    // For simplicity, we don't implement full removal here
    // In a production system, you'd iterate through all values for the key
}

std::vector<EdgeId> IndexManager::find_edges_by_property(const std::string& key, const std::string& value) const {
    PropertyKey prop_key{key, value};
    
    std::shared_lock<std::shared_mutex> lock(edge_property_mutex_);
    
    std::vector<EdgeId> edges;
    if (edge_property_index_->find(prop_key, edges)) {
        return edges;
    }
    
    return {};
}

void IndexManager::add_edge_to_adjacency(NodeId from_node, NodeId to_node, EdgeId edge_id) {
    std::shared_lock<std::shared_mutex> lock(adjacency_mutex_);
    
    // Add to outgoing adjacency
    LockFreeAdjacencyList* outgoing = get_or_create_outgoing_list(from_node);
    if (outgoing) {
        outgoing->add_edge(edge_id, to_node);
    }
    
    // Add to incoming adjacency
    LockFreeAdjacencyList* incoming = get_or_create_incoming_list(to_node);
    if (incoming) {
        incoming->add_edge(edge_id, from_node);
    }
}

void IndexManager::remove_edge_from_adjacency(NodeId from_node, NodeId to_node, EdgeId edge_id) {
    std::shared_lock<std::shared_mutex> lock(adjacency_mutex_);
    
    // Remove from outgoing adjacency
    std::unique_ptr<LockFreeAdjacencyList> outgoing;
    if (outgoing_adjacency_->find(from_node, outgoing)) {
        outgoing->remove_edge(edge_id);
    }
    
    // Remove from incoming adjacency
    std::unique_ptr<LockFreeAdjacencyList> incoming;
    if (incoming_adjacency_->find(to_node, incoming)) {
        incoming->remove_edge(edge_id);
    }
}

std::vector<EdgeId> IndexManager::get_outgoing_edges(NodeId node_id) const {
    std::shared_lock<std::shared_mutex> lock(adjacency_mutex_);
    
    std::unique_ptr<LockFreeAdjacencyList> list;
    if (outgoing_adjacency_->find(node_id, list)) {
        return list->get_edges();
    }
    
    return {};
}

std::vector<EdgeId> IndexManager::get_incoming_edges(NodeId node_id) const {
    std::shared_lock<std::shared_mutex> lock(adjacency_mutex_);
    
    std::unique_ptr<LockFreeAdjacencyList> list;
    if (incoming_adjacency_->find(node_id, list)) {
        return list->get_edges();
    }
    
    return {};
}

std::vector<NodeId> IndexManager::get_adjacent_nodes(NodeId node_id) const {
    std::vector<NodeId> adjacent;
    
    // Get outgoing nodes
    auto outgoing_edges = get_outgoing_edges(node_id);
    std::shared_lock<std::shared_mutex> lock(adjacency_mutex_);
    
    std::unique_ptr<LockFreeAdjacencyList> outgoing_list;
    if (outgoing_adjacency_->find(node_id, outgoing_list)) {
        auto outgoing_nodes = outgoing_list->get_adjacent_nodes();
        adjacent.insert(adjacent.end(), outgoing_nodes.begin(), outgoing_nodes.end());
    }
    
    // Get incoming nodes
    std::unique_ptr<LockFreeAdjacencyList> incoming_list;
    if (incoming_adjacency_->find(node_id, incoming_list)) {
        auto incoming_nodes = incoming_list->get_adjacent_nodes();
        adjacent.insert(adjacent.end(), incoming_nodes.begin(), incoming_nodes.end());
    }
    
    // Remove duplicates
    std::sort(adjacent.begin(), adjacent.end());
    adjacent.erase(std::unique(adjacent.begin(), adjacent.end()), adjacent.end());
    
    return adjacent;
}

size_t IndexManager::get_node_property_index_size() const {
    std::shared_lock<std::shared_mutex> lock(node_property_mutex_);
    return node_property_index_->size();
}

size_t IndexManager::get_edge_property_index_size() const {
    std::shared_lock<std::shared_mutex> lock(edge_property_mutex_);
    return edge_property_index_->size();
}

size_t IndexManager::get_adjacency_list_count() const {
    std::shared_lock<std::shared_mutex> lock(adjacency_mutex_);
    return outgoing_adjacency_->size() + incoming_adjacency_->size();
}

void IndexManager::compact_indexes() {
    // TODO: Implement index compaction
}

void IndexManager::clear_all_indexes() {
    std::unique_lock<std::shared_mutex> node_lock(node_property_mutex_);
    std::unique_lock<std::shared_mutex> edge_lock(edge_property_mutex_);
    std::unique_lock<std::shared_mutex> adj_lock(adjacency_mutex_);
    
    node_property_index_->clear();
    edge_property_index_->clear();
    outgoing_adjacency_->clear();
    incoming_adjacency_->clear();
}

LockFreeAdjacencyList* IndexManager::get_or_create_outgoing_list(NodeId node_id) {
    std::unique_ptr<LockFreeAdjacencyList> list;
    if (!outgoing_adjacency_->find(node_id, list)) {
        list = std::make_unique<LockFreeAdjacencyList>();
        outgoing_adjacency_->insert(node_id, std::move(list));
        outgoing_adjacency_->find(node_id, list);
    }
    return list.get();
}

LockFreeAdjacencyList* IndexManager::get_or_create_incoming_list(NodeId node_id) {
    std::unique_ptr<LockFreeAdjacencyList> list;
    if (!incoming_adjacency_->find(node_id, list)) {
        list = std::make_unique<LockFreeAdjacencyList>();
        incoming_adjacency_->insert(node_id, std::move(list));
        incoming_adjacency_->find(node_id, list);
    }
    return list.get();
}

// Explicit template instantiations
template class LockFreeHashTable<IndexManager::PropertyKey, std::vector<NodeId>>;
template class LockFreeHashTable<IndexManager::PropertyKey, std::vector<EdgeId>>;
template class LockFreeHashTable<NodeId, std::unique_ptr<LockFreeAdjacencyList>>;

}  // namespace graphdb::storage