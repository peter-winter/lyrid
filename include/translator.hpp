#pragma once

#include "parser.hpp"
#include "semantic_analyzer.hpp"
#include "prototype.hpp"
#include "type.hpp"
#include "assembly.hpp"

#include <string>
#include <vector>
#include <span>
#include <utility>

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

    size_t get_int_memory_size() const { return program_.int_memory_.size(); }
    size_t get_float_memory_size() const { return program_.float_memory_.size(); }
    
    const assembly::program& get_program() const { return program_; }
    assembly::program& get_program() { return program_; }
    
private:
    constexpr static size_t should_be_more_than_enough_memory = (1 << 13); // 8KB
    
    struct function
    {
        std::string name_;
        prototype proto_;
    };

    static assembly::pool get_scalar_pool(scalar_type t);
    static assembly::pool get_pool(type t);
    
    void translation_error(const ast::source_location& loc, const std::string& message);
    bool expect(bool condition, const ast::source_location& loc, const std::string& message);

    std::vector<std::string> errors_;
    std::vector<function> functions_;
    assembly::program program_;
};

}
