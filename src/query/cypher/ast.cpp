#include "ast.h"

namespace loredb::query::cypher {

// Comparison implementations
Comparison::Comparison(std::unique_ptr<Expression> l, ComparisonOperator o, std::unique_ptr<Expression> r)
    : left(std::move(l)), op(o), right(std::move(r)) {}

Comparison::~Comparison() = default;

Comparison::Comparison(Comparison&&) noexcept = default;
Comparison& Comparison::operator=(Comparison&&) noexcept = default;

// LogicalAnd implementations
LogicalAnd::LogicalAnd(std::unique_ptr<Expression> l, std::unique_ptr<Expression> r)
    : left(std::move(l)), right(std::move(r)) {}

LogicalAnd::~LogicalAnd() = default;

LogicalAnd::LogicalAnd(LogicalAnd&&) noexcept = default;
LogicalAnd& LogicalAnd::operator=(LogicalAnd&&) noexcept = default;

// LogicalOr implementations
LogicalOr::LogicalOr(std::unique_ptr<Expression> l, std::unique_ptr<Expression> r)
    : left(std::move(l)), right(std::move(r)) {}

LogicalOr::~LogicalOr() = default;

LogicalOr::LogicalOr(LogicalOr&&) noexcept = default;
LogicalOr& LogicalOr::operator=(LogicalOr&&) noexcept = default;

// LogicalNot implementations
LogicalNot::LogicalNot(std::unique_ptr<Expression> op)
    : operand(std::move(op)) {}

LogicalNot::~LogicalNot() = default;

LogicalNot::LogicalNot(LogicalNot&&) noexcept = default;
LogicalNot& LogicalNot::operator=(LogicalNot&&) noexcept = default;

} // namespace loredb::query::cypher