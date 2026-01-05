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

    
    const assembly::const_int_memory& get_const_int_memory() const { return program_.const_int_memory_; }
    const assembly::const_float_memory& get_const_float_memory() const { return program_.const_float_memory_; }

    const assembly::const_int_spans& get_const_int_array_spans() const { return program_.const_int_array_memory_spans_; }
    const assembly::const_float_spans& get_const_float_array_spans() const { return program_.const_float_array_memory_spans_; }

    const assembly::array_spans& get_int_array_spans(memory_type mt) const { return program_.get_int_array_spans(mt); }
    const assembly::array_spans& get_float_array_spans(memory_type mt) const { return program_.get_float_array_spans(mt); }

    size_t get_mutable_int_memory_size() const { return program_.mutable_int_memory_size_; }
    size_t get_mutable_float_memory_size() const { return program_.mutable_float_memory_size_; }
    
    const assembly::program& get_program() const { return program_; }
    assembly::program& get_program() { return program_; }
    
private:
    constexpr static size_t should_be_more_than_enough_constants = 256;
    constexpr static size_t should_be_more_than_enough_registers = 256;
    
    struct function
    {
        std::string name_;
        prototype proto_;
    };

    void prepare_memory_model(ast::program& prog);
    
    enum class reg_file : size_t { i_scalar = 0, f_scalar = 1, i_span = 2, f_span = 3 };
    
    static reg_file get_span_reg_file(scalar_type t);
    static reg_file get_reg_file(type t);
    
    void translation_error(const ast::source_location& loc, const std::string& message);
    bool expect(bool condition, const ast::source_location& loc, const std::string& message);

    void compute_global_max_args();

    void emit_declarations(const ast::program& prog);
    void emit_expr(const ast::expr_wrapper& ew, reg_file target_reg_file, size_t target_reg_idx);
    void emit_move_const_src(reg_file file, size_t dst_reg_idx, size_t src_const_idx);
    void emit_move_reg_src(reg_file file, size_t dst_reg_idx, size_t src_reg_idx);
    void emit_call(size_t proto_idx, reg_file res_reg_file, size_t res_reg_idx);
    
    std::vector<std::string> errors_;
    std::vector<function> functions_;
    assembly::program program_;
    size_t global_max_args_ = 0;
    std::vector<std::pair<reg_file, size_t>> decl_regs_;
};

}
