#include "translator.hpp"
#include "utility.hpp"

#include <limits>
#include <cmath>

namespace lyrid
{

using namespace ast;


bool translator::expect(bool condition, const source_location& loc, const std::string& message)
{
    if (!condition)
        translation_error(loc, message);

    return condition;
}

void translator::translation_error(const source_location& loc, const std::string& message)
{
    std::string prefix = "Translation error [" + std::to_string(loc.line_) + ":" +
                         std::to_string(loc.column_) + "]: ";
    errors_.emplace_back(prefix + message);
}

void translator::translate(const std::string& source)
{
    errors_.clear();

    parser p;
    p.parse(source);

    const auto& parse_errors = p.get_errors();
    if (!parse_errors.empty())
    {
        errors_ = parse_errors;
        return;
    }

    semantic_analyzer sa;

    for (const auto& func : functions_)
        sa.register_function_prototype(func.name_, func.proto_.arg_types_, func.proto_.arg_names_, func.proto_.return_type_);

    ast::program& prog_ast = p.get_program();
    sa.analyze(prog_ast);

    const auto& sem_errors = sa.get_errors();
    if (!sem_errors.empty())
    {
        errors_ = sem_errors;
        return;
    }
    
    create_vm(prog_ast);
}

void translator::register_function(
    std::string name,
    std::vector<type> arg_types,
    std::vector<std::string> arg_names,
    type return_type)
{
    functions_.push_back({ std::move(name), { std::move(arg_types), std::move(arg_names), return_type } });
}

pool_tag translator::get_pool(type t)
{
    return std::visit(
        overloaded
        {
            [&](int_scalar_type) -> pool_tag { return int_pool{}; },
            [&](float_scalar_type) -> pool_tag { return float_pool{}; },
            [&](array_type x) -> pool_tag { return std::visit([&](auto t){ return get_pool(type{t}); }, x.sc_); }
        },
        t
    );
}

std::optional<size_t> translator::get_length(type t)
{
    return std::visit(
        overloaded
        {
            [&](int_scalar_type) -> std::optional<size_t> { return 1; },
            [&](float_scalar_type) -> std::optional<size_t> { return 1; },
            [](array_type x) -> std::optional<size_t> { return x.fixed_length_; }
        },
        t
    );
}

bool translator::expect_valid_symbol_ref(const symbol_ref& sr, ast::source_location loc)
{
    return expect(sr.declaration_idx_.has_value(), loc, "Unresolved symbol") &&
        expect(sr.declaration_idx_.value() < assignments_block_ids_.size(), loc, "Invalid declaration idx");
}

bool translator::expect_valid_type(std::optional<type> t, ast::source_location loc)
{
    return expect(t.has_value(), loc, "No inferred type") &&
        expect(get_length(*t).has_value(), loc, "No fixed length in type");
}

bool translator::expect_valid_fcall(const f_call& fc, ast::source_location loc)
{
    return expect(fc.fn_.proto_idx_.has_value(), loc, "Unresolved function");
}

void translator::create_array_element_fill_instruction(const ast::expr_wrapper& ew, size_t target_block_id, size_t offset)
{
    if (!expect_valid_type(ew.inferred_type_, ew.loc_))
        return;
        
    type t = *ew.inferred_type_;
    
    return std::visit(overloaded
    {
        [&](const int_scalar& lit)
        {
            vm_.add_instruction(place_const_int{target_block_id, offset, lit.value_});
        },
        [&](const float_scalar& lit) 
        {
            vm_.add_instruction(place_const_float{target_block_id, offset, lit.value_});
        },
        [&](const symbol_ref& sr)
        {                
            if (!expect_valid_symbol_ref(sr, ew.loc_))
                return;
                
            size_t source_block_id = assignments_block_ids_[*sr.declaration_idx_];
            
            std::visit(
                overloaded
                {
                    [&](int_scalar_type i) { vm_.add_instruction(copy_element_int{source_block_id, 0, target_block_id, offset}); },
                    [&](float_scalar_type f) { vm_.add_instruction(copy_element_float{source_block_id, 0, target_block_id, offset}); }
                },
                get_scalar_type(t)
            );
        },
        [&](const f_call& fc)
        {
            create_call_instructions(fc, ew.loc_, t, target_block_id);
        },
        [&](const index_access&)
        {
            translation_error(ew.loc_, "Indexing not yet supported in code generation");
        },
        [&](const auto&){}
    }, ew.wrapped_);
}

void translator::create_call_instructions(const f_call& fc, source_location loc, type t, size_t target_block_id)
{
    if (!expect_valid_fcall(fc, loc))
        return;
            
    std::vector<call_arg> args;
    
    for (const auto& a : fc.args_)
    {
        if (!expect_valid_type(a.inferred_type_, a.loc_))
            return;
            
        type arg_type = *a.inferred_type_;
        size_t len = get_length(arg_type).value();
        size_t arg_block_id = vm_.allocate_block(get_pool(arg_type), len);
        create_assignment_instructions(a, arg_block_id, len);
    }
    
    vm_.add_instruction(call_func{fc.fn_.proto_idx_.value(), args, call_arg{get_pool(t), target_block_id}});
}

void translator::create_assignment_instructions(const ast::expr_wrapper& ew, size_t target_block_id, size_t target_block_len)
{
    if (!expect_valid_type(ew.inferred_type_, ew.loc_))
        return;
        
    type t = *ew.inferred_type_;
        
    std::visit(overloaded
    {
        [&](const int_scalar& lit)
        {
            return vm_.add_instruction(place_const_int{target_block_id, 0, lit.value_});
        },
        [&](const float_scalar& lit)
        {
            return vm_.add_instruction(place_const_float{target_block_id, 0, lit.value_});
        },
        [&](const symbol_ref& sr)
        {                
            if (!expect_valid_symbol_ref(sr, ew.loc_))
                return;
                
            vm_.add_instruction(copy_block{assignments_block_ids_[*sr.declaration_idx_], target_block_id});
        },
        [&](const f_call& fc)
        {
            create_call_instructions(fc, ew.loc_, t, target_block_id);
        },
        [&](const index_access&)
        {
            translation_error(ew.loc_, "Indexing not yet supported in code generation");
        },
        [&](const array_construction& ac)
        {
            for (size_t i = 0; i < ac.elements_.size(); ++i)
            {
                const auto& el = ac.elements_[i];
                create_array_element_fill_instruction(el, target_block_id, i);
            }
        },
        [&](const comprehension&)
        {
            translation_error(ew.loc_, "Array comprehensions not yet supported in code generation");
        }
    }, ew.wrapped_);
}

void translator::create_vm(const ast::program& prog_ast)
{
    for (const auto& decl : prog_ast.declarations_)
    {   
        if (!expect_valid_type(decl.type_, decl.loc_))
            break;
            
        size_t len = get_length(decl.type_).value();
        size_t block_id = vm_.allocate_block(get_pool(decl.type_), len);
        create_assignment_instructions(decl.value_, block_id, len);
        
        assignments_block_ids_.push_back(block_id);
    }
}


}
