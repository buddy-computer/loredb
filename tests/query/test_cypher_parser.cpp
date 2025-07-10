#include <gtest/gtest.h>
#include "../../src/query/cypher/parser.h"
#include "../../src/query/cypher/executor.h"
#include "../../src/storage/file_page_store.h"
#include "../../src/storage/graph_store.h"
#include "../../src/storage/simple_index_manager.h"
#include "../../src/transaction/mvcc_manager.h"
#include "../../src/transaction/mvcc.h"
#include <filesystem>
#include <memory>

using namespace loredb::query::cypher;
using namespace loredb::transaction;
namespace storage = loredb::storage;

class CypherParserTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create temporary files
        std::string temp_dir = "/tmp";
        page_store_path_ = temp_dir + "/test_cypher_" + std::to_string(getpid()) + ".db";
        
        // Clean up any existing files
        std::filesystem::remove(page_store_path_);
        
        // Create components
        auto page_store = std::make_unique<storage::FilePageStore>(page_store_path_);
        txn_manager_ = std::make_shared<TransactionManager>();
        mvcc_manager_ = std::make_shared<MVCCManager>(txn_manager_);
        index_manager_ = std::make_shared<storage::SimpleIndexManager>();
        graph_store_ = std::make_unique<storage::GraphStore>(std::move(page_store), mvcc_manager_);
        
        executor_ = std::make_unique<CypherExecutor>(graph_store_, index_manager_, mvcc_manager_);
    }
    
    void TearDown() override {
        executor_.reset();
        graph_store_.reset();
        index_manager_.reset();
        mvcc_manager_.reset();
        txn_manager_.reset();
        if (page_store_) {
            page_store_->close();
            page_store_.reset();
        }
        std::filesystem::remove(page_store_path_);
    }
    
    std::string page_store_path_;
    std::shared_ptr<storage::FilePageStore> page_store_;
    std::shared_ptr<TransactionManager> txn_manager_;
    std::shared_ptr<MVCCManager> mvcc_manager_;
    std::shared_ptr<storage::SimpleIndexManager> index_manager_;
    std::shared_ptr<storage::GraphStore> graph_store_;
    std::unique_ptr<CypherExecutor> executor_;
};

TEST_F(CypherParserTest, ParseSimpleMatchQuery) {
    CypherParser parser;
    
    std::string query = "MATCH (n) RETURN n";
    auto result = parser.parse(query);
    
    ASSERT_TRUE(result.has_value()) << "Failed to parse query: " << result.error().message;
    
    auto& parsed_query = *result.value();
    EXPECT_TRUE(parsed_query.match.has_value());
    EXPECT_TRUE(parsed_query.return_clause.has_value());
    EXPECT_FALSE(parsed_query.where.has_value());
    EXPECT_FALSE(parsed_query.create.has_value());
}

TEST_F(CypherParserTest, ParseMatchWithProperties) {
    CypherParser parser;
    
    std::string query = "MATCH (n {name: \"Alice\"}) RETURN n.name";
    auto result = parser.parse(query);
    
    ASSERT_TRUE(result.has_value()) << "Failed to parse query: " << result.error().message;
    
    auto& parsed_query = *result.value();
    EXPECT_TRUE(parsed_query.match.has_value());
    EXPECT_TRUE(parsed_query.return_clause.has_value());
    
    // Check that the node has property constraints
    auto& match_clause = parsed_query.match.value();
    ASSERT_FALSE(match_clause.patterns.empty());
    auto& pattern = match_clause.patterns[0];
    ASSERT_FALSE(pattern.nodes.empty());
    auto& node = pattern.nodes[0];
    EXPECT_FALSE(node.properties.empty());
    EXPECT_EQ(node.properties.size(), 1);
    EXPECT_TRUE(node.properties.find("name") != node.properties.end());
}

TEST_F(CypherParserTest, ParseCreateQuery) {
    CypherParser parser;
    
    std::string query = "CREATE (n:Person {name: \"Bob\", age: 30})";
    auto result = parser.parse(query);
    
    ASSERT_TRUE(result.has_value()) << "Failed to parse query: " << result.error().message;
    
    auto& parsed_query = *result.value();
    EXPECT_TRUE(parsed_query.create.has_value());
    EXPECT_FALSE(parsed_query.match.has_value());
    EXPECT_FALSE(parsed_query.return_clause.has_value());
}

