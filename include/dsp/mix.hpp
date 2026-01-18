#pragma once

#include <tuple>
#include <array>

#include <voice_parameters.hpp>

namespace lyrid
{
 
namespace dsp
{
    
template<typename... Vals>
struct mix
{
    inline float sample(const voice_parameters& params)
    {
        auto arr = std::apply(
            [&](auto&... val)
            {
                return std::array{ val.sample(params)... };
            }, 
            vals_
        );
             
        return arr_sum(arr) / sizeof...(Vals);
    }
    
    std::tuple<Vals...> vals_;
};

}

}
