#pragma once

#include <variant>
#include <vector>
#include <string>
#include <cstdint>
#include <memory>

namespace lyrid
{
    
enum class type
{
    int_scalar,
    float_scalar,
    int_array,
    float_array
};

using id = std::string;

struct expr_wrapper;

struct f_call
{
    id name_;
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
    std::vector<std::string> variables_;
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
    id,
    f_call,
    index_access,
    array_construction,
    comprehension
>;

struct expr_wrapper
{
    expr wrapped_;
};

struct declaration
{
    type type_;
    id name_;
    expr value_;
    size_t line_number_;
};

struct program
{
    std::vector<declaration> declarations_;
    std::vector<std::string> errors_;

    bool is_valid() const { return errors_.empty(); }

    const std::vector<std::string>& get_errors() const { return errors_; }
};

}
