#pragma once

namespace lyrid
{
    
template<class... Ts>
struct overloaded : Ts... { using Ts::operator()...; };

}

