#pragma once

#include "ast.h"
#include "parser.h"
#include "../query_types.h"
#include "../../storage/graph_store.h"
#include "../../storage/simple_index_manager.h"
#include "../../transaction/mvcc.h"
#include "../../util/expected.h"
#include <memory>
#include <unordered_map>
#include <vector>

namespace loredb::query::cypher {

// Cypher query executor
class CypherExecutor {
public:
    CypherExecutor(std::shared_ptr<storage::GraphStore> graph_store,
                   std::shared_ptr<storage::SimpleIndexManager> index_manager,
                   std::shared_ptr<transaction::MVCCManager> mvcc_manager);
    ~CypherExecutor();
    
    // Execute a Cypher query string
    util::expected<QueryResult, storage::Error> execute_query(const std::string& cypher_query);
    
    // Execute a parsed query AST
    util::expected<QueryResult, storage::Error> execute_query(const Query& query);

private:
    std::shared_ptr<storage::GraphStore> graph_store_;
    std::shared_ptr<storage::SimpleIndexManager> index_manager_;
    std::shared_ptr<transaction::MVCCManager> mvcc_manager_;
    CypherParser parser_;
    
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
    util::expected<ResultSet, storage::Error> match_variable_length_path(const Node& from_node,
                                                                         const Edge& edge,
                                                                         const Node& to_node,
                                                                         ExecutionContext& ctx);
    util::expected<ResultSet, storage::Error> expand_results(const ResultSet& previous_results,
                                                             const Node& from_node_pattern,
                                                             const Edge& edge_pattern,
                                                             const Node& to_node_pattern,
                                                             ExecutionContext& ctx);
    util::expected<std::vector<storage::NodeId>, storage::Error> find_nodes_by_pattern(const Node& node, 
                                                                                       ExecutionContext& ctx);
    util::expected<std::vector<storage::EdgeId>, storage::Error> find_edges_by_pattern(const Edge& edge,
                                                                                       storage::NodeId from_node,
                                                                                       storage::NodeId to_node,
                                                                                       ExecutionContext& ctx);
    
    // Helper methods
    QueryResult result_set_to_query_result(const ResultSet& result_set, 
                                          const std::vector<std::string>& columns);
    
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
                                                              const OrderByClause& /*order_by*/);
};

}