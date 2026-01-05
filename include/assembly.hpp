#pragma once

#include <variant>
#include <vector>
#include <span>
#include <cstddef>
#include <utility>

namespace lyrid
{

namespace assembly
{

using reg_index = size_t;
using const_index = size_t;
using function_id = size_t;

struct mov_i_reg_reg
{
    reg_index dst_;
    reg_index src_;
};

struct mov_i_reg_const
{
    reg_index dst_;
    const_index src_;
};

struct mov_f_reg_reg
{
    reg_index dst_;
    reg_index src_;
};

struct mov_f_reg_const
{
    reg_index dst_;
    const_index src_;
};

struct mov_is_reg_reg
{
    reg_index dst_;
    reg_index src_;
};

struct mov_is_reg_const
{
    reg_index dst_;
    const_index src_;
};

struct mov_fs_reg_reg
{
    reg_index dst_;
    reg_index src_;
};

struct mov_fs_reg_const
{
    reg_index dst_;
    const_index src_;
};

struct jmp
{
    size_t target_index_;
};

struct jmp_eq_i_reg_reg
{
    reg_index lhs_;
    reg_index rhs_;
    size_t target_index_;
};

struct jmp_eq_i_reg_const
{
    reg_index lhs_;
    const_index rhs_;
    size_t target_index_;
};

struct jmp_eq_f_reg_reg
{
    reg_index lhs_;
    reg_index rhs_;
    size_t target_index_;
};

struct jmp_eq_f_reg_const
{
    reg_index lhs_;
    const_index rhs_;
    size_t target_index_;
};

struct call_i
{
    function_id id_;
    reg_index res_;
};

struct call_f
{
    function_id id_;
    reg_index res_;
};

struct scall_i
{
    function_id id_;
    reg_index res_;
};

struct scall_f
{
    function_id id_;
    reg_index res_;
};

using instruction = std::variant<
    mov_i_reg_reg,
    mov_i_reg_const,
    mov_f_reg_reg,
    mov_f_reg_const,
    mov_is_reg_reg,
    mov_is_reg_const,
    mov_fs_reg_reg,
    mov_fs_reg_const,
    jmp,
    jmp_eq_i_reg_reg,
    jmp_eq_i_reg_const,
    jmp_eq_f_reg_reg,
    jmp_eq_f_reg_const,
    call_i,
    call_f,
    scall_i,
    scall_f
>;

using const_int_memory = std::vector<int64_t>;
using const_float_memory = std::vector<double>;
using const_int_spans = std::vector<std::span<const int64_t>>;
using const_float_spans = std::vector<std::span<const double>>;

struct span
{
    size_t offset_;
    size_t len_;
};

using array_spans = std::vector<span>;
    
struct program
{
    std::vector<instruction> instructions_;

    size_t mutable_int_memory_size_ = 0;
    size_t mutable_float_memory_size_ = 0;
    
    const_int_memory const_int_memory_;
    const_float_memory const_float_memory_;
    const_int_spans const_int_array_memory_spans_;
    const_float_spans const_float_array_memory_spans_;

    array_spans int_array_spans_[2];
    array_spans float_array_spans_[2];
    
    const auto& get_int_array_spans(memory_type mt) const { return int_array_spans_[std::to_underlying(mt)]; }
    const auto& get_float_array_spans(memory_type mt) const { return float_array_spans_[std::to_underlying(mt)]; }
    auto& get_int_array_spans(memory_type mt) { return int_array_spans_[std::to_underlying(mt)]; }
    auto& get_float_array_spans(memory_type mt) { return float_array_spans_[std::to_underlying(mt)]; }
};

}

}
