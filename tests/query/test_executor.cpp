#include <gtest/gtest.h>
#include "../../src/query/executor.h"
#include "../../src/storage/file_page_store.h"
#include "../../src/storage/graph_store.h"
#include "../../src/storage/simple_index_manager.h"
#include <unistd.h>

using namespace loredb::query;
using namespace loredb::storage;

class QueryExecutorTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create temporary file for testing
        db_filename_ = "/tmp/test_query_loredb_" + std::to_string(getpid()) + ".db";
        
        auto page_store = std::make_unique<FilePageStore>(db_filename_);
        graph_store_ = std::make_shared<GraphStore>(std::move(page_store));
        index_manager_ = std::make_shared<SimpleIndexManager>();
        query_executor_ = std::make_unique<QueryExecutor>(graph_store_, index_manager_);
        
        // Create some test data
        setup_test_data();
    }
    
    void TearDown() override {
        query_executor_.reset();
        index_manager_.reset();
        graph_store_.reset();
        unlink(db_filename_.c_str());
    }
    
    void setup_test_data() {
        // Create nodes
        std::vector<Property> node1_props = {
            {"title", PropertyValue{std::string("Document 1")}},
            {"type", PropertyValue{std::string("document")}},
            {"category", PropertyValue{std::string("tech")}}
        };
        auto node1_result = graph_store_->create_node(node1_props);
        ASSERT_TRUE(node1_result.has_value());
        node1_id_ = node1_result.value();
        
        std::vector<Property> node2_props = {
            {"title", PropertyValue{std::string("Document 2")}},
            {"type", PropertyValue{std::string("document")}},
            {"category", PropertyValue{std::string("science")}}
        };
        auto node2_result = graph_store_->create_node(node2_props);
        ASSERT_TRUE(node2_result.has_value());
        node2_id_ = node2_result.value();
        
        std::vector<Property> node3_props = {
            {"title", PropertyValue{std::string("Document 3")}},
            {"type", PropertyValue{std::string("document")}},
            {"category", PropertyValue{std::string("tech")}}
        };
        auto node3_result = graph_store_->create_node(node3_props);
        ASSERT_TRUE(node3_result.has_value());
        node3_id_ = node3_result.value();
        
        // Index node properties
        for (const auto& prop : node1_props) {
            if (std::holds_alternative<std::string>(prop.value)) {
                index_manager_->index_node_property(node1_id_, prop.key, std::get<std::string>(prop.value));
            }
        }
        for (const auto& prop : node2_props) {
            if (std::holds_alternative<std::string>(prop.value)) {
                index_manager_->index_node_property(node2_id_, prop.key, std::get<std::string>(prop.value));
            }
        }
        for (const auto& prop : node3_props) {
            if (std::holds_alternative<std::string>(prop.value)) {
                index_manager_->index_node_property(node3_id_, prop.key, std::get<std::string>(prop.value));
            }
        }
        
        // Create edges
        std::vector<Property> edge1_props = {
            {"type", PropertyValue{std::string("links_to")}},
            {"context", PropertyValue{std::string("introduction")}}
        };
        auto edge1_result = graph_store_->create_edge(node1_id_, node2_id_, "links_to", edge1_props);
        ASSERT_TRUE(edge1_result.has_value());
        edge1_id_ = edge1_result.value();
        
        std::vector<Property> edge2_props = {
            {"type", PropertyValue{std::string("links_to")}},
            {"context", PropertyValue{std::string("conclusion")}}
        };
        auto edge2_result = graph_store_->create_edge(node2_id_, node3_id_, "links_to", edge2_props);
        ASSERT_TRUE(edge2_result.has_value());
        edge2_id_ = edge2_result.value();
        
        // Index edge properties and update adjacency lists
        for (const auto& prop : edge1_props) {
            if (std::holds_alternative<std::string>(prop.value)) {
                index_manager_->index_edge_property(edge1_id_, prop.key, std::get<std::string>(prop.value));
            }
        }
        for (const auto& prop : edge2_props) {
            if (std::holds_alternative<std::string>(prop.value)) {
                index_manager_->index_edge_property(edge2_id_, prop.key, std::get<std::string>(prop.value));
            }
        }
        
        index_manager_->add_edge_to_adjacency(node1_id_, node2_id_, edge1_id_);
        index_manager_->add_edge_to_adjacency(node2_id_, node3_id_, edge2_id_);
    }
    
    std::string db_filename_;
    std::shared_ptr<GraphStore> graph_store_;
    std::shared_ptr<SimpleIndexManager> index_manager_;
    std::unique_ptr<QueryExecutor> query_executor_;
    
    NodeId node1_id_, node2_id_, node3_id_;
    EdgeId edge1_id_, edge2_id_;
};

