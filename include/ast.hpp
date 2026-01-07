#pragma once

#include <variant>
#include <vector>
#include <string>
#include <cstdint>
#include <memory>
#include <optional>

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

struct symbol_ref
{
    identifier ident_;
    std::optional<size_t> declaration_idx_;
};

struct fun_ref
{
    identifier ident_;
    std::optional<size_t> proto_idx_;
};

struct scalar_offset
{
    size_t value_;
};

struct span_offset
{
    size_t value_;
};


struct expr_wrapper;

struct f_call
{
    fun_ref fn_;
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
    symbol_ref,
    f_call,
    index_access,
    array_construction,
    comprehension
>;

struct expr_wrapper
{
    expr wrapped_;
    source_location loc_;
    std::optional<type> inferred_type_;
};

struct declaration
{
    type type_;
    identifier name_;
    expr_wrapper value_;
    source_location loc_;
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
