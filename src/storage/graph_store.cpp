#include "graph_store.h"
#include "../transaction/mvcc_manager.h"
#include "wal_manager.h"
#include <sstream>
#include <chrono>
#include <algorithm>
#include <cstring>

namespace loredb::storage {

GraphStore::GraphStore(std::unique_ptr<PageStore> page_store)
    : page_store_(std::move(page_store)), next_node_id_(1), next_edge_id_(1),
      node_count_(0), edge_count_(0) {
}

GraphStore::GraphStore(std::unique_ptr<PageStore> page_store,
                       std::shared_ptr<transaction::MVCCManager> mvcc_manager,
                       std::shared_ptr<WALManager> wal_manager)
    : page_store_(std::move(page_store)),
      next_node_id_(1),
      next_edge_id_(1),
      node_count_(0),
      edge_count_(0),
      mvcc_manager_(std::move(mvcc_manager)),
      wal_manager_(std::move(wal_manager)) {
}

GraphStore::~GraphStore() = default;

util::expected<NodeId, Error> GraphStore::create_node(transaction::TransactionId tx_id,
                                                     const std::vector<Property>& properties) {
    // Create node via existing path
    auto id_result = create_node(properties);
    if (!id_result.has_value()) {
        return util::unexpected(id_result.error());
    }
    NodeId node_id = id_result.value();

    if (mvcc_manager_) {
        transaction::Version ver;
        ver.created_tx_id = tx_id;
        {
            NodeRecord nr;
            nr.id = node_id;
            nr.property_count = static_cast<uint32_t>(properties.size());
            ver.data = nr;
        }
        ver.properties = properties; // Store properties in version
        auto wres = mvcc_manager_->write_version(node_id, std::move(ver));
        if (!wres.has_value()) {
            return util::unexpected(Error{ErrorCode::CORRUPTION, "MVCC write failed"});
        }
        if (wal_manager_) {
            std::stringstream ss; ss << "{\"op\":\"create_node\",\"id\":" << node_id << "}";
            wal_manager_->log_operation({ss.str()});
        }
    }

    return node_id;
}

util::expected<void, Error> GraphStore::update_node(transaction::TransactionId tx_id,
                                                    NodeId node_id,
                                                    const std::vector<Property>& properties) {
    // First perform legacy update so underlying storage is consistent
    auto res = update_node(node_id, properties);
    if (!res.has_value()) return res;

    if (mvcc_manager_) {
        NodeRecord nr{};
        nr.id = node_id;
        nr.property_count = properties.size();

        transaction::Version ver;
        ver.created_tx_id = tx_id;
        ver.data = nr;
        ver.properties = properties;
        auto wr = mvcc_manager_->write_version(node_id, ver);
        if (!wr.has_value()) {
            return util::unexpected(Error{ErrorCode::CORRUPTION, "MVCC write failed"});
        }
    }
    if (wal_manager_) {
        std::stringstream ss; ss << "{\"op\":\"update_node\",\"id\":" << node_id << "}";
        wal_manager_->log_operation({ss.str()});
    }
    return {};
}

util::expected<void, Error> GraphStore::delete_node(transaction::TransactionId tx_id,
                                                    NodeId node_id) {
    // For MVCC, we only create a tombstone version without physically deleting
    // The legacy delete_node would remove the node from storage, making it invisible to all transactions
    
    // First check if the node exists
    auto node_result = get_node(node_id);
    if (!node_result.has_value()) {
        return util::unexpected(node_result.error());
    }

    if (mvcc_manager_) {
        NodeRecord tomb{}; tomb.id = node_id;
        transaction::Version ver{tx_id, tx_id, tomb, std::vector<Property>{}};
        auto write_result = mvcc_manager_->write_version(node_id, ver);
        if (!write_result.has_value()) {
            return util::unexpected(Error{ErrorCode::CORRUPTION, "Failed to write tombstone version"});
        }
    } else {
        // If no MVCC manager, fall back to legacy delete
        auto res = delete_node(node_id);
        if (!res.has_value()) return res;
    }
    
    if (wal_manager_) {
        std::stringstream ss; ss << "{\"op\":\"delete_node\",\"id\":" << node_id << "}";
        wal_manager_->log_operation({ss.str()});
    }
    return {};
}

util::expected<std::pair<NodeRecord, std::vector<Property>>, Error> GraphStore::get_node(transaction::TransactionId tx_id,
                                                                                        NodeId node_id) {
    if (mvcc_manager_) {
        auto vres = mvcc_manager_->read_version(node_id, tx_id);
        if (vres.has_value()) {
            // Return NodeRecord with versioned properties
            if (std::holds_alternative<NodeRecord>(vres->data)) {
                return std::make_pair(std::get<NodeRecord>(vres->data), vres->properties);
            }
        }
        // If no visible version, treat as not found
        if (!vres.has_value() && vres.error().code == transaction::MVCCErrorCode::NOT_FOUND) {
            return util::unexpected(Error{ErrorCode::NOT_FOUND, "Node not visible"});
        }
    }
    // Fallback to legacy path
    return get_node(node_id);
}

util::expected<EdgeId, Error> GraphStore::create_edge(transaction::TransactionId tx_id, NodeId from_node, NodeId to_node,
                                                   const std::string& label,
                                                   const std::vector<Property>& properties) {
     auto id_res = create_edge(from_node, to_node, label, properties);
     if (!id_res.has_value()) return util::unexpected(id_res.error());
     EdgeId eid = id_res.value();
     if (mvcc_manager_) {
         transaction::Version ver;
         ver.created_tx_id = tx_id;
         {
             EdgeRecord er{};
             er.id = eid;
             er.from_node = from_node;
             er.to_node = to_node;
             er.label_id = std::hash<std::string>{}(label);
             er.property_count = properties.size();
             ver.data = er;
         }
         ver.properties = properties; // Store properties in version
         mvcc_manager_->write_version(eid, ver);
     }
     if (wal_manager_) {
         std::stringstream ss; ss << "{\"op\":\"create_edge\",\"id\":" << eid << "}";
         wal_manager_->log_operation({ss.str()});
     }
     return eid;
 }
 