TEST_F(QueryExecutorTest, GetNodeById) {
    auto result = query_executor_->get_node_by_id(node1_id_);
    ASSERT_TRUE(result.has_value()) << "Failed to get node by ID: " << result.error().message;
    
    const auto& query_result = result.value();
    ASSERT_EQ(query_result.columns.size(), 2);
    ASSERT_EQ(query_result.columns[0], "id");
    ASSERT_EQ(query_result.columns[1], "properties");
    ASSERT_EQ(query_result.rows.size(), 1);
    ASSERT_EQ(query_result.rows[0][0], std::to_string(node1_id_));
}

TEST_F(QueryExecutorTest, GetNonexistentNodeById) {
    NodeId nonexistent_id = 999999;
    auto result = query_executor_->get_node_by_id(nonexistent_id);
    ASSERT_FALSE(result.has_value());
    ASSERT_EQ(result.error().code, ErrorCode::NOT_FOUND);
}

TEST_F(QueryExecutorTest, GetEdgeById) {
    auto result = query_executor_->get_edge_by_id(edge1_id_);
    ASSERT_TRUE(result.has_value()) << "Failed to get edge by ID: " << result.error().message;
    
    const auto& query_result = result.value();
    ASSERT_EQ(query_result.columns.size(), 4);
    ASSERT_EQ(query_result.columns[0], "id");
    ASSERT_EQ(query_result.columns[1], "from_node");
    ASSERT_EQ(query_result.columns[2], "to_node");
    ASSERT_EQ(query_result.columns[3], "properties");
    ASSERT_EQ(query_result.rows.size(), 1);
    ASSERT_EQ(query_result.rows[0][0], std::to_string(edge1_id_));
    ASSERT_EQ(query_result.rows[0][1], std::to_string(node1_id_));
    ASSERT_EQ(query_result.rows[0][2], std::to_string(node2_id_));
}

TEST_F(QueryExecutorTest, GetNodesByProperty) {
    auto result = query_executor_->get_nodes_by_property("category", "tech");
    ASSERT_TRUE(result.has_value()) << "Failed to get nodes by property: " << result.error().message;
    
    const auto& query_result = result.value();
    ASSERT_EQ(query_result.columns.size(), 2);
    ASSERT_EQ(query_result.rows.size(), 2); // node1 and node3 have category "tech"
    
    // Check that both nodes are returned
    std::vector<std::string> returned_ids;
    for (const auto& row : query_result.rows) {
        returned_ids.push_back(row[0]);
    }
    
    ASSERT_TRUE(std::find(returned_ids.begin(), returned_ids.end(), std::to_string(node1_id_)) != returned_ids.end());
    ASSERT_TRUE(std::find(returned_ids.begin(), returned_ids.end(), std::to_string(node3_id_)) != returned_ids.end());
}

TEST_F(QueryExecutorTest, GetEdgesByProperty) {
    auto result = query_executor_->get_edges_by_property("type", "links_to");
    ASSERT_TRUE(result.has_value()) << "Failed to get edges by property: " << result.error().message;
    
    const auto& query_result = result.value();
    ASSERT_EQ(query_result.columns.size(), 4);
    ASSERT_EQ(query_result.rows.size(), 2); // Both edges have type "links_to"
    
    // Check that both edges are returned
    std::vector<std::string> returned_ids;
    for (const auto& row : query_result.rows) {
        returned_ids.push_back(row[0]);
    }
    
    ASSERT_TRUE(std::find(returned_ids.begin(), returned_ids.end(), std::to_string(edge1_id_)) != returned_ids.end());
    ASSERT_TRUE(std::find(returned_ids.begin(), returned_ids.end(), std::to_string(edge2_id_)) != returned_ids.end());
}

TEST_F(QueryExecutorTest, GetAdjacentNodes) {
    auto result = query_executor_->get_adjacent_nodes(node1_id_);
    ASSERT_TRUE(result.has_value()) << "Failed to get adjacent nodes: " << result.error().message;
    
    const auto& query_result = result.value();
    ASSERT_EQ(query_result.columns.size(), 1);
    ASSERT_EQ(query_result.columns[0], "node_id");
    ASSERT_EQ(query_result.rows.size(), 1);
    ASSERT_EQ(query_result.rows[0][0], std::to_string(node2_id_));
}

