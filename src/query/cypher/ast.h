#pragma once

#include <string>
#include <vector>
#include <memory>
#include <variant>
#include <optional>
#include <unordered_map>

namespace graphdb::query::cypher {

// Forward declarations
struct Expression;
struct Pattern;
struct Node;
struct Edge;

// Base types
using PropertyValue = std::variant<std::string, int64_t, double, bool>;
using PropertyMap = std::unordered_map<std::string, PropertyValue>;

// Expression types
enum class ExpressionType {
    LITERAL,
    PROPERTY_ACCESS,
    IDENTIFIER,
    COMPARISON,
    LOGICAL_AND,
    LOGICAL_OR,
    LOGICAL_NOT
};

enum class ComparisonOperator {
    EQUAL,
    NOT_EQUAL,
    LESS_THAN,
    LESS_EQUAL,
    GREATER_THAN,
    GREATER_EQUAL
};

struct Literal {
    PropertyValue value;
    
    explicit Literal(PropertyValue val) : value(std::move(val)) {}
};

struct PropertyAccess {
    std::string entity;    // Variable name (e.g., "n")
    std::string property;  // Property name (e.g., "name")
    
    PropertyAccess(std::string ent, std::string prop) 
        : entity(std::move(ent)), property(std::move(prop)) {}
};

struct Identifier {
    std::string name;
    
    explicit Identifier(std::string n) : name(std::move(n)) {}
};

struct Comparison {
    std::unique_ptr<Expression> left;
    ComparisonOperator op;
    std::unique_ptr<Expression> right;
    
    Comparison(std::unique_ptr<Expression> l, ComparisonOperator o, std::unique_ptr<Expression> r)
        : left(std::move(l)), op(o), right(std::move(r)) {}
};

struct LogicalAnd {
    std::unique_ptr<Expression> left;
    std::unique_ptr<Expression> right;
    
    LogicalAnd(std::unique_ptr<Expression> l, std::unique_ptr<Expression> r)
        : left(std::move(l)), right(std::move(r)) {}
};

struct LogicalOr {
    std::unique_ptr<Expression> left;
    std::unique_ptr<Expression> right;
    
    LogicalOr(std::unique_ptr<Expression> l, std::unique_ptr<Expression> r)
        : left(std::move(l)), right(std::move(r)) {}
};

struct LogicalNot {
    std::unique_ptr<Expression> operand;
    
    explicit LogicalNot(std::unique_ptr<Expression> op) : operand(std::move(op)) {}
};

// Expression variant
struct Expression {
    std::variant<
        Literal,
        PropertyAccess,
        Identifier,
        Comparison,
        LogicalAnd,
        LogicalOr,
        LogicalNot
    > content;
    
    template<typename T>
    Expression(T&& val) : content(std::forward<T>(val)) {}
    
    ExpressionType type() const {
        return static_cast<ExpressionType>(content.index());
    }
};

// Pattern matching structures
struct Node {
    std::optional<std::string> variable;     // Variable name (e.g., "n")
    std::vector<std::string> labels;         // Node labels (e.g., ["Person", "User"])
    PropertyMap properties;                  // Property constraints
    
    Node() = default;
    Node(std::optional<std::string> var, std::vector<std::string> lbls, PropertyMap props)
        : variable(std::move(var)), labels(std::move(lbls)), properties(std::move(props)) {}
};

struct Edge {
    std::optional<std::string> variable;     // Variable name (e.g., "r")
    std::vector<std::string> types;          // Edge types (e.g., ["KNOWS", "FRIEND"])
    PropertyMap properties;                  // Property constraints
    bool directed = true;                    // Direction (true for ->, false for -)
    
    Edge() = default;
    Edge(std::optional<std::string> var, std::vector<std::string> typs, PropertyMap props, bool dir = true)
        : variable(std::move(var)), types(std::move(typs)), properties(std::move(props)), directed(dir) {}
};

struct Pattern {
    std::vector<Node> nodes;
    std::vector<Edge> edges;
    
    Pattern() = default;
    Pattern(std::vector<Node> n, std::vector<Edge> e) : nodes(std::move(n)), edges(std::move(e)) {}
};

// Query clause types
enum class ClauseType {
    MATCH,
    WHERE,
    RETURN,
    CREATE,
    SET,
    DELETE,
    LIMIT,
    ORDER_BY
};

struct MatchClause {
    std::vector<Pattern> patterns;
    std::optional<std::unique_ptr<Expression>> where_condition;
    
    MatchClause() = default;
    explicit MatchClause(std::vector<Pattern> pats) : patterns(std::move(pats)) {}
};

struct WhereClause {
    std::unique_ptr<Expression> condition;
    
    explicit WhereClause(std::unique_ptr<Expression> cond) : condition(std::move(cond)) {}
};

struct ReturnItem {
    std::unique_ptr<Expression> expression;
    std::optional<std::string> alias;
    
    ReturnItem(std::unique_ptr<Expression> expr, std::optional<std::string> a = std::nullopt)
        : expression(std::move(expr)), alias(std::move(a)) {}
};

struct ReturnClause {
    std::vector<ReturnItem> items;
    bool distinct = false;
    
    ReturnClause() = default;
    explicit ReturnClause(std::vector<ReturnItem> its, bool dist = false) 
        : items(std::move(its)), distinct(dist) {}
};

struct CreateClause {
    std::vector<Pattern> patterns;
    
    CreateClause() = default;
    explicit CreateClause(std::vector<Pattern> pats) : patterns(std::move(pats)) {}
};

struct SetClause {
    std::string variable;
    std::string property;
    std::unique_ptr<Expression> value;
    
    SetClause(std::string var, std::string prop, std::unique_ptr<Expression> val)
        : variable(std::move(var)), property(std::move(prop)), value(std::move(val)) {}
};

struct DeleteClause {
    std::vector<std::string> variables;
    bool detach = false;
    
    DeleteClause() = default;
    explicit DeleteClause(std::vector<std::string> vars, bool det = false) 
        : variables(std::move(vars)), detach(det) {}
};

struct LimitClause {
    int64_t count;
    
    explicit LimitClause(int64_t c) : count(c) {}
};

enum class OrderDirection {
    ASC,
    DESC
};

struct OrderByItem {
    std::unique_ptr<Expression> expression;
    OrderDirection direction = OrderDirection::ASC;
    
    OrderByItem(std::unique_ptr<Expression> expr, OrderDirection dir = OrderDirection::ASC)
        : expression(std::move(expr)), direction(dir) {}
};

struct OrderByClause {
    std::vector<OrderByItem> items;
    
    OrderByClause() = default;
    explicit OrderByClause(std::vector<OrderByItem> its) : items(std::move(its)) {}
};

// Main query structure
struct Query {
    std::optional<MatchClause> match;
    std::optional<WhereClause> where;
    std::optional<ReturnClause> return_clause;
    std::optional<CreateClause> create;
    std::optional<SetClause> set;
    std::optional<DeleteClause> delete_clause;
    std::optional<LimitClause> limit;
    std::optional<OrderByClause> order_by;
    
    Query() = default;
    
    // Helper methods to check query type
    bool is_read_query() const {
        return match.has_value() && return_clause.has_value() && !create.has_value() && !set.has_value() && !delete_clause.has_value();
    }
    
    bool is_write_query() const {
        return create.has_value() || set.has_value() || delete_clause.has_value();
    }
    
    bool is_valid() const {
        // Basic validation - must have at least one clause
        return match.has_value() || create.has_value() || set.has_value() || delete_clause.has_value();
    }
};

} // namespace graphdb::query::cypher