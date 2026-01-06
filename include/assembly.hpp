#pragma once

#include <variant>
#include <vector>
#include <cstddef>

namespace lyrid::assembly
{

struct int_pool {};
struct float_pool {};

using pool = std::variant<int_pool, float_pool>;

// Scalar move: copy a single element between offsets in the same pool
struct move_scalar
{
    pool pool_;
    size_t dst_offset_;
    size_t src_offset_;
};

struct move_scalar_indirect
{
    pool pool_;                          // Target data pool (int_pool or float_pool)
    size_t span_offset_in_int_memory_;   // Offset to span pair (data_offset + length)
    size_t index_offset_in_int_memory_;  // Offset to runtime index scalar (int64_t)
    size_t dst_offset_;                  // Destination scalar offset in the specified pool
};

// Argument abstractions for calls
struct scalar_arg
{
    pool pool_;      // pool containing the array data
    size_t offset_;  // offset in the specified pool to the scalar value
};

struct span_arg
{
    pool pool_;           // pool containing the array data
    size_t span_offset_;  // offset in int_memory to the span pair (offset + length)
};

using arg = std::variant<scalar_arg, span_arg>;

// Call instruction: invokes an external function with typed arguments and a single return value
struct call
{
    size_t func_idx_;
    std::vector<arg> args_;   // input arguments (passed by memory offsets)
    arg return_;              // return: scalar or pre-allocated span in memory
};


using instruction = std::variant<
    move_scalar,
    move_scalar_indirect,
    call
>;

struct program
{
    std::vector<instruction> instructions_;

    std::vector<int64_t> int_memory_;    // Contains scalars, array elements, and all span metadata
    std::vector<double> float_memory_;   // Contains float scalars and array elements only
};

}
