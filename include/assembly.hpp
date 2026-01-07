#pragma once

#include <variant>
#include <vector>
#include <cstddef>

namespace lyrid::assembly
{

struct int_pool {};
struct float_pool {};

using pool = std::variant<int_pool, float_pool>;


struct program
{
    std::vector<int64_t> int_memory_;    // Contains scalars, array elements, and all span metadata
    std::vector<double> float_memory_;   // Contains float scalars and array elements only
};

}