TEST_F(CypherParserTest, ExecuteSimpleNodeQuery) {
    // First create some test data using MVCC-aware creation
    std::vector<storage::Property> properties = {
        storage::Property("name", storage::PropertyValue(std::string("Alice"))),
        storage::Property("age", storage::PropertyValue(int64_t(25)))
    };
    
    // Create the node within a transaction so MVCC queries can see it
    auto tx = txn_manager_->begin_transaction();
    auto node_result = graph_store_->create_node(tx->id, properties);
    ASSERT_TRUE(node_result.has_value()) << "Failed to create node: " << node_result.error().message;
    txn_manager_->commit_transaction(tx);
    
    // Execute a query to find the node
    std::string query = "MATCH (n) RETURN n";
    auto query_result = executor_->execute_query(query);
    
    ASSERT_TRUE(query_result.has_value()) << "Query failed: " << query_result.error().message;
    
    auto& result = query_result.value();
    EXPECT_FALSE(result.rows.empty());
    EXPECT_EQ(result.columns.size(), 1);
}

TEST_F(CypherParserTest, ExecuteNodeWithPropertiesQuery) {
    // Create test data using MVCC-aware transactions
    std::vector<storage::Property> alice_props = {
        storage::Property("name", storage::PropertyValue(std::string("Alice"))),
        storage::Property("age", storage::PropertyValue(int64_t(25)))
    };
    std::vector<storage::Property> bob_props = {
        storage::Property("name", storage::PropertyValue(std::string("Bob"))),
        storage::Property("age", storage::PropertyValue(int64_t(30)))
    };
    
    auto tx = txn_manager_->begin_transaction();
    auto alice_result = graph_store_->create_node(tx->id, alice_props);
    auto bob_result = graph_store_->create_node(tx->id, bob_props);
    ASSERT_TRUE(alice_result.has_value());
    ASSERT_TRUE(bob_result.has_value());
    txn_manager_->commit_transaction(tx);
    
    // Query for Alice specifically
    std::string query = "MATCH (n {name: \"Alice\"}) RETURN n";
    auto query_result = executor_->execute_query(query);
    
    ASSERT_TRUE(query_result.has_value()) << "Query failed: " << query_result.error().message;
    
    auto& result = query_result.value();
    EXPECT_EQ(result.rows.size(), 1); // Should only find Alice
}

TEST_F(CypherParserTest, ExecuteEdgePatternQuery) {
    // Create test data: Alice knows Bob using MVCC-aware transactions
    std::vector<storage::Property> alice_props = {
        storage::Property("name", storage::PropertyValue(std::string("Alice")))
    };
    std::vector<storage::Property> bob_props = {
        storage::Property("name", storage::PropertyValue(std::string("Bob")))
    };
    std::vector<storage::Property> edge_props = {
        storage::Property("type", storage::PropertyValue(std::string("KNOWS"))),
        storage::Property("since", storage::PropertyValue(int64_t(2020)))
    };
    
    auto tx = txn_manager_->begin_transaction();
    auto alice_result = graph_store_->create_node(tx->id, alice_props);
    auto bob_result = graph_store_->create_node(tx->id, bob_props);
    ASSERT_TRUE(alice_result.has_value());
    ASSERT_TRUE(bob_result.has_value());
    
    auto alice_id = alice_result.value();
    auto bob_id = bob_result.value();
    
    auto edge_result = graph_store_->create_edge(tx->id, alice_id, bob_id, "KNOWS", edge_props);
    ASSERT_TRUE(edge_result.has_value()) << "Failed to create edge: " << edge_result.error().message;
    txn_manager_->commit_transaction(tx);
    
    // Query for the relationship pattern
    std::string query = "MATCH (a)-[r]->(b) RETURN a, r, b";
    auto query_result = executor_->execute_query(query);
    
    ASSERT_TRUE(query_result.has_value()) << "Query failed: " << query_result.error().message;
    
    auto& result = query_result.value();
    EXPECT_EQ(result.rows.size(), 1); // Should find one relationship
    EXPECT_EQ(result.columns.size(), 3); // a, r, b
}

