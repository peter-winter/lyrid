#pragma once

namespace lyrid
{
    enum class memory_type : size_t { mem_const = 0, mem_mutable = 1 };
    
    struct memory_span_annotation
    {
        memory_type type_;
        size_t idx_;
    };
    
    struct memory_cell_annotation
    {
        memory_type type_;
        size_t idx_;
    };
    
}
