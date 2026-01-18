#pragma once

#include <voice_parameters.hpp>

#include "math.hpp"

namespace lyrid
{
 
namespace dsp
{
    
template<typename Val, typename Cents>
struct detune
{
    inline float sample(const voice_parameters& params)
    {
        float base = val_.sample(params);
        float dt = base * (std::pow(2.0, cents_.sample(params) / 1200.0f) - 1.0f);
        return base + dt;
    }
    
    Val val_;
    Cents cents_;
};

}

}
