#pragma once

#include <voice_parameters.hpp>

#include <tuple>
#include <array>

#include "math.hpp"

namespace lyrid
{
 
namespace dsp
{
    
template<typename Coeff, size_t I>
struct coeff_wrapper
{
    Coeff coeff_;
    static constexpr size_t power = I;
};

template<typename Coeffs, typename Seq>
struct coeff_tuple_type
{};

template<typename... Coeffs, size_t... I>
struct coeff_tuple_type<std::tuple<Coeffs...>, std::index_sequence<I...>>
{
    using type = std::tuple<coeff_wrapper<Coeffs, I>...>;
};
    
template<typename... Coeffs>
using coeff_tuple_t = typename coeff_tuple_type<std::tuple<Coeffs...>, std::make_index_sequence<sizeof...(Coeffs)>>::type;

template <std::size_t N, std::size_t... Is>
void fill_powers_impl(std::array<float, N>& arr, float x, std::index_sequence<Is...>)
{
    float current = 1.0f;
    ((arr[Is] = current, current *= x), ...);
}

template <std::size_t N>
void fill_powers(std::array<float, N>& arr, float x)
{
    fill_powers_impl(arr, x, std::make_index_sequence<N>{});
}

template<typename Val, typename... Coeffs>
struct polynomial
{
    inline float sample(const voice_parameters& params)
    {
        std::array<float, sizeof...(Coeffs)> powers;
        fill_powers(powers, val_.sample(params));
        
        auto arr = std::apply(
            [&]<typename... Coeff, size_t... I>(coeff_wrapper<Coeff, I>&... c)
            {
                return std::array{ c.coeff_.sample(params) * powers[I] ... };
            }, 
            coeffs_
        );
        
        return arr_sum(arr);
    }
    
    Val val_;
    coeff_tuple_t<Coeffs...> coeffs_;
};

template<typename Val, typename Coeff0, typename Coeff1>
using linear = polynomial<Val, Coeff0, Coeff1>;

}

}
