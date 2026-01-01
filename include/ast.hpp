#pragma once

#include <variant>
#include <vector>
#include <string>
#include <cstdint>

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

struct int_array
{
    std::vector<int32_t> values_;
};

struct float_array
{
    std::vector<float> values_;
};

using expr = std::variant<
    int32_t,
    float,
    id,
    f_call,
    int_array,
    float_array
>;

struct expr_wrapper
{
    expr wrapped_;
};

struct declaration
{
    type type_;
    id name_;
    expr_wrapper value_;
};

struct program
{
    std::vector<declaration> declarations_;
    std::vector<std::string> errors_;

    bool is_valid() const { return errors_.empty(); }

    const std::vector<std::string>& get_errors() const { return errors_; }
};

}


