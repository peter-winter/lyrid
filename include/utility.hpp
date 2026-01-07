#pragma once

namespace lyrid
{
    
template<class... Ts>
struct overloaded : Ts... { using Ts::operator()...; };


inline std::int64_t normalized_modulo(std::int64_t idx, std::int64_t len)
{
    return (idx % len + len) % len;
}

}

