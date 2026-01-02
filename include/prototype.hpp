#pragma once

#include "type.hpp"

#include <string>
#include <vector>

namespace lyrid
{

struct prototype
{
    std::vector<type> arg_types_;
    std::vector<std::string> arg_names_;
    type return_type_;
};

}
