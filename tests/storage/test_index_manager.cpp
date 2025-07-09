#include <gtest/gtest.h>
#include "../../src/storage/simple_index_manager.h"
#include <thread>
#include <vector>
#include <atomic>

using namespace graphdb::storage;

class IndexManagerTest : public ::testing::Test {
protected:
    void SetUp() override {
        index_manager_ = std::make_unique<SimpleIndexManager>();
    }
    
    void TearDown() override {
        index_manager_.reset();
    }
    
    std::unique_ptr<SimpleIndexManager> index_manager_;
};

TEST_F(IndexManagerTest, NodePropertyIndexing) {
    NodeId node1 = 1;
    NodeId node2 = 2;
    NodeId node3 = 3;
    
    // Index some properties
    index_manager_->index_node_property(node1, "title", "Document 1");
    index_manager_->index_node_property(node2, "title", "Document 2");
    index_manager_->index_node_property(node3, "title", "Document 1"); // Same value as node1
    
    index_manager_->index_node_property(node1, "category", "Tech");
    index_manager_->index_node_property(node2, "category", "Science");
    
    // Test finding nodes by property
    auto nodes_with_title1 = index_manager_->find_nodes_by_property("title", "Document 1");
    ASSERT_EQ(nodes_with_title1.size(), 2);
    ASSERT_TRUE(std::find(nodes_with_title1.begin(), nodes_with_title1.end(), node1) != nodes_with_title1.end());
    ASSERT_TRUE(std::find(nodes_with_title1.begin(), nodes_with_title1.end(), node3) != nodes_with_title1.end());
    
    auto nodes_with_title2 = index_manager_->find_nodes_by_property("title", "Document 2");
    ASSERT_EQ(nodes_with_title2.size(), 1);
    ASSERT_EQ(nodes_with_title2[0], node2);
    
    auto nodes_with_tech = index_manager_->find_nodes_by_property("category", "Tech");
    ASSERT_EQ(nodes_with_tech.size(), 1);
    ASSERT_EQ(nodes_with_tech[0], node1);
    
    // Test non-existent property
    auto nodes_nonexistent = index_manager_->find_nodes_by_property("nonexistent", "value");
    ASSERT_TRUE(nodes_nonexistent.empty());
}

TEST_F(IndexManagerTest, EdgePropertyIndexing) {
    EdgeId edge1 = 1;
    EdgeId edge2 = 2;
    EdgeId edge3 = 3;
    
    // Index some properties
    index_manager_->index_edge_property(edge1, "type", "links_to");
    index_manager_->index_edge_property(edge2, "type", "links_to");
    index_manager_->index_edge_property(edge3, "type", "references");
    
    index_manager_->index_edge_property(edge1, "context", "introduction");
    index_manager_->index_edge_property(edge2, "context", "conclusion");
    
    // Test finding edges by property
    auto edges_links_to = index_manager_->find_edges_by_property("type", "links_to");
    ASSERT_EQ(edges_links_to.size(), 2);
    ASSERT_TRUE(std::find(edges_links_to.begin(), edges_links_to.end(), edge1) != edges_links_to.end());
    ASSERT_TRUE(std::find(edges_links_to.begin(), edges_links_to.end(), edge2) != edges_links_to.end());
    
    auto edges_references = index_manager_->find_edges_by_property("type", "references");
    ASSERT_EQ(edges_references.size(), 1);
    ASSERT_EQ(edges_references[0], edge3);
    
    auto edges_intro = index_manager_->find_edges_by_property("context", "introduction");
    ASSERT_EQ(edges_intro.size(), 1);
    ASSERT_EQ(edges_intro[0], edge1);
}

