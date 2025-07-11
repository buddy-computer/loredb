#pragma once

#include "cypher/ast.h"
#include "../storage/graph_store.h"
#include "query_types.h"
#include <memory>
#include <vector>

namespace loredb::query {

// Forward declaration
struct ExecutionContext;

// Base class for all physical query operators
class PhysicalOperator {
public:
    virtual ~PhysicalOperator() = default;
    
    // Initializes the operator
    virtual void init() = 0;
    
    // Executes the operator and returns a result set
    virtual util::expected<ResultSet, storage::Error> execute(ExecutionContext& ctx) = 0;
    
    // Returns the children of this operator
    virtual std::vector<std::shared_ptr<PhysicalOperator>> children() const = 0;
};

// Scans all nodes in the graph
class PhysicalScan : public PhysicalOperator {
public:
    PhysicalScan(std::string variable, std::optional<std::string> label)
        : variable_(std::move(variable)), label_(std::move(label)) {}
        
    void init() override;
    util::expected<ResultSet, storage::Error> execute(ExecutionContext& ctx) override;
    std::vector<std::shared_ptr<PhysicalOperator>> children() const override { return {}; }

private:
    std::string variable_;
    std::optional<std::string> label_;
};

// Filters a result set based on a predicate
class PhysicalFilter : public PhysicalOperator {
public:
    PhysicalFilter(std::shared_ptr<PhysicalOperator> input, cypher::Expression* predicate)
        : input_(std::move(input)), predicate_(predicate) {}
        
    void init() override;
    util::expected<ResultSet, storage::Error> execute(ExecutionContext& ctx) override;
    std::vector<std::shared_ptr<PhysicalOperator>> children() const override { return {input_}; }

private:
    std::shared_ptr<PhysicalOperator> input_;
    std::unique_ptr<cypher::Expression> predicate_;
};

// Represents the query execution plan
class ExecutionPlan {
public:
    explicit ExecutionPlan(std::shared_ptr<PhysicalOperator> root) : root_(std::move(root)) {}
    
    util::expected<QueryResult, storage::Error> execute(ExecutionContext& ctx);
    
private:
    std::shared_ptr<PhysicalOperator> root_;
};

} // namespace loredb::query 