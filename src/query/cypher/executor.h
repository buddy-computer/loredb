#pragma once

#include "ast.h"
#include "parser.h"
#include "../executor.h"
#include "../../storage/graph_store.h"
#include "../../storage/simple_index_manager.h"
#include "../../transaction/mvcc.h"
#include "../../util/expected.h"
#include <memory>
#include <unordered_map>
#include <vector>

namespace graphdb::query::cypher {

// Variable binding during query execution
struct VariableBinding {
    enum class Type {
        NODE,
        EDGE,
        LITERAL
    };
    
    Type type;
    uint64_t id_value;  // For NODE and EDGE types
    PropertyValue literal_value;  // For LITERAL type
    
    // Default constructor
    VariableBinding() : type(Type::LITERAL), id_value(0), literal_value(std::string("")) {}
    
    // Constructor for NODE/EDGE types
    VariableBinding(Type t, uint64_t id) : type(t), id_value(id), literal_value(std::string("")) {
        if (t != Type::NODE && t != Type::EDGE) {
            throw std::invalid_argument("Invalid type for ID constructor");
        }
    }
    
    // Constructor for LITERAL type
    VariableBinding(Type t, PropertyValue value) : type(t), id_value(0), literal_value(std::move(value)) {
        if (t != Type::LITERAL) {
            throw std::invalid_argument("Invalid type for literal constructor");
        }
    }
};

using VariableMap = std::unordered_map<std::string, VariableBinding>;

// Execution context for a single query
struct ExecutionContext {
    std::shared_ptr<storage::GraphStore> graph_store;
    std::shared_ptr<storage::SimpleIndexManager> index_manager;
    transaction::TransactionId tx_id;
    VariableMap variables;
    
    ExecutionContext(std::shared_ptr<storage::GraphStore> gs, 
                     std::shared_ptr<storage::SimpleIndexManager> im,
                     transaction::TransactionId tx)
        : graph_store(std::move(gs)), index_manager(std::move(im)), tx_id(tx) {}
};

// Result row for intermediate processing
struct ResultRow {
    VariableMap bindings;
    
    ResultRow() = default;
    explicit ResultRow(VariableMap vars) : bindings(std::move(vars)) {}
};

using ResultSet = std::vector<ResultRow>;

// Cypher query executor
class CypherExecutor {
public:
    CypherExecutor(std::shared_ptr<storage::GraphStore> graph_store,
                   std::shared_ptr<storage::SimpleIndexManager> index_manager);
    ~CypherExecutor();
    
    // Execute a Cypher query string
    util::expected<QueryResult, storage::Error> execute_query(const std::string& cypher_query);
    
    // Execute a parsed query AST
    util::expected<QueryResult, storage::Error> execute_query(const Query& query);

private:
    std::shared_ptr<storage::GraphStore> graph_store_;
    std::shared_ptr<storage::SimpleIndexManager> index_manager_;
    CypherParser parser_;
    transaction::TransactionManager txn_manager_;
    
    // Query execution methods
    util::expected<ResultSet, storage::Error> execute_match(const MatchClause& match_clause, 
                                                           ExecutionContext& ctx);
    util::expected<ResultSet, storage::Error> apply_where(const WhereClause& where_clause, 
                                                         const ResultSet& input, 
                                                         ExecutionContext& ctx);
    util::expected<QueryResult, storage::Error> execute_return(const ReturnClause& return_clause, 
                                                              const ResultSet& input, 
                                                              ExecutionContext& ctx);
    util::expected<QueryResult, storage::Error> execute_create(const CreateClause& create_clause, 
                                                              ExecutionContext& ctx);
    
    // Pattern matching
    util::expected<ResultSet, storage::Error> match_pattern(const Pattern& pattern, 
                                                           ExecutionContext& ctx);
    util::expected<ResultSet, storage::Error> match_node(const Node& node, 
                                                        ExecutionContext& ctx);
    util::expected<ResultSet, storage::Error> match_node_edge_node_pattern(const Node& from_node,
                                                                          const Edge& edge,
                                                                          const Node& to_node,
                                                                          ExecutionContext& ctx);
    util::expected<std::vector<storage::NodeId>, storage::Error> find_nodes_by_pattern(const Node& node, 
                                                                                       ExecutionContext& ctx);
    util::expected<std::vector<storage::EdgeId>, storage::Error> find_edges_by_pattern(const Edge& edge,
                                                                                       storage::NodeId from_node,
                                                                                       storage::NodeId to_node,
                                                                                       ExecutionContext& ctx);
    
    // Expression evaluation
    util::expected<PropertyValue, storage::Error> evaluate_expression(const Expression& expr, 
                                                                     const VariableMap& variables, 
                                                                     ExecutionContext& ctx);
    util::expected<bool, storage::Error> evaluate_boolean_expression(const Expression& expr, 
                                                                    const VariableMap& variables, 
                                                                    ExecutionContext& ctx);
    
    // Helper methods
    QueryResult result_set_to_query_result(const ResultSet& result_set, 
                                          const std::vector<std::string>& columns);
    std::string property_value_to_string(const PropertyValue& value);
    std::string variable_binding_to_string(const VariableBinding& binding);
    
    // Property matching
    bool matches_property_constraints(const PropertyMap& constraints,
                                    const std::vector<storage::Property>& node_properties);
    bool matches_node_pattern(const Node& pattern, storage::NodeId node_id, ExecutionContext& ctx);
    bool matches_edge_pattern(const Edge& pattern, 
                             const storage::EdgeRecord& edge_record,
                             const std::vector<storage::Property>& edge_properties);
    
    // Node/edge creation
    util::expected<storage::NodeId, storage::Error> create_node_from_pattern(const Node& node,
                                                                            ExecutionContext& ctx);
    util::expected<storage::EdgeId, storage::Error> create_edge_from_pattern(const Edge& edge,
                                                                            storage::NodeId from_node,
                                                                            storage::NodeId to_node,
                                                                            ExecutionContext& ctx);
    
    // Result processing
    util::expected<QueryResult, storage::Error> apply_limit(const QueryResult& result, 
                                                           const LimitClause& limit);
    util::expected<QueryResult, storage::Error> apply_order_by(const QueryResult& result, 
                                                              const OrderByClause& order_by);
};

} // namespace graphdb::query::cypher