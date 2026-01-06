#pragma once

#include <cstddef>
#include <utility>

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>

#include "assembly.hpp"

using namespace lyrid;
using namespace lyrid::assembly;

template<typename Ins, typename... Args>
inline void require(size_t idx, const instructions& ins, Args... args)
{
    INFO("Instruction " << idx);
    REQUIRE(std::holds_alternative<Ins>(ins[idx]));
    const auto& m = std::get<Ins>(ins[idx]);
    auto test_one = [](size_t i, auto val, auto exp) { INFO("Argument: " << i); REQUIRE(val == exp); };
    auto test = [&]<size_t... I>(std::index_sequence<I...>) { (test_one(I, m.*(std::get<I>(Ins::args)), args), ...); };
    test(std::make_index_sequence<sizeof...(args)>{});
}
