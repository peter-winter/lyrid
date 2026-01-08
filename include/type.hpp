#pragma once

#include <variant>
#include <cstdint>

#include "utility.hpp"

namespace lyrid
{
    
struct int_scalar_type
{
    using value_type = int64_t;
};

struct float_scalar_type
{
    using value_type = double;
};

inline bool operator == (int_scalar_type, int_scalar_type) { return true; }
inline bool operator == (float_scalar_type, float_scalar_type) { return true; }
inline bool operator == (float_scalar_type, int_scalar_type) { return false; }
inline bool operator == (int_scalar_type, float_scalar_type) { return false; }

using scalar_type = std::variant<int_scalar_type, float_scalar_type>;

inline bool operator == (scalar_type t1, scalar_type t2)
{
    return std::visit([](auto x1, auto x2){ return x1 == x2; }, t1, t2);
}

struct array_type
{
    scalar_type sc_;
    std::optional<size_t> fixed_length_ = std::nullopt;
};

inline bool operator == (array_type a1, array_type a2)
{
    return a1.sc_ == a2.sc_ && a1.fixed_length_ == a2.fixed_length_;
}

using type = std::variant<array_type, float_scalar_type, int_scalar_type>;

inline bool operator == (type t1, type t2)
{
    return std::visit([](auto x1, auto x2){ return x1 == x2; }, t1, t2);
}

inline bool is_array_type(type t)
{
    return std::visit(
        overloaded
        {
            [](auto) { return false; },
            [](array_type) { return true; }
        },
        t
    );
}

inline bool is_scalar_type(type t)
{
    return !is_array_type(t);
}

inline type to_array_type(type t, size_t size)
{
    return std::visit(
        overloaded
        {
            [&](int_scalar_type i) { return type(array_type{i, size}); },
            [&](float_scalar_type f) { return type(array_type{f, size}); },
            [](array_type x) { return type(x); }
        },
        t
    );
}

inline type get_element_type(array_type ar)
{
    return type(std::visit([](auto x){ return type(x); }, ar.sc_));
}

inline scalar_type get_scalar_type(type t)
{
    return std::visit(
        overloaded
        {
            [](int_scalar_type i) { return scalar_type(i); },
            [](float_scalar_type f) { return scalar_type(f); },
            [](array_type x) { return x.sc_; }
        },
        t
    );
}

}
