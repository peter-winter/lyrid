#pragma once

#include <voice_parameters.hpp>

namespace lyrid
{
 
namespace dsp
{
    
struct base_freq
{
    inline float sample(const voice_parameters& params)
    {
        return params.base_freq_;
    }
};

}

}
