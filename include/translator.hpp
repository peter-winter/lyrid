// include/translator.hpp (updated)
#pragma once

#include "parser.hpp"
#include "semantic_analyzer.hpp"
#include "prototype.hpp"
#include "type.hpp"
#include "assembly.hpp"

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

    const std::vector<int64_t>& get_const_int_memory() const { return program_.const_int_memory_; }
    const std::vector<double>& get_const_float_memory() const { return program_.const_float_memory_; }

    const std::vector<std::span<const int64_t>>& get_const_int_array_spans() const { return program_.const_int_array_spans_; }
    const std::vector<std::span<const double>>& get_const_float_array_spans() const { return program_.const_float_array_spans_; }

    // New accessor for the generated assembly program (optional, for future use)
    const assembly::program& get_program() const { return program_; }
    assembly::program& get_program() { return program_; }

private:
    struct function
    {
        std::string name_;
        prototype proto_;
    };

    void compute_constant_pool_sizes(const ast::program& prog);

    void hoist_scalar_constants(ast::program& prog);

    std::vector<std::string> errors_;
    std::vector<function> functions_;

    assembly::program program_;
};

}
