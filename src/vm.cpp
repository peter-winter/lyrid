#include "vm.hpp"

#include <algorithm>

namespace lyrid
{

size_t vm::get_pool_size(pool_tag p, size_t len) const
{
    return std::visit(overloaded
        {
            [&](int_pool) { return int_pool_.size(); },
            [&](float_pool) { return float_pool_.size(); }
        },
    p);
}

size_t vm::allocate_block(pool_tag p, size_t len)
{
    size_t size = get_pool_size(p);
    
    std::visit(overloaded
        {
            [&](int_pool) { return int_pool_.resize(size + len, int_sentinel); },
            [&](float_pool) { return float_pool_.resize(size + len, float_sentinel); }
        },
    p);
    
    size_t res = block_metadata_.size();
    block_metadata_.push_back({ size, len });
    
    return res;
}

void vm::add_instructions(instruction&& ins)
{
    all_instructions_.emplace_back(std::move(ins));
}

}