 util::expected<void, Error> GraphStore::update_edge(transaction::TransactionId tx_id, EdgeId edge_id,
                                                     const std::vector<Property>& properties) {
     auto res = update_edge(edge_id, properties);
     if (!res.has_value()) return res;
     if (mvcc_manager_) {
         EdgeRecord er{};
         er.id = edge_id;
         er.property_count = properties.size();
         transaction::Version ver{tx_id, 0, er, properties};
         mvcc_manager_->write_version(edge_id, ver);
     }
     if (wal_manager_) {
         std::stringstream ss; ss << "{\"op\":\"update_edge\",\"id\":" << edge_id << "}";
         wal_manager_->log_operation({ss.str()});
     }
     return {};
 }
 
 util::expected<void, Error> GraphStore::delete_edge(transaction::TransactionId tx_id, EdgeId edge_id) {
     auto res = delete_edge(edge_id);
     if (!res.has_value()) return res;
     if (mvcc_manager_) {
         EdgeRecord tomb{}; tomb.id = edge_id;
         transaction::Version ver{tx_id, tx_id, tomb, std::vector<storage::Property>{}};
         mvcc_manager_->write_version(edge_id, ver);
     }
     if (wal_manager_) {
         std::stringstream ss; ss << "{\"op\":\"delete_edge\",\"id\":" << edge_id << "}";
         wal_manager_->log_operation({ss.str()});
     }
     return {};
 }

util::expected<std::pair<EdgeRecord, std::vector<Property>>, Error> GraphStore::get_edge(transaction::TransactionId tx_id,
                                                                                         EdgeId edge_id) {
    if (mvcc_manager_) {
        auto vres = mvcc_manager_->read_version(edge_id, tx_id);
        if (vres.has_value()) {
            if (std::holds_alternative<EdgeRecord>(vres->data)) {
                return std::make_pair(std::get<EdgeRecord>(vres->data), vres->properties);
            }
        }
        if (!vres.has_value() && vres.error().code == transaction::MVCCErrorCode::NOT_FOUND) {
            return util::unexpected(Error{ErrorCode::NOT_FOUND, "Edge not visible"});
        }
    }
    return get_edge(edge_id);
}

util::expected<std::pair<EdgeRecord, std::vector<Property>>, Error> GraphStore::get_edge(EdgeId edge_id) {
    // Find page containing the edge
    PageId page_id;
    {
        std::lock_guard<std::mutex> lock(edge_index_mutex_);
        auto it = edge_page_index_.find(edge_id);
        if (it == edge_page_index_.end()) {
            return util::unexpected(Error{ErrorCode::NOT_FOUND, "Edge not found"});
        }
        page_id = it->second;
    }
    
    // Read the page
    auto page_result = page_store_->read_page(page_id);
    if (!page_result.has_value()) {
        return util::unexpected(page_result.error());
    }
    
    auto page_data = page_result.value();
    
    // Parse page to find the edge record
    PageHeader* header = reinterpret_cast<PageHeader*>(page_data.data());
    
    if (header->page_type != static_cast<uint32_t>(PageType::EDGE)) {
        return util::unexpected(Error{ErrorCode::CORRUPTION, "Invalid page type for edge"});
    }
    
    // For now, assume one edge per page (simplified)
    auto edge_data = page_data.subspan(sizeof(PageHeader));
    return RecordSerializer::deserialize_edge(edge_data);
}

util::expected<std::vector<EdgeId>, Error> GraphStore::get_outgoing_edges(NodeId node_id) {
    std::lock_guard<std::mutex> lock(adjacency_mutex_);
    auto it = outgoing_edges_.find(node_id);
    if (it == outgoing_edges_.end()) {
        return std::vector<EdgeId>{};
    }
    return it->second;
}

util::expected<std::vector<EdgeId>, Error> GraphStore::get_incoming_edges(NodeId node_id) {
    std::lock_guard<std::mutex> lock(adjacency_mutex_);
    auto it = incoming_edges_.find(node_id);
    if (it == incoming_edges_.end()) {
        return std::vector<EdgeId>{};
    }
    return it->second;
}

util::expected<std::vector<NodeId>, Error> GraphStore::get_adjacent_nodes(NodeId node_id) {
    std::vector<NodeId> adjacent_nodes;
    
    // Get outgoing edges
    auto outgoing_result = get_outgoing_edges(node_id);
    if (!outgoing_result.has_value()) {
        return util::unexpected(outgoing_result.error());
    }
    
    for (EdgeId edge_id : outgoing_result.value()) {
        auto edge_result = get_edge(edge_id);
        if (edge_result.has_value()) {
            adjacent_nodes.push_back(edge_result.value().first.to_node);
        }
    }
    
    // Get incoming edges
    auto incoming_result = get_incoming_edges(node_id);
    if (!incoming_result.has_value()) {
        return util::unexpected(incoming_result.error());
    }
    
    for (EdgeId edge_id : incoming_result.value()) {
        auto edge_result = get_edge(edge_id);
        if (edge_result.has_value()) {
            adjacent_nodes.push_back(edge_result.value().first.from_node);
        }
    }
    
    // Remove duplicates
    std::sort(adjacent_nodes.begin(), adjacent_nodes.end());
    adjacent_nodes.erase(std::unique(adjacent_nodes.begin(), adjacent_nodes.end()), adjacent_nodes.end());
    
    return adjacent_nodes;
}

util::expected<void, Error> GraphStore::batch_create_nodes(const std::vector<std::vector<Property>>& node_properties,
                                                         std::vector<NodeId>& created_node_ids) {
    created_node_ids.clear();
    created_node_ids.reserve(node_properties.size());
    
    for (const auto& properties : node_properties) {
        auto node_result = create_node(properties);
        if (!node_result.has_value()) {
            return util::unexpected(node_result.error());
        }
        created_node_ids.push_back(node_result.value());
    }
    
    return {};
}

util::expected<void, Error> GraphStore::batch_create_edges(const std::vector<std::tuple<NodeId, NodeId, std::string, std::vector<Property>>>& edges,
                                                         std::vector<EdgeId>& created_edge_ids) {
    created_edge_ids.clear();
    created_edge_ids.reserve(edges.size());
    
    for (const auto& [from_node, to_node, label, properties] : edges) {
        auto edge_result = create_edge(from_node, to_node, label, properties);
        if (!edge_result.has_value()) {
            return util::unexpected(edge_result.error());
        }
        created_edge_ids.push_back(edge_result.value());
    }
    
    return {};
}

size_t GraphStore::get_node_count() const {
    return node_count_.load();
}

size_t GraphStore::get_edge_count() const {
    return edge_count_.load();
}

util::expected<void, Error> GraphStore::sync() {
    return page_store_->sync();
}

util::expected<void, Error> GraphStore::compact() {
    // TODO: Implement compaction
    return {};
}

util::expected<void, Error> GraphStore::store_node_record(NodeId node_id, const NodeRecord& node, 
                                                        const std::vector<Property>& properties) {
    // Serialize node data
    auto serialized_data = RecordSerializer::serialize_node(node, properties);
    
    // Allocate page if needed
    auto page_result = allocate_node_page();
    if (!page_result.has_value()) {
        return util::unexpected(page_result.error());
    }
    
    PageId page_id = page_result.value();
    
    // Prepare page data
    std::vector<uint8_t> page_data(PAGE_SIZE, 0);
    
    // Write page header
    PageHeader header;
    header.page_id = page_id;
    header.page_type = static_cast<uint32_t>(PageType::NODE);
    header.record_count = 1;
    header.next_free_offset = sizeof(PageHeader) + serialized_data.size();
    
    std::memcpy(page_data.data(), &header, sizeof(PageHeader));
    std::memcpy(page_data.data() + sizeof(PageHeader), serialized_data.data(), serialized_data.size());
    
    // Write page
    if (auto result = page_store_->write_page(page_id, page_data); !result.has_value()) {
        return util::unexpected(result.error());
    }
    
    // Update node index
    {
        std::lock_guard<std::mutex> lock(node_index_mutex_);
        node_page_index_[node_id] = page_id;
    }
    
    return {};
}

util::expected<void, Error> GraphStore::store_edge_record(EdgeId edge_id, const EdgeRecord& edge, 
                                                        const std::vector<Property>& properties) {
    // Serialize edge data
    auto serialized_data = RecordSerializer::serialize_edge(edge, properties);
    
    // Allocate page if needed
    auto page_result = allocate_edge_page();
    if (!page_result.has_value()) {
        return util::unexpected(page_result.error());
    }
    
    PageId page_id = page_result.value();
    
    // Prepare page data
    std::vector<uint8_t> page_data(PAGE_SIZE, 0);
    
    // Write page header
    PageHeader header;
    header.page_id = page_id;
    header.page_type = static_cast<uint32_t>(PageType::EDGE);
    header.record_count = 1;
    header.next_free_offset = sizeof(PageHeader) + serialized_data.size();
    
    std::memcpy(page_data.data(), &header, sizeof(PageHeader));
    std::memcpy(page_data.data() + sizeof(PageHeader), serialized_data.data(), serialized_data.size());
    
    // Write page
    if (auto result = page_store_->write_page(page_id, page_data); !result.has_value()) {
        return util::unexpected(result.error());
    }
    
    // Update edge index
    {
        std::lock_guard<std::mutex> lock(edge_index_mutex_);
        edge_page_index_[edge_id] = page_id;
    }
    
    return {};
}

util::expected<void, Error> GraphStore::update_adjacency_lists(NodeId from_node, NodeId to_node, EdgeId edge_id, bool add) {
    std::lock_guard<std::mutex> lock(adjacency_mutex_);
    
    if (add) {
        outgoing_edges_[from_node].push_back(edge_id);
        incoming_edges_[to_node].push_back(edge_id);
    } else {
        // Remove from outgoing edges
        auto& outgoing = outgoing_edges_[from_node];
        outgoing.erase(std::remove(outgoing.begin(), outgoing.end(), edge_id), outgoing.end());
        
        // Remove from incoming edges
        auto& incoming = incoming_edges_[to_node];
        incoming.erase(std::remove(incoming.begin(), incoming.end(), edge_id), incoming.end());
    }
    
    return {};
}

util::expected<PageId, Error> GraphStore::allocate_node_page() {
    std::lock_guard<std::mutex> lock(page_alloc_mutex_);
    return page_store_->allocate_page();
}

util::expected<PageId, Error> GraphStore::allocate_edge_page() {
    std::lock_guard<std::mutex> lock(page_alloc_mutex_);
    return page_store_->allocate_page();
}

NodeId GraphStore::get_next_node_id() {
    return next_node_id_.fetch_add(1);
}

EdgeId GraphStore::get_next_edge_id() {
    return next_edge_id_.fetch_add(1);
}

// Legacy create_node without transaction context
util::expected<NodeId, Error> GraphStore::create_node(const std::vector<Property>& properties) {
    NodeId node_id = get_next_node_id();

    NodeRecord node;
    node.id = node_id;
    node.property_count = properties.size();
    node.in_degree = 0;
    node.out_degree = 0;

    if (auto result = store_node_record(node_id, node, properties); !result.has_value()) {
        return util::unexpected(result.error());
    }

    node_count_.fetch_add(1);
    return node_id;
}

// Legacy get_node without transaction context (existing implementation preserved below if missing)
util::expected<std::pair<NodeRecord, std::vector<Property>>, Error> GraphStore::get_node(NodeId node_id) {
    // Find page containing the node
    PageId page_id;
    {
        std::lock_guard<std::mutex> lock(node_index_mutex_);
        auto it = node_page_index_.find(node_id);
        if (it == node_page_index_.end()) {
            return util::unexpected(Error{ErrorCode::NOT_FOUND, "Node not found"});
        }
        page_id = it->second;
    }
    
    // Read the page
    auto page_result = page_store_->read_page(page_id);
    if (!page_result.has_value()) {
        return util::unexpected(page_result.error());
    }
    
    auto page_data = page_result.value();
    
    // Parse page to find the node record
    PageHeader* header = reinterpret_cast<PageHeader*>(page_data.data());
    
    if (header->page_type != static_cast<uint32_t>(PageType::NODE)) {
        return util::unexpected(Error{ErrorCode::CORRUPTION, "Invalid page type for node"});
    }
    
    // For now, assume one node per page (simplified)
    auto node_data = page_data.subspan(sizeof(PageHeader));
    return RecordSerializer::deserialize_node(node_data);
}

// Legacy edge operations without transaction context
util::expected<EdgeId, Error> GraphStore::create_edge(NodeId from_node, NodeId to_node,
                                                      const std::string& label,
                                                      const std::vector<Property>& properties) {
    EdgeId edge_id = get_next_edge_id();

    EdgeRecord edge;
    edge.id = edge_id;
    edge.from_node = from_node;
    edge.to_node = to_node;
    edge.label_id = std::hash<std::string>{}(label);
    edge.property_count = properties.size();
    edge.timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();

    if (auto result = store_edge_record(edge_id, edge, properties); !result.has_value()) {
        return util::unexpected(result.error());
    }

    // Update adjacency lists
    if (auto result = update_adjacency_lists(from_node, to_node, edge_id, true); !result.has_value()) {
        return util::unexpected(result.error());
    }

    edge_count_.fetch_add(1);
    return edge_id;
}

util::expected<void, Error> GraphStore::update_edge(EdgeId edge_id, const std::vector<Property>& properties) {
    auto edge_result = get_edge(edge_id);
    if (!edge_result.has_value()) {
        return util::unexpected(edge_result.error());
    }

    auto [edge, _] = edge_result.value();
    edge.property_count = properties.size();

    return store_edge_record(edge_id, edge, properties);
}

util::expected<void, Error> GraphStore::delete_edge(EdgeId edge_id) {
    auto edge_result = get_edge(edge_id);
    if (!edge_result.has_value()) {
        return util::unexpected(edge_result.error());
    }

    auto [edge, _] = edge_result.value();

    if (auto result = update_adjacency_lists(edge.from_node, edge.to_node, edge_id, false); !result.has_value()) {
        return util::unexpected(result.error());
    }

    {
        std::lock_guard<std::mutex> lock(edge_index_mutex_);
        edge_page_index_.erase(edge_id);
    }

    edge_count_.fetch_sub(1);
    return {};
}

util::expected<void, Error> GraphStore::update_node(NodeId node_id,
                                                    const std::vector<Property>& properties) {
    // Get existing node
    auto node_result = get_node(node_id);
    if (!node_result.has_value()) {
        return util::unexpected(node_result.error());
    }

    auto [node, _] = node_result.value();
    node.property_count = properties.size();

    return store_node_record(node_id, node, properties);
}

util::expected<void, Error> GraphStore::delete_node(NodeId node_id) {
    // Check if node has edges
    auto outgoing_result = get_outgoing_edges(node_id);
    auto incoming_result = get_incoming_edges(node_id);

    if (outgoing_result.has_value() && !outgoing_result.value().empty()) {
        return util::unexpected(Error{ErrorCode::INVALID_ARGUMENT, "Cannot delete node with outgoing edges"});
    }

    if (incoming_result.has_value() && !incoming_result.value().empty()) {
        return util::unexpected(Error{ErrorCode::INVALID_ARGUMENT, "Cannot delete node with incoming edges"});
    }

    // Remove from node index
    {
        std::lock_guard<std::mutex> lock(node_index_mutex_);
        node_page_index_.erase(node_id);
    }

    // Remove from adjacency lists
    {
        std::lock_guard<std::mutex> lock(adjacency_mutex_);
        outgoing_edges_.erase(node_id);
        incoming_edges_.erase(node_id);
    }

    node_count_.fetch_sub(1);
    return {};
}

}  // namespace loredb::storage