TEST_F(CypherParserTest, ExecuteWhereClause) {
    // Create test data using MVCC transactions
    std::vector<storage::Property> alice_props = {
        storage::Property("name", storage::PropertyValue(std::string("Alice"))),
        storage::Property("age", storage::PropertyValue(int64_t(25)))
    };
    std::vector<storage::Property> bob_props = {
        storage::Property("name", storage::PropertyValue(std::string("Bob"))),
        storage::Property("age", storage::PropertyValue(int64_t(30)))
    };
    
    auto tx = txn_manager_->begin_transaction();
    auto alice_result = graph_store_->create_node(tx->id, alice_props);
    auto bob_result = graph_store_->create_node(tx->id, bob_props);
    ASSERT_TRUE(alice_result.has_value());
    ASSERT_TRUE(bob_result.has_value());
    txn_manager_->commit_transaction(tx);
    
    // Query with WHERE clause
    std::string query = "MATCH (n) WHERE n.age > 27 RETURN n.name";
    auto query_result = executor_->execute_query(query);
    
    ASSERT_TRUE(query_result.has_value()) << "Query failed: " << query_result.error().message;
    
    auto& result = query_result.value();
    EXPECT_EQ(result.rows.size(), 1); // Should only find Bob (age 30 > 27)
}

TEST_F(CypherParserTest, ExecuteCreateQuery) {
    std::string query = "CREATE (n:Person {name: \"Charlie\", age: 35})";
    auto query_result = executor_->execute_query(query);
    
    ASSERT_TRUE(query_result.has_value()) << "Query failed: " << query_result.error().message;
    
    auto& result = query_result.value();
    EXPECT_EQ(result.columns.size(), 2); // created_nodes, created_edges
    EXPECT_EQ(result.rows.size(), 1);
    
    // Verify the node was actually created
    std::string verify_query = "MATCH (n {name: \"Charlie\"}) RETURN n.name, n.age";
    auto verify_result = executor_->execute_query(verify_query);
    
    ASSERT_TRUE(verify_result.has_value());
    auto& verify_data = verify_result.value();
    EXPECT_EQ(verify_data.rows.size(), 1);
}

TEST_F(CypherParserTest, ParseErrorHandling) {
    CypherParser parser;
    
    // Test invalid syntax
    std::string invalid_query = "MATCH (n RETURN n"; // Missing closing parenthesis
    auto result = parser.parse(invalid_query);
    
    EXPECT_FALSE(result.has_value()); // Should fail to parse
}

TEST_F(CypherParserTest, ExecuteComplexEdgePattern) {
    // Create a more complex graph: Alice knows Bob, Bob knows Charlie using MVCC transactions
    std::vector<storage::Property> alice_props = {storage::Property("name", storage::PropertyValue(std::string("Alice")))};
    std::vector<storage::Property> bob_props = {storage::Property("name", storage::PropertyValue(std::string("Bob")))};
    std::vector<storage::Property> charlie_props = {storage::Property("name", storage::PropertyValue(std::string("Charlie")))};
    
    auto tx = txn_manager_->begin_transaction();
    auto alice_id = graph_store_->create_node(tx->id, alice_props).value();
    auto bob_id = graph_store_->create_node(tx->id, bob_props).value();
    auto charlie_id = graph_store_->create_node(tx->id, charlie_props).value();
    
    std::vector<storage::Property> knows_props = {storage::Property("type", storage::PropertyValue(std::string("KNOWS")))};
    
    graph_store_->create_edge(tx->id, alice_id, bob_id, "KNOWS", knows_props);
    graph_store_->create_edge(tx->id, bob_id, charlie_id, "KNOWS", knows_props);
    txn_manager_->commit_transaction(tx);
    
    // Query for specific edge pattern with property constraints
    std::string query = "MATCH (a {name: \"Alice\"})-[r]->(b) RETURN a.name, b.name";
    auto query_result = executor_->execute_query(query);
    
    ASSERT_TRUE(query_result.has_value()) << "Query failed: " << query_result.error().message;
    
    auto& result = query_result.value();
    EXPECT_EQ(result.rows.size(), 1); // Alice -> Bob
    EXPECT_EQ(result.columns.size(), 2); // a.name, b.name
}

