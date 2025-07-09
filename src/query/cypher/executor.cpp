#include "executor.h"
#include "../../util/logger.h"
#include <algorithm>
#include <sstream>

namespace graphdb::query::cypher {

CypherExecutor::CypherExecutor(std::shared_ptr<storage::GraphStore> graph_store,
                               std::shared_ptr<storage::SimpleIndexManager> index_manager)
    : graph_store_(std::move(graph_store)), index_manager_(std::move(index_manager)) {
}

CypherExecutor::~CypherExecutor() = default;

util::expected<QueryResult, storage::Error> CypherExecutor::execute_query(const std::string& cypher_query) {
    LOG_INFO("Executing Cypher query: {}", cypher_query);
    
    // Parse the query
    auto parse_result = parser_.parse(cypher_query);
    if (!parse_result.has_value()) {
        return util::unexpected(parse_result.error());
    }
    
    return execute_query(*parse_result.value());
}

util::expected<QueryResult, storage::Error> CypherExecutor::execute_query(const Query& query) {
    // Start transaction
    auto tx = txn_manager_.begin_transaction();
    ExecutionContext ctx(graph_store_, index_manager_, tx->id);
    
    try {
        // Handle read queries (MATCH ... RETURN)
        if (query.is_read_query()) {
            ResultSet result_set;
            
            // Execute MATCH clause
            if (query.match.has_value()) {
                auto match_result = execute_match(query.match.value(), ctx);
                if (!match_result.has_value()) {
                    txn_manager_.abort_transaction(tx);
                    return util::unexpected(match_result.error());
                }
                result_set = std::move(match_result.value());
            }
            
            // Apply WHERE clause
            if (query.where.has_value()) {
                auto where_result = apply_where(query.where.value(), result_set, ctx);
                if (!where_result.has_value()) {
                    txn_manager_.abort_transaction(tx);
                    return util::unexpected(where_result.error());
                }
                result_set = std::move(where_result.value());
            }
            
            // Execute RETURN clause
            if (query.return_clause.has_value()) {
                auto return_result = execute_return(query.return_clause.value(), result_set, ctx);
                if (!return_result.has_value()) {
                    txn_manager_.abort_transaction(tx);
                    return util::unexpected(return_result.error());
                }
                
                auto final_result = return_result.value();
                
                // Apply ORDER BY if present
                if (query.order_by.has_value()) {
                    auto order_result = apply_order_by(final_result, query.order_by.value());
                    if (!order_result.has_value()) {
                        txn_manager_.abort_transaction(tx);
                        return util::unexpected(order_result.error());
                    }
                    final_result = std::move(order_result.value());
                }
                
                // Apply LIMIT if present
                if (query.limit.has_value()) {
                    auto limit_result = apply_limit(final_result, query.limit.value());
                    if (!limit_result.has_value()) {
                        txn_manager_.abort_transaction(tx);
                        return util::unexpected(limit_result.error());
                    }
                    final_result = std::move(limit_result.value());
                }
                
                txn_manager_.commit_transaction(tx);
                return final_result;
            }
        }
        
        // Handle write queries (CREATE)
        if (query.create.has_value()) {
            auto create_result = execute_create(query.create.value(), ctx);
            if (!create_result.has_value()) {
                txn_manager_.abort_transaction(tx);
                return util::unexpected(create_result.error());
            }
            
            txn_manager_.commit_transaction(tx);
            return create_result.value();
        }
        
        // If we get here, the query was not recognized
        txn_manager_.abort_transaction(tx);
        return util::unexpected(storage::Error{
            storage::ErrorCode::INVALID_ARGUMENT, 
            "Unsupported query type"
        });
        
    } catch (const std::exception& e) {
        txn_manager_.abort_transaction(tx);
        return util::unexpected(storage::Error{
            storage::ErrorCode::INVALID_ARGUMENT, 
            "Query execution error: " + std::string(e.what())
        });
    }
}

util::expected<ResultSet, storage::Error> CypherExecutor::execute_match(const MatchClause& match_clause, 
                                                                       ExecutionContext& ctx) {
    ResultSet result_set;
    
    // For now, handle single pattern matching
    if (match_clause.patterns.empty()) {
        return result_set; // Empty result
    }
    
    // Start with the first pattern
    auto pattern_result = match_pattern(match_clause.patterns[0], ctx);
    if (!pattern_result.has_value()) {
        return util::unexpected(pattern_result.error());
    }
    
    result_set = std::move(pattern_result.value());
    
    // TODO: Handle multiple patterns with joins
    
    return result_set;
}

util::expected<ResultSet, storage::Error> CypherExecutor::match_pattern(const Pattern& pattern, 
                                                                       ExecutionContext& ctx) {
    ResultSet result_set;
    
    if (pattern.nodes.empty()) {
        return result_set;
    }
    
    // Handle simple node patterns
    if (pattern.nodes.size() == 1 && pattern.edges.empty()) {
        return match_node(pattern.nodes[0], ctx);
    }
    
    // Handle node-edge-node patterns: (a)-[r]->(b)
    if (pattern.nodes.size() == 2 && pattern.edges.size() == 1) {
        return match_node_edge_node_pattern(pattern.nodes[0], pattern.edges[0], pattern.nodes[1], ctx);
    }
    
    // TODO: Handle longer path patterns and variable-length paths
    return util::unexpected(storage::Error{
        storage::ErrorCode::INVALID_ARGUMENT, 
        "Complex multi-hop pattern matching not yet implemented"
    });
}

util::expected<ResultSet, storage::Error> CypherExecutor::match_node(const Node& node, 
                                                                    ExecutionContext& ctx) {
    auto node_ids_result = find_nodes_by_pattern(node, ctx);
    if (!node_ids_result.has_value()) {
        return util::unexpected(node_ids_result.error());
    }
    
    ResultSet result_set;
    for (auto node_id : node_ids_result.value()) {
        ResultRow row;
        if (node.variable.has_value()) {
            row.bindings[node.variable.value()] = VariableBinding(
                VariableBinding::Type::NODE, 
                node_id
            );
        }
        result_set.push_back(std::move(row));
    }
    
    return result_set;
}

util::expected<std::vector<storage::NodeId>, storage::Error> CypherExecutor::find_nodes_by_pattern(const Node& node, 
                                                                                                   ExecutionContext& ctx) {
    std::vector<storage::NodeId> result;
    
    // If there are property constraints, use them for filtering
    if (!node.properties.empty()) {
        // For now, do a simple scan - in a real implementation, we'd use indexes
        for (storage::NodeId node_id = 1; node_id <= 1000; ++node_id) {
            if (ctx.graph_store->has_mvcc()) {
                auto node_result = ctx.graph_store->get_node(ctx.tx_id, node_id);
                if (node_result.has_value()) {
                    auto [node_record, properties] = node_result.value();
                    if (matches_property_constraints(node.properties, properties)) {
                        result.push_back(node_id);
                    }
                }
            } else {
                auto node_result = ctx.graph_store->get_node(node_id);
                if (node_result.has_value()) {
                    auto [node_record, properties] = node_result.value();
                    if (matches_property_constraints(node.properties, properties)) {
                        result.push_back(node_id);
                    }
                }
            }
        }
    } else {
        // No constraints - return all nodes (up to a limit)
        for (storage::NodeId node_id = 1; node_id <= 100; ++node_id) {
            if (ctx.graph_store->has_mvcc()) {
                auto node_result = ctx.graph_store->get_node(ctx.tx_id, node_id);
                if (node_result.has_value()) {
                    result.push_back(node_id);
                }
            } else {
                auto node_result = ctx.graph_store->get_node(node_id);
                if (node_result.has_value()) {
                    result.push_back(node_id);
                }
            }
        }
    }
    
    return result;
}

util::expected<ResultSet, storage::Error> CypherExecutor::match_node_edge_node_pattern(const Node& from_node,
                                                                                      const Edge& edge,
                                                                                      const Node& to_node,
                                                                                      ExecutionContext& ctx) {
    ResultSet result_set;
    
    // Strategy: Find matching source nodes, then find edges from those nodes, then verify target nodes
    auto from_node_ids_result = find_nodes_by_pattern(from_node, ctx);
    if (!from_node_ids_result.has_value()) {
        return util::unexpected(from_node_ids_result.error());
    }
    
    for (auto from_id : from_node_ids_result.value()) {
        // Find edges from this source node
        auto edge_ids_result = find_edges_by_pattern(edge, from_id, 0, ctx);
        if (!edge_ids_result.has_value()) {
            continue; // Skip if no matching edges
        }
        
        for (auto edge_id : edge_ids_result.value()) {
            // Get the edge to find its target node
            auto edge_result = ctx.graph_store->has_mvcc() 
                ? ctx.graph_store->get_edge(ctx.tx_id, edge_id)
                : ctx.graph_store->get_edge(edge_id);
                
            if (!edge_result.has_value()) {
                continue;
            }
            
            auto [edge_record, edge_properties] = edge_result.value();
            auto to_id = edge_record.to_node;
            
            // Verify the target node matches the pattern
            if (matches_node_pattern(to_node, to_id, ctx)) {
                ResultRow row;
                
                // Bind variables
                if (from_node.variable.has_value()) {
                    row.bindings[from_node.variable.value()] = VariableBinding(
                        VariableBinding::Type::NODE, from_id);
                }
                
                if (edge.variable.has_value()) {
                    row.bindings[edge.variable.value()] = VariableBinding(
                        VariableBinding::Type::EDGE, edge_id);
                }
                
                if (to_node.variable.has_value()) {
                    row.bindings[to_node.variable.value()] = VariableBinding(
                        VariableBinding::Type::NODE, to_id);
                }
                
                result_set.push_back(std::move(row));
            }
        }
    }
    
    return result_set;
}

util::expected<std::vector<storage::EdgeId>, storage::Error> CypherExecutor::find_edges_by_pattern(const Edge& edge,
                                                                                                   storage::NodeId from_node,
                                                                                                   storage::NodeId to_node,
                                                                                                   ExecutionContext& ctx) {
    std::vector<storage::EdgeId> result;
    
    // Get outgoing edges from the source node
    auto edge_ids = ctx.index_manager->get_outgoing_edges(from_node);
    
    for (auto edge_id : edge_ids) {
        // Get edge details
        auto edge_result = ctx.graph_store->has_mvcc()
            ? ctx.graph_store->get_edge(ctx.tx_id, edge_id)
            : ctx.graph_store->get_edge(edge_id);
            
        if (!edge_result.has_value()) {
            continue;
        }
        
        auto [edge_record, edge_properties] = edge_result.value();
        
        // Check if this edge matches our pattern
        if (matches_edge_pattern(edge, edge_record, edge_properties)) {
            // If to_node is specified (non-zero), check it matches
            if (to_node == 0 || edge_record.to_node == to_node) {
                result.push_back(edge_id);
            }
        }
    }
    
    return result;
}

bool CypherExecutor::matches_node_pattern(const Node& pattern, storage::NodeId node_id, ExecutionContext& ctx) {
    // Get node properties
    auto node_result = ctx.graph_store->has_mvcc()
        ? ctx.graph_store->get_node(ctx.tx_id, node_id)
        : ctx.graph_store->get_node(node_id);
        
    if (!node_result.has_value()) {
        return false;
    }
    
    auto [node_record, properties] = node_result.value();
    
    // Check property constraints
    return matches_property_constraints(pattern.properties, properties);
}

bool CypherExecutor::matches_edge_pattern(const Edge& pattern, 
                                         const storage::EdgeRecord& edge_record,
                                         const std::vector<storage::Property>& edge_properties) {
    // Check type constraints (if any)
    if (!pattern.types.empty()) {
        // For now, we don't store edge types in EdgeRecord, so we'll check properties
        // In a full implementation, edge types would be stored separately
        bool type_matches = false;
        for (const auto& prop : edge_properties) {
            if (prop.key == "type") {
                auto type_str = std::visit([](const auto& v) -> std::string {
                    if constexpr (std::is_same_v<std::decay_t<decltype(v)>, std::string>) {
                        return v;
                    } else {
                        return "unknown";
                    }
                }, prop.value);
                
                for (const auto& expected_type : pattern.types) {
                    if (type_str == expected_type) {
                        type_matches = true;
                        break;
                    }
                }
                break;
            }
        }
        
        if (!pattern.types.empty() && !type_matches) {
            return false;
        }
    }
    
    // Check property constraints
    return matches_property_constraints(pattern.properties, edge_properties);
}

util::expected<ResultSet, storage::Error> CypherExecutor::apply_where(const WhereClause& where_clause, 
                                                                     const ResultSet& input, 
                                                                     ExecutionContext& ctx) {
    ResultSet result_set;
    
    for (const auto& row : input) {
        auto condition_result = evaluate_boolean_expression(*where_clause.condition, row.bindings, ctx);
        if (!condition_result.has_value()) {
            return util::unexpected(condition_result.error());
        }
        
        if (condition_result.value()) {
            result_set.push_back(row);
        }
    }
    
    return result_set;
}

util::expected<QueryResult, storage::Error> CypherExecutor::execute_return(const ReturnClause& return_clause, 
                                                                          const ResultSet& input, 
                                                                          ExecutionContext& ctx) {
    std::vector<std::string> columns;
    std::vector<std::vector<std::string>> rows;
    
    // Extract column names
    for (const auto& item : return_clause.items) {
        if (item.alias.has_value()) {
            columns.push_back(item.alias.value());
        } else {
            // Generate column name from expression
            columns.push_back("column_" + std::to_string(columns.size()));
        }
    }
    
    // Process each row
    for (const auto& row : input) {
        std::vector<std::string> result_row;
        
        for (const auto& item : return_clause.items) {
            auto value_result = evaluate_expression(*item.expression, row.bindings, ctx);
            if (!value_result.has_value()) {
                return util::unexpected(value_result.error());
            }
            
            result_row.push_back(property_value_to_string(value_result.value()));
        }
        
        rows.push_back(std::move(result_row));
    }
    
    QueryResult result(std::move(columns));
    result.rows = std::move(rows);
    
    return result;
}

util::expected<QueryResult, storage::Error> CypherExecutor::execute_create(const CreateClause& create_clause, 
                                                                          ExecutionContext& ctx) {
    std::vector<std::string> columns = {"created_nodes", "created_edges"};
    QueryResult result(std::move(columns));
    
    size_t created_nodes = 0;
    size_t created_edges = 0;
    
    for (const auto& pattern : create_clause.patterns) {
        // Create nodes
        for (const auto& node : pattern.nodes) {
            auto node_result = create_node_from_pattern(node, ctx);
            if (!node_result.has_value()) {
                return util::unexpected(node_result.error());
            }
            created_nodes++;
        }
        
        // TODO: Create edges
        created_edges += pattern.edges.size();
    }
    
    result.add_row({std::to_string(created_nodes), std::to_string(created_edges)});
    return result;
}

util::expected<PropertyValue, storage::Error> CypherExecutor::evaluate_expression(const Expression& expr, 
                                                                                 const VariableMap& variables, 
                                                                                 ExecutionContext& ctx) {
    switch (expr.type()) {
        case ExpressionType::LITERAL:
            return std::get<Literal>(expr.content).value;
            
        case ExpressionType::IDENTIFIER: {
            const auto& id = std::get<Identifier>(expr.content);
            auto it = variables.find(id.name);
            if (it != variables.end()) {
                if (it->second.type == VariableBinding::Type::LITERAL) {
                    return it->second.literal_value;
                } else {
                    return PropertyValue(std::to_string(it->second.id_value));
                }
            }
            return util::unexpected(storage::Error{
                storage::ErrorCode::INVALID_ARGUMENT, 
                "Undefined variable: " + id.name
            });
        }
        
        case ExpressionType::PROPERTY_ACCESS: {
            const auto& prop_access = std::get<PropertyAccess>(expr.content);
            auto it = variables.find(prop_access.entity);
            if (it != variables.end() && it->second.type == VariableBinding::Type::NODE) {
                auto node_id = it->second.id_value;
                
                // Get node properties
                if (ctx.graph_store->has_mvcc()) {
                    auto node_result = ctx.graph_store->get_node(ctx.tx_id, node_id);
                    if (node_result.has_value()) {
                        auto [node_record, properties] = node_result.value();
                        for (const auto& prop : properties) {
                            if (prop.key == prop_access.property) {
                                // Convert storage::PropertyValue to cypher::PropertyValue
                                return std::visit([](const auto& v) -> PropertyValue {
                                    using T = std::decay_t<decltype(v)>;
                                    if constexpr (std::is_same_v<T, std::string>) {
                                        return v;
                                    } else if constexpr (std::is_same_v<T, int64_t>) {
                                        return v;
                                    } else if constexpr (std::is_same_v<T, double>) {
                                        return v;
                                    } else if constexpr (std::is_same_v<T, bool>) {
                                        return v;
                                    } else {
                                        // For vector<uint8_t>, convert to string
                                        return std::string("binary_data");
                                    }
                                }, prop.value);
                            }
                        }
                    }
                } else {
                    auto node_result = ctx.graph_store->get_node(node_id);
                    if (node_result.has_value()) {
                        auto [node_record, properties] = node_result.value();
                        for (const auto& prop : properties) {
                            if (prop.key == prop_access.property) {
                                // Convert storage::PropertyValue to cypher::PropertyValue
                                return std::visit([](const auto& v) -> PropertyValue {
                                    using T = std::decay_t<decltype(v)>;
                                    if constexpr (std::is_same_v<T, std::string>) {
                                        return v;
                                    } else if constexpr (std::is_same_v<T, int64_t>) {
                                        return v;
                                    } else if constexpr (std::is_same_v<T, double>) {
                                        return v;
                                    } else if constexpr (std::is_same_v<T, bool>) {
                                        return v;
                                    } else {
                                        // For vector<uint8_t>, convert to string
                                        return std::string("binary_data");
                                    }
                                }, prop.value);
                            }
                        }
                    }
                }
            }
            return util::unexpected(storage::Error{
                storage::ErrorCode::INVALID_ARGUMENT, 
                "Property not found: " + prop_access.entity + "." + prop_access.property
            });
        }
        
        default:
            return util::unexpected(storage::Error{
                storage::ErrorCode::INVALID_ARGUMENT, 
                "Expression type not implemented"
            });
    }
}

util::expected<bool, storage::Error> CypherExecutor::evaluate_boolean_expression(const Expression& expr, 
                                                                                const VariableMap& variables, 
                                                                                ExecutionContext& ctx) {
    switch (expr.type()) {
        case ExpressionType::COMPARISON: {
            const auto& comp = std::get<Comparison>(expr.content);
            auto left_result = evaluate_expression(*comp.left, variables, ctx);
            auto right_result = evaluate_expression(*comp.right, variables, ctx);
            
            if (!left_result.has_value() || !right_result.has_value()) {
                return false;
            }
            
            // Simple comparison (would need proper type handling in production)
            std::string left_str = property_value_to_string(left_result.value());
            std::string right_str = property_value_to_string(right_result.value());
            
            switch (comp.op) {
                case ComparisonOperator::EQUAL:
                    return left_str == right_str;
                case ComparisonOperator::NOT_EQUAL:
                    return left_str != right_str;
                case ComparisonOperator::LESS_THAN:
                    return left_str < right_str;
                case ComparisonOperator::LESS_EQUAL:
                    return left_str <= right_str;
                case ComparisonOperator::GREATER_THAN:
                    return left_str > right_str;
                case ComparisonOperator::GREATER_EQUAL:
                    return left_str >= right_str;
            }
            break;
        }
        
        case ExpressionType::LOGICAL_AND: {
            const auto& and_expr = std::get<LogicalAnd>(expr.content);
            auto left_result = evaluate_boolean_expression(*and_expr.left, variables, ctx);
            auto right_result = evaluate_boolean_expression(*and_expr.right, variables, ctx);
            
            if (!left_result.has_value() || !right_result.has_value()) {
                return false;
            }
            
            return left_result.value() && right_result.value();
        }
        
        case ExpressionType::LOGICAL_OR: {
            const auto& or_expr = std::get<LogicalOr>(expr.content);
            auto left_result = evaluate_boolean_expression(*or_expr.left, variables, ctx);
            auto right_result = evaluate_boolean_expression(*or_expr.right, variables, ctx);
            
            if (!left_result.has_value() || !right_result.has_value()) {
                return false;
            }
            
            return left_result.value() || right_result.value();
        }
        
        default:
            return util::unexpected(storage::Error{
                storage::ErrorCode::INVALID_ARGUMENT, 
                "Boolean expression type not implemented"
            });
    }
    
    return false;
}

bool CypherExecutor::matches_property_constraints(const PropertyMap& constraints,
                                                 const std::vector<storage::Property>& node_properties) {
    for (const auto& [key, expected_value] : constraints) {
        bool found = false;
        for (const auto& prop : node_properties) {
            if (prop.key == key) {
                // Convert storage PropertyValue to cypher PropertyValue for comparison
                auto converted_value = std::visit([](const auto& v) -> PropertyValue {
                    using T = std::decay_t<decltype(v)>;
                    if constexpr (std::is_same_v<T, std::string>) {
                        return v;
                    } else if constexpr (std::is_same_v<T, int64_t>) {
                        return v;
                    } else if constexpr (std::is_same_v<T, double>) {
                        return v;
                    } else if constexpr (std::is_same_v<T, bool>) {
                        return v;
                    } else {
                        // For vector<uint8_t>, convert to string
                        return std::string("binary_data");
                    }
                }, prop.value);
                
                // Simple comparison - in production, need proper type handling
                std::string prop_str = property_value_to_string(converted_value);
                std::string expected_str = property_value_to_string(expected_value);
                if (prop_str == expected_str) {
                    found = true;
                    break;
                }
            }
        }
        if (!found) {
            return false;
        }
    }
    return true;
}

util::expected<storage::NodeId, storage::Error> CypherExecutor::create_node_from_pattern(const Node& node,
                                                                                        ExecutionContext& ctx) {
    // Convert PropertyMap to vector<Property>
    std::vector<storage::Property> properties;
    for (const auto& [key, value] : node.properties) {
        // Convert cypher PropertyValue to storage PropertyValue
        auto storage_value = std::visit([](const auto& v) -> storage::PropertyValue {
            using T = std::decay_t<decltype(v)>;
            if constexpr (std::is_same_v<T, std::string>) {
                return v;
            } else if constexpr (std::is_same_v<T, int64_t>) {
                return v;
            } else if constexpr (std::is_same_v<T, double>) {
                return v;
            } else if constexpr (std::is_same_v<T, bool>) {
                return v;
            } else {
                return std::string("unknown");
            }
        }, value);
        properties.emplace_back(key, storage_value);
    }
    
    if (ctx.graph_store->has_mvcc()) {
        return ctx.graph_store->create_node(ctx.tx_id, properties);
    } else {
        return ctx.graph_store->create_node(properties);
    }
}

std::string CypherExecutor::property_value_to_string(const PropertyValue& value) {
    return std::visit([](const auto& v) -> std::string {
        if constexpr (std::is_same_v<std::decay_t<decltype(v)>, std::string>) {
            return v;
        } else if constexpr (std::is_same_v<std::decay_t<decltype(v)>, int64_t>) {
            return std::to_string(v);
        } else if constexpr (std::is_same_v<std::decay_t<decltype(v)>, double>) {
            return std::to_string(v);
        } else if constexpr (std::is_same_v<std::decay_t<decltype(v)>, bool>) {
            return v ? "true" : "false";
        } else {
            return "unknown";
        }
    }, value);
}

std::string CypherExecutor::variable_binding_to_string(const VariableBinding& binding) {
    switch (binding.type) {
        case VariableBinding::Type::NODE:
        case VariableBinding::Type::EDGE:
            return std::to_string(binding.id_value);
        case VariableBinding::Type::LITERAL:
            return property_value_to_string(binding.literal_value);
    }
    return "unknown";
}

util::expected<QueryResult, storage::Error> CypherExecutor::apply_limit(const QueryResult& result, 
                                                                       const LimitClause& limit) {
    QueryResult limited_result = result;
    if (static_cast<int64_t>(limited_result.rows.size()) > limit.count) {
        limited_result.rows.resize(static_cast<size_t>(limit.count));
    }
    return limited_result;
}

util::expected<QueryResult, storage::Error> CypherExecutor::apply_order_by(const QueryResult& result, 
                                                                          const OrderByClause& order_by) {
    // For now, just return the result unchanged
    // TODO: Implement proper ordering
    return result;
}

} // namespace graphdb::query::cypher