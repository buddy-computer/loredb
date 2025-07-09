#pragma once

#include "ast.h"
#include "grammar.h"
#include "../../../src/util/expected.h"
#include "../../../src/storage/page_store.h"
#include <tao/pegtl.hpp>
#include <tao/pegtl/contrib/parse_tree.hpp>
#include <stack>

namespace graphdb::query::cypher {

// Parser state to build AST
struct ParserState {
    std::unique_ptr<Query> query;
    std::stack<std::unique_ptr<Expression>> expression_stack;
    std::stack<Pattern> pattern_stack;
    std::stack<Node> node_stack;
    std::stack<Edge> edge_stack;
    std::stack<PropertyMap> property_stack;
    std::stack<std::vector<ReturnItem>> return_stack;
    
    ParserState() : query(std::make_unique<Query>()) {}
};

// PEGTL parse tree selector - determines which nodes to create tree nodes for
template<typename Rule>
struct parse_tree_selector : tao::pegtl::parse_tree::selector<Rule> {
    // Select rules that we want to handle in actions
    static constexpr bool value = 
        std::is_same_v<Rule, grammar::query> ||
        std::is_same_v<Rule, grammar::match_clause> ||
        std::is_same_v<Rule, grammar::where_clause> ||
        std::is_same_v<Rule, grammar::return_clause> ||
        std::is_same_v<Rule, grammar::create_clause> ||
        std::is_same_v<Rule, grammar::set_clause> ||
        std::is_same_v<Rule, grammar::delete_clause> ||
        std::is_same_v<Rule, grammar::limit_clause> ||
        std::is_same_v<Rule, grammar::order_by_clause> ||
        std::is_same_v<Rule, grammar::pattern> ||
        std::is_same_v<Rule, grammar::node_pattern> ||
        std::is_same_v<Rule, grammar::edge_pattern> ||
        std::is_same_v<Rule, grammar::expression> ||
        std::is_same_v<Rule, grammar::property_map> ||
        std::is_same_v<Rule, grammar::property_value> ||
        std::is_same_v<Rule, grammar::identifier> ||
        std::is_same_v<Rule, grammar::string_value> ||
        std::is_same_v<Rule, grammar::number> ||
        std::is_same_v<Rule, grammar::boolean>;
};

// Helper functions to extract values from parse tree nodes
std::string extract_string(const tao::pegtl::parse_tree::node& node);
PropertyValue extract_property_value(const tao::pegtl::parse_tree::node& node);
ComparisonOperator extract_comparison_operator(const std::string& op);
OrderDirection extract_order_direction(const std::string& dir);

// Main parser actions
template<typename Rule>
struct action {};

// String literal action
template<>
struct action<grammar::string_value> {
    template<typename ActionInput>
    static void apply(const ActionInput& input, ParserState& state) {
        std::string content = input.string();
        // Remove quotes
        if (content.length() >= 2 && (content[0] == '"' || content[0] == '\'')) {
            content = content.substr(1, content.length() - 2);
        }
        state.expression_stack.push(std::make_unique<Expression>(Literal(content)));
    }
};

// Number literal action
template<>
struct action<grammar::number> {
    template<typename ActionInput>
    static void apply(const ActionInput& input, ParserState& state) {
        std::string content = input.string();
        if (content.find('.') != std::string::npos) {
            // Double
            state.expression_stack.push(std::make_unique<Expression>(Literal(std::stod(content))));
        } else {
            // Integer
            state.expression_stack.push(std::make_unique<Expression>(Literal(std::stoll(content))));
        }
    }
};

// Boolean literal action
template<>
struct action<grammar::boolean> {
    template<typename ActionInput>
    static void apply(const ActionInput& input, ParserState& state) {
        bool value = input.string() == "true";
        state.expression_stack.push(std::make_unique<Expression>(Literal(value)));
    }
};

// Identifier action
template<>
struct action<grammar::identifier> {
    template<typename ActionInput>
    static void apply(const ActionInput& input, ParserState& state) {
        state.expression_stack.push(std::make_unique<Expression>(Identifier(input.string())));
    }
};

// Property access action
template<>
struct action<grammar::property_access> {
    template<typename ActionInput>
    static void apply(const ActionInput& input, ParserState& state) {
        std::string content = input.string();
        size_t dot_pos = content.find('.');
        if (dot_pos != std::string::npos) {
            std::string entity = content.substr(0, dot_pos);
            std::string property = content.substr(dot_pos + 1);
            state.expression_stack.push(std::make_unique<Expression>(PropertyAccess(entity, property)));
        }
    }
};

// Comparison expression action
template<>
struct action<grammar::comparison_expression> {
    template<typename ActionInput>
    static void apply(const ActionInput& input, ParserState& state) {
        if (state.expression_stack.size() >= 2) {
            auto right = std::move(state.expression_stack.top());
            state.expression_stack.pop();
            auto left = std::move(state.expression_stack.top());
            state.expression_stack.pop();
            
            // Extract operator from input (simplified)
            std::string content = input.string();
            ComparisonOperator op = ComparisonOperator::EQUAL; // Default
            if (content.find("=") != std::string::npos) op = ComparisonOperator::EQUAL;
            else if (content.find("<>") != std::string::npos) op = ComparisonOperator::NOT_EQUAL;
            else if (content.find("<=") != std::string::npos) op = ComparisonOperator::LESS_EQUAL;
            else if (content.find(">=") != std::string::npos) op = ComparisonOperator::GREATER_EQUAL;
            else if (content.find("<") != std::string::npos) op = ComparisonOperator::LESS_THAN;
            else if (content.find(">") != std::string::npos) op = ComparisonOperator::GREATER_THAN;
            
            state.expression_stack.push(std::make_unique<Expression>(
                Comparison(std::move(left), op, std::move(right))
            ));
        }
    }
};

// Logical AND expression action
template<>
struct action<grammar::and_expression> {
    template<typename ActionInput>
    static void apply(const ActionInput& input, ParserState& state) {
        if (state.expression_stack.size() >= 2) {
            auto right = std::move(state.expression_stack.top());
            state.expression_stack.pop();
            auto left = std::move(state.expression_stack.top());
            state.expression_stack.pop();
            
            state.expression_stack.push(std::make_unique<Expression>(
                LogicalAnd(std::move(left), std::move(right))
            ));
        }
    }
};

// Logical OR expression action
template<>
struct action<grammar::or_expression> {
    template<typename ActionInput>
    static void apply(const ActionInput& input, ParserState& state) {
        if (state.expression_stack.size() >= 2) {
            auto right = std::move(state.expression_stack.top());
            state.expression_stack.pop();
            auto left = std::move(state.expression_stack.top());
            state.expression_stack.pop();
            
            state.expression_stack.push(std::make_unique<Expression>(
                LogicalOr(std::move(left), std::move(right))
            ));
        }
    }
};

// NOT expression action
template<>
struct action<grammar::not_expression> {
    template<typename ActionInput>
    static void apply(const ActionInput& input, ParserState& state) {
        if (!state.expression_stack.empty()) {
            auto operand = std::move(state.expression_stack.top());
            state.expression_stack.pop();
            
            state.expression_stack.push(std::make_unique<Expression>(
                LogicalNot(std::move(operand))
            ));
        }
    }
};

// MATCH clause action
template<>
struct action<grammar::match_clause> {
    template<typename ActionInput>
    static void apply(const ActionInput& input, ParserState& state) {
        // Create match clause with collected patterns
        std::vector<Pattern> patterns;
        while (!state.pattern_stack.empty()) {
            patterns.push_back(std::move(state.pattern_stack.top()));
            state.pattern_stack.pop();
        }
        std::reverse(patterns.begin(), patterns.end());
        
        state.query->match = MatchClause(std::move(patterns));
    }
};

// WHERE clause action
template<>
struct action<grammar::where_clause> {
    template<typename ActionInput>
    static void apply(const ActionInput& input, ParserState& state) {
        if (!state.expression_stack.empty()) {
            auto condition = std::move(state.expression_stack.top());
            state.expression_stack.pop();
            state.query->where = WhereClause(std::move(condition));
        }
    }
};

// RETURN clause action
template<>
struct action<grammar::return_clause> {
    template<typename ActionInput>
    static void apply(const ActionInput& input, ParserState& state) {
        if (!state.return_stack.empty()) {
            auto items = std::move(state.return_stack.top());
            state.return_stack.pop();
            
            bool distinct = input.string().find("DISTINCT") != std::string::npos;
            state.query->return_clause = ReturnClause(std::move(items), distinct);
        }
    }
};

// Return item action
template<>
struct action<grammar::return_item> {
    template<typename ActionInput>
    static void apply(const ActionInput& input, ParserState& state) {
        if (!state.expression_stack.empty()) {
            auto expr = std::move(state.expression_stack.top());
            state.expression_stack.pop();
            
            // Extract alias if present (simplified)
            std::optional<std::string> alias;
            std::string content = input.string();
            size_t as_pos = content.find(" AS ");
            if (as_pos != std::string::npos) {
                alias = content.substr(as_pos + 4);
                // Remove trailing whitespace
                while (!alias->empty() && std::isspace(alias->back())) {
                    alias->pop_back();
                }
            }
            
            if (state.return_stack.empty()) {
                state.return_stack.push(std::vector<ReturnItem>());
            }
            state.return_stack.top().emplace_back(std::move(expr), alias);
        }
    }
};

// LIMIT clause action
template<>
struct action<grammar::limit_clause> {
    template<typename ActionInput>
    static void apply(const ActionInput& input, ParserState& state) {
        std::string content = input.string();
        size_t space_pos = content.find(' ');
        if (space_pos != std::string::npos) {
            std::string number_str = content.substr(space_pos + 1);
            int64_t limit = std::stoll(number_str);
            state.query->limit = LimitClause(limit);
        }
    }
};

// Main parser class
class CypherParser {
public:
    CypherParser() = default;
    
    util::expected<std::unique_ptr<Query>, storage::Error> parse(const std::string& query_string) {
        try {
            ParserState state;
            tao::pegtl::memory_input input(query_string, "query");
            
            if (tao::pegtl::parse<grammar::query, action>(input, state)) {
                if (state.query && state.query->is_valid()) {
                    return std::move(state.query);
                } else {
                    return util::unexpected(storage::Error{
                        storage::ErrorCode::INVALID_ARGUMENT, 
                        "Invalid query structure"
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
    
private:
    // Helper methods for parsing
    static std::string trim(const std::string& str) {
        size_t first = str.find_first_not_of(' ');
        if (first == std::string::npos) return "";
        size_t last = str.find_last_not_of(' ');
        return str.substr(first, (last - first + 1));
    }
};

} // namespace graphdb::query::cypher