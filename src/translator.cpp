#include "translator.hpp"
#include "utility.hpp"

#include <span>

namespace lyrid
{

void translator::compute_constant_pool_sizes(const ast::program& prog)
{
    auto count_visit = [&](auto&& self, const ast::expr_wrapper& ew) -> void
    {
        std::visit(overloaded
        {
            [&](const ast::f_call& call) -> void
            {
                for (const auto& a : call.args_)
                    self(self, a);
            },
            [&](const ast::index_access& ia) -> void
            {
                self(self, *ia.base_); 
                self(self, *ia.index_);
            },
            [&](const ast::array_construction& ac) -> void
            {
                for (const auto& e : ac.elements_)
                    self(self, e); 
            },
            [&](const ast::comprehension& comp) -> void
            {
                for (const auto& i : comp.in_exprs_)
                    self(self, i); 
                self(self, *comp.do_expr_);
            },
            [&](const ast::int_scalar&) -> void
            {
                ++program_.const_int_memory_size_;
            },
            [&](const ast::float_scalar&) -> void
            {
                ++program_.const_float_memory_size_;
            },
            [&](const auto&) -> void {}
        }, ew.wrapped_);
    };

    for (const auto& decl : prog.declarations_)
    {
        count_visit(count_visit, decl.value_);
    }
}

void translator::hoist_scalar_constants(ast::program& prog)
{
    program_.const_int_memory_.reserve(program_.const_int_memory_size_);
    program_.const_float_memory_.reserve(program_.const_float_memory_size_);

    auto get_start_offset = [this](type arr_type)
    {
        if (arr_type == type::int_array)
            return program_.const_int_memory_.size();
        else
            return program_.const_float_memory_.size();
    };

    auto add_span_generic = [](auto& memory, auto& spans, size_t offset, size_t len)
    {
        auto begin_it = memory.cbegin() + static_cast<std::ptrdiff_t>(offset);
        auto end_it = begin_it + static_cast<std::ptrdiff_t>(len);
        std::span sp(begin_it, end_it);
        spans.push_back(sp);
        return spans.size() - 1;
    };

    auto add_span = [this, add_span_generic](type arr_type, size_t offset, size_t len)
    {
        if (arr_type == type::int_array)
            return add_span_generic(program_.const_int_memory_, program_.const_int_array_spans_, offset, len);
        else
            return add_span_generic(program_.const_float_memory_, program_.const_float_array_spans_, offset, len);
    };

    auto hoist_visit = [&](auto&& self, ast::expr_wrapper& ew) -> bool
    {
        return std::visit(overloaded
        {
            [&](ast::int_scalar& s) -> bool
            {
                program_.const_int_memory_.push_back(s.value_);
                s.const_memory_idx_ = program_.const_int_memory_.size() - 1;
                return true;
            },
            [&](ast::float_scalar& s) -> bool
            {
                program_.const_float_memory_.push_back(s.value_);
                s.const_memory_idx_ = program_.const_float_memory_.size() - 1;
                return true;
            },
            [&](ast::array_construction& ac) -> bool
            {
                type arr_type = ew.inferred_type_.value();
                size_t start_offset = get_start_offset(arr_type);
                bool valid = true;

                for (auto& e : ac.elements_)
                    valid = self(self, e) && valid;
                
                if (valid)
                {
                    size_t span_idx = add_span(arr_type, start_offset, ac.elements_.size());
                    ac.const_memory_span_idx_ = span_idx;
                }

                return false;
            },
            [&](ast::f_call& call) -> bool
            {
                for (auto& a : call.args_)
                    self(self, a);
                return false;
            },
            [&](ast::index_access& ia) -> bool
            {
                self(self, *ia.base_);
                self(self, *ia.index_);
                return false;
            },
            [&](ast::comprehension& comp) -> bool
            {
                for (auto& i : comp.in_exprs_)
                    self(self, i);
                self(self, *comp.do_expr_);
                return false;
            },
            [&](ast::symbol_ref&) -> bool
            {
                return false;
            }
        }, ew.wrapped_);
    };

    for (auto& decl : prog.declarations_)
    {
        hoist_visit(hoist_visit, decl.value_);
    }
}

void translator::translate(const std::string& source)
{
    errors_.clear();
    program_.const_int_memory_.clear();
    program_.const_float_memory_.clear();
    program_.const_int_array_spans_.clear();
    program_.const_float_array_spans_.clear();
    program_.const_int_memory_size_ = 0;
    program_.const_float_memory_size_ = 0;

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
    {
        sa.register_function_prototype(func.name_, func.proto_.arg_types_, func.proto_.arg_names_, func.proto_.return_type_);
    }

    ast::program& prog = p.get_program();
    sa.analyze(prog);

    const auto& sem_errors = sa.get_errors();
    if (!sem_errors.empty())
    {
        errors_ = sem_errors;
        return;
    }

    compute_constant_pool_sizes(prog);
    hoist_scalar_constants(prog);
}

void translator::register_function(
    std::string name,
    std::vector<type> arg_types,
    std::vector<std::string> arg_names,
    type return_type)
{
    functions_.push_back({ std::move(name), { std::move(arg_types), std::move(arg_names), return_type } });
}

}
