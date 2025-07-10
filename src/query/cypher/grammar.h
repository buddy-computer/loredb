#pragma once

#include <tao/pegtl.hpp>
#include <tao/pegtl/contrib/abnf.hpp>

namespace loredb::query::cypher::grammar {

using namespace tao::pegtl;

// Basic tokens
struct ws : plus<space> {};
struct opt_ws : star<space> {};

// Identifiers and literals
struct identifier : seq<ranges<'a', 'z', 'A', 'Z', '_'>, star<ranges<'a', 'z', 'A', 'Z', '0', '9', '_'>>> {};
struct label : seq<one<':'>, identifier> {};
struct property_key : identifier {};

// String literals
struct string_literal : if_must<one<'"'>, until<one<'"'>>> {};
struct single_quote_string : if_must<one<'\''>, until<one<'\''>>> {};
struct string_value : sor<string_literal, single_quote_string> {};

// Numeric literals
struct integer : seq<opt<one<'-'>>, plus<digit>> {};
struct decimal : seq<integer, one<'.'>, plus<digit>> {};
struct number : sor<decimal, integer> {};

// Boolean literals
struct true_literal : TAO_PEGTL_STRING("true") {};
struct false_literal : TAO_PEGTL_STRING("false") {};
struct boolean : sor<true_literal, false_literal> {};

// Property values
struct property_value : sor<string_value, number, boolean> {};

// Property access
struct property_access : seq<identifier, one<'.'>, property_key> {};

// Comparison operators
struct eq_op : TAO_PEGTL_STRING("=") {};
struct ne_op : TAO_PEGTL_STRING("<>") {};
struct lt_op : TAO_PEGTL_STRING("<") {};
struct le_op : TAO_PEGTL_STRING("<=") {};
struct gt_op : TAO_PEGTL_STRING(">") {};
struct ge_op : TAO_PEGTL_STRING(">=") {};
struct comparison_op : sor<le_op, ge_op, ne_op, eq_op, lt_op, gt_op> {};

// Logical operators
struct and_op : TAO_PEGTL_KEYWORD("AND") {};
struct or_op : TAO_PEGTL_KEYWORD("OR") {};
struct not_op : TAO_PEGTL_KEYWORD("NOT") {};

// Forward declarations
struct expression;
struct parenthesized_expression;

// Basic expressions
struct literal_expression : property_value {};
struct identifier_expression : identifier {};
struct property_access_expression : property_access {};
struct primary_expression : sor<parenthesized_expression, literal_expression, property_access_expression, identifier_expression> {};

// Comparison expressions
struct comparison_expression : seq<primary_expression, opt_ws, comparison_op, opt_ws, primary_expression> {};

// Logical expressions
struct not_expression : seq<not_op, ws, primary_expression> {};
struct and_expression : seq<primary_expression, opt_ws, and_op, opt_ws, primary_expression> {};
struct or_expression : seq<primary_expression, opt_ws, or_op, opt_ws, primary_expression> {};

// Expression hierarchy
struct logical_expression : sor<or_expression, and_expression, not_expression> {};
struct expression : sor<logical_expression, comparison_expression, primary_expression> {};
struct parenthesized_expression : seq<one<'('>, opt_ws, expression, opt_ws, one<')'>> {};

// Property map
struct property_pair : seq<property_key, opt_ws, one<':'>, opt_ws, property_value> {};
struct property_list : list<property_pair, seq<opt_ws, one<','>, opt_ws>> {};
struct property_map : seq<one<'{'>, opt_ws, opt<property_list>, opt_ws, one<'}'>> {};

// Node patterns
struct node_labels : star<label> {};
struct node_pattern : seq<
    one<'('>,
    opt_ws,
    opt<identifier>,
    opt_ws,
    opt<node_labels>,
    opt_ws,
    opt<property_map>,
    opt_ws,
    one<')'>
> {};

// Edge patterns
struct edge_type : seq<one<':'>, identifier> {};
struct edge_types : star<edge_type> {};

// Variable length path
struct min_hops : integer {};
struct max_hops : integer {};
struct range_spec : sor<
    seq<min_hops, string<'.','.'>, max_hops>,
    seq<min_hops, string<'.','.'>>,
    seq<string<'.','.'>, max_hops>,
    min_hops // for case like *2
> {};
struct variable_length_range : seq<one<'*'>, opt<range_spec>> {};

struct directed_edge : seq<
    one<'-'>,
    one<'['>,
    opt_ws,
    opt<identifier>,
    opt_ws,
    opt<edge_types>,
    opt_ws,
    opt<variable_length_range>,
    opt_ws,
    opt<property_map>,
    opt_ws,
    one<']'>,
    TAO_PEGTL_STRING("->")
> {};
struct undirected_edge : seq<
    one<'-'>,
    one<'['>,
    opt_ws,
    opt<identifier>,
    opt_ws,
    opt<edge_types>,
    opt_ws,
    opt<variable_length_range>,
    opt_ws,
    opt<property_map>,
    opt_ws,
    one<']'>,
    one<'-'>
> {};
struct simple_undirected_edge : seq<one<'-'>, one<'-'>> {};
struct edge_pattern : sor<directed_edge, undirected_edge, simple_undirected_edge> {};

// Path patterns
struct path_element : sor<node_pattern, edge_pattern> {};
struct path_pattern : seq<node_pattern, star<seq<edge_pattern, node_pattern>>> {};
struct pattern : path_pattern {};

// Query clauses
struct match_keyword : TAO_PEGTL_KEYWORD("MATCH") {};
struct where_keyword : TAO_PEGTL_KEYWORD("WHERE") {};
struct return_keyword : TAO_PEGTL_KEYWORD("RETURN") {};
struct create_keyword : TAO_PEGTL_KEYWORD("CREATE") {};
struct set_keyword : TAO_PEGTL_KEYWORD("SET") {};
struct delete_keyword : TAO_PEGTL_KEYWORD("DELETE") {};
struct detach_keyword : TAO_PEGTL_KEYWORD("DETACH") {};
struct limit_keyword : TAO_PEGTL_KEYWORD("LIMIT") {};
struct order_keyword : TAO_PEGTL_KEYWORD("ORDER") {};
struct by_keyword : TAO_PEGTL_KEYWORD("BY") {};
struct asc_keyword : TAO_PEGTL_KEYWORD("ASC") {};
struct desc_keyword : TAO_PEGTL_KEYWORD("DESC") {};
struct distinct_keyword : TAO_PEGTL_KEYWORD("DISTINCT") {};
struct as_keyword : TAO_PEGTL_KEYWORD("AS") {};

// Pattern lists
struct pattern_list : list<pattern, seq<opt_ws, one<','>, opt_ws>> {};

// MATCH clause
struct match_clause : seq<match_keyword, ws, pattern_list> {};

// WHERE clause
struct where_clause : seq<where_keyword, ws, expression> {};

// RETURN clause
struct return_item : seq<expression, opt<seq<ws, as_keyword, ws, identifier>>> {};
struct return_list : list<return_item, seq<opt_ws, one<','>, opt_ws>> {};
struct return_clause : seq<
    return_keyword,
    opt<seq<ws, distinct_keyword>>,
    ws,
    return_list
> {};

// CREATE clause
struct create_clause : seq<create_keyword, ws, pattern_list> {};

// SET clause
struct set_assignment : seq<property_access, opt_ws, one<'='>, opt_ws, expression> {};
struct set_clause : seq<set_keyword, ws, set_assignment> {};

// DELETE clause
struct delete_list : list<identifier, seq<opt_ws, one<','>, opt_ws>> {};
struct delete_clause : seq<
    opt<seq<detach_keyword, ws>>,
    delete_keyword,
    ws,
    delete_list
> {};

// LIMIT clause
struct limit_clause : seq<limit_keyword, ws, integer> {};

// ORDER BY clause
struct order_direction : sor<asc_keyword, desc_keyword> {};
struct order_item : seq<expression, opt<seq<ws, order_direction>>> {};
struct order_list : list<order_item, seq<opt_ws, one<','>, opt_ws>> {};
struct order_by_clause : seq<order_keyword, ws, by_keyword, ws, order_list> {};

// Complete query - reorganized to allow any clause to start
struct query : seq<
    opt_ws,
    sor<
        // Queries starting with MATCH
        seq<
            match_clause,
            opt<seq<ws, where_clause>>,
            opt<sor<
                seq<ws, return_clause, opt<seq<ws, order_by_clause>>, opt<seq<ws, limit_clause>>>,
                seq<ws, set_clause>,
                seq<ws, delete_clause>
            >>
        >,
        // Queries starting with CREATE  
        seq<
            create_clause,
            opt<seq<ws, return_clause>>
        >,
        // Queries starting with SET
        seq<
            set_clause,
            opt<seq<ws, return_clause>>
        >,
        // Queries starting with DELETE
        seq<
            delete_clause,
            opt<seq<ws, return_clause>>
        >
    >,
    opt_ws,
    eof
> {};

} // namespace loredb::query::cypher::grammar