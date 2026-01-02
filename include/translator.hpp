#pragma once

#include "parser.hpp"
#include "semantic_analyzer.hpp"
#include "prototype.hpp"
#include "type.hpp"

#include <string>
#include <vector>
#include <span>
#include <optional>

namespace lyrid
{

class translator
{
public:
    translator() = default;

    void translate(const std::string& source);

    void register_function(
        std::string name,
        std::vector<type> arg_types,
        std::vector<std::string> arg_names,
        type return_type);

    const std::vector<std::string>& get_errors() const { return errors_; }

    bool is_valid() const { return errors_.empty(); }

    using const_int_memory = std::vector<int64_t>;
    using const_float_memory = std::vector<double>;
    using indices = std::vector<size_t>;
    using const_int_spans = std::vector<std::span<const int64_t>>;
    using const_float_spans = std::vector<std::span<const double>>;
    
    const const_int_memory& get_const_int_memory() const { return const_int_memory_; }
    const const_float_memory& get_const_float_memory() const { return const_float_memory_; }

    const indices& get_int_scalars() const { return int_scalars_; }
    const indices& get_float_scalars() const { return float_scalars_; }

    const const_int_spans& get_const_int_array_spans() const { return const_int_array_spans_; }
    const const_float_spans& get_const_float_array_spans() const { return const_float_array_spans_; }
    
private:
    struct function
    {
        std::string name_;
        prototype proto_;
    };

    void compute_constant_pool_sizes(const ast::program& prog);

    void hoist_scalar_constants(ast::program& prog);

    size_t const_int_memory_size_ = 0;
    size_t const_float_memory_size_ = 0;

    std::vector<std::string> errors_;
    std::vector<function> functions_;

    const_int_memory const_int_memory_;
    const_float_memory const_float_memory_;

    indices int_scalars_;
    indices float_scalars_;

    const_int_spans const_int_array_spans_;
    const_float_spans const_float_array_spans_;
};

}   
