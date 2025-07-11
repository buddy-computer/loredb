#include "parser.h"
#include <algorithm>
#include <cctype>

namespace loredb::query::cypher {

// This is a simplified parser implementation.
// In a production system, you'd have more robust error handling
// and a more complete implementation of the Cypher grammar.

std::string extract_string(const tao::pegtl::parse_tree::node& node) {
    return node.string();
}

PropertyValue extract_property_value(const tao::pegtl::parse_tree::node& node) {
    std::string content = node.string();
    
    // Try to determine the type based on content
    if (content == "true" || content == "false") {
        return content == "true";
    }
    
    if (content.front() == '"' || content.front() == '\'') {
        // String literal - remove quotes
        return content.substr(1, content.length() - 2);
    }
    
    if (content.find('.') != std::string::npos) {
        // Double
        try {
            return std::stod(content);
        } catch (...) {
            return content; // Fallback to string
        }
    }
    
    // Try integer
    try {
        return std::stoll(content);
    } catch (...) {
        return content; // Fallback to string
    }
}

ComparisonOperator extract_comparison_operator(const std::string& op) {
    if (op == "=") return ComparisonOperator::EQUAL;
    if (op == "<>") return ComparisonOperator::NOT_EQUAL;
    if (op == "<") return ComparisonOperator::LESS_THAN;
    if (op == "<=") return ComparisonOperator::LESS_EQUAL;
    if (op == ">") return ComparisonOperator::GREATER_THAN;
    if (op == ">=") return ComparisonOperator::GREATER_EQUAL;
    return ComparisonOperator::EQUAL; // Default
}

OrderDirection extract_order_direction(const std::string& dir) {
    std::string upper_dir = dir;
    std::transform(upper_dir.begin(), upper_dir.end(), upper_dir.begin(), ::toupper);
    if (upper_dir == "DESC") return OrderDirection::DESC;
    return OrderDirection::ASC;
}

} // namespace loredb::query::cypher