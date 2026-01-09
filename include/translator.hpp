#pragma once

#include "parser.hpp"
#include "semantic_analyzer.hpp"
#include "prototype.hpp"
#include "type.hpp"
#include "vm.hpp"

#include <string>
#include <vector>
#include <utility>
#include <limits>

namespace lyrid
{

struct function
{
    std::string name_;
    prototype proto_;
};

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
    
private:
    void translation_error(const ast::source_location& loc, const std::string& message);
    bool expect(bool condition, const ast::source_location& loc, const std::string& message);
    pool_tag get_pool(type t);
    std::optional<size_t> get_length(type t);
    data get_arg_data(type t, size_t arg_block_id);

    void create_vm(const ast::program& prog_ast);
    
    void create_top_assignment_instruction(const ast::expr_wrapper& ew);
    void create_array_element_fill_instructions(const ast::array_construction& ac, size_t target_block_id);
    void create_array_element_fill_instruction(const ast::expr_wrapper& ew, size_t target_block_id, size_t offset);
    void create_call_instruction(const ast::f_call& fc, ast::source_location loc, type t, data target);
    
    bool expect_valid_symbol_ref(const ast::symbol_ref& sr, ast::source_location loc);
    bool expect_valid_type(std::optional<type> t, ast::source_location loc);
    bool expect_valid_fcall(const ast::f_call& fc, ast::source_location loc);
    
    std::vector<std::string> errors_;
    std::vector<function> functions_;
    vm vm_;
    std::vector<size_t> assignments_block_ids_;
};

}