TEST_F(IndexManagerTest, AdjacencyListOperations) {
    NodeId node1 = 1;
    NodeId node2 = 2;
    NodeId node3 = 3;
    EdgeId edge1 = 1;
    EdgeId edge2 = 2;
    EdgeId edge3 = 3;
    
    // Add edges to adjacency lists
    index_manager_->add_edge_to_adjacency(node1, node2, edge1);
    index_manager_->add_edge_to_adjacency(node1, node3, edge2);
    index_manager_->add_edge_to_adjacency(node2, node3, edge3);
    
    // Test outgoing edges
    auto outgoing_node1 = index_manager_->get_outgoing_edges(node1);
    ASSERT_EQ(outgoing_node1.size(), 2);
    ASSERT_TRUE(std::find(outgoing_node1.begin(), outgoing_node1.end(), edge1) != outgoing_node1.end());
    ASSERT_TRUE(std::find(outgoing_node1.begin(), outgoing_node1.end(), edge2) != outgoing_node1.end());
    
    auto outgoing_node2 = index_manager_->get_outgoing_edges(node2);
    ASSERT_EQ(outgoing_node2.size(), 1);
    ASSERT_EQ(outgoing_node2[0], edge3);
    
    auto outgoing_node3 = index_manager_->get_outgoing_edges(node3);
    ASSERT_TRUE(outgoing_node3.empty());
    
    // Test incoming edges
    auto incoming_node1 = index_manager_->get_incoming_edges(node1);
    ASSERT_TRUE(incoming_node1.empty());
    
    auto incoming_node2 = index_manager_->get_incoming_edges(node2);
    ASSERT_EQ(incoming_node2.size(), 1);
    ASSERT_EQ(incoming_node2[0], edge1);
    
    auto incoming_node3 = index_manager_->get_incoming_edges(node3);
    ASSERT_EQ(incoming_node3.size(), 2);
    ASSERT_TRUE(std::find(incoming_node3.begin(), incoming_node3.end(), edge2) != incoming_node3.end());
    ASSERT_TRUE(std::find(incoming_node3.begin(), incoming_node3.end(), edge3) != incoming_node3.end());
    
    // Test adjacent nodes
    auto adjacent_node1 = index_manager_->get_adjacent_nodes(node1);
    ASSERT_EQ(adjacent_node1.size(), 2);
    ASSERT_TRUE(std::find(adjacent_node1.begin(), adjacent_node1.end(), node2) != adjacent_node1.end());
    ASSERT_TRUE(std::find(adjacent_node1.begin(), adjacent_node1.end(), node3) != adjacent_node1.end());
}

TEST_F(IndexManagerTest, RemoveEdgeFromAdjacency) {
    NodeId node1 = 1;
    NodeId node2 = 2;
    EdgeId edge1 = 1;
    EdgeId edge2 = 2;
    
    // Add edges
    index_manager_->add_edge_to_adjacency(node1, node2, edge1);
    index_manager_->add_edge_to_adjacency(node1, node2, edge2);
    
    // Verify edges are there
    auto outgoing_initial = index_manager_->get_outgoing_edges(node1);
    ASSERT_EQ(outgoing_initial.size(), 2);
    
    auto incoming_initial = index_manager_->get_incoming_edges(node2);
    ASSERT_EQ(incoming_initial.size(), 2);
    
    // Remove one edge
    index_manager_->remove_edge_from_adjacency(node1, node2, edge1);
    
    // Verify edge is removed
    auto outgoing_after = index_manager_->get_outgoing_edges(node1);
    ASSERT_EQ(outgoing_after.size(), 1);
    ASSERT_EQ(outgoing_after[0], edge2);
    
    auto incoming_after = index_manager_->get_incoming_edges(node2);
    ASSERT_EQ(incoming_after.size(), 1);
    ASSERT_EQ(incoming_after[0], edge2);
}

TEST_F(IndexManagerTest, Statistics) {
    NodeId node1 = 1;
    NodeId node2 = 2;
    EdgeId edge1 = 1;
    
    // Initially should be empty
    ASSERT_EQ(index_manager_->get_node_property_index_size(), 0);
    ASSERT_EQ(index_manager_->get_edge_property_index_size(), 0);
    ASSERT_EQ(index_manager_->get_adjacency_list_count(), 0);
    
    // Add some data
    index_manager_->index_node_property(node1, "title", "Document 1");
    index_manager_->index_edge_property(edge1, "type", "links_to");
    index_manager_->add_edge_to_adjacency(node1, node2, edge1);
    
    // Check statistics
    ASSERT_GT(index_manager_->get_node_property_index_size(), 0);
    ASSERT_GT(index_manager_->get_edge_property_index_size(), 0);
    ASSERT_GT(index_manager_->get_adjacency_list_count(), 0);
}

