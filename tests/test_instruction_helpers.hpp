#pragma once

#include <cstddef>

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>

#include "assembly.hpp"

using namespace lyrid;

inline void require_mov_i_const(size_t idx, const assembly::instruction& i, size_t dst, size_t src_const)
{
    INFO("Instruction " << idx);
    REQUIRE(std::holds_alternative<assembly::mov_i_reg_const>(i));
    const auto& m = std::get<assembly::mov_i_reg_const>(i);
    REQUIRE(m.dst_ == dst);
    REQUIRE(m.src_ == src_const);
}

inline void require_mov_f_const(size_t idx, const assembly::instruction& i, size_t dst, size_t src_const)
{
    INFO("Instruction " << idx);
    REQUIRE(std::holds_alternative<assembly::mov_f_reg_const>(i));
    const auto& m = std::get<assembly::mov_f_reg_const>(i);
    REQUIRE(m.dst_ == dst);
    REQUIRE(m.src_ == src_const);
}

inline void require_mov_is_const(size_t idx, const assembly::instruction& i, size_t dst, size_t src_const)
{
    INFO("Instruction " << idx);
    REQUIRE(std::holds_alternative<assembly::mov_is_reg_const>(i));
    const auto& m = std::get<assembly::mov_is_reg_const>(i);
    REQUIRE(m.dst_ == dst);
    REQUIRE(m.span_idx_ == src_const);
}

inline void require_mov_fs_const(size_t idx, const assembly::instruction& i, size_t dst, size_t src_const)
{
    INFO("Instruction " << idx);
    REQUIRE(std::holds_alternative<assembly::mov_fs_reg_const>(i));
    const auto& m = std::get<assembly::mov_fs_reg_const>(i);
    REQUIRE(m.dst_ == dst);
    REQUIRE(m.span_idx_ == src_const);
}

inline void require_mov_i_reg(size_t idx, const assembly::instruction& i, size_t dst, size_t src)
{
    INFO("Instruction " << idx);
    REQUIRE(std::holds_alternative<assembly::mov_i_reg_reg>(i));
    const auto& m = std::get<assembly::mov_i_reg_reg>(i);
    REQUIRE(m.dst_ == dst);
    REQUIRE(m.src_ == src);
}

inline void require_mov_f_reg(size_t idx, const assembly::instruction& i, size_t dst, size_t src)
{
    INFO("Instruction " << idx);
    REQUIRE(std::holds_alternative<assembly::mov_f_reg_reg>(i));
    const auto& m = std::get<assembly::mov_f_reg_reg>(i);
    REQUIRE(m.dst_ == dst);
    REQUIRE(m.src_ == src);
}

inline void require_mov_is_reg(size_t idx, const assembly::instruction& i, size_t dst, size_t src)
{
    INFO("Instruction " << idx);
    REQUIRE(std::holds_alternative<assembly::mov_is_reg_reg>(i));
    const auto& m = std::get<assembly::mov_is_reg_reg>(i);
    REQUIRE(m.dst_ == dst);
    REQUIRE(m.src_ == src);
}

inline void require_mov_fs_reg(size_t idx, const assembly::instruction& i, size_t dst, size_t src)
{
    INFO("Instruction " << idx);
    REQUIRE(std::holds_alternative<assembly::mov_fs_reg_reg>(i));
    const auto& m = std::get<assembly::mov_fs_reg_reg>(i);
    REQUIRE(m.dst_ == dst);
    REQUIRE(m.src_ == src);
}

inline void require_call_i_reg(size_t idx, const assembly::instruction& i, size_t fid, size_t res)
{
    INFO("Instruction " << idx);
    REQUIRE(std::holds_alternative<assembly::call_i_reg>(i));
    const auto& c = std::get<assembly::call_i_reg>(i);
    REQUIRE(c.id_ == fid);
    REQUIRE(c.res_ == res);
}

inline void require_call_f_reg(size_t idx, const assembly::instruction& i, size_t fid, size_t res)
{
    INFO("Instruction " << idx);
    REQUIRE(std::holds_alternative<assembly::call_f_reg>(i));
    const auto& c = std::get<assembly::call_f_reg>(i);
    REQUIRE(c.id_ == fid);
    REQUIRE(c.res_ == res);
}

inline void require_call_is_reg(size_t idx, const assembly::instruction& i, size_t fid, size_t res)
{
    INFO("Instruction " << idx);
    REQUIRE(std::holds_alternative<assembly::call_is_reg>(i));
    const auto& c = std::get<assembly::call_is_reg>(i);
    REQUIRE(c.id_ == fid);
    REQUIRE(c.res_ == res);
}

inline void require_call_fs_reg(size_t idx, const assembly::instruction& i, size_t fid, size_t res)
{
    INFO("Instruction " << idx);
    REQUIRE(std::holds_alternative<assembly::call_fs_reg>(i));
    const auto& c = std::get<assembly::call_fs_reg>(i);
    REQUIRE(c.id_ == fid);
    REQUIRE(c.res_ == res);
}
