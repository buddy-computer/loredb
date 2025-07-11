#include "planner.h"

namespace loredb::query {

std::unique_ptr<ExecutionPlan> Planner::create_plan(cypher::Query& query) {
    std::shared_ptr<PhysicalOperator> plan = nullptr;

    if (query.match.has_value()) {
        plan = plan_match(query.match.value());
    }

    if (plan && query.where.has_value()) {
        plan = std::make_shared<PhysicalFilter>(plan, query.where.value().condition.release());
    }

    // TODO: Add planning for RETURN, ORDER BY, LIMIT etc.

    if (plan) {
        return std::make_unique<ExecutionPlan>(plan);
    }

    return nullptr;
}

std::shared_ptr<PhysicalOperator> Planner::plan_match(const cypher::MatchClause& match_clause) {
    // This is a very simplified planner. It only handles a single node pattern.
    if (match_clause.patterns.empty() || match_clause.patterns[0].nodes.empty()) {
        return nullptr;
    }

    const auto& node_pattern = match_clause.patterns[0].nodes[0];
    std::optional<std::string> label;
    if (!node_pattern.labels.empty()) {
        label = node_pattern.labels[0];
    }
    
    return std::make_shared<PhysicalScan>(node_pattern.variable.value_or(""), label);
}

} // namespace loredb::query 