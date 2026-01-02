// Updated include/ast.hpp
#pragma once

#include <variant>
#include <vector>
#include <string>
#include <cstdint>
#include <memory>

#include "type.hpp"

namespace lyrid
{
    
namespace ast
{

struct source_location
{
    size_t line_;
    size_t column_;
};

struct identifier
{
    std::string value_;
    source_location loc_;
};

struct expr_wrapper;

struct f_call
{
    identifier name_;
    std::vector<expr_wrapper> args_;
};

struct index_access
{
    std::unique_ptr<expr_wrapper> base_;
    std::unique_ptr<expr_wrapper> index_;
};

struct array_construction
{
    std::vector<expr_wrapper> elements_;
};

struct comprehension
{
    std::vector<identifier> variables_;
    std::vector<expr_wrapper> in_exprs_;
    std::unique_ptr<expr_wrapper> do_expr_;
};

struct int_scalar
{
    using value_type = int64_t;
    value_type value_;
};

struct float_scalar
{
    using value_type = double;
    value_type value_;
};

using expr = std::variant<
    int_scalar,
    float_scalar,
    identifier,
    f_call,
    index_access,
    array_construction,
    comprehension
>;

struct expr_wrapper
{
    expr wrapped_;
    source_location loc;

    expr_wrapper(expr e, source_location l)
        : wrapped_(std::move(e)), loc(std::move(l)) {}
};

struct declaration
{
    type type_;
    identifier name_;
    expr_wrapper value_;
    source_location loc;
};

struct program
{
    std::vector<declaration> declarations_;
    std::vector<std::string> errors_;

    bool is_valid() const { return errors_.empty(); }

    const std::vector<std::string>& get_errors() const { return errors_; }
};

}

}
