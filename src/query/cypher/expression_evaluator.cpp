#include "expression_evaluator.h"
#include "../../storage/graph_store.h"
#include <iostream>

namespace loredb::query::cypher {

// Forward declaration to break dependency cycle
std::string property_value_to_string(const PropertyValue& value);

util::expected<PropertyValue, storage::Error> evaluate_expression(const Expression& expr, 
                                                                 const VariableMap& variables, 
                                                                 ExecutionContext& ctx) {

    switch (expr.type()) {
        case ExpressionType::LITERAL:
            return std::get<Literal>(expr.content).value;
            
        case ExpressionType::IDENTIFIER: {
            const auto& id = std::get<Identifier>(expr.content);
            auto it = variables.find(id.name);
            if (it != variables.end()) {
                if (it->second.type == VariableBinding::Type::LITERAL) {
                    return it->second.literal_value;
                } else {
                    return PropertyValue(std::to_string(it->second.id_value));
                }
            }
            return util::unexpected<storage::Error>(storage::Error{
                storage::ErrorCode::INVALID_ARGUMENT, 
                "Undefined variable: " + id.name
            });
        }
        
        case ExpressionType::PROPERTY_ACCESS: {
            const auto& prop_access = std::get<PropertyAccess>(expr.content);

            auto it = variables.find(prop_access.entity);
            if (it != variables.end() && it->second.type == VariableBinding::Type::NODE) {
                auto node_id = it->second.id_value;
                
                if (ctx.graph_store->has_mvcc()) {
                    auto node_result = ctx.graph_store->get_node(ctx.tx_id, node_id);
                    if (node_result.has_value()) {
                        auto [node_record, properties] = node_result.value();
                        for (const auto& prop : properties) {
                            if (prop.key == prop_access.property) {
                                return std::visit([](const auto& v) -> PropertyValue {
                                    using T = std::decay_t<decltype(v)>;
                                    if constexpr (std::is_same_v<T, std::string>) return v;
                                    else if constexpr (std::is_same_v<T, int64_t>) return v;
                                    else if constexpr (std::is_same_v<T, double>) return v;
                                    else if constexpr (std::is_same_v<T, bool>) return v;
                                    else return std::string("binary_data");
                                }, prop.value);
                            }
                        }
                    }
                } else {
                    auto node_result = ctx.graph_store->get_node(node_id);
                    if (node_result.has_value()) {
                        auto [node_record, properties] = node_result.value();
                        for (const auto& prop : properties) {
                            if (prop.key == prop_access.property) {
                                return std::visit([](const auto& v) -> PropertyValue {
                                    using T = std::decay_t<decltype(v)>;
                                    if constexpr (std::is_same_v<T, std::string>) return v;
                                    else if constexpr (std::is_same_v<T, int64_t>) return v;
                                    else if constexpr (std::is_same_v<T, double>) return v;
                                    else if constexpr (std::is_same_v<T, bool>) return v;
                                    else return std::string("binary_data");
                                }, prop.value);
                            }
                        }
                    }
                }
            }
            return util::unexpected<storage::Error>(storage::Error{
                storage::ErrorCode::INVALID_ARGUMENT, 
                "Property not found: " + prop_access.entity + "." + prop_access.property
            });
        }
        
        default:
            return util::unexpected<storage::Error>(storage::Error{
                storage::ErrorCode::INVALID_ARGUMENT, 
                "Expression type not implemented"
            });
    }
}

util::expected<bool, storage::Error> evaluate_boolean_expression(const Expression& expr, 
                                                                 const VariableMap& variables, 
                                                                 ExecutionContext& ctx) {
    switch (expr.type()) {
        case ExpressionType::COMPARISON: {
            const auto& comp = std::get<Comparison>(expr.content);
            auto left_result = evaluate_expression(*comp.left, variables, ctx);
            auto right_result = evaluate_expression(*comp.right, variables, ctx);
            
            if (!left_result.has_value() || !right_result.has_value()) {
                return false;
            }
            
            // Handle numeric comparisons properly
            const auto& left_value = left_result.value();
            const auto& right_value = right_result.value();
            
            // Check if both values are numeric (int64_t or double)
            bool left_is_numeric = std::holds_alternative<int64_t>(left_value) || std::holds_alternative<double>(left_value);
            bool right_is_numeric = std::holds_alternative<int64_t>(right_value) || std::holds_alternative<double>(right_value);
            
            if (left_is_numeric && right_is_numeric) {
                // Both values are numeric - do numeric comparison
                double left_num = std::holds_alternative<int64_t>(left_value) 
                    ? static_cast<double>(std::get<int64_t>(left_value))
                    : std::get<double>(left_value);
                double right_num = std::holds_alternative<int64_t>(right_value)
                    ? static_cast<double>(std::get<int64_t>(right_value))
                    : std::get<double>(right_value);
                    
                switch (comp.op) {
                    case ComparisonOperator::EQUAL: return left_num == right_num;
                    case ComparisonOperator::NOT_EQUAL: return left_num != right_num;
                    case ComparisonOperator::LESS_THAN: return left_num < right_num;
                    case ComparisonOperator::LESS_EQUAL: return left_num <= right_num;
                    case ComparisonOperator::GREATER_THAN: return left_num > right_num;
                    case ComparisonOperator::GREATER_EQUAL: return left_num >= right_num;
                }
            } else {
                // Fall back to string comparison for non-numeric values
                std::string left_str = property_value_to_string(left_value);
                std::string right_str = property_value_to_string(right_value);
                
                switch (comp.op) {
                    case ComparisonOperator::EQUAL: return left_str == right_str;
                    case ComparisonOperator::NOT_EQUAL: return left_str != right_str;
                    case ComparisonOperator::LESS_THAN: return left_str < right_str;
                    case ComparisonOperator::LESS_EQUAL: return left_str <= right_str;
                    case ComparisonOperator::GREATER_THAN: return left_str > right_str;
                    case ComparisonOperator::GREATER_EQUAL: return left_str >= right_str;
                }
            }
            break;
        }
        
        case ExpressionType::LOGICAL_AND: {
            const auto& and_expr = std::get<LogicalAnd>(expr.content);
            auto left_result = evaluate_boolean_expression(*and_expr.left, variables, ctx);
            if (!left_result.has_value() || !left_result.value()) return false;
            auto right_result = evaluate_boolean_expression(*and_expr.right, variables, ctx);
            return right_result.has_value() && right_result.value();
        }
        
        case ExpressionType::LOGICAL_OR: {
            const auto& or_expr = std::get<LogicalOr>(expr.content);
            auto left_result = evaluate_boolean_expression(*or_expr.left, variables, ctx);
            if (left_result.has_value() && left_result.value()) return true;
            auto right_result = evaluate_boolean_expression(*or_expr.right, variables, ctx);
            return right_result.has_value() && right_result.value();
        }
        
        default:
            return util::unexpected<storage::Error>(storage::Error{
                storage::ErrorCode::INVALID_ARGUMENT, 
                "Boolean expression type not implemented"
            });
    }
    
    return false;
}

std::string property_value_to_string(const PropertyValue& value) {
    return std::visit([](const auto& v) -> std::string {
        if constexpr (std::is_same_v<std::decay_t<decltype(v)>, std::string>) return v;
        else if constexpr (std::is_same_v<std::decay_t<decltype(v)>, int64_t>) return std::to_string(v);
        else if constexpr (std::is_same_v<std::decay_t<decltype(v)>, double>) return std::to_string(v);
        else if constexpr (std::is_same_v<std::decay_t<decltype(v)>, bool>) return v ? "true" : "false";
        else return "unknown";
    }, value);
}

std::string variable_binding_to_string(const VariableBinding& binding) {
    switch (binding.type) {
        case VariableBinding::Type::NODE:
        case VariableBinding::Type::EDGE:
            return std::to_string(binding.id_value);
        case VariableBinding::Type::LITERAL:
            return property_value_to_string(binding.literal_value);
    }
    return "unknown";
}

} 