TEST_F(QueryExecutorTest, GetOutgoingEdges) {
    auto result = query_executor_->get_outgoing_edges(node1_id_);
    ASSERT_TRUE(result.has_value()) << "Failed to get outgoing edges: " << result.error().message;
    
    const auto& query_result = result.value();
    ASSERT_EQ(query_result.columns.size(), 1);
    ASSERT_EQ(query_result.columns[0], "edge_id");
    ASSERT_EQ(query_result.rows.size(), 1);
    ASSERT_EQ(query_result.rows[0][0], std::to_string(edge1_id_));
}

TEST_F(QueryExecutorTest, GetIncomingEdges) {
    auto result = query_executor_->get_incoming_edges(node2_id_);
    ASSERT_TRUE(result.has_value()) << "Failed to get incoming edges: " << result.error().message;
    
    const auto& query_result = result.value();
    ASSERT_EQ(query_result.columns.size(), 1);
    ASSERT_EQ(query_result.columns[0], "edge_id");
    ASSERT_EQ(query_result.rows.size(), 1);
    ASSERT_EQ(query_result.rows[0][0], std::to_string(edge1_id_));
}

TEST_F(QueryExecutorTest, FindShortestPath) {
    auto result = query_executor_->find_shortest_path(node1_id_, node3_id_);
    ASSERT_TRUE(result.has_value()) << "Failed to find shortest path: " << result.error().message;
    
    const auto& query_result = result.value();
    ASSERT_EQ(query_result.columns.size(), 2);
    ASSERT_EQ(query_result.columns[0], "path_length");
    ASSERT_EQ(query_result.columns[1], "path");
    ASSERT_EQ(query_result.rows.size(), 1);
    ASSERT_EQ(query_result.rows[0][0], "2"); // Path length should be 2
}

TEST_F(QueryExecutorTest, FindShortestPathNoPath) {
    // Create an isolated node
    std::vector<Property> isolated_props = {
        {"title", PropertyValue{std::string("Isolated Document")}},
        {"type", PropertyValue{std::string("document")}}
    };
    auto isolated_result = graph_store_->create_node(isolated_props);
    ASSERT_TRUE(isolated_result.has_value());
    NodeId isolated_id = isolated_result.value();
    
    auto result = query_executor_->find_shortest_path(node1_id_, isolated_id);
    ASSERT_TRUE(result.has_value()) << "Failed to find shortest path: " << result.error().message;
    
    const auto& query_result = result.value();
    ASSERT_EQ(query_result.rows.size(), 1);
    ASSERT_EQ(query_result.rows[0][0], "0"); // No path
}

TEST_F(QueryExecutorTest, CountNodes) {
    auto result = query_executor_->count_nodes();
    ASSERT_TRUE(result.has_value()) << "Failed to count nodes: " << result.error().message;
    
    const auto& query_result = result.value();
    ASSERT_EQ(query_result.columns.size(), 1);
    ASSERT_EQ(query_result.columns[0], "count");
    ASSERT_EQ(query_result.rows.size(), 1);
    ASSERT_EQ(query_result.rows[0][0], "3"); // We created 3 nodes
}

TEST_F(QueryExecutorTest, CountEdges) {
    auto result = query_executor_->count_edges();
    ASSERT_TRUE(result.has_value()) << "Failed to count edges: " << result.error().message;
    
    const auto& query_result = result.value();
    ASSERT_EQ(query_result.columns.size(), 1);
    ASSERT_EQ(query_result.columns[0], "count");
    ASSERT_EQ(query_result.rows.size(), 1);
    ASSERT_EQ(query_result.rows[0][0], "2"); // We created 2 edges
}

TEST_F(QueryExecutorTest, GetNodeDegreeStats) {
    auto result = query_executor_->get_node_degree_stats();
    ASSERT_TRUE(result.has_value()) << "Failed to get node degree stats: " << result.error().message;
    
    const auto& query_result = result.value();
    ASSERT_EQ(query_result.columns.size(), 2);
    ASSERT_EQ(query_result.columns[0], "metric");
    ASSERT_EQ(query_result.columns[1], "value");
    ASSERT_GE(query_result.rows.size(), 2); // At least total_nodes and total_edges
}

