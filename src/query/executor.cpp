#include "executor.h"
#include <queue>
#include <unordered_set>
#include <algorithm>
#include <sstream>
#include <fmt/core.h>
#include <fmt/format.h>
#include <fmt/ranges.h>

namespace loredb::query {

QueryExecutor::QueryExecutor(std::shared_ptr<storage::GraphStore> graph_store,
                             std::shared_ptr<storage::SimpleIndexManager> index_manager)
    : graph_store_(std::move(graph_store)), index_manager_(std::move(index_manager)) {
    // Begin a transaction if the graph store has MVCC support
    if (graph_store_->has_mvcc()) {
        auto txn = txn_manager_.begin_transaction();
        tx_id_ = txn->id;
    } else {
        tx_id_ = 0; // legacy path
    }
}

QueryExecutor::~QueryExecutor() = default;

util::expected<QueryResult, storage::Error> QueryExecutor::get_node_by_id(storage::NodeId node_id) {
    util::expected<std::pair<storage::NodeRecord, std::vector<storage::Property>>, storage::Error> result;
    if (graph_store_->has_mvcc() && tx_id_ != 0) {
        result = graph_store_->get_node(tx_id_, node_id);
    } else {
        result = graph_store_->get_node(node_id);
    }
    if (!result.has_value()) {
        return util::unexpected<storage::Error>(result.error());
    }
    
    auto [node, properties] = result.value();
    return node_to_result(node, properties);
}

util::expected<QueryResult, storage::Error> QueryExecutor::get_nodes_by_property(const std::string& key, const std::string& value) {
    auto node_ids = index_manager_->find_nodes_by_property(key, value);
    
    QueryResult query_result({"id", "properties"});
    
    for (auto node_id : node_ids) {
        auto result = graph_store_->get_node(node_id);
        if (result.has_value()) {
            auto [node, properties] = result.value();
            
            std::stringstream props_stream;
            for (size_t i = 0; i < properties.size(); ++i) {
                if (i > 0) props_stream << ", ";
                props_stream << properties[i].key << ":" << property_value_to_string(properties[i].value);
            }
            
            query_result.add_row({std::to_string(node.id), props_stream.str()});
        }
    }
    
    return query_result;
}

util::expected<QueryResult, storage::Error> QueryExecutor::get_all_nodes(size_t limit) {
    QueryResult query_result({"id", "properties"});
    
    // For now, iterate through node IDs (in production, use better iteration)
    for (storage::NodeId node_id = 1; node_id <= limit && query_result.size() < limit; ++node_id) {
        auto result = graph_store_->get_node(node_id);
        if (result.has_value()) {
            auto [node, properties] = result.value();
            
            std::stringstream props_stream;
            for (size_t i = 0; i < properties.size(); ++i) {
                if (i > 0) props_stream << ", ";
                props_stream << properties[i].key << ":" << property_value_to_string(properties[i].value);
            }
            
            query_result.add_row({std::to_string(node.id), props_stream.str()});
        }
    }
    
    return query_result;
}

util::expected<QueryResult, storage::Error> QueryExecutor::get_edge_by_id(storage::EdgeId edge_id) {
    util::expected<std::pair<storage::EdgeRecord, std::vector<storage::Property>>, storage::Error> result;
    if (graph_store_->has_mvcc() && tx_id_ != 0) {
        result = graph_store_->get_edge(tx_id_, edge_id);
    } else {
        result = graph_store_->get_edge(edge_id);
    }
    if (!result.has_value()) {
        return util::unexpected<storage::Error>(result.error());
    }
    
    auto [edge, properties] = result.value();
    return edge_to_result(edge, properties);
}

util::expected<QueryResult, storage::Error> QueryExecutor::get_edges_by_property(const std::string& key, const std::string& value) {
    auto edge_ids = index_manager_->find_edges_by_property(key, value);
    
    QueryResult query_result({"id", "from_node", "to_node", "properties"});
    
    for (auto edge_id : edge_ids) {
        auto result = graph_store_->get_edge(edge_id);
        if (result.has_value()) {
            auto [edge, properties] = result.value();
            
            std::stringstream props_stream;
            for (size_t i = 0; i < properties.size(); ++i) {
                if (i > 0) props_stream << ", ";
                props_stream << properties[i].key << ":" << property_value_to_string(properties[i].value);
            }
            
            query_result.add_row({
                std::to_string(edge.id),
                std::to_string(edge.from_node),
                std::to_string(edge.to_node),
                props_stream.str()
            });
        }
    }
    
    return query_result;
}

util::expected<QueryResult, storage::Error> QueryExecutor::get_all_edges(size_t limit) {
    QueryResult query_result({"id", "from_node", "to_node", "properties"});
    
    // For now, iterate through edge IDs (in production, use better iteration)
    for (storage::EdgeId edge_id = 1; edge_id <= limit && query_result.size() < limit; ++edge_id) {
        auto result = graph_store_->get_edge(edge_id);
        if (result.has_value()) {
            auto [edge, properties] = result.value();
            
            std::stringstream props_stream;
            for (size_t i = 0; i < properties.size(); ++i) {
                if (i > 0) props_stream << ", ";
                props_stream << properties[i].key << ":" << property_value_to_string(properties[i].value);
            }
            
            query_result.add_row({
                std::to_string(edge.id),
                std::to_string(edge.from_node),
                std::to_string(edge.to_node),
                props_stream.str()
            });
        }
    }
    
    return query_result;
}

util::expected<QueryResult, storage::Error> QueryExecutor::get_adjacent_nodes(storage::NodeId node_id) {
    auto result = graph_store_->get_adjacent_nodes(node_id);
    if (!result.has_value()) {
        return util::unexpected<storage::Error>(result.error());
    }
    
    QueryResult query_result({"node_id"});
    
    for (auto adjacent_node_id : result.value()) {
        query_result.add_row({std::to_string(adjacent_node_id)});
    }
    
    return query_result;
}

util::expected<QueryResult, storage::Error> QueryExecutor::get_outgoing_edges(storage::NodeId node_id) {
    auto result = graph_store_->get_outgoing_edges(node_id);
    if (!result.has_value()) {
        return util::unexpected<storage::Error>(result.error());
    }
    
    QueryResult query_result({"edge_id"});
    
    for (auto edge_id : result.value()) {
        query_result.add_row({std::to_string(edge_id)});
    }
    
    return query_result;
}

util::expected<QueryResult, storage::Error> QueryExecutor::get_incoming_edges(storage::NodeId node_id) {
    auto result = graph_store_->get_incoming_edges(node_id);
    if (!result.has_value()) {
        return util::unexpected<storage::Error>(result.error());
    }
    
    QueryResult query_result({"edge_id"});
    
    for (auto edge_id : result.value()) {
        query_result.add_row({std::to_string(edge_id)});
    }
    
    return query_result;
}

util::expected<QueryResult, storage::Error> QueryExecutor::find_shortest_path(storage::NodeId from_node, storage::NodeId to_node) {
    auto path = bfs_shortest_path(from_node, to_node);
    
    QueryResult query_result({"path_length", "path"});
    
    if (path.empty()) {
        query_result.add_row({"0", "No path found"});
    } else {
        std::stringstream path_stream;
        for (size_t i = 0; i < path.size(); ++i) {
            if (i > 0) path_stream << " -> ";
            path_stream << path[i];
        }
        
        query_result.add_row({std::to_string(path.size() - 1), path_stream.str()});
    }
    
    return query_result;
}

util::expected<QueryResult, storage::Error> QueryExecutor::find_paths_with_length(storage::NodeId from_node, storage::NodeId to_node, size_t max_length) {
    auto paths = bfs_paths_with_length(from_node, to_node, max_length);
    
    QueryResult query_result({"path_length", "path"});
    
    for (const auto& path : paths) {
        std::stringstream path_stream;
        for (size_t i = 0; i < path.size(); ++i) {
            if (i > 0) path_stream << " -> ";
            path_stream << path[i];
        }
        
        query_result.add_row({std::to_string(path.size() - 1), path_stream.str()});
    }
    
    return query_result;
}

util::expected<QueryResult, storage::Error> QueryExecutor::count_nodes() {
    QueryResult query_result({"count"});
    query_result.add_row({std::to_string(graph_store_->get_node_count())});
    return query_result;
}

util::expected<QueryResult, storage::Error> QueryExecutor::count_edges() {
    QueryResult query_result({"count"});
    query_result.add_row({std::to_string(graph_store_->get_edge_count())});
    return query_result;
}

util::expected<QueryResult, storage::Error> QueryExecutor::get_node_degree_stats() {
    QueryResult query_result({"metric", "value"});
    
    // For now, just return basic stats
    query_result.add_row({"total_nodes", std::to_string(graph_store_->get_node_count())});
    query_result.add_row({"total_edges", std::to_string(graph_store_->get_edge_count())});
    
    return query_result;
}

util::expected<QueryResult, storage::Error> QueryExecutor::batch_get_nodes(const std::vector<storage::NodeId>& node_ids) {
    QueryResult query_result({"id", "properties"});
    
    for (auto node_id : node_ids) {
        auto result = graph_store_->get_node(node_id);
        if (result.has_value()) {
            auto [node, properties] = result.value();
            
            std::stringstream props_stream;
            for (size_t i = 0; i < properties.size(); ++i) {
                if (i > 0) props_stream << ", ";
                props_stream << properties[i].key << ":" << property_value_to_string(properties[i].value);
            }
            
            query_result.add_row({std::to_string(node.id), props_stream.str()});
        }
    }
    
    return query_result;
}

util::expected<QueryResult, storage::Error> QueryExecutor::batch_get_edges(const std::vector<storage::EdgeId>& edge_ids) {
    QueryResult query_result({"id", "from_node", "to_node", "properties"});
    
    for (auto edge_id : edge_ids) {
        auto result = graph_store_->get_edge(edge_id);
        if (result.has_value()) {
            auto [edge, properties] = result.value();
            
            std::stringstream props_stream;
            for (size_t i = 0; i < properties.size(); ++i) {
                if (i > 0) props_stream << ", ";
                props_stream << properties[i].key << ":" << property_value_to_string(properties[i].value);
            }
            
            query_result.add_row({
                std::to_string(edge.id),
                std::to_string(edge.from_node),
                std::to_string(edge.to_node),
                props_stream.str()
            });
        }
    }
    
    return query_result;
}

// Document-specific queries
util::expected<QueryResult, storage::Error> QueryExecutor::get_document_backlinks(storage::NodeId document_id) {
    return get_incoming_edges(document_id);
}

util::expected<QueryResult, storage::Error> QueryExecutor::get_document_outlinks(storage::NodeId document_id) {
    return get_outgoing_edges(document_id);
}

util::expected<QueryResult, storage::Error> QueryExecutor::find_related_documents(storage::NodeId document_id, size_t max_results) {
    auto adjacent_result = graph_store_->get_adjacent_nodes(document_id);
    if (!adjacent_result.has_value()) {
        return util::unexpected<storage::Error>(adjacent_result.error());
    }
    
    QueryResult query_result({"document_id", "relation_type"});
    
    auto adjacent_nodes = adjacent_result.value();
    for (size_t i = 0; i < adjacent_nodes.size() && i < max_results; ++i) {
        query_result.add_row({std::to_string(adjacent_nodes[i]), "adjacent"});
    }
    
    return query_result;
}

util::expected<QueryResult, storage::Error> QueryExecutor::suggest_links_for_document(storage::NodeId document_id, const std::string& /*content_snippet*/) {
    // Simple link suggestion based on adjacent nodes
    auto related_result = find_related_documents(document_id, 5);
    if (!related_result.has_value()) {
        return util::unexpected<storage::Error>(related_result.error());
    }
    
    QueryResult query_result({"suggested_document_id", "reason"});
    
    // For now, just return related documents as suggestions
    for (const auto& row : related_result.value().rows) {
        query_result.add_row({row[0], "related_document"});
    }
    
    return query_result;
}

std::string QueryExecutor::property_value_to_string(const storage::PropertyValue& value) {
    return std::visit([](const auto& v) -> std::string {
        using T = std::decay_t<decltype(v)>;
        
        if constexpr (std::is_same_v<T, std::string>) {
            return v;
        } else if constexpr (std::is_same_v<T, int64_t>) {
            return std::to_string(v);
        } else if constexpr (std::is_same_v<T, double>) {
            return std::to_string(v);
        } else if constexpr (std::is_same_v<T, bool>) {
            return v ? "true" : "false";
        } else if constexpr (std::is_same_v<T, std::vector<uint8_t>>) {
            return "[binary data]";
        } else {
            return "[unknown]";
        }
    }, value);
}

QueryResult QueryExecutor::node_to_result(const storage::NodeRecord& node, const std::vector<storage::Property>& properties) {
    QueryResult result({"id", "properties"});
    
    std::vector<std::string> prop_pairs;
    for (const auto& prop : properties) {
        prop_pairs.push_back(fmt::format("{}:{}", prop.key, property_value_to_string(prop.value)));
    }
    std::string props_str = fmt::format("{}", fmt::join(prop_pairs, ", "));
    
    result.add_row({std::to_string(node.id), props_str});
    return result;
}

QueryResult QueryExecutor::edge_to_result(const storage::EdgeRecord& edge, const std::vector<storage::Property>& properties) {
    QueryResult result({"id", "from_node", "to_node", "properties"});
    
    std::stringstream props_stream;
    for (size_t i = 0; i < properties.size(); ++i) {
        if (i > 0) props_stream << ", ";
        props_stream << properties[i].key << ":" << property_value_to_string(properties[i].value);
    }
    
    result.add_row({
        std::to_string(edge.id),
        std::to_string(edge.from_node),
        std::to_string(edge.to_node),
        props_stream.str()
    });
    
    return result;
}

std::vector<storage::NodeId> QueryExecutor::bfs_shortest_path(storage::NodeId from_node, storage::NodeId to_node) {
    if (from_node == to_node) {
        return {from_node};
    }
    
    std::queue<storage::NodeId> queue;
    std::unordered_set<storage::NodeId> visited;
    std::unordered_map<storage::NodeId, storage::NodeId> parent;
    
    queue.push(from_node);
    visited.insert(from_node);
    
    while (!queue.empty()) {
        storage::NodeId current = queue.front();
        queue.pop();
        
        auto adjacent_result = graph_store_->get_adjacent_nodes(current);
        if (!adjacent_result.has_value()) {
            continue;
        }
        
        for (storage::NodeId neighbor : adjacent_result.value()) {
            if (visited.find(neighbor) == visited.end()) {
                visited.insert(neighbor);
                parent[neighbor] = current;
                queue.push(neighbor);
                
                if (neighbor == to_node) {
                    // Reconstruct path
                    std::vector<storage::NodeId> path;
                    storage::NodeId node = to_node;
                    while (node != from_node) {
                        path.push_back(node);
                        node = parent[node];
                    }
                    path.push_back(from_node);
                    std::reverse(path.begin(), path.end());
                    return path;
                }
            }
        }
    }
    
    return {}; // No path found
}

std::vector<std::vector<storage::NodeId>> QueryExecutor::bfs_paths_with_length(storage::NodeId from_node, storage::NodeId to_node, size_t max_length) {
    std::vector<std::vector<storage::NodeId>> paths;
    
    // Simple DFS with path length limit
    std::function<void(storage::NodeId, std::vector<storage::NodeId>&, std::unordered_set<storage::NodeId>&)> dfs = 
        [&](storage::NodeId current, std::vector<storage::NodeId>& path, std::unordered_set<storage::NodeId>& visited) {
            if (path.size() > max_length + 1) {
                return;
            }
            
            if (current == to_node && path.size() > 1) {
                paths.push_back(path);
                return;
            }
            
            auto adjacent_result = graph_store_->get_adjacent_nodes(current);
            if (!adjacent_result.has_value()) {
                return;
            }
            
            for (storage::NodeId neighbor : adjacent_result.value()) {
                if (visited.find(neighbor) == visited.end()) {
                    visited.insert(neighbor);
                    path.push_back(neighbor);
                    dfs(neighbor, path, visited);
                    path.pop_back();
                    visited.erase(neighbor);
                }
            }
        };
    
    std::vector<storage::NodeId> path = {from_node};
    std::unordered_set<storage::NodeId> visited = {from_node};
    dfs(from_node, path, visited);
    
    return paths;
}

// Streaming implementations removed for C++20 compatibility
// Will be re-implemented when proper coroutine support is available

}  // namespace loredb::query