#pragma once

#include <variant>
#include <vector>
#include <span>
#include <cstddef>
#include <utility>
#include <tuple>

namespace lyrid
{

namespace assembly
{

using reg_index = size_t;
using const_index = size_t;
using function_index = size_t;
using span_index = size_t;  // Alias for indices into the mutable/constant span tables

// Scalar moves — integer
struct mov_i_reg_reg
{
    reg_index dst_;
    reg_index src_;
    
    constexpr static auto args = std::tuple{&mov_i_reg_reg::dst_, &mov_i_reg_reg::src_};
};

struct mov_i_reg_const
{
    reg_index dst_;
    const_index src_;
    
    constexpr static auto args = std::tuple{&mov_i_reg_const::dst_, &mov_i_reg_const::src_};
};

// Scalar moves — float
struct mov_f_reg_reg
{
    reg_index dst_;
    reg_index src_;
    
    constexpr static auto args = std::tuple{&mov_f_reg_reg::dst_, &mov_f_reg_reg::src_};
};

struct mov_f_reg_const
{
    reg_index dst_;
    const_index src_;
    
    constexpr static auto args = std::tuple{&mov_f_reg_const::dst_, &mov_f_reg_const::src_};
};

// Span moves — integer arrays
struct mov_is_reg_reg
{
    reg_index dst_;
    reg_index src_;
    
    constexpr static auto args = std::tuple{&mov_is_reg_reg::dst_, &mov_is_reg_reg::src_};
};

struct mov_is_reg_const
{
    reg_index dst_;
    span_index span_idx_;
    
    constexpr static auto args = std::tuple{&mov_is_reg_const::dst_, &mov_is_reg_const::span_idx_};
};

struct mov_is_reg_mut
{
    reg_index dst_;
    span_index span_idx_;
    
    constexpr static auto args = std::tuple{&mov_is_reg_mut::dst_, &mov_is_reg_mut::span_idx_};
};

// Mutable stores — integer elements (into mutable memory)
struct mov_i_mut_const
{
    span_index span_idx_;
    size_t offset_;
    const_index const_src_;
    
    constexpr static auto args = std::tuple{&mov_i_mut_const::span_idx_, &mov_i_mut_const::offset_, &mov_i_mut_const::const_src_};
};

struct mov_i_mut_reg
{
    span_index span_idx_;
    size_t offset_;
    reg_index src_;
    
    constexpr static auto args = std::tuple{&mov_i_mut_reg::span_idx_, &mov_i_mut_reg::offset_, &mov_i_mut_reg::src_};
};

// Span moves — float arrays
struct mov_fs_reg_reg
{
    reg_index dst_;
    reg_index src_;
    
    constexpr static auto args = std::tuple{&mov_fs_reg_reg::dst_, &mov_fs_reg_reg::src_};
};

struct mov_fs_reg_const
{
    reg_index dst_;
    span_index span_idx_;
    
    constexpr static auto args = std::tuple{&mov_fs_reg_const::dst_, &mov_fs_reg_const::span_idx_};
};

struct mov_fs_reg_mut
{
    reg_index dst_;
    span_index span_idx_;
    
    constexpr static auto args = std::tuple{&mov_fs_reg_mut::dst_, &mov_fs_reg_mut::span_idx_};
};

// Mutable stores — float elements (into mutable memory)
struct mov_f_mut_const
{
    span_index span_idx_;
    size_t offset_;
    const_index const_src_;
    
    constexpr static auto args = std::tuple{&mov_f_mut_const::span_idx_, &mov_f_mut_const::offset_, &mov_f_mut_const::const_src_};
};

struct mov_f_mut_reg
{
    span_index span_idx_;
    size_t offset_;
    reg_index src_;
    
    constexpr static auto args = std::tuple{&mov_f_mut_reg::span_idx_, &mov_f_mut_reg::offset_, &mov_f_mut_reg::src_};
};

// Control flow — jumps
struct jmp
{
    size_t target_index_;
    
