#include "executor.h"
#include "expression_evaluator.h"
#include "../../util/logger.h"
#include <algorithm>
#include <sstream>
#include <queue> // Added for BFS
#include <cmath>
#include <iostream>

namespace loredb::query::cypher {

CypherExecutor::CypherExecutor(std::shared_ptr<storage::GraphStore> graph_store,
                               std::shared_ptr<storage::SimpleIndexManager> index_manager,
                               std::shared_ptr<transaction::MVCCManager> mvcc_manager)
    : graph_store_(std::move(graph_store)), 
      index_manager_(std::move(index_manager)),
      mvcc_manager_(std::move(mvcc_manager)) {
}

CypherExecutor::~CypherExecutor() = default;

util::expected<QueryResult, storage::Error> CypherExecutor::execute_query(const std::string& cypher_query) {
    LOG_INFO("Executing Cypher query: {}", cypher_query);
    
    auto parse_result = parser_.parse(cypher_query);
    if (!parse_result.has_value()) {
        return util::unexpected<storage::Error>(parse_result.error());
    }
    
    return execute_query(*parse_result.value());
}

util::expected<QueryResult, storage::Error> CypherExecutor::execute_query(const Query& query) {
    auto tx = mvcc_manager_->get_transaction_manager().begin_transaction();
    ExecutionContext ctx(graph_store_, index_manager_, tx->id);
    
    try {
        // Handle MATCH + SET/DELETE queries (write operations)
        if (query.match.has_value() && (query.set.has_value() || query.delete_clause.has_value())) {
            // Run match to get bindings
            auto match_result = execute_match(query.match.value(), ctx);
            if (!match_result.has_value()) {
                mvcc_manager_->get_transaction_manager().abort_transaction(tx);
                mvcc_manager_->get_lock_manager().unlock_all(tx->id);
                return util::unexpected<storage::Error>(match_result.error());
            }

            ResultSet result_set = std::move(match_result.value());

            if (query.where.has_value()) {
                auto where_result = apply_where(query.where.value(), result_set, ctx);
                if (!where_result.has_value()) {
                    mvcc_manager_->get_transaction_manager().abort_transaction(tx);
                    mvcc_manager_->get_lock_manager().unlock_all(tx->id);
                    return util::unexpected<storage::Error>(where_result.error());
                }
                result_set = std::move(where_result.value());
            }

            QueryResult write_result({"updated_nodes"});
            if (query.set.has_value()) {
                auto set_res = execute_set(query.set.value(), result_set, ctx);
                if (!set_res.has_value()) {
                    mvcc_manager_->get_transaction_manager().abort_transaction(tx);
                    mvcc_manager_->get_lock_manager().unlock_all(tx->id);
                    return util::unexpected<storage::Error>(set_res.error());
                }
                write_result = std::move(set_res.value());
            }

            if (query.delete_clause.has_value()) {
                auto del_res = execute_delete(query.delete_clause.value(), result_set, ctx);
                if (!del_res.has_value()) {
                    mvcc_manager_->get_transaction_manager().abort_transaction(tx);
                    mvcc_manager_->get_lock_manager().unlock_all(tx->id);
                    return util::unexpected<storage::Error>(del_res.error());
                }
                write_result = std::move(del_res.value());
            }

            mvcc_manager_->get_transaction_manager().commit_transaction(tx);
            mvcc_manager_->get_lock_manager().unlock_all(tx->id);
            return write_result;
        }
        
        if (query.is_read_query()) {
            ResultSet result_set;
            
            if (query.match.has_value()) {
                auto match_result = execute_match(query.match.value(), ctx);
                if (!match_result.has_value()) {
                    mvcc_manager_->get_transaction_manager().abort_transaction(tx);
                    mvcc_manager_->get_lock_manager().unlock_all(tx->id);
                    return util::unexpected<storage::Error>(match_result.error());
                }
                result_set = std::move(match_result.value());
            }
            
            if (query.where.has_value()) {
                auto where_result = apply_where(query.where.value(), result_set, ctx);
                if (!where_result.has_value()) {
                    mvcc_manager_->get_transaction_manager().abort_transaction(tx);
                    mvcc_manager_->get_lock_manager().unlock_all(tx->id);
                    return util::unexpected<storage::Error>(where_result.error());
                }
                result_set = std::move(where_result.value());
            }
            
            if (query.return_clause.has_value()) {
                auto return_result = execute_return(query.return_clause.value(), result_set, ctx);
                if (!return_result.has_value()) {
                    mvcc_manager_->get_transaction_manager().abort_transaction(tx);
                    mvcc_manager_->get_lock_manager().unlock_all(tx->id);
                    return util::unexpected<storage::Error>(return_result.error());
                }
                
                auto final_result = return_result.value();
                
                if (query.order_by.has_value()) {
                    auto order_result = apply_order_by(final_result, query.order_by.value());
                    if (!order_result.has_value()) {
                        mvcc_manager_->get_transaction_manager().abort_transaction(tx);
                        mvcc_manager_->get_lock_manager().unlock_all(tx->id);
                        return util::unexpected<storage::Error>(order_result.error());
                    }
                    final_result = std::move(order_result.value());
                }
                
                if (query.limit.has_value()) {
                    auto limit_result = apply_limit(final_result, query.limit.value());
                    if (!limit_result.has_value()) {
                        mvcc_manager_->get_transaction_manager().abort_transaction(tx);
                        mvcc_manager_->get_lock_manager().unlock_all(tx->id);
                        return util::unexpected<storage::Error>(limit_result.error());
                    }
                    final_result = std::move(limit_result.value());
                }
                
                mvcc_manager_->get_transaction_manager().commit_transaction(tx);
                mvcc_manager_->get_lock_manager().unlock_all(tx->id);
                return final_result;
            }
        }
        
        if (query.create.has_value()) {
            auto create_result = execute_create(query.create.value(), ctx);
            if (!create_result.has_value()) {
                mvcc_manager_->get_transaction_manager().abort_transaction(tx);
                mvcc_manager_->get_lock_manager().unlock_all(tx->id);
                return util::unexpected<storage::Error>(create_result.error());
            }
            
            mvcc_manager_->get_transaction_manager().commit_transaction(tx);
            mvcc_manager_->get_lock_manager().unlock_all(tx->id);
            return create_result.value();
        }
        
        mvcc_manager_->get_transaction_manager().abort_transaction(tx);
        mvcc_manager_->get_lock_manager().unlock_all(tx->id);
        return util::unexpected<storage::Error>(storage::Error{
            storage::ErrorCode::INVALID_ARGUMENT,
            "Unsupported query type"
        });
        
    } catch (const std::exception& e) {
        mvcc_manager_->get_transaction_manager().abort_transaction(tx);
        mvcc_manager_->get_lock_manager().unlock_all(tx->id);
        return util::unexpected<storage::Error>(storage::Error{
            storage::ErrorCode::INVALID_ARGUMENT, 
            "Query execution error: " + std::string(e.what())
        });
    }
}

util::expected<ResultSet, storage::Error> CypherExecutor::execute_match(const MatchClause& match_clause, 
                                                                       ExecutionContext& ctx) {
    ResultSet result_set;
    
    if (match_clause.patterns.empty()) {
        return result_set;
    }
    
    auto pattern_result = match_pattern(match_clause.patterns[0], ctx);
    if (!pattern_result.has_value()) {
        return util::unexpected<storage::Error>(pattern_result.error());
    }
    
    result_set = std::move(pattern_result.value());
    
    // TODO: Handle multiple patterns with joins
    
    return result_set;
}

util::expected<ResultSet, storage::Error> CypherExecutor::match_pattern(const Pattern& pattern, 
                                                                       ExecutionContext& ctx) {
    if (pattern.nodes.empty()) {
        return ResultSet{};
    }

    // Handle simple node patterns, e.g., MATCH (n)
    if (pattern.nodes.size() == 1 && pattern.edges.empty()) {
        return match_node(pattern.nodes[0], ctx);
    }

    // Handle variable-length paths
    if (pattern.edges.size() == 1 && (pattern.edges[0].min_hops != 1 || pattern.edges[0].max_hops != 1)) {
        return match_variable_length_path(pattern.nodes[0], pattern.edges[0], pattern.nodes[1], ctx);
    }

    // For patterns longer than a simple node match, we'll use an iterative approach.
    // Start by finding all candidates for the first node in the pattern.
    auto result_set_or_error = match_node(pattern.nodes[0], ctx);
    if (!result_set_or_error.has_value()) {
        return util::unexpected<storage::Error>(result_set_or_error.error());
    }
    auto result_set = result_set_or_error.value();

    // Iteratively expand the match for each edge and subsequent node in the pattern.
    for (size_t i = 0; i < pattern.edges.size(); ++i) {
        const auto& from_node_pattern = pattern.nodes[i];
        const auto& edge_pattern = pattern.edges[i];
        const auto& to_node_pattern = pattern.nodes[i + 1];
        
        if (!from_node_pattern.variable.has_value()) {
            return util::unexpected<storage::Error>(storage::Error{
                storage::ErrorCode::INVALID_ARGUMENT,
                "Path expansion requires intermediate nodes to be named."
            });
        }

        auto expanded_result = expand_results(result_set, from_node_pattern, edge_pattern, to_node_pattern, ctx);
        if (!expanded_result.has_value()) {
            return util::unexpected<storage::Error>(expanded_result.error());
        }
        result_set = std::move(expanded_result.value());

        if (result_set.empty()) {
            break;
        }
    }
    
    return result_set;
}

util::expected<ResultSet, storage::Error> CypherExecutor::match_variable_length_path(const Node& from_node,
                                                                                     const Edge& edge,
                                                                                     const Node& to_node,
                                                                                     ExecutionContext& ctx) {
    ResultSet result_set;

    auto from_node_ids_result = find_nodes_by_pattern(from_node, ctx);
    if (!from_node_ids_result.has_value()) {
        return util::unexpected<storage::Error>(from_node_ids_result.error());
    }

    int max_hops = (edge.max_hops == -1) ? 10 : edge.max_hops; // Limit search depth for safety

    for (auto start_node_id : from_node_ids_result.value()) {
        std::queue<std::vector<storage::NodeId>> queue;
        queue.push({start_node_id});

        while (!queue.empty()) {
            std::vector<storage::NodeId> current_path = queue.front();
            queue.pop();

            if (current_path.size() - 1 > static_cast<size_t>(max_hops)) {
                continue;
            }

            storage::NodeId last_node_id = current_path.back();

            if (current_path.size() - 1 >= static_cast<size_t>(edge.min_hops)) {
                if (matches_node_pattern(to_node, last_node_id, ctx)) {
                    ResultRow row;
                    if (from_node.variable.has_value()) {
                        row.bindings[from_node.variable.value()] = VariableBinding(VariableBinding::Type::NODE, start_node_id);
                    }
                    if (to_node.variable.has_value()) {
                        row.bindings[to_node.variable.value()] = VariableBinding(VariableBinding::Type::NODE, last_node_id);
                    }
                    // Note: path variable binding not implemented
                    result_set.push_back(std::move(row));

                }
            }

            // Expand to neighbors
            if (current_path.size() - 1 < static_cast<size_t>(max_hops)) {
                auto edge_ids_result = find_edges_by_pattern(edge, last_node_id, 0, ctx);
                if (edge_ids_result.has_value()) {
                    for (auto edge_id : edge_ids_result.value()) {
                        auto edge_result = ctx.graph_store->has_mvcc()
                            ? ctx.graph_store->get_edge(ctx.tx_id, edge_id)
                            : ctx.graph_store->get_edge(edge_id);
                        
                        if (edge_result.has_value()) {
                            auto neighbor_id = edge_result.value().first.to_node;
                            // Avoid cycles in the path
                            if (std::find(current_path.begin(), current_path.end(), neighbor_id) == current_path.end()) {
                                std::vector<storage::NodeId> new_path = current_path;
                                new_path.push_back(neighbor_id);
                                queue.push(new_path);
                            }
                        }
                    }
                }
            }
        }
    }

    return result_set;
}

util::expected<ResultSet, storage::Error> CypherExecutor::match_node(const Node& node, 
                                                                    ExecutionContext& ctx) {
    auto node_ids_result = find_nodes_by_pattern(node, ctx);
    if (!node_ids_result.has_value()) {
        return util::unexpected<storage::Error>(node_ids_result.error());
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
    
    if (!node.properties.empty()) {
        // Search for nodes with specific properties
        size_t node_count = ctx.graph_store->get_node_count();
        for (storage::NodeId node_id = 1; node_id <= node_count; ++node_id) {
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
        // Get all nodes by checking based on node count
        // This is inefficient but works for now - better solution would be to add
        // a get_all_node_ids() method to GraphStore
        size_t node_count = ctx.graph_store->get_node_count();
        for (storage::NodeId node_id = 1; node_id <= node_count; ++node_id) {
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

util::expected<ResultSet, storage::Error> CypherExecutor::expand_results(const ResultSet& previous_results,
                                                                       const Node& from_node_pattern,
                                                                       const Edge& edge_pattern,
                                                                       const Node& to_node_pattern,
                                                                       ExecutionContext& ctx) {
    ResultSet new_results;

    if (!from_node_pattern.variable.has_value()) {
        return util::unexpected<storage::Error>(storage::Error{
            storage::ErrorCode::INVALID_ARGUMENT,
            "Cannot expand from a node without a variable."
        });
    }

    for (const auto& row : previous_results) {
        auto it = row.bindings.find(from_node_pattern.variable.value());
        if (it == row.bindings.end() || it->second.type != VariableBinding::Type::NODE) {
            continue;
        }
        
        storage::NodeId from_id = it->second.id_value;

        auto edge_ids_result = find_edges_by_pattern(edge_pattern, from_id, 0, ctx);
        if (!edge_ids_result.has_value()) {
            continue;
        }

        for (auto edge_id : edge_ids_result.value()) {
            auto edge_result = ctx.graph_store->has_mvcc()
                ? ctx.graph_store->get_edge(ctx.tx_id, edge_id)
                : ctx.graph_store->get_edge(edge_id);

            if (!edge_result.has_value()) {
                continue;
            }

            auto& [edge_record, edge_properties] = edge_result.value();
            storage::NodeId to_id = edge_record.to_node;
            
            if (matches_node_pattern(to_node_pattern, to_id, ctx)) {
                ResultRow new_row = row;

                if (edge_pattern.variable.has_value()) {
                    new_row.bindings[edge_pattern.variable.value()] = VariableBinding(
                        VariableBinding::Type::EDGE, edge_id);
                }
                if (to_node_pattern.variable.has_value()) {
                    auto to_var_it = new_row.bindings.find(to_node_pattern.variable.value());
                    if (to_var_it != new_row.bindings.end()) {
                        if (to_var_it->second.type == VariableBinding::Type::NODE && to_var_it->second.id_value == to_id) {
                            // This path is a cycle, which is valid.
                        } else {
                            // This variable is already bound to a different node, so this path is not a match.
                            continue;
                        }
                    } else {
                        new_row.bindings[to_node_pattern.variable.value()] = VariableBinding(
                            VariableBinding::Type::NODE, to_id);
                    }
                }
                new_results.push_back(std::move(new_row));
            }
        }
    }

    return new_results;
}

util::expected<ResultSet, storage::Error> CypherExecutor::match_node_edge_node_pattern(const Node& from_node,
                                                                                      const Edge& edge,
                                                                                      const Node& to_node,
                                                                                      ExecutionContext& ctx) {
    ResultSet result_set;
    
    auto from_node_ids_result = find_nodes_by_pattern(from_node, ctx);
    if (!from_node_ids_result.has_value()) {
        return util::unexpected<storage::Error>(from_node_ids_result.error());
    }
    
    for (auto from_id : from_node_ids_result.value()) {
        auto edge_ids_result = find_edges_by_pattern(edge, from_id, 0, ctx);
        if (!edge_ids_result.has_value()) {
            continue;
        }
        
        for (auto edge_id : edge_ids_result.value()) {
            auto edge_result = ctx.graph_store->has_mvcc() 
                ? ctx.graph_store->get_edge(ctx.tx_id, edge_id)
                : ctx.graph_store->get_edge(edge_id);
                
            if (!edge_result.has_value()) {
                continue;
            }
            
            auto [edge_record, edge_properties] = edge_result.value();
            auto to_id = edge_record.to_node;
            
            if (matches_node_pattern(to_node, to_id, ctx)) {
                ResultRow row;
                
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
    
    // Get outgoing edges from the source node using GraphStore directly
    auto outgoing_edges_result = ctx.graph_store->get_outgoing_edges(from_node);
    if (outgoing_edges_result.has_value()) {
        auto outgoing_edges = outgoing_edges_result.value();
        result.insert(result.end(), outgoing_edges.begin(), outgoing_edges.end());
    }

    // If the edge is undirected, also get incoming edges
    if (!edge.directed) {
        auto incoming_edges_result = ctx.graph_store->get_incoming_edges(from_node);
        if (incoming_edges_result.has_value()) {
            auto incoming_edges = incoming_edges_result.value();
            result.insert(result.end(), incoming_edges.begin(), incoming_edges.end());
        }
    }
    
    std::vector<storage::EdgeId> filtered_result;
    for (auto edge_id : result) {
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
            if (to_node == 0 || edge_record.to_node == to_node || (!edge.directed && edge_record.from_node == to_node)) {
                filtered_result.push_back(edge_id);
            }
        }
    }
    
    return filtered_result;
}

bool CypherExecutor::matches_node_pattern(const Node& pattern, storage::NodeId node_id, ExecutionContext& ctx) {
    auto node_result = ctx.graph_store->has_mvcc()
        ? ctx.graph_store->get_node(ctx.tx_id, node_id)
        : ctx.graph_store->get_node(node_id);
        
    if (!node_result.has_value()) {
        return false;
    }
    
    auto [node_record, properties] = node_result.value();
    
    return matches_property_constraints(pattern.properties, properties);
}

bool CypherExecutor::matches_edge_pattern(const Edge& pattern, 
                                         const storage::EdgeRecord& /*edge_record*/,
                                         const std::vector<storage::Property>& edge_properties) {
    if (!pattern.types.empty()) {
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
        
        if (!type_matches) {
            return false;
        }
    }
    
    return matches_property_constraints(pattern.properties, edge_properties);
}

util::expected<ResultSet, storage::Error> CypherExecutor::apply_where(const WhereClause& where_clause, 
                                                                     const ResultSet& input, 
                                                                     ExecutionContext& ctx) {
    ResultSet result_set;
    

    for (const auto& row : input) {
        auto condition_result = evaluate_boolean_expression(*where_clause.condition, row.bindings, ctx);
        if (!condition_result.has_value()) {
            return util::unexpected<storage::Error>(condition_result.error());
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
    
    for (const auto& item : return_clause.items) {
        if (item.alias.has_value()) {
            columns.push_back(item.alias.value());
        } else {
            // Generate a human-readable column name from the expression
            const auto& expr = *item.expression;
            switch (expr.type()) {
                case ExpressionType::PROPERTY_ACCESS: {
                    const auto& pa = std::get<PropertyAccess>(expr.content);
                    columns.push_back(pa.entity + "." + pa.property);
                    break;
                }
                case ExpressionType::IDENTIFIER: {
                    const auto& id = std::get<Identifier>(expr.content);
                    columns.push_back(id.name);
                    break;
                }
                default:
                    columns.push_back("column_" + std::to_string(columns.size()));
                    break;
            }
        }
    }
    
    for (const auto& row : input) {
        std::vector<std::string> result_row;
        
        for (const auto& item : return_clause.items) {
            auto value_result = evaluate_expression(*item.expression, row.bindings, ctx);
            if (!value_result.has_value()) {
                return util::unexpected<storage::Error>(value_result.error());
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
        for (const auto& node : pattern.nodes) {
            auto node_result = create_node_from_pattern(node, ctx);
            if (!node_result.has_value()) {
                return util::unexpected<storage::Error>(node_result.error());
            }
            created_nodes++;
        }
        
        // TODO: Create edges
        created_edges += pattern.edges.size();
    }
    
    result.add_row({std::to_string(created_nodes), std::to_string(created_edges)});
    return result;
}

util::expected<QueryResult, storage::Error> CypherExecutor::execute_set(const SetClause& set_clause,
                                                                       const ResultSet& input,
                                                                       ExecutionContext& ctx) {
    size_t updated_nodes = 0;

    for (const auto& row : input) {
        auto it = row.bindings.find(set_clause.variable);
        if (it == row.bindings.end() || it->second.type != VariableBinding::Type::NODE) {
            continue; // variable not bound to node in this row
        }

        storage::NodeId node_id = it->second.id_value;

        // Evaluate value expression in context of this row
        auto val_result = evaluate_expression(*set_clause.value, row.bindings, ctx);
        if (!val_result.has_value()) {
            return util::unexpected<storage::Error>(val_result.error());
        }

        PropertyValue new_value = val_result.value();

        // Fetch existing properties
        auto node_res = ctx.graph_store->has_mvcc() ? ctx.graph_store->get_node(ctx.tx_id, node_id)
                                                    : ctx.graph_store->get_node(node_id);
        if (!node_res.has_value()) {
            continue;
        }

        auto [node_record, properties] = node_res.value();

        bool found = false;
        for (auto& prop : properties) {
            if (prop.key == set_clause.property) {
                // update
                prop.value = std::visit([](const auto& v) -> storage::PropertyValue {
                    using T = std::decay_t<decltype(v)>;
                    if constexpr (std::is_same_v<T, std::string>) return v;
                    else if constexpr (std::is_same_v<T, int64_t>) return v;
                    else if constexpr (std::is_same_v<T, double>) return v;
                    else if constexpr (std::is_same_v<T, bool>) return v;
                    else return std::string("binary_data");
                }, new_value);
                found = true;
                break;
            }
        }
        if (!found) {
            // add new property
            storage::PropertyValue sv = std::visit([](const auto& v) -> storage::PropertyValue {
                using T = std::decay_t<decltype(v)>;
                if constexpr (std::is_same_v<T, std::string>) return v;
                else if constexpr (std::is_same_v<T, int64_t>) return v;
                else if constexpr (std::is_same_v<T, double>) return v;
                else if constexpr (std::is_same_v<T, bool>) return v;
                else return std::string("binary_data");
            }, new_value);
            properties.emplace_back(set_clause.property, sv);
        }

        // Apply update
        auto upd_res = ctx.graph_store->has_mvcc() ?
            ctx.graph_store->update_node(ctx.tx_id, node_id, properties) :
            ctx.graph_store->update_node(node_id, properties);
        if (!upd_res.has_value()) {
            return util::unexpected<storage::Error>(upd_res.error());
        }

        updated_nodes++;
    }

    QueryResult result({"updated_nodes"});
    result.add_row({std::to_string(updated_nodes)});
    return result;
}

util::expected<QueryResult, storage::Error> CypherExecutor::execute_delete(const DeleteClause& delete_clause,
                                                                          const ResultSet& input,
                                                                          ExecutionContext& ctx) {
    size_t deleted_nodes = 0;
    size_t deleted_edges = 0;

    // For each row in bindings
    for (const auto& row : input) {
        for (const auto& var : delete_clause.variables) {
            auto it = row.bindings.find(var);
            if (it == row.bindings.end()) continue;

            if (it->second.type == VariableBinding::Type::NODE) {
                storage::NodeId node_id = it->second.id_value;
                auto del_res = ctx.graph_store->has_mvcc() ?
                    ctx.graph_store->delete_node(ctx.tx_id, node_id) :
                    ctx.graph_store->delete_node(node_id);
                if (del_res.has_value()) deleted_nodes++;
            } else if (it->second.type == VariableBinding::Type::EDGE) {
                storage::EdgeId edge_id = it->second.id_value;
                auto del_res = ctx.graph_store->has_mvcc() ?
                    ctx.graph_store->delete_edge(ctx.tx_id, edge_id) :
                    ctx.graph_store->delete_edge(edge_id);
                if (del_res.has_value()) deleted_edges++;
            }
        }
    }

    QueryResult result({"deleted_nodes", "deleted_edges"});
    result.add_row({std::to_string(deleted_nodes), std::to_string(deleted_edges)});
    return result;
}

bool CypherExecutor::matches_property_constraints(const PropertyMap& constraints,
                                                 const std::vector<storage::Property>& node_properties) {
    for (const auto& [key, expected_value] : constraints) {
        bool found_match = false;
        for (const auto& prop : node_properties) {
            if (prop.key != key) continue;

            // Convert storage::PropertyValue to AST PropertyValue
            PropertyValue actual_value = std::visit([](const auto& v) -> PropertyValue {
                using T = std::decay_t<decltype(v)>;
                if constexpr (std::is_same_v<T, std::string>) return v;
                else if constexpr (std::is_same_v<T, int64_t>) return v;
                else if constexpr (std::is_same_v<T, double>) return v;
                else if constexpr (std::is_same_v<T, bool>) return v;
                else return std::string("binary_data");
            }, prop.value);

            // Compare based on contained types
            if (actual_value.index() == expected_value.index()) {
                // Same type – direct comparison via std::visit
                bool equal = std::visit([](const auto& a, const auto& b) -> bool {
                    using Ta = std::decay_t<decltype(a)>;
                    using Tb = std::decay_t<decltype(b)>;
                    if constexpr (std::is_same_v<Ta, Tb>) {
                        return a == b;
                    } else {
                        // This case should not happen due to index check, but keep safe
                        return false;
                    }
                }, actual_value, expected_value);
                if (equal) {
                    found_match = true;
                    break;
                }
            } else {
                // Handle numeric cross-type comparisons (int vs double)
                bool actual_is_num = std::holds_alternative<int64_t>(actual_value) || std::holds_alternative<double>(actual_value);
                bool expected_is_num = std::holds_alternative<int64_t>(expected_value) || std::holds_alternative<double>(expected_value);
                if (actual_is_num && expected_is_num) {
                    double act = std::holds_alternative<int64_t>(actual_value) ? static_cast<double>(std::get<int64_t>(actual_value)) : std::get<double>(actual_value);
                    double exp = std::holds_alternative<int64_t>(expected_value) ? static_cast<double>(std::get<int64_t>(expected_value)) : std::get<double>(expected_value);
                    if (std::abs(act - exp) < 1e-9) {
                        found_match = true;
                        break;
                    }
                } else {
                    // Fallback to string comparison
                    if (property_value_to_string(actual_value) == property_value_to_string(expected_value)) {
                        found_match = true;
                        break;
                    }
                }
            }
        }
        if (!found_match) return false;
    }
    return true;
}

util::expected<storage::NodeId, storage::Error> CypherExecutor::create_node_from_pattern(const Node& node,
                                                                                        ExecutionContext& ctx) {
    std::vector<storage::Property> properties;
    for (const auto& [key, value] : node.properties) {
        auto storage_value = std::visit([](const auto& v) -> storage::PropertyValue {
            using T = std::decay_t<decltype(v)>;
            if constexpr (std::is_same_v<T, std::string>) return v;
            else if constexpr (std::is_same_v<T, int64_t>) return v;
            else if constexpr (std::is_same_v<T, double>) return v;
            else if constexpr (std::is_same_v<T, bool>) return v;
            else return std::string("unknown");
        }, value);
        properties.emplace_back(key, storage_value);
    }
    
    if (ctx.graph_store->has_mvcc()) {
        return ctx.graph_store->create_node(ctx.tx_id, properties);
    } else {
        return ctx.graph_store->create_node(properties);
    }
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
    if (order_by.items.empty()) {
        return result;
    }
    
    QueryResult sorted_result = result;
    
    // Sort the rows based on the order by items
    std::sort(sorted_result.rows.begin(), sorted_result.rows.end(), 
        [&](const std::vector<std::string>& row1, const std::vector<std::string>& row2) {
            for (const auto& item : order_by.items) {
                // For now, assume the order by expression is a simple column reference
                // In the future, we'd need to evaluate the expression against each row
                
                // Get the column name from the expression
                std::string column_name;
                if (item.expression->type() == ExpressionType::PROPERTY_ACCESS) {
                    const auto& prop_access = std::get<PropertyAccess>(item.expression->content);
                    column_name = prop_access.entity + "." + prop_access.property;
                } else if (item.expression->type() == ExpressionType::IDENTIFIER) {
                    const auto& identifier = std::get<Identifier>(item.expression->content);
                    column_name = identifier.name;
                }
                
                // Find the column index
                auto col_it = std::find(result.columns.begin(), result.columns.end(), column_name);
                if (col_it == result.columns.end()) {
                    continue; // Column not found, skip this sort key
                }
                
                size_t col_index = std::distance(result.columns.begin(), col_it);
                if (col_index >= row1.size() || col_index >= row2.size()) {
                    continue; // Invalid column index
                }
                
                const std::string& val1 = row1[col_index];
                const std::string& val2 = row2[col_index];
                
                int comparison = val1.compare(val2);
                if (comparison != 0) {
                    if (item.direction == OrderDirection::ASC) {
                        return comparison < 0;
                    } else {
                        return comparison > 0;
                    }
                }
                
                // Values are equal, continue to next sort key
            }
            
            return false; // All sort keys are equal
        });
    
    return sorted_result;
}

} 