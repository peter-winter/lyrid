#pragma once

namespace lyrid
{
    
struct voice_parameters;

using sampler = float (*)(const voice_parameters&, void*);
using in_place_constructor = void (*)(void*);
using destructor = void (*)(void*);

struct patch
{
    sampler sampler_;
    in_place_constructor cnstr_;
    destructor dstr_;
    size_t state_size_;
};

}
