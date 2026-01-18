#pragma once

#include <voice_parameters.hpp>

namespace lyrid
{
 
namespace dsp
{
    
template<float Cnst>
struct constant
{
    inline float sample(const voice_parameters&)
    {
        return Cnst;
    }
};

}

}