TEST_F(IndexManagerTest, ClearAllIndexes) {
    NodeId node1 = 1;
    NodeId node2 = 2;
    EdgeId edge1 = 1;
    
    // Add some data
    index_manager_->index_node_property(node1, "title", "Document 1");
    index_manager_->index_edge_property(edge1, "type", "links_to");
    index_manager_->add_edge_to_adjacency(node1, node2, edge1);
    
    // Verify data is there
    ASSERT_FALSE(index_manager_->find_nodes_by_property("title", "Document 1").empty());
    ASSERT_FALSE(index_manager_->find_edges_by_property("type", "links_to").empty());
    ASSERT_FALSE(index_manager_->get_outgoing_edges(node1).empty());
    
    // Clear all indexes
    index_manager_->clear_all_indexes();
    
    // Verify data is gone
    ASSERT_TRUE(index_manager_->find_nodes_by_property("title", "Document 1").empty());
    ASSERT_TRUE(index_manager_->find_edges_by_property("type", "links_to").empty());
    ASSERT_TRUE(index_manager_->get_outgoing_edges(node1).empty());
}

TEST_F(IndexManagerTest, ConcurrentPropertyIndexing) {
    const size_t num_threads = 4;
    const size_t items_per_thread = 100;
    std::atomic<size_t> node_id_counter(1);
    
    std::vector<std::thread> threads;
    
    for (size_t t = 0; t < num_threads; ++t) {
        threads.emplace_back([this, &node_id_counter, items_per_thread, t]() {
            for (size_t i = 0; i < items_per_thread; ++i) {
                NodeId node_id = node_id_counter.fetch_add(1);
                std::string value = "Thread_" + std::to_string(t) + "_Item_" + std::to_string(i);
                index_manager_->index_node_property(node_id, "test_key", value);
            }
        });
    }
    
    for (auto& thread : threads) {
        thread.join();
    }
    
    // Verify all items were indexed
    size_t total_found = 0;
    for (size_t t = 0; t < num_threads; ++t) {
        for (size_t i = 0; i < items_per_thread; ++i) {
            std::string value = "Thread_" + std::to_string(t) + "_Item_" + std::to_string(i);
            auto nodes = index_manager_->find_nodes_by_property("test_key", value);
            ASSERT_EQ(nodes.size(), 1);
            total_found++;
        }
    }
    
    ASSERT_EQ(total_found, num_threads * items_per_thread);
}

TEST_F(IndexManagerTest, ConcurrentAdjacencyOperations) {
    const size_t num_threads = 4;
    const size_t edges_per_thread = 100;
    std::atomic<EdgeId> edge_id_counter(1);
    
    std::vector<std::thread> threads;
    
    for (size_t t = 0; t < num_threads; ++t) {
        threads.emplace_back([this, &edge_id_counter, edges_per_thread, t]() {
            for (size_t i = 0; i < edges_per_thread; ++i) {
                EdgeId edge_id = edge_id_counter.fetch_add(1);
                NodeId from_node = t * 1000 + i;
                NodeId to_node = t * 1000 + i + 1;
                
                index_manager_->add_edge_to_adjacency(from_node, to_node, edge_id);
            }
        });
    }
    
    for (auto& thread : threads) {
        thread.join();
    }
    
    // Verify all edges were added
    size_t total_edges = 0;
    for (size_t t = 0; t < num_threads; ++t) {
        for (size_t i = 0; i < edges_per_thread; ++i) {
            NodeId from_node = t * 1000 + i;
            auto outgoing = index_manager_->get_outgoing_edges(from_node);
            ASSERT_EQ(outgoing.size(), 1);
            total_edges++;
        }
    }
    
    ASSERT_EQ(total_edges, num_threads * edges_per_thread);
}

// Lock-free adjacency list tests removed (depends on removed IndexManager)
// SimpleIndexManager uses regular mutexes instead