    constexpr static auto args = std::tuple{&jmp::target_index_};
};

struct jmp_eq_i_reg_reg
{
    reg_index lhs_;
    reg_index rhs_;
    size_t target_index_;
    
    constexpr static auto args = std::tuple{&jmp_eq_i_reg_reg::lhs_, &jmp_eq_i_reg_reg::rhs_, &jmp_eq_i_reg_reg::target_index_};
};

struct jmp_eq_i_reg_const
{
    reg_index lhs_;
    const_index rhs_;
    size_t target_index_;
    
    constexpr static auto args = std::tuple{&jmp_eq_i_reg_const::lhs_, &jmp_eq_i_reg_const::rhs_, &jmp_eq_i_reg_const::target_index_};
};

struct jmp_eq_f_reg_reg
{
    reg_index lhs_;
    reg_index rhs_;
    size_t target_index_;
    
    constexpr static auto args = std::tuple{&jmp_eq_f_reg_reg::lhs_, &jmp_eq_f_reg_reg::rhs_, &jmp_eq_f_reg_reg::target_index_};
};

struct jmp_eq_f_reg_const
{
    reg_index lhs_;
    const_index rhs_;
    size_t target_index_;
    
    constexpr static auto args = std::tuple{&jmp_eq_f_reg_const::lhs_, &jmp_eq_f_reg_const::rhs_, &jmp_eq_f_reg_const::target_index_};
};

// Function calls — scalar-returning
struct call_i_reg
{
    function_index id_;
    reg_index res_;
    
    constexpr static auto args = std::tuple{&call_i_reg::id_, &call_i_reg::res_};
};

struct call_f_reg
{
    function_index id_;
    reg_index res_;
    
    constexpr static auto args = std::tuple{&call_f_reg::id_, &call_f_reg::res_};
};

struct call_i_mut
{
    function_index id_;
    span_index span_idx_;
    size_t offset_;
    
    constexpr static auto args = std::tuple{&call_i_mut::id_, &call_i_mut::span_idx_, &call_i_mut::offset_};
};

struct call_f_mut
{
    function_index id_;
    span_index span_idx_;
    size_t offset_;
    
    constexpr static auto args = std::tuple{&call_f_mut::id_, &call_f_mut::span_idx_, &call_f_mut::offset_};
};

// Function calls — span-returning (array-returning)
struct call_is_reg
{
    function_index id_;
    reg_index res_;
    
    constexpr static auto args = std::tuple{&call_is_reg::id_, &call_is_reg::res_};
};

struct call_fs_reg
{
    function_index id_;
    reg_index res_;
    
    constexpr static auto args = std::tuple{&call_fs_reg::id_, &call_fs_reg::res_};
};

using instruction = std::variant<
    // Scalar moves — integer
    mov_i_reg_reg,
    mov_i_reg_const,

    // Scalar moves — float
    mov_f_reg_reg,
    mov_f_reg_const,

    // Span moves — integer arrays
    mov_is_reg_reg,
    mov_is_reg_const,
    mov_is_reg_mut,

    // Mutable stores — integer elements
    mov_i_mut_const,
    mov_i_mut_reg,

    // Span moves — float arrays
    mov_fs_reg_reg,
    mov_fs_reg_const,
    mov_fs_reg_mut,

    // Mutable stores — float elements
    mov_f_mut_const,
    mov_f_mut_reg,

    // Control flow — jumps
    jmp,
    jmp_eq_i_reg_reg,
    jmp_eq_i_reg_const,
    jmp_eq_f_reg_reg,
    jmp_eq_f_reg_const,

    // Function calls — scalar-returning
    call_i_reg,
    call_f_reg,
    call_i_mut,
    call_f_mut,
    
    // Function calls — span-returning
    call_is_reg,
    call_fs_reg
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
using instructions = std::vector<instruction>;

struct program
{
    instructions instructions_;

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
