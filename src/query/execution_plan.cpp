#include "execution_plan.h"
#include "cypher/expression_evaluator.h"

namespace loredb::query {

void PhysicalScan::init() {
    // Nothing to do for now
}

util::expected<ResultSet, storage::Error> PhysicalScan::execute(ExecutionContext& ctx) {
    // Simplified implementation: Scans all nodes, ignoring label for now.
    // In a real implementation, this would use an index scan if a label is provided.
    ResultSet result_set;
    for (storage::NodeId node_id = 1; node_id <= 100; ++node_id) { // Limiting to 100 for safety
        if (ctx.graph_store->has_mvcc()) {
            if (ctx.graph_store->get_node(ctx.tx_id, node_id).has_value()) {
                ResultRow row;
                row.bindings[variable_] = VariableBinding(VariableBinding::Type::NODE, node_id);
                result_set.push_back(std::move(row));
            }
        } else {
            if (ctx.graph_store->get_node(node_id).has_value()) {
                ResultRow row;
                row.bindings[variable_] = VariableBinding(VariableBinding::Type::NODE, node_id);
                result_set.push_back(std::move(row));
            }
        }
    }
    return result_set;
}

void PhysicalFilter::init() {
    input_->init();
}

util::expected<ResultSet, storage::Error> PhysicalFilter::execute(ExecutionContext& ctx) {
    auto input_result = input_->execute(ctx);
    if (!input_result.has_value()) {
        return util::unexpected<storage::Error>(input_result.error());
    }

    ResultSet filtered_result;
    
    for (const auto& row : input_result.value()) {
        auto condition_result = cypher::evaluate_boolean_expression(*predicate_, row.bindings, ctx);
        if (!condition_result.has_value()) {
            return util::unexpected<storage::Error>(condition_result.error());
        }
        if (condition_result.value()) {
            filtered_result.push_back(row);
        }
    }

    return filtered_result;
}

util::expected<QueryResult, storage::Error> ExecutionPlan::execute(ExecutionContext& ctx) {
    root_->init();
    auto result_set = root_->execute(ctx);
    if (!result_set.has_value()) {
        return util::unexpected<storage::Error>(result_set.error());
    }

    // This is a simplified conversion. A real implementation would need to
    // extract the correct columns based on the RETURN clause.
    QueryResult final_result({"_id"});
    for (const auto& row : result_set.value()) {
        for (const auto& [var, binding] : row.bindings) {
            if (binding.type == VariableBinding::Type::NODE) {
                final_result.add_row({std::to_string(binding.id_value)});
            }
        }
    }

    return final_result;
}

} // namespace loredb::query 