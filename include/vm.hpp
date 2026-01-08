#pragma once

#include <variant>
#include <vector>
#include <limits>
#include <limits>

#include "type.hpp"

namespace lyrid
{

struct int_pool
{
};

struct float_pool
{
};

using pool_tag = std::variant<int_pool, float_pool>;

struct noop
{};

using const_value = std::variant<int_scalar_type::value_type, float_scalar_type::value_type>;

struct place_const
{
    size_t block_id_;
    size_t offset_;
    const_value value_;
};

struct copy_block
{
    pool_tag pool_;
    size_t src_block_id_;
    size_t target_block_id_;
};

struct copy_element
{
    pool_tag pool_;
    size_t src_block_id_;
    size_t src_offset_;
    size_t target_block_id_;
    size_t target_offset_;
};

struct call_arg
{
    pool_tag pool_;
    size_t block_id_;
};

struct call_func
{
    size_t func_idx_;
    std::vector<call_arg> args_;
    call_arg result_;
};

using instruction = std::variant<
    noop,
    place_const,
    copy_block,
    copy_element,
    call_func
>;

constexpr int_scalar_type::value_type int_sentinel = 0xDEADBEEFDEADBEEFULL;
constexpr float_scalar_type::value_type float_sentinel = std::numeric_limits<float_scalar_type::value_type>::quiet_NaN();

struct block_meta
{
    size_t offset_ = 0;
    size_t len_ = 0;
};

using instructions = std::vector<instruction>;

class vm
{
public:
    size_t allocate_block(pool_tag p, size_t len);
    void add_instruction(instruction&& ins);
    
private:
    size_t get_pool_size(pool_tag p, size_t len) const;
    
    std::vector<int_scalar_type::value_type> int_pool_;
    std::vector<float_scalar_type::value_type> float_pool_;
    std::vector<block_meta> block_metadata_;
    
    instructions all_instructions_;
    instructions runtime_instructions_;
};

}
