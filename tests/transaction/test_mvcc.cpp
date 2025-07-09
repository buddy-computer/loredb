#include <gtest/gtest.h>
#include "../../src/transaction/mvcc_manager.h"
#include "../../src/transaction/mvcc.h"

using namespace loredb::transaction;

class MVCCManagerTest : public ::testing::Test {
protected:
    void SetUp() override {
        txn_mgr_ = std::make_shared<TransactionManager>();
        mvcc_ = std::make_unique<MVCCManager>(txn_mgr_);
    }

    std::shared_ptr<TransactionManager> txn_mgr_;
    std::unique_ptr<MVCCManager> mvcc_;
};

TEST_F(MVCCManagerTest, ReadWriteVersionVisibility) {
    TransactionId tx1 = 1; // pretend transaction IDs
    TransactionId tx2 = 2;

    Version v1;
    v1.created_tx_id = tx1;
    v1.data = loredb::storage::NodeRecord{};
    v1.properties = {};

    // Write first version
    auto wres = mvcc_->write_version(42, v1);
    ASSERT_TRUE(wres.has_value());

    // tx1 should see its own version
    auto r1 = mvcc_->read_version(42, tx1);
    ASSERT_TRUE(r1.has_value());
    ASSERT_EQ(r1.value().created_tx_id, tx1);

    // tx2 should also see v1 (since not deleted)
    auto r2 = mvcc_->read_version(42, tx2);
    ASSERT_TRUE(r2.has_value());
    ASSERT_EQ(r2.value().created_tx_id, tx1);

    // Write second version with tx2
    Version v2;
    v2.created_tx_id = tx2;
    v2.data = loredb::storage::NodeRecord{};
    v2.properties = {};
    auto wres2 = mvcc_->write_version(42, v2);
    ASSERT_TRUE(wres2.has_value());

    // After update, tx1 should still see v1 (snapshot isolation)
    auto r1_after = mvcc_->read_version(42, tx1);
    ASSERT_TRUE(r1_after.has_value());
    ASSERT_EQ(r1_after.value().created_tx_id, tx1);

    // tx2 should now see its own version
    auto r2_after = mvcc_->read_version(42, tx2);
    ASSERT_TRUE(r2_after.has_value());
    ASSERT_EQ(r2_after.value().created_tx_id, tx2);
}

TEST_F(MVCCManagerTest, GarbageCollect) {
    TransactionId tx1 = 1;
    TransactionId tx2 = 2;
    Version v1{tx1, tx2, loredb::storage::NodeRecord{}, {}}; // deleted by tx2
    mvcc_->write_version(100, v1);

    // Add live version
    Version v2{tx2, 0, loredb::storage::NodeRecord{}, {}};
    mvcc_->write_version(100, v2);

    // GC with min_active_tx_id = 3 (both tx1 and tx2 finished)
    mvcc_->garbage_collect(3);

    // Old version should be removed; only one version left
    auto res = mvcc_->read_version(100, tx2);
    ASSERT_TRUE(res.has_value());
    ASSERT_EQ(res.value().created_tx_id, tx2);
} 