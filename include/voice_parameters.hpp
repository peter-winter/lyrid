#pragma once

#include <cstdint>

namespace lyrid
{
    
enum class voice_state { free, active, releasing };

struct voice_parameters
{
    float base_freq_;
    voice_state state_{voice_state::free};
    uint64_t id_;
    float smoothed_power_;
};

}
