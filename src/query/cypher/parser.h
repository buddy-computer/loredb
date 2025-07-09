#pragma once

#include "ast.h"
#include "grammar.h"
#include "../../../src/util/expected.h"
#include "../../../src/storage/page_store.h"
#include <tao/pegtl.hpp>
#include <tao/pegtl/contrib/parse_tree.hpp>
#include <tao/pegtl/contrib/parse_tree_to_dot.hpp>
#include <stack>

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

template<>
struct ast_action<grammar::string_value> {
    static void apply(const tao::pegtl::parse_tree::node& n, ParserState& state) {
        std::string content = n.string();
        if (content.length() >= 2 && (content.front() == '"' || content.front() == '\'')) {
            content = content.substr(1, content.length() - 2);
        }
        state.expression_stack.push(std::make_unique<Expression>(Literal(content)));
    }
};

template<>
struct ast_action<grammar::number> {
    static void apply(const tao::pegtl::parse_tree::node& n, ParserState& state) {
        std::string content = n.string();
        if (content.find('.') != std::string::npos) {
            state.expression_stack.push(std::make_unique<Expression>(Literal(std::stod(content))));
        } else {
            state.expression_stack.push(std::make_unique<Expression>(Literal(std::stoll(content))));
        }
    }
};

template<>
struct ast_action<grammar::boolean> {
    static void apply(const tao::pegtl::parse_tree::node& n, ParserState& state) {
        state.expression_stack.push(std::make_unique<Expression>(Literal(n.string() == "true")));
    }
};

template<>
struct ast_action<grammar::identifier> {
     static void apply(const tao::pegtl::parse_tree::node& n, ParserState& state) {
        state.expression_stack.push(std::make_unique<Expression>(Identifier(n.string())));
    }
};


class CypherParser {
public:
    CypherParser() = default;
    
    util::expected<std::unique_ptr<Query>, storage::Error> parse(const std::string& query_string) {
        try {
            ParserState state;
            tao::pegtl::memory_input input(query_string, "query");
            auto root = tao::pegtl::parse_tree::parse<grammar::query, parse_tree_selector>(input);

            if (root) {
                // tao::pegtl::parse_tree::apply<ast_action>(*root, state); // Commented out to fix build
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