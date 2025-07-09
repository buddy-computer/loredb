#include "simple_index_manager.h"
#include <algorithm>

namespace loredb::storage {

void SimpleIndexManager::index_node_property(NodeId node_id, const std::string& key, const std::string& value) {
    PropertyKey prop_key{key, value};
    tbb::concurrent_hash_map<PropertyKey, tbb::concurrent_vector<NodeId>, PropertyKeyHash>::accessor accessor;
    node_property_index_.insert(accessor, prop_key);
    accessor->second.push_back(node_id);
}

void SimpleIndexManager::remove_node_property_index(NodeId node_id, const std::string& key, const std::string& value) {
    PropertyKey prop_key{key, value};
    tbb::concurrent_hash_map<PropertyKey, tbb::concurrent_vector<NodeId>, PropertyKeyHash>::accessor accessor;
    if (node_property_index_.find(accessor, prop_key)) {
        auto& vec = accessor->second;
        tbb::concurrent_vector<NodeId> new_vec;
        for (const auto& id : vec) {
            if (id != node_id) {
                new_vec.push_back(id);
            }
        }
        vec = new_vec;
    }
}

std::vector<NodeId> SimpleIndexManager::find_nodes_by_property(const std::string& key, const std::string& value) const {
    PropertyKey prop_key{key, value};
    tbb::concurrent_hash_map<PropertyKey, tbb::concurrent_vector<NodeId>, PropertyKeyHash>::const_accessor accessor;
    if (node_property_index_.find(accessor, prop_key)) {
        return std::vector<NodeId>(accessor->second.begin(), accessor->second.end());
    }
    return {};
}

void SimpleIndexManager::index_edge_property(EdgeId edge_id, const std::string& key, const std::string& value) {
    PropertyKey prop_key{key, value};
    tbb::concurrent_hash_map<PropertyKey, tbb::concurrent_vector<EdgeId>, PropertyKeyHash>::accessor accessor;
    edge_property_index_.insert(accessor, prop_key);
    accessor->second.push_back(edge_id);
}

void SimpleIndexManager::remove_edge_property_index(EdgeId edge_id, const std::string& key, const std::string& value) {
    PropertyKey prop_key{key, value};
    tbb::concurrent_hash_map<PropertyKey, tbb::concurrent_vector<EdgeId>, PropertyKeyHash>::accessor accessor;
    if (edge_property_index_.find(accessor, prop_key)) {
        auto& vec = accessor->second;
        tbb::concurrent_vector<EdgeId> new_vec;
        for (const auto& id : vec) {
            if (id != edge_id) {
                new_vec.push_back(id);
            }
        }
        vec = new_vec;
    }
}

std::vector<EdgeId> SimpleIndexManager::find_edges_by_property(const std::string& key, const std::string& value) const {
    PropertyKey prop_key{key, value};
    tbb::concurrent_hash_map<PropertyKey, tbb::concurrent_vector<EdgeId>, PropertyKeyHash>::const_accessor accessor;
    if (edge_property_index_.find(accessor, prop_key)) {
        return std::vector<EdgeId>(accessor->second.begin(), accessor->second.end());
    }
    return {};
}

void SimpleIndexManager::add_edge_to_adjacency(NodeId from_node, NodeId to_node, EdgeId edge_id) {
    std::unique_lock<std::shared_mutex> lock(adjacency_mutex_);
    outgoing_adjacency_[from_node].push_back(edge_id);
    incoming_adjacency_[to_node].push_back(edge_id);
    edge_endpoints_[edge_id] = {from_node, to_node};
}

void SimpleIndexManager::remove_edge_from_adjacency(NodeId from_node, NodeId to_node, EdgeId edge_id) {
    std::unique_lock<std::shared_mutex> lock(adjacency_mutex_);
    
    auto& outgoing = outgoing_adjacency_[from_node];
    outgoing.erase(std::remove(outgoing.begin(), outgoing.end(), edge_id), outgoing.end());
    
    auto& incoming = incoming_adjacency_[to_node];
    incoming.erase(std::remove(incoming.begin(), incoming.end(), edge_id), incoming.end());

    edge_endpoints_.erase(edge_id);
}

std::vector<EdgeId> SimpleIndexManager::get_outgoing_edges(NodeId node_id) const {
    std::shared_lock<std::shared_mutex> lock(adjacency_mutex_);
    auto it = outgoing_adjacency_.find(node_id);
    if (it != outgoing_adjacency_.end()) {
        return it->second;
    }
    return {};
}

std::vector<EdgeId> SimpleIndexManager::get_incoming_edges(NodeId node_id) const {
    std::shared_lock<std::shared_mutex> lock(adjacency_mutex_);
    auto it = incoming_adjacency_.find(node_id);
    if (it != incoming_adjacency_.end()) {
        return it->second;
    }
    return {};
}

std::vector<NodeId> SimpleIndexManager::get_adjacent_nodes(NodeId node_id) const {
    std::vector<NodeId> adjacent;
    std::shared_lock<std::shared_mutex> lock(adjacency_mutex_);

    auto collect = [&](const std::vector<EdgeId>& edges) {
        for (EdgeId eid : edges) {
            auto ep_it = edge_endpoints_.find(eid);
            if (ep_it != edge_endpoints_.end()) {
                const auto& [from, to] = ep_it->second;
                NodeId other = (from == node_id) ? to : from;
                adjacent.push_back(other);
            }
        }
    };

    if (auto out_it = outgoing_adjacency_.find(node_id); out_it != outgoing_adjacency_.end()) {
        collect(out_it->second);
    }
    if (auto in_it = incoming_adjacency_.find(node_id); in_it != incoming_adjacency_.end()) {
        collect(in_it->second);
    }

    // Deduplicate adjacent list
    std::sort(adjacent.begin(), adjacent.end());
    adjacent.erase(std::unique(adjacent.begin(), adjacent.end()), adjacent.end());
    return adjacent;
}

size_t SimpleIndexManager::get_node_property_index_size() const {
    return node_property_index_.size();
}

size_t SimpleIndexManager::get_edge_property_index_size() const {
    return edge_property_index_.size();
}

size_t SimpleIndexManager::get_adjacency_list_count() const {
    std::shared_lock<std::shared_mutex> lock(adjacency_mutex_);
    return outgoing_adjacency_.size() + incoming_adjacency_.size();
}

void SimpleIndexManager::clear_all_indexes() {
    node_property_index_.clear();
    edge_property_index_.clear();

    std::unique_lock<std::shared_mutex> adj_lock(adjacency_mutex_);
    outgoing_adjacency_.clear();
    incoming_adjacency_.clear();
}

}  // namespace loredb::storage