TEST_F(CypherParserTest, ExecuteMultiHopQuery) {
    // Create a chain: Alice -> Bob -> Charlie using MVCC transactions
    std::vector<storage::Property> alice_props = {storage::Property("name", storage::PropertyValue(std::string("Alice")))};
    std::vector<storage::Property> bob_props = {storage::Property("name", storage::PropertyValue(std::string("Bob")))};
    std::vector<storage::Property> charlie_props = {storage::Property("name", storage::PropertyValue(std::string("Charlie")))};
    
    auto tx = txn_manager_->begin_transaction();
    auto alice_id = graph_store_->create_node(tx->id, alice_props).value();
    auto bob_id = graph_store_->create_node(tx->id, bob_props).value();
    auto charlie_id = graph_store_->create_node(tx->id, charlie_props).value();
    
    std::vector<storage::Property> knows_props = {storage::Property("type", storage::PropertyValue(std::string("KNOWS")))};
    
    graph_store_->create_edge(tx->id, alice_id, bob_id, "KNOWS", knows_props);
    graph_store_->create_edge(tx->id, bob_id, charlie_id, "KNOWS", knows_props);
    txn_manager_->commit_transaction(tx);

    // Query for the 2-hop path
    std::string query = "MATCH (a {name: \"Alice\"})-[r1]->(b)-[r2]->(c) RETURN a.name, b.name, c.name";
    auto query_result = executor_->execute_query(query);

    ASSERT_TRUE(query_result.has_value()) << "Query failed: " << query_result.error().message;

    auto& result = query_result.value();
    EXPECT_EQ(result.rows.size(), 1);
    EXPECT_EQ(result.columns.size(), 3);
    if (!result.rows.empty()) {
        EXPECT_EQ(result.rows[0][0], "Alice");
        EXPECT_EQ(result.rows[0][1], "Bob");
        EXPECT_EQ(result.rows[0][2], "Charlie");
    }
}

TEST_F(CypherParserTest, ExecuteUndirectedQuery) {
    // Create a directed edge: Alice -> Bob using MVCC transactions
    std::vector<storage::Property> alice_props = {storage::Property("name", storage::PropertyValue(std::string("Alice")))};
    std::vector<storage::Property> bob_props = {storage::Property("name", storage::PropertyValue(std::string("Bob")))};
    
    auto tx = txn_manager_->begin_transaction();
    auto alice_id = graph_store_->create_node(tx->id, alice_props).value();
    auto bob_id = graph_store_->create_node(tx->id, bob_props).value();
    
    std::vector<storage::Property> knows_props = {storage::Property("type", storage::PropertyValue(std::string("KNOWS")))};
    graph_store_->create_edge(tx->id, alice_id, bob_id, "KNOWS", knows_props);
    txn_manager_->commit_transaction(tx);

    // Query with an undirected edge, it should find the directed edge
    std::string query = "MATCH (a {name: \"Alice\"})--(b) RETURN a.name, b.name";
    auto query_result = executor_->execute_query(query);

    ASSERT_TRUE(query_result.has_value()) << "Query failed: " << query_result.error().message;

    auto& result = query_result.value();
    EXPECT_EQ(result.rows.size(), 1);
    if (!result.rows.empty()) {
        EXPECT_EQ(result.rows[0][0], "Alice");
        EXPECT_EQ(result.rows[0][1], "Bob");
    }
}

TEST_F(CypherParserTest, ExecuteLimitQuery) {
    // Create some nodes using MVCC transactions
    auto tx = txn_manager_->begin_transaction();
    graph_store_->create_node(tx->id, {});
    graph_store_->create_node(tx->id, {});
    graph_store_->create_node(tx->id, {});
    txn_manager_->commit_transaction(tx);

    // Query with LIMIT
    std::string query = "MATCH (n) RETURN n LIMIT 2";
    auto query_result = executor_->execute_query(query);

    ASSERT_TRUE(query_result.has_value()) << "Query failed: " << query_result.error().message;

    auto& result = query_result.value();
    EXPECT_EQ(result.rows.size(), 2);
}

