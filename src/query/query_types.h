#pragma once

#include "../storage/graph_store.h"
#include "../transaction/mvcc.h"
#include "cypher/ast.h"
#include <vector>
#include <string>
#include <variant>
#include <unordered_map>
#include <memory>

namespace loredb::storage {
    class SimpleIndexManager;
}

namespace loredb::query {

// Represents a single result from a query
struct QueryResult {
    std::vector<std::string> columns;
    std::vector<std::vector<std::string>> rows;

    QueryResult() = default;
    explicit QueryResult(std::vector<std::string> cols) : columns(std::move(cols)) {}

    void add_row(std::vector<std::string> row) {
        rows.push_back(std::move(row));
    }

    size_t size() const { return rows.size(); }
    bool empty() const { return rows.empty(); }
};

// Variable binding during query execution
struct VariableBinding {
    enum class Type {
        NODE,
        EDGE,
        LITERAL
    };
    
    Type type;
    uint64_t id_value;  // For NODE and EDGE types
    cypher::PropertyValue literal_value;  // For LITERAL type
    
    VariableBinding() : type(Type::LITERAL), id_value(0), literal_value(std::string("")) {}
    
    VariableBinding(Type t, uint64_t id) : type(t), id_value(id), literal_value(std::string("")) {
        if (t != Type::NODE && t != Type::EDGE) {
            throw std::invalid_argument("Invalid type for ID constructor");
        }
    }
    
    VariableBinding(Type t, cypher::PropertyValue value) : type(t), id_value(0), literal_value(std::move(value)) {
        if (t != Type::LITERAL) {
            throw std::invalid_argument("Invalid type for literal constructor");
        }
    }
};

using VariableMap = std::unordered_map<std::string, VariableBinding>;

// Result row for intermediate processing
struct ResultRow {
    VariableMap bindings;
    
    ResultRow() = default;
    explicit ResultRow(VariableMap vars) : bindings(std::move(vars)) {}
};

using ResultSet = std::vector<ResultRow>;

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

} // namespace loredb::query 