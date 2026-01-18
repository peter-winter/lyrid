#pragma once

#include "math.hpp"
#include <voice_parameters.hpp>

namespace lyrid
{
 
namespace dsp
{
    
template<typename Val, typename Vol>
struct volume
{
    inline float sample(const voice_parameters& params)
    {
        return val_.sample(params) * pow4(vol_.sample(params));
    }
    
    Val val_;
    Vol vol_;
};

}

}