TEST_F(CypherParserTest, ExecuteOrderByQuery) {
    // Create some nodes with names for ordering using MVCC transactions
    auto tx = txn_manager_->begin_transaction();
    graph_store_->create_node(tx->id, {storage::Property("name", storage::PropertyValue(std::string("Charlie")))});
    graph_store_->create_node(tx->id, {storage::Property("name", storage::PropertyValue(std::string("Alice")))});
    graph_store_->create_node(tx->id, {storage::Property("name", storage::PropertyValue(std::string("Bob")))});
    txn_manager_->commit_transaction(tx);

    // Query with ORDER BY
    std::string query = "MATCH (n) RETURN n.name ORDER BY n.name";
    auto query_result = executor_->execute_query(query);

    ASSERT_TRUE(query_result.has_value()) << "Query failed: " << query_result.error().message;

    auto& result = query_result.value();
    EXPECT_EQ(result.rows.size(), 3);
    if (result.rows.size() == 3) {
        EXPECT_EQ(result.rows[0][0], "Alice");
        EXPECT_EQ(result.rows[1][0], "Bob");
        EXPECT_EQ(result.rows[2][0], "Charlie");
    }
}

TEST_F(CypherParserTest, ParseMatchWithLabels) {
    CypherParser parser;
    std::string query = "MATCH (n:Person) RETURN n";
    auto result = parser.parse(query);
    ASSERT_TRUE(result.has_value());
    auto& parsed_query = *result.value();
    ASSERT_TRUE(parsed_query.match.has_value());
    auto& pattern = parsed_query.match->patterns[0];
    ASSERT_EQ(pattern.nodes.size(), 1);
    EXPECT_EQ(pattern.nodes[0].labels.size(), 1);
    EXPECT_EQ(pattern.nodes[0].labels[0], "Person");
}

TEST_F(CypherParserTest, ParseMatchWithMultipleLabels) {
    CypherParser parser;
    std::string query = "MATCH (n:Person:Admin) RETURN n";
    auto result = parser.parse(query);
    ASSERT_TRUE(result.has_value());
    auto& parsed_query = *result.value();
    ASSERT_TRUE(parsed_query.match.has_value());
    auto& pattern = parsed_query.match->patterns[0];
    ASSERT_EQ(pattern.nodes.size(), 1);
    EXPECT_EQ(pattern.nodes[0].labels.size(), 2);
    EXPECT_EQ(pattern.nodes[0].labels[0], "Person");
    EXPECT_EQ(pattern.nodes[0].labels[1], "Admin");
}

TEST_F(CypherParserTest, ParseMatchWithEdgeType) {
    CypherParser parser;
    std::string query = "MATCH ()-[r:KNOWS]->() RETURN r";
    auto result = parser.parse(query);
    ASSERT_TRUE(result.has_value());
    auto& parsed_query = *result.value();
    ASSERT_TRUE(parsed_query.match.has_value());
    auto& pattern = parsed_query.match->patterns[0];
    ASSERT_EQ(pattern.edges.size(), 1);
    EXPECT_EQ(pattern.edges[0].types.size(), 1);
    EXPECT_EQ(pattern.edges[0].types[0], "KNOWS");
}

TEST_F(CypherParserTest, ParseMatchWithMultipleProperties) {
    CypherParser parser;
    std::string query = "MATCH (n {name: \"Alice\", age: 30}) RETURN n";
    auto result = parser.parse(query);
    ASSERT_TRUE(result.has_value());
    auto& parsed_query = *result.value();
    ASSERT_TRUE(parsed_query.match.has_value());
    auto& pattern = parsed_query.match->patterns[0];
    ASSERT_EQ(pattern.nodes.size(), 1);
    auto& node = pattern.nodes[0];
    EXPECT_EQ(node.properties.size(), 2);
    EXPECT_EQ(std::get<std::string>(node.properties["name"]), "Alice");
    EXPECT_EQ(std::get<int64_t>(node.properties["age"]), 30);
}

TEST_F(CypherParserTest, ParseErrorHandlingInvalidKeyword) {
    CypherParser parser;
    std::string invalid_query = "FETCH (n) RETURN n";
    auto result = parser.parse(invalid_query);
    EXPECT_FALSE(result.has_value());
}

