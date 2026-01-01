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

struct int_scalar
{
    using value_type = int64_t;
    using vector_type = std::vector<value_type>;
    value_type value_;
};

struct float_scalar
{
    using value_type = double;
    value_type value_;
};


struct int_array
{
    using vector_type = std::vector<int_scalar::value_type>;
    vector_type values_;
};

struct float_array
{
    using vector_type = std::vector<float_scalar::value_type>;
    vector_type values_;
};

template<typename T>
struct to_array_t
{};

template<>
struct to_array_t<int_scalar>
{
    using type = int_array;
};

template<>
struct to_array_t<float_scalar>
{
    using type = float_array;
};

using expr = std::variant<
    int_scalar,
    float_scalar,
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