TEST_F(QueryExecutorTest, BatchGetNodes) {
    std::vector<NodeId> node_ids = {node1_id_, node2_id_, node3_id_};
    
    auto result = query_executor_->batch_get_nodes(node_ids);
    ASSERT_TRUE(result.has_value()) << "Failed to batch get nodes: " << result.error().message;
    
    const auto& query_result = result.value();
    ASSERT_EQ(query_result.columns.size(), 2);
    ASSERT_EQ(query_result.rows.size(), 3);
    
    // Check that all nodes are returned
    std::vector<std::string> returned_ids;
    for (const auto& row : query_result.rows) {
        returned_ids.push_back(row[0]);
    }
    
    ASSERT_TRUE(std::find(returned_ids.begin(), returned_ids.end(), std::to_string(node1_id_)) != returned_ids.end());
    ASSERT_TRUE(std::find(returned_ids.begin(), returned_ids.end(), std::to_string(node2_id_)) != returned_ids.end());
    ASSERT_TRUE(std::find(returned_ids.begin(), returned_ids.end(), std::to_string(node3_id_)) != returned_ids.end());
}

TEST_F(QueryExecutorTest, BatchGetEdges) {
    std::vector<EdgeId> edge_ids = {edge1_id_, edge2_id_};
    
    auto result = query_executor_->batch_get_edges(edge_ids);
    ASSERT_TRUE(result.has_value()) << "Failed to batch get edges: " << result.error().message;
    
    const auto& query_result = result.value();
    ASSERT_EQ(query_result.columns.size(), 4);
    ASSERT_EQ(query_result.rows.size(), 2);
    
    // Check that all edges are returned
    std::vector<std::string> returned_ids;
    for (const auto& row : query_result.rows) {
        returned_ids.push_back(row[0]);
    }
    
    ASSERT_TRUE(std::find(returned_ids.begin(), returned_ids.end(), std::to_string(edge1_id_)) != returned_ids.end());
    ASSERT_TRUE(std::find(returned_ids.begin(), returned_ids.end(), std::to_string(edge2_id_)) != returned_ids.end());
}

TEST_F(QueryExecutorTest, GetDocumentBacklinks) {
    auto result = query_executor_->get_document_backlinks(node2_id_);
    ASSERT_TRUE(result.has_value()) << "Failed to get document backlinks: " << result.error().message;
    
    const auto& query_result = result.value();
    ASSERT_EQ(query_result.columns.size(), 1);
    ASSERT_EQ(query_result.columns[0], "edge_id");
    ASSERT_EQ(query_result.rows.size(), 1);
    ASSERT_EQ(query_result.rows[0][0], std::to_string(edge1_id_));
}

TEST_F(QueryExecutorTest, GetDocumentOutlinks) {
    auto result = query_executor_->get_document_outlinks(node2_id_);
    ASSERT_TRUE(result.has_value()) << "Failed to get document outlinks: " << result.error().message;
    
    const auto& query_result = result.value();
    ASSERT_EQ(query_result.columns.size(), 1);
    ASSERT_EQ(query_result.columns[0], "edge_id");
    ASSERT_EQ(query_result.rows.size(), 1);
    ASSERT_EQ(query_result.rows[0][0], std::to_string(edge2_id_));
}

TEST_F(QueryExecutorTest, FindRelatedDocuments) {
    auto result = query_executor_->find_related_documents(node1_id_, 10);
    ASSERT_TRUE(result.has_value()) << "Failed to find related documents: " << result.error().message;
    
    const auto& query_result = result.value();
    ASSERT_EQ(query_result.columns.size(), 2);
    ASSERT_EQ(query_result.columns[0], "document_id");
    ASSERT_EQ(query_result.columns[1], "relation_type");
    ASSERT_EQ(query_result.rows.size(), 1);
    ASSERT_EQ(query_result.rows[0][0], std::to_string(node2_id_));
    ASSERT_EQ(query_result.rows[0][1], "adjacent");
}

TEST_F(QueryExecutorTest, SuggestLinksForDocument) {
    auto result = query_executor_->suggest_links_for_document(node1_id_, "sample content");
    ASSERT_TRUE(result.has_value()) << "Failed to suggest links for document: " << result.error().message;
    
    const auto& query_result = result.value();
    ASSERT_EQ(query_result.columns.size(), 2);
    ASSERT_EQ(query_result.columns[0], "suggested_document_id");
    ASSERT_EQ(query_result.columns[1], "reason");
    ASSERT_GE(query_result.rows.size(), 1);
}

// Streaming query tests removed for C++20 compatibility
// TODO: Re-implement with proper coroutine support