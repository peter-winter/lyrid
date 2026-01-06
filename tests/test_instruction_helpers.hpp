#pragma once

#include <cstddef>

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>

#include "assembly.hpp"

using namespace lyrid;
using namespace lyrid::assembly;

inline void require_mov_i_reg_const(size_t idx, const instructions& ins, size_t dst, size_t src_const)
{
    INFO("Instruction " << idx);
    REQUIRE(std::holds_alternative<mov_i_reg_const>(ins[idx]));
    const auto& m = std::get<mov_i_reg_const>(ins[idx]);
    REQUIRE(m.dst_ == dst);
    REQUIRE(m.src_ == src_const);
}

inline void require_mov_f_reg_const(size_t idx, const instructions& ins, size_t dst, size_t src_const)
{
    INFO("Instruction " << idx);
    REQUIRE(std::holds_alternative<mov_f_reg_const>(ins[idx]));
    const auto& m = std::get<mov_f_reg_const>(ins[idx]);
    REQUIRE(m.dst_ == dst);
    REQUIRE(m.src_ == src_const);
}

inline void require_mov_is_reg_const(size_t idx, const instructions& ins, size_t dst, size_t src_const)
{
    INFO("Instruction " << idx);
    REQUIRE(std::holds_alternative<mov_is_reg_const>(ins[idx]));
    const auto& m = std::get<mov_is_reg_const>(ins[idx]);
    REQUIRE(m.dst_ == dst);
    REQUIRE(m.span_idx_ == src_const);
}

inline void require_mov_fs_reg_const(size_t idx, const instructions& ins, size_t dst, size_t src_const)
{
    INFO("Instruction " << idx);
    REQUIRE(std::holds_alternative<mov_fs_reg_const>(ins[idx]));
    const auto& m = std::get<mov_fs_reg_const>(ins[idx]);
    REQUIRE(m.dst_ == dst);
    REQUIRE(m.span_idx_ == src_const);
}

inline void require_mov_i_reg_reg(size_t idx, const instructions& ins, size_t dst, size_t src)
{
    INFO("Instruction " << idx);
    REQUIRE(std::holds_alternative<mov_i_reg_reg>(ins[idx]));
    const auto& m = std::get<mov_i_reg_reg>(ins[idx]);
    REQUIRE(m.dst_ == dst);
    REQUIRE(m.src_ == src);
}

inline void require_mov_f_reg_reg(size_t idx, const instructions& ins, size_t dst, size_t src)
{
    INFO("Instruction " << idx);
    REQUIRE(std::holds_alternative<mov_f_reg_reg>(ins[idx]));
    const auto& m = std::get<mov_f_reg_reg>(ins[idx]);
    REQUIRE(m.dst_ == dst);
    REQUIRE(m.src_ == src);
}

inline void require_mov_is_reg_reg(size_t idx, const instructions& ins, size_t dst, size_t src)
{
    INFO("Instruction " << idx);
    REQUIRE(std::holds_alternative<mov_is_reg_reg>(ins[idx]));
    const auto& m = std::get<mov_is_reg_reg>(ins[idx]);
    REQUIRE(m.dst_ == dst);
    REQUIRE(m.src_ == src);
}

inline void require_mov_fs_reg_reg(size_t idx, const instructions& ins, size_t dst, size_t src)
{
    INFO("Instruction " << idx);
    REQUIRE(std::holds_alternative<mov_fs_reg_reg>(ins[idx]));
    const auto& m = std::get<mov_fs_reg_reg>(ins[idx]);
    REQUIRE(m.dst_ == dst);
    REQUIRE(m.src_ == src);
}

inline void require_call_i_reg(size_t idx, const instructions& ins, size_t fid, size_t res)
{
    INFO("Instruction " << idx);
    REQUIRE(std::holds_alternative<call_i_reg>(ins[idx]));
    const auto& c = std::get<call_i_reg>(ins[idx]);
    REQUIRE(c.id_ == fid);
    REQUIRE(c.res_ == res);
}

inline void require_call_f_reg(size_t idx, const instructions& ins, size_t fid, size_t res)
{
    INFO("Instruction " << idx);
    REQUIRE(std::holds_alternative<call_f_reg>(ins[idx]));
    const auto& c = std::get<call_f_reg>(ins[idx]);
    REQUIRE(c.id_ == fid);
    REQUIRE(c.res_ == res);
}

inline void require_call_is_reg(size_t idx, const instructions& ins, size_t fid, size_t res)
{
    INFO("Instruction " << idx);
    REQUIRE(std::holds_alternative<call_is_reg>(ins[idx]));
    const auto& c = std::get<call_is_reg>(ins[idx]);
    REQUIRE(c.id_ == fid);
    REQUIRE(c.res_ == res);
}

inline void require_call_fs_reg(size_t idx, const instructions& ins, size_t fid, size_t res)
{
    INFO("Instruction " << idx);
    REQUIRE(std::holds_alternative<call_fs_reg>(ins[idx]));
    const auto& c = std::get<call_fs_reg>(ins[idx]);
    REQUIRE(c.id_ == fid);
    REQUIRE(c.res_ == res);
}
