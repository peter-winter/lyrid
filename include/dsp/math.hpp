#pragma once

#include <global_constants.hpp>

#include <cmath>
#include <numbers>
#include <utility>
#include <array>

namespace lyrid
{
 
namespace dsp
{
    
inline float pow2(float x) { return x * x; }
inline float pow3(float x) { return x * x * x; }
inline float pow4(float x) { return x * x * x * x; }
inline float pow5(float x) { return x * x * x * x * x; }

template<typename T, size_t... I>
inline T arr_sum_impl(const std::array<T, sizeof...(I)>& arr, std::index_sequence<I...>)
{
    return (T(0) + ... + arr[I]);
}

template<typename T, size_t I>
inline T arr_sum(const std::array<T, I>& arr)
{
    return arr_sum_impl(arr, std::make_index_sequence<I>{});
}


}

}