TEST_F(CypherParserTest, ExecuteVariableLengthQuery) {
    // Create a chain: Alice -> Bob -> Charlie -> David using MVCC transactions
    auto tx = txn_manager_->begin_transaction();
    auto alice_id = graph_store_->create_node(tx->id, {storage::Property("name", storage::PropertyValue(std::string("Alice")))}) .value();
    auto bob_id = graph_store_->create_node(tx->id, {storage::Property("name", storage::PropertyValue(std::string("Bob")))}) .value();
    auto charlie_id = graph_store_->create_node(tx->id, {storage::Property("name", storage::PropertyValue(std::string("Charlie")))}) .value();
    auto david_id = graph_store_->create_node(tx->id, {storage::Property("name", storage::PropertyValue(std::string("David")))}) .value();
    
    graph_store_->create_edge(tx->id, alice_id, bob_id, "KNOWS", {});
    graph_store_->create_edge(tx->id, bob_id, charlie_id, "KNOWS", {});
    graph_store_->create_edge(tx->id, charlie_id, david_id, "KNOWS", {});
    txn_manager_->commit_transaction(tx);

    // Query for paths of length 1 to 3
    std::string query = "MATCH (a {name: \"Alice\"})-[*1..3]->(b) RETURN b.name";
    auto query_result = executor_->execute_query(query);

    ASSERT_TRUE(query_result.has_value()) << "Query failed: " << query_result.error().message;

    auto& result = query_result.value();
    EXPECT_EQ(result.rows.size(), 3);

    std::vector<std::string> names;
    for(const auto& row : result.rows) {
        names.push_back(row[0]);
    }
    std::sort(names.begin(), names.end());
    
    ASSERT_EQ(names.size(), 3);
    EXPECT_EQ(names[0], "Bob");
    EXPECT_EQ(names[1], "Charlie");
    EXPECT_EQ(names[2], "David");
}

TEST_F(CypherParserTest, ExecuteSetClause) {
    // Create a node first using MVCC transactions
    std::vector<storage::Property> alice_props = {
        storage::Property("name", storage::PropertyValue(std::string("Alice"))),
        storage::Property("age", storage::PropertyValue(int64_t(30)))
    };
    
    auto tx = txn_manager_->begin_transaction();
    auto alice_result = graph_store_->create_node(tx->id, alice_props);
    ASSERT_TRUE(alice_result.has_value());
    txn_manager_->commit_transaction(tx);
    
    // Test SET query
    std::string query_str = "MATCH (n) WHERE n.name = \"Alice\" SET n.age = 31";
    
    auto exec_result = executor_->execute_query(query_str);
    ASSERT_TRUE(exec_result.has_value()) << "Execution failed: " << exec_result.error().message;
    
    // Verify the age was updated
    std::string verify_query = "MATCH (n) RETURN n.name, n.age";
    auto verify_exec = executor_->execute_query(verify_query);
    ASSERT_TRUE(verify_exec.has_value());
    
    auto result = verify_exec.value();
    ASSERT_EQ(result.rows.size(), 1);
    ASSERT_EQ(result.rows[0].size(), 2);
    EXPECT_EQ(result.rows[0][0], "Alice");
    EXPECT_EQ(result.rows[0][1], "31");  // Should be updated to 31
}

TEST_F(CypherParserTest, ExecuteDeleteClause) {
    // Create some nodes first using MVCC transactions
    std::vector<storage::Property> alice_props = {
        storage::Property("name", storage::PropertyValue(std::string("Alice"))),
        storage::Property("age", storage::PropertyValue(int64_t(30)))
    };
    std::vector<storage::Property> bob_props = {
        storage::Property("name", storage::PropertyValue(std::string("Bob"))),
        storage::Property("age", storage::PropertyValue(int64_t(25)))
    };
    
    auto tx = txn_manager_->begin_transaction();
    auto alice_result = graph_store_->create_node(tx->id, alice_props);
    auto bob_result = graph_store_->create_node(tx->id, bob_props);
    ASSERT_TRUE(alice_result.has_value());
    ASSERT_TRUE(bob_result.has_value());
    txn_manager_->commit_transaction(tx);
    
    // Test DELETE query
    std::string query_str = "MATCH (n) WHERE n.name = \"Alice\" DELETE n";
    auto exec_result = executor_->execute_query(query_str);
    ASSERT_TRUE(exec_result.has_value()) << "Execution failed: " << exec_result.error().message;
    
    // Verify Alice was deleted but Bob remains
    std::string verify_query = "MATCH (n) RETURN n.name";
    auto verify_exec = executor_->execute_query(verify_query);
    ASSERT_TRUE(verify_exec.has_value());
    
    auto result = verify_exec.value();
    ASSERT_EQ(result.rows.size(), 1);  // Only Bob should remain
    EXPECT_EQ(result.rows[0][0], "Bob");
}