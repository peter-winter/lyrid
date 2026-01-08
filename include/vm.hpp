#pragma once

#include <variant>
#include <vector>

#include "type.hpp"

namespace lyrid
{


struct vm
{
    std::vector<int_scalar_type::value_type> int_memory_;
    std::vector<float_scalar_type::value_type> float_memory_;
};

struct vm_program
{
};

}
