#pragma once

#include "cypher/ast.h"
#include "execution_plan.h"
#include <memory>

namespace loredb::query {

class Planner {
public:
    Planner() = default;

    std::unique_ptr<ExecutionPlan> create_plan(cypher::Query& query);

private:
    std::shared_ptr<PhysicalOperator> plan_match(const cypher::MatchClause& match_clause);
    // Add more helper methods for other clauses as needed
};

} // namespace loredb::query 