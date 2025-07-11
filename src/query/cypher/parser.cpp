#include "parser.h"
#include <algorithm>
#include <cctype>
#include <string_view>

namespace loredb::query::cypher {

// This is a simplified parser implementation.
// In a production system, you'd have more robust error handling
// and a more complete implementation of the Cypher grammar.

std::string extract_string(const tao::pegtl::parse_tree::node& node) {
    return node.string();
}

PropertyValue extract_property_value(const tao::pegtl::parse_tree::node& node) {
    // C++23 string processing optimizations: use string_view for better performance
    std::string content = node.string();
    std::string_view content_view{content};
    
    // Try to determine the type based on content
    if (content_view == "true" || content_view == "false") {
        return content_view == "true";
    }
    
    if (!content_view.empty() && (content_view.front() == '"' || content_view.front() == '\'')) {
        // String literal - remove quotes using string_view for efficiency
        auto unquoted_view = content_view.substr(1, content_view.length() - 2);
        return std::string{unquoted_view};
    }
    
    if (content_view.find('.') != std::string_view::npos) {
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
    // C++23 string processing optimization: use string_view for efficient comparisons
    std::string_view op_view{op};
    if (op_view == "=") return ComparisonOperator::EQUAL;
    if (op_view == "<>") return ComparisonOperator::NOT_EQUAL;
    if (op_view == "<") return ComparisonOperator::LESS_THAN;
    if (op_view == "<=") return ComparisonOperator::LESS_EQUAL;
    if (op_view == ">") return ComparisonOperator::GREATER_THAN;
    if (op_view == ">=") return ComparisonOperator::GREATER_EQUAL;
    return ComparisonOperator::EQUAL; // Default
}

OrderDirection extract_order_direction(const std::string& dir) {
    // C++23 string processing optimization: use string_view for efficient case-insensitive comparison
    std::string_view dir_view{dir};
    
    // More efficient case-insensitive comparison using string_view
    if (dir_view.size() == 4 && 
        (dir_view[0] == 'D' || dir_view[0] == 'd') &&
        (dir_view[1] == 'E' || dir_view[1] == 'e') &&
        (dir_view[2] == 'S' || dir_view[2] == 's') &&
        (dir_view[3] == 'C' || dir_view[3] == 'c')) {
        return OrderDirection::DESC;
    }
    return OrderDirection::ASC;
}

} // namespace loredb::query::cypher