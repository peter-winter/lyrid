#pragma once

#include <variant>
#include <vector>
#include <string>
#include <cstdint>
#include <memory>
#include <optional>

#include "type.hpp"
#include "memory_annotation.hpp"

namespace lyrid
{
    
namespace ast
{

struct source_location
{
    size_t line_;
    size_t column_;
};

struct expr_wrapper;

struct identifier
{
    std::string value_;
    source_location loc_;
};

struct symbol_ref
{
    identifier ident_;
    std::optional<size_t> declaration_idx_ = std::nullopt;
};

struct fun_ref
{
    identifier ident_;
    std::optional<size_t> proto_idx_ = std::nullopt;
};

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
    std::optional<memory_span_annotation> memory_annotation_ = std::nullopt;
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
    std::optional<size_t> const_memory_idx_ = std::nullopt;
};

struct float_scalar
{
    using value_type = double;
    value_type value_;
    std::optional<size_t> const_memory_idx_ = std::nullopt;
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
    std::optional<type> inferred_type_ = std::nullopt;

    expr_wrapper(expr e, source_location l)
        : wrapped_(std::move(e)), loc_(std::move(l)) {}
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
