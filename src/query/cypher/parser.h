#pragma once

#include "ast.h"
#include "grammar.h"
#include "../../../src/util/expected.h"
#include "../../../src/storage/page_store.h"
#include <tao/pegtl.hpp>
#include <tao/pegtl/contrib/parse_tree.hpp>
#include <tao/pegtl/contrib/parse_tree_to_dot.hpp>
#include <stack>
#include <algorithm>
#include <iostream>

namespace loredb::query::cypher {

// Parser state to build AST
struct ParserState {
    std::unique_ptr<Query> query;
    std::stack<std::unique_ptr<Expression>> expression_stack;
    std::stack<Pattern> pattern_stack;
    std::stack<Node> node_stack;
    std::stack<Edge> edge_stack;
    std::stack<PropertyMap> property_stack;
    std::stack<std::vector<ReturnItem>> return_stack;
    
    // Temporary storage for labels and edge types during parsing
    std::vector<std::string> temp_labels;
    std::vector<std::string> temp_edge_types;
    
    // For variable-length paths
    int temp_min_hops = 1;
    int temp_max_hops = 1;
    bool in_variable_range = false;

    ParserState() : query(std::make_unique<Query>()) {}
};

// PEGTL parse tree selector - determines which nodes to create tree nodes for
template<typename Rule>
struct parse_tree_selector : tao::pegtl::parse_tree::selector<Rule> {
    static constexpr bool value = true; // Select all rules to build a full tree
};

// Main parser actions
template<typename Rule>
struct ast_action : tao::pegtl::nothing<Rule> {};



// Function to build AST from parse tree
inline void build_ast_from_tree(const tao::pegtl::parse_tree::node& node, ParserState& state) {
    // First, recursively process all children to populate stacks
    for (const auto& child : node.children) {
        build_ast_from_tree(*child, state);
    }
    

    
    // Then handle the current node, consuming from the stacks that children populated
    if (node.is_type<grammar::identifier>()) {
        state.expression_stack.push(std::make_unique<Expression>(Identifier(node.string())));
    }
    else if (node.is_type<grammar::string_value>()) {
        std::string content = node.string();
        if (content.length() >= 2 && (content.front() == '"' || content.front() == '\'')) {
            content = content.substr(1, content.length() - 2);
        }
        state.expression_stack.push(std::make_unique<Expression>(Literal(content)));
    }
    else if (node.is_type<grammar::number>()) {
        std::string content = node.string();
        if (content.find('.') != std::string::npos) {
            state.expression_stack.push(std::make_unique<Expression>(Literal(std::stod(content))));
        } else {
            state.expression_stack.push(std::make_unique<Expression>(Literal(std::stoll(content))));
        }
    }
    else if (node.is_type<grammar::boolean>()) {
        state.expression_stack.push(std::make_unique<Expression>(Literal(node.string() == "true")));
    }
    else if (node.is_type<grammar::property_access_expression>()) {

        // Extract entity and property from property access (e.g., "a.name")
        std::string entity;
        std::string property;
        
        // The structure is: identifier '.' property_key
        for (const auto& child : node.children) {
            if (child->is_type<grammar::identifier>()) {
                if (entity.empty()) {
                    entity = child->string();  // First identifier is the entity
                } else {
                    property = child->string();  // Second identifier is the property
                }
            }
            else if (child->is_type<grammar::property_key>()) {
                property = child->string();  // property_key is also an identifier
            }
        }
        
        if (!entity.empty() && !property.empty()) {
            state.expression_stack.push(std::make_unique<Expression>(PropertyAccess(entity, property)));
        }
    }
    else if (node.is_type<grammar::comparison_expression>()) {
        // Comparison expression: left op right
        if (state.expression_stack.size() >= 2) {
            auto right = std::move(state.expression_stack.top());
            state.expression_stack.pop();
            auto left = std::move(state.expression_stack.top()); 
            state.expression_stack.pop();
            
            // Extract the comparison operator from the node's content
            ComparisonOperator op = ComparisonOperator::EQUAL; // default
            for (const auto& child : node.children) {
                if (child->is_type<grammar::comparison_op>()) {
                    std::string op_str = child->string();
                    if (op_str == "=") op = ComparisonOperator::EQUAL;
                    else if (op_str == "<>") op = ComparisonOperator::NOT_EQUAL;
                    else if (op_str == "<") op = ComparisonOperator::LESS_THAN;
                    else if (op_str == "<=") op = ComparisonOperator::LESS_EQUAL;
                    else if (op_str == ">") op = ComparisonOperator::GREATER_THAN;
                    else if (op_str == ">=") op = ComparisonOperator::GREATER_EQUAL;
                    break;
                }
            }
            
            state.expression_stack.push(std::make_unique<Expression>(
                Comparison(std::move(left), op, std::move(right))));
        }
    }
    else if (node.is_type<grammar::property_pair>()) {
        // Extract key and value from property pair
        std::string key;
        PropertyValue value;
        
        // Find property_key and property_value children
        for (const auto& child : node.children) {
            if (child->is_type<grammar::property_key>()) {
                key = child->string();
            }
            else if (child->is_type<grammar::property_value>()) {
                // property_value can contain string_value, number, or boolean
                for (const auto& grandchild : child->children) {
                    if (grandchild->is_type<grammar::string_value>()) {
                        std::string str_content = grandchild->string();
                        if (str_content.length() >= 2 && (str_content.front() == '"' || str_content.front() == '\'')) {
                            str_content = str_content.substr(1, str_content.length() - 2);
                        }
                        value = PropertyValue(str_content);
                    }
                    else if (grandchild->is_type<grammar::number>()) {
                        std::string num_content = grandchild->string();
                        if (num_content.find('.') != std::string::npos) {
                            value = PropertyValue(std::stod(num_content));
                        } else {
                            value = PropertyValue(std::stoll(num_content));
                        }
                    }
                    else if (grandchild->is_type<grammar::boolean>()) {
                        value = PropertyValue(grandchild->string() == "true");
                    }
                }
            }
        }
        
        if (!key.empty()) {
            if (state.property_stack.empty()) {
                state.property_stack.push(PropertyMap());
            }
            state.property_stack.top()[key] = value;
        }
    }
    else if (node.is_type<grammar::property_map>()) {
        // property_map processing happens after property_pairs have been processed
        // The property_stack should already contain the PropertyMap
        if (state.property_stack.empty()) {
            state.property_stack.push(PropertyMap()); // Empty property map
        }
    }
    else if (node.is_type<grammar::label>()) {
        // Extract label name (skip the colon)
        for (const auto& child : node.children) {
            if (child->is_type<grammar::identifier>()) {
                // Labels are accumulated in a temporary vector during parsing
                // We'll collect them when we process node_pattern
                state.temp_labels.push_back(child->string());
                break;
            }
        }
    }
    else if (node.is_type<grammar::edge_type>()) {
        // Extract edge type name (skip the colon)
        for (const auto& child : node.children) {
            if (child->is_type<grammar::identifier>()) {
                // Edge types are accumulated in a temporary vector during parsing
                // We'll collect them when we process edge patterns
                state.temp_edge_types.push_back(child->string());
                break;
            }
        }
    }
    else if (node.is_type<grammar::node_pattern>()) {
        Node new_node;
        
        // Find identifier child for variable name - it's wrapped in opt<identifier>
        for (const auto& child : node.children) {
            if (child->is_type<tao::pegtl::opt<grammar::identifier>>() && !child->string().empty()) {
                new_node.variable = child->string();
                break;
            }
        }
        
        // Attach accumulated labels to this node
        if (!state.temp_labels.empty()) {
            new_node.labels = std::move(state.temp_labels);
            state.temp_labels.clear();
        }
        
        // If there's a property map on the stack, attach it to this node
        if (!state.property_stack.empty()) {
            new_node.properties = std::move(state.property_stack.top());
            state.property_stack.pop();
        }
        
        state.node_stack.push(std::move(new_node));
    }
    else if (node.is_type<grammar::directed_edge>() || node.is_type<grammar::undirected_edge>() || node.is_type<grammar::simple_undirected_edge>()) {
        Edge new_edge;
        
        // Set direction based on edge type
        new_edge.directed = node.is_type<grammar::directed_edge>();
        
        // Find identifier child for variable name - it's wrapped in opt<identifier>
        for (const auto& child : node.children) {
            if (child->is_type<tao::pegtl::opt<grammar::identifier>>() && !child->string().empty()) {
                new_edge.variable = child->string();
                break;
            }
        }
        
        // Attach accumulated edge types to this edge
        if (!state.temp_edge_types.empty()) {
            new_edge.types = std::move(state.temp_edge_types);
            state.temp_edge_types.clear();
        }
        
        // If there's a property map on the stack, attach it to this edge
        if (!state.property_stack.empty()) {
            new_edge.properties = std::move(state.property_stack.top());
            state.property_stack.pop();
        }
        
        // If there's a variable length range, parse it
        for (const auto& child : node.children) {
            if (child->is_type<grammar::variable_length_range>()) {
                // The child string includes the '*' prefix; strip it
                std::string range_str = child->string();
                if (!range_str.empty() && range_str[0] == '*') {
                    range_str = range_str.substr(1);
                }
                if (range_str.empty()) {
                    // Unbounded - treat as 1..-1 (infinite)
                    new_edge.min_hops = 1;
                    new_edge.max_hops = -1;
                } else if (range_str.find("..") != std::string::npos) {
                    // Range specification
                    size_t pos = range_str.find("..");
                    std::string first = range_str.substr(0, pos);
                    std::string second = range_str.substr(pos + 2);
                    if (!first.empty()) {
                        new_edge.min_hops = std::stoi(first);
                    }
                    if (!second.empty()) {
                        new_edge.max_hops = std::stoi(second);
                    } else {
                        new_edge.max_hops = -1; // open-ended upper bound
                    }
                } else {
                    // Single hop count like *2
                    new_edge.min_hops = std::stoi(range_str);
                    new_edge.max_hops = new_edge.min_hops;
                }
                break;
            }
        }
        
        // Fallback: if min/max hops still default (1), attempt to parse from raw string
        if (new_edge.min_hops == 1 && new_edge.max_hops == 1) {
            std::string raw = node.string();
            auto star_pos = raw.find('*');
            if (star_pos != std::string::npos) {
                std::string range_part = raw.substr(star_pos + 1);
                // range_part may include until ']' or '-'; trim non-digit/dot chars at end
                auto end_pos = range_part.find(']');
                if (end_pos != std::string::npos) range_part = range_part.substr(0, end_pos);
                if (range_part.empty()) {
                    new_edge.max_hops = -1;
                } else if (range_part.find("..") != std::string::npos) {
                    size_t pos = range_part.find("..");
                    std::string first = range_part.substr(0, pos);
                    std::string second = range_part.substr(pos + 2);
                    if (!first.empty()) new_edge.min_hops = std::stoi(first);
                    if (!second.empty()) new_edge.max_hops = std::stoi(second); else new_edge.max_hops = -1;
                } else {
                    new_edge.min_hops = std::stoi(range_part);
                    new_edge.max_hops = new_edge.min_hops;
                }
            }
        }
        
        state.edge_stack.push(std::move(new_edge));
    }
    else if (node.is_type<grammar::pattern>()) {
        Pattern pattern;
        
        // Collect all nodes from node stack for this pattern
        std::vector<Node> nodes;
        while (!state.node_stack.empty()) {
            nodes.push_back(std::move(state.node_stack.top()));
            state.node_stack.pop();
        }
        
        // Collect all edges from edge stack for this pattern
        std::vector<Edge> edges;
        while (!state.edge_stack.empty()) {
            edges.push_back(std::move(state.edge_stack.top()));
            state.edge_stack.pop();
        }
        
        // Reverse to maintain correct order (stack reverses)
        std::reverse(nodes.begin(), nodes.end());
        std::reverse(edges.begin(), edges.end());
        
        pattern.nodes = std::move(nodes);
        pattern.edges = std::move(edges);
        
        state.pattern_stack.push(std::move(pattern));
    }
    else if (node.is_type<grammar::match_clause>()) {
        MatchClause match_clause;
        
        // Collect all patterns
        std::vector<Pattern> patterns;
        while (!state.pattern_stack.empty()) {
            patterns.push_back(std::move(state.pattern_stack.top()));
            state.pattern_stack.pop();
        }
        
        std::reverse(patterns.begin(), patterns.end());
        match_clause.patterns = std::move(patterns);
        
        state.query->match = std::move(match_clause);
    }
    else if (node.is_type<grammar::create_clause>()) {
        CreateClause create_clause;
        
        // Collect all patterns
        std::vector<Pattern> patterns;
        while (!state.pattern_stack.empty()) {
            patterns.push_back(std::move(state.pattern_stack.top()));
            state.pattern_stack.pop();
        }
        
        std::reverse(patterns.begin(), patterns.end());
        create_clause.patterns = std::move(patterns);
        
        state.query->create = std::move(create_clause);
    }
    else if (node.is_type<grammar::return_item>()) {
        if (!state.expression_stack.empty()) {
            auto expr = std::move(state.expression_stack.top());
            state.expression_stack.pop();
            
            ReturnItem item(std::move(expr));
            
            if (state.return_stack.empty()) {
                state.return_stack.push(std::vector<ReturnItem>());
            }
            state.return_stack.top().push_back(std::move(item));
        }
    }
    else if (node.is_type<grammar::return_clause>()) {
        ReturnClause return_clause;
        
        if (!state.return_stack.empty()) {
            return_clause.items = std::move(state.return_stack.top());
            state.return_stack.pop();
        }
        
        state.query->return_clause = std::move(return_clause);
    }
    else if (node.is_type<grammar::where_clause>()) {
        // WHERE clause contains a single expression
        if (!state.expression_stack.empty()) {
            auto condition = std::move(state.expression_stack.top());
            state.expression_stack.pop();
            
            WhereClause where_clause(std::move(condition));
            state.query->where = std::move(where_clause);
        }
    }
    else if (node.is_type<grammar::limit_clause>()) {
        // LIMIT clause contains an integer
        for (const auto& child : node.children) {
            if (child->is_type<grammar::integer>()) {
                int64_t limit_value = std::stoll(child->string());
                LimitClause limit_clause(limit_value);
                state.query->limit = std::move(limit_clause);
                break;
            }
        }
    }
    else if (node.is_type<grammar::order_by_clause>()) {
        // ORDER BY clause contains order items
        OrderByClause order_by_clause;
        
        // Collect expressions from the expression stack for order items
        std::vector<OrderByItem> items;
        while (!state.expression_stack.empty()) {
            auto expr = std::move(state.expression_stack.top());
            state.expression_stack.pop();
            
            // Default to ASC, we'll handle DESC detection from grammar if needed
            OrderDirection direction = OrderDirection::ASC;
            
            // Look for direction in the node children
            for (const auto& child : node.children) {
                if (child->is_type<grammar::order_direction>()) {
                    if (child->string() == "DESC") {
                        direction = OrderDirection::DESC;
                    }
                    break;
                }
            }
            
            items.emplace_back(std::move(expr), direction);
        }
        
        // Reverse to maintain correct order (stack reverses)
        std::reverse(items.begin(), items.end());
        order_by_clause.items = std::move(items);
        
        state.query->order_by = std::move(order_by_clause);
    }
}


class CypherParser {
public:
    CypherParser() = default;
    
    util::expected<std::unique_ptr<Query>, storage::Error> parse(const std::string& query_string) {
        try {
            ParserState state;
            tao::pegtl::memory_input input(query_string, "query");
            auto root = tao::pegtl::parse_tree::parse<grammar::query, parse_tree_selector>(input);

            if (root) {
                build_ast_from_tree(*root, state);
                if (state.query && state.query->is_valid()) {
                    return std::move(state.query);
                } else {
                    return util::unexpected(storage::Error{
                        storage::ErrorCode::INVALID_ARGUMENT, 
                        "Parser not fully implemented"
                    });
                }
            } else {
                return util::unexpected(storage::Error{
                    storage::ErrorCode::INVALID_ARGUMENT, 
                    "Parse error: invalid syntax"
                });
            }
        } catch (const std::exception& e) {
            return util::unexpected(storage::Error{
                storage::ErrorCode::INVALID_ARGUMENT, 
                "Parse error: " + std::string(e.what())
            });
        }
    }
};

} // namespace loredb::query::cypher