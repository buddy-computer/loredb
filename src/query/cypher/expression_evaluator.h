#pragma once

#include "ast.h"
#include "../query_types.h"
#include "../../storage/page_store.h"
#include "../../util/expected.h"

namespace loredb::query::cypher {

util::expected<PropertyValue, storage::Error> evaluate_expression(const Expression& expr, 
                                                                 const VariableMap& variables, 
                                                                 ExecutionContext& ctx);

util::expected<bool, storage::Error> evaluate_boolean_expression(const Expression& expr, 
                                                                 const VariableMap& variables, 
                                                                 ExecutionContext& ctx);

std::string property_value_to_string(const PropertyValue& value);
std::string variable_binding_to_string(const VariableBinding& binding);

} // namespace loredb::query::cypher 