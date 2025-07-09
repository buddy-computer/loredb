#include <gtest/gtest.h>
#include "../../src/storage/wal_manager.h"
#include "../../src/storage/record.h"
#include <unistd.h>
#include <filesystem>

using namespace graphdb::storage;
using namespace graphdb::transaction;

class WALManagerTest : public ::testing::Test {
protected:
    void SetUp() override {
        wal_file_ = "/tmp/test_wal_" + std::to_string(getpid()) + ".log";
        wal_manager_ = std::make_unique<WALManager>(wal_file_);
    }

    void TearDown() override {
        wal_manager_.reset();
        std::filesystem::remove(wal_file_);
    }

    std::string wal_file_;
    std::unique_ptr<WALManager> wal_manager_;
};

TEST_F(WALManagerTest, BasicTransactionLifecycle) {
    TransactionId tx_id = 42;
    
    // Log transaction begin
    auto begin_lsn = wal_manager_->log_begin_transaction(tx_id);
    ASSERT_TRUE(begin_lsn.has_value());
    EXPECT_EQ(begin_lsn.value(), 1);
    
    // Log some operations
    std::vector<Property> props = {
        {"name", std::string("test_node")},
        {"value", int64_t(123)}
    };
    
    auto create_lsn = wal_manager_->log_create_node(tx_id, 100, props);
    ASSERT_TRUE(create_lsn.has_value());
    EXPECT_EQ(create_lsn.value(), 2);
    
    auto edge_lsn = wal_manager_->log_create_edge(tx_id, 200, 100, 101, "connects", props);
    ASSERT_TRUE(edge_lsn.has_value());
    EXPECT_EQ(edge_lsn.value(), 3);
    
    // Log transaction commit
    auto commit_lsn = wal_manager_->log_commit_transaction(tx_id);
    ASSERT_TRUE(commit_lsn.has_value());
    EXPECT_EQ(commit_lsn.value(), 4);
    
    // Check current LSN
    EXPECT_EQ(wal_manager_->get_current_lsn(), 5);
}

TEST_F(WALManagerTest, Checkpointing) {
    TransactionId tx_id = 1;
    
    // Log some operations
    wal_manager_->log_begin_transaction(tx_id);
    wal_manager_->log_create_node(tx_id, 1, {});
    wal_manager_->log_commit_transaction(tx_id);
    
    // Perform checkpoint
    auto checkpoint_lsn = wal_manager_->checkpoint();
    ASSERT_TRUE(checkpoint_lsn.has_value());
    
    EXPECT_EQ(wal_manager_->get_last_checkpoint_lsn(), checkpoint_lsn.value());
    EXPECT_GT(checkpoint_lsn.value(), 3);
}

TEST_F(WALManagerTest, Recovery) {
    TransactionId tx_id = 123;
    LSN expected_max_lsn = 0;
    
    // Write some records
    auto lsn1 = wal_manager_->log_begin_transaction(tx_id);
    ASSERT_TRUE(lsn1.has_value());
    expected_max_lsn = lsn1.value();
    
    std::vector<Property> props = {{"key", std::string("value")}};
    auto lsn2 = wal_manager_->log_create_node(tx_id, 42, props);
    ASSERT_TRUE(lsn2.has_value());
    expected_max_lsn = lsn2.value();
    
    auto lsn3 = wal_manager_->log_commit_transaction(tx_id);
    ASSERT_TRUE(lsn3.has_value());
    expected_max_lsn = lsn3.value();
    
    // Force sync
    wal_manager_->force_sync();
    
    // Close and reopen WAL to simulate recovery
    wal_manager_.reset();
    wal_manager_ = std::make_unique<WALManager>(wal_file_);
    
    // The WAL should have recovered the LSN state
    EXPECT_EQ(wal_manager_->get_current_lsn(), expected_max_lsn + 1);
    
    // Test recovery callback
    std::vector<WALRecord> recovered_records;
    auto recovery_result = wal_manager_->recover_from_log([&](const WALRecord& record) -> graphdb::util::expected<void, Error> {
        recovered_records.push_back(record);
        return {};
    });
    
    ASSERT_TRUE(recovery_result.has_value());
    EXPECT_EQ(recovered_records.size(), 3); // BEGIN, CREATE_NODE, COMMIT
    
    // Verify record types
    EXPECT_EQ(recovered_records[0].type, WALRecordType::BEGIN_TRANSACTION);
    EXPECT_EQ(recovered_records[0].tx_id, tx_id);
    
    EXPECT_EQ(recovered_records[1].type, WALRecordType::CREATE_NODE);
    EXPECT_EQ(recovered_records[1].tx_id, tx_id);
    
    EXPECT_EQ(recovered_records[2].type, WALRecordType::COMMIT_TRANSACTION);
    EXPECT_EQ(recovered_records[2].tx_id, tx_id);
}

TEST_F(WALManagerTest, BackwardCompatibility) {
    // Test the old log_operation interface still works
    OperationLog old_style_log;
    old_style_log.json_line = R"({"op":"test","data":"backward_compat"})";
    
    auto result = wal_manager_->log_operation(old_style_log);
    EXPECT_TRUE(result.has_value());
}

TEST_F(WALManagerTest, AllOperationTypes) {
    TransactionId tx_id = 999;
    
    // Test all operation types
    EXPECT_TRUE(wal_manager_->log_begin_transaction(tx_id).has_value());
    
    std::vector<Property> props = {{"test", std::string("data")}};
    EXPECT_TRUE(wal_manager_->log_create_node(tx_id, 1, props).has_value());
    EXPECT_TRUE(wal_manager_->log_update_node(tx_id, 1, props).has_value());
    EXPECT_TRUE(wal_manager_->log_delete_node(tx_id, 1).has_value());
    
    EXPECT_TRUE(wal_manager_->log_create_edge(tx_id, 1, 10, 20, "test_edge", props).has_value());
    EXPECT_TRUE(wal_manager_->log_update_edge(tx_id, 1, props).has_value());
    EXPECT_TRUE(wal_manager_->log_delete_edge(tx_id, 1).has_value());
    
    EXPECT_TRUE(wal_manager_->log_commit_transaction(tx_id).has_value());
    
    // Should have 8 operations
    EXPECT_EQ(wal_manager_->get_current_lsn(), 9); // Next LSN to assign
}

TEST_F(WALManagerTest, ForceSync) {
    // Test that force_sync doesn't fail
    auto result = wal_manager_->force_sync();
    EXPECT_TRUE(result.has_value());
}