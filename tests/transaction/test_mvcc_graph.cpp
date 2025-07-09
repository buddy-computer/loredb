#include <gtest/gtest.h>
#include "../../src/storage/graph_store.h"
#include "../../src/storage/file_page_store.h"
#include "../../src/transaction/mvcc_manager.h"
#include "../../src/transaction/mvcc.h"
#include <unistd.h>

using namespace graphdb;

class GraphMVCCIntegrationTest : public ::testing::Test {
protected:
    void SetUp() override {
        db_file_ = "/tmp/test_graphdb_mvcc_" + std::to_string(getpid()) + ".db";
        auto page_store = std::make_unique<storage::FilePageStore>(db_file_);
        txn_mgr_ = std::make_shared<transaction::TransactionManager>();
        mvcc_mgr_ = std::make_shared<transaction::MVCCManager>(txn_mgr_);
        graph_store_ = std::make_unique<storage::GraphStore>(std::move(page_store), mvcc_mgr_);
    }

    void TearDown() override {
        graph_store_.reset();
        unlink(db_file_.c_str());
    }

    std::string db_file_;
    std::shared_ptr<transaction::TransactionManager> txn_mgr_;
    std::shared_ptr<transaction::MVCCManager> mvcc_mgr_;
    std::unique_ptr<storage::GraphStore> graph_store_;
};

TEST_F(GraphMVCCIntegrationTest, SnapshotIsolationBasic) {
    auto tx1 = txn_mgr_->begin_transaction();

    // tx1 creates a node
    auto node_res = graph_store_->create_node(tx1->id, {});
    ASSERT_TRUE(node_res.has_value());
    storage::NodeId node_id = node_res.value();

    // tx2 starts after tx1's write but before commit
    auto tx2 = txn_mgr_->begin_transaction();

    auto node_visible_tx2 = graph_store_->get_node(tx2->id, node_id);
    // tx2 should NOT see the node because tx1 hasn't committed yet (proper MVCC)
    ASSERT_FALSE(node_visible_tx2.has_value());

    // Now commit tx1
    ASSERT_TRUE(txn_mgr_->commit_transaction(tx1));
    
    // After commit, tx2 should see the node
    auto node_visible_tx2_after_commit = graph_store_->get_node(tx2->id, node_id);
    ASSERT_TRUE(node_visible_tx2_after_commit.has_value());
    ASSERT_EQ(node_visible_tx2_after_commit.value().first.id, node_id);
    
    // tx1 should still see its original version
    auto node_visible_tx1 = graph_store_->get_node(tx1->id, node_id);
    ASSERT_TRUE(node_visible_tx1.has_value());
    ASSERT_EQ(node_visible_tx1.value().first.id, node_id);
}

TEST_F(GraphMVCCIntegrationTest, PropertyVersioning) {
    // Create a transaction and add a node with properties
    auto tx1 = txn_mgr_->begin_transaction();
    
    std::vector<storage::Property> initial_props = {
        {"name", std::string("Node1")},
        {"value", int64_t(42)}
    };
    
    auto node_res = graph_store_->create_node(tx1->id, initial_props);
    ASSERT_TRUE(node_res.has_value());
    storage::NodeId node_id = node_res.value();
    
    // Commit the transaction
    ASSERT_TRUE(txn_mgr_->commit_transaction(tx1));
    
    // Create a second transaction and read the node
    auto tx2 = txn_mgr_->begin_transaction();
    auto read_res = graph_store_->get_node(tx2->id, node_id);
    ASSERT_TRUE(read_res.has_value());
    
    auto [node_record, properties] = read_res.value();
    ASSERT_EQ(properties.size(), 2);
    
    // Check property values
    ASSERT_EQ(properties[0].key, "name");
    ASSERT_EQ(std::get<std::string>(properties[0].value), "Node1");
    ASSERT_EQ(properties[1].key, "value");
    ASSERT_EQ(std::get<int64_t>(properties[1].value), 42);
    
    // Update the node with new properties in tx2
    std::vector<storage::Property> updated_props = {
        {"name", std::string("UpdatedNode")},
        {"description", std::string("Updated description")}
    };
    
    auto update_res = graph_store_->create_node(tx2->id, updated_props);
    ASSERT_TRUE(update_res.has_value());
    storage::NodeId updated_node_id = update_res.value();
    
    // tx2 should see the updated properties
    auto tx2_read = graph_store_->get_node(tx2->id, updated_node_id);
    ASSERT_TRUE(tx2_read.has_value());
    
    auto [updated_record, updated_properties] = tx2_read.value();
    ASSERT_EQ(updated_properties.size(), 2);
    ASSERT_EQ(updated_properties[0].key, "name");
    ASSERT_EQ(std::get<std::string>(updated_properties[0].value), "UpdatedNode");
    
    // Commit tx2
    ASSERT_TRUE(txn_mgr_->commit_transaction(tx2));
    
    // A new transaction should see the committed changes
    auto tx3 = txn_mgr_->begin_transaction();
    auto tx3_read = graph_store_->get_node(tx3->id, updated_node_id);
    ASSERT_TRUE(tx3_read.has_value());
    
    auto [final_record, final_properties] = tx3_read.value();
    ASSERT_EQ(final_properties.size(), 2);
    ASSERT_EQ(final_properties[0].key, "name");
    ASSERT_EQ(std::get<std::string>(final_properties[0].value), "UpdatedNode");
} 