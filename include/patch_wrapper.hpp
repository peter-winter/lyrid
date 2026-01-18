#pragma once

#include "patch.hpp"
#include "voice_parameters.hpp"

namespace lyrid
{
    
template<typename T>
struct patch_wrapper
{
    static void construct(void* ptr)
    {
        new (ptr) T();
    }

    static void destruct(void* ptr)
    {
        static_cast<T*>(ptr)->~T();
    }

    static float sample(const voice_parameters& params, void* state_memory)
    {
        return static_cast<T*>(state_memory)->sample(params);
    }
};

template<typename Patch>
auto wrap()
{
    return patch
    {
        patch_wrapper<Patch>::sample,
        patch_wrapper<Patch>::construct,
        patch_wrapper<Patch>::destruct,
        sizeof(Patch)
    };
}

}
