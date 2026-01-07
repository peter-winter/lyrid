#include "translator.hpp"
#include "utility.hpp"

#include <utility>
#include <limits>
#include <cmath>

namespace lyrid
{

using namespace assembly;
using namespace ast;

constexpr int64_t int_sentinel = 0xDEADBEEFDEADBEEFLL;
constexpr double float_sentinel = 0.123456789;

pool translator::get_scalar_pool(scalar_type t)
{
    return std::visit(
        overloaded
        {
            [](int_scalar_type) -> pool { return int_pool{}; },
            [](float_scalar_type) -> pool { return float_pool{}; }
        },
        t
    );  
}

pool translator::get_pool(type t)
{
    return std::visit(
        overloaded
        {
            [](int_scalar_type) -> pool { return int_pool{}; },
            [](float_scalar_type) -> pool { return float_pool{}; },
            [](array_type ar) -> pool { return get_scalar_pool(ar.sc_); }
        },
        t
    );
}

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

size_t translator::get_memory_size(pool p) const
{
    return std::visit(overloaded
    {
        [&](int_pool) { return program_.int_memory_.size(); },
        [&](float_pool) { return program_.float_memory_.size(); }
    },
    p);
}

void translator::push_sentinel(pool p)
{
    std::visit(overloaded
    {
        [&](int_pool) { program_.int_memory_.push_back(int_sentinel); },
        [&](float_pool) { program_.float_memory_.push_back(float_sentinel); }
    },
    p);
}

void translator::push_sentinels(pool p, size_t count)
{
    if (count == 0)
        return;

    size_t old_size = get_memory_size(p);
    
    std::visit(overloaded
    {
        [&](int_pool) { program_.int_memory_.resize(old_size + count, int_sentinel); },
        [&](float_pool) { program_.float_memory_.resize(old_size + count, float_sentinel); }
    },
    p);
}

std::optional<size_t> translator::get_span_offset(storage_annotation annot) const
{
    return std::visit(overloaded
    {
        [](scalar_offset) -> std::optional<size_t> { return {}; },
        [](span_offset s) -> std::optional<size_t> { return s.value_; }
    },
    annot);
}

void translator::layout_exprs(ast::program& prog_ast)
{
    auto resolve_span_offset = [&](auto&& self, const expr_wrapper& ew) -> std::optional<size_t>
    {
        if (!ew.inferred_type_.has_value() || !is_array_type(*ew.inferred_type_))
            return {};

        return std::visit(overloaded
        {
            [](const array_construction& ac) -> std::optional<size_t>
            {
                return ac.span_offset_in_int_memory_;
            },
            [](const comprehension& comp) -> std::optional<size_t>
            {
                return comp.span_offset_in_int_memory_;
            },
            [&](const f_call& call) -> std::optional<size_t>
            {
                return call.result_storage_.and_then([this](const auto& annot)
                {
                    return get_span_offset(annot);
                });
            },
            [&](const symbol_ref& ref) -> std::optional<size_t>
            {
                return ref.declaration_idx_.and_then([&](size_t idx)
                {
                    const declaration& decl = prog_ast.declarations_[idx];
                    return self(self, decl.value_);
                });
            },
            [](const auto&) -> std::optional<size_t>
            {
                return {};
            }
        },
        ew.wrapped_);
    };
    
    auto resolve_scalar_offset = [&](auto&& self, const expr_wrapper& ew) -> std::optional<size_t>
    {
        if (!ew.inferred_type_.has_value() || is_array_type(*ew.inferred_type_))
            return {};

        return std::visit(overloaded
        {
            [](const int_scalar& lit) -> std::optional<size_t>
            {
                return lit.scalar_offset_in_int_memory_;
            },
            [](const float_scalar& lit) -> std::optional<size_t>
            {
                return lit.scalar_offset_in_float_memory_;
            },
            [&](const symbol_ref& ref) -> std::optional<size_t>
            {
                return ref.declaration_idx_.and_then([&](size_t idx)
                {
                    const declaration& decl = prog_ast.declarations_[idx];
                    return self(self, decl.value_);
                });
            },
            [&](const index_access& ia) -> std::optional<size_t>
            {
                auto base_span_opt = resolve_span_offset(resolve_span_offset, *ia.base_);
                if (!base_span_opt.has_value())
                    return {};

                size_t span_off = *base_span_opt;
                size_t data_off = program_.int_memory_[span_off];

                auto index_offset_opt = self(self, *ia.index_);
                if (!index_offset_opt.has_value())
                    return {};

                // Index is resolved to hoisted literal offset; load the literal value
                int64_t index_val = program_.int_memory_[*index_offset_opt];
                int64_t length = program_.int_memory_[span_off + 1];
                
                return data_off + normalized_modulo(index_val, length);
            },
            [](const auto&) -> std::optional<size_t>
            {
                return {};
            }
        },
        ew.wrapped_);
    };

    auto layout_expr = [&](auto&& self, expr_wrapper& ew)
    {
        if (!expect(ew.inferred_type_.has_value(), ew.loc_, "Expression has no inferred type"))
            return;
     
        type t = *ew.inferred_type_;

        std::visit(overloaded
        {
            [&] (int_scalar& lit)
            {
                size_t off = program_.int_memory_.size();
                program_.int_memory_.push_back(lit.value_);
                lit.scalar_offset_in_int_memory_ = off;
            },
            [&] (float_scalar& lit)
            {
                size_t off = program_.float_memory_.size();
                program_.float_memory_.push_back(lit.value_);
                lit.scalar_offset_in_float_memory_ = off;
            },
            [&] (symbol_ref&)
            {
                // Resolution deferred
            },
            [&] (f_call& call)
            {
                if (!expect(call.fn_.proto_idx_.has_value(), ew.loc_, "Unresolved function call"))
                    return;
                
                for (auto& arg : call.args_)
                    self(self, arg);
                
                const prototype& proto = functions_[*call.fn_.proto_idx_].proto_;
                type rt = proto.return_type_;
                                
                if (is_scalar_type(rt))
                {
                    pool p = get_pool(rt);
                    size_t off = get_memory_size(p);
                    push_sentinel(p);
                    call.result_storage_ = scalar_offset{off};
                }
                else
                {
                    array_type ar = std::get<array_type>(rt);
                    if (!expect(ar.fixed_length_.has_value(), ew.loc_, "Array return type must have fixed length"))
                        return;
                    
                    size_t len = ar.fixed_length_.value();
                    pool p = get_pool(rt);

                    size_t data_start = get_memory_size(p);
                    push_sentinels(p, len);

                    size_t span_start = program_.int_memory_.size();
                    program_.int_memory_.push_back(data_start);
                    program_.int_memory_.push_back(static_cast<int64_t>(len));

                    call.result_storage_ = span_offset{span_start};
                }
            },
            [&] (array_construction& ac)
            {
                size_t len = ac.elements_.size();
                pool p = get_pool(t);

                size_t data_start = get_memory_size(p);
                push_sentinels(p, len);

                for (size_t i = 0; i < len; ++i)
                {
                    expr_wrapper& elem = ac.elements_[i];

                    std::visit(overloaded
                    {
                        [&](int_scalar& lit)
                        {
                            program_.int_memory_[data_start + i] = lit.value_;
                            lit.scalar_offset_in_int_memory_ = data_start + i;
                        },
                        [&](float_scalar& lit)
                        {
                            program_.float_memory_[data_start + i] = lit.value_;
                            lit.scalar_offset_in_float_memory_ = data_start + i;
                        },
                        [&](auto&)
                        {
                            self(self, elem);
                        }
                    },
                    elem.wrapped_);
                }

                size_t span_start = program_.int_memory_.size();
                program_.int_memory_.push_back(data_start);
                program_.int_memory_.push_back(static_cast<int64_t>(len));

                ac.span_offset_in_int_memory_ = span_start;
            },
            [&] (index_access& ia)
            {
                self(self, *ia.base_);
                self(self, *ia.index_);

                auto span_opt = resolve_span_offset(resolve_span_offset, *ia.base_);
                if (!expect(span_opt.has_value(), ew.loc_, "Index access base has no resolved span offset"))
                    return;
                
                std::visit(overloaded
                {
                    [&](int_scalar& lit)
                    {
                        size_t span_off = *span_opt;
                        size_t data_off = program_.int_memory_[span_off];

                        int64_t index_val = lit.value_;
                        int64_t length = program_.int_memory_[span_off + 1];
                        size_t elem_off = data_off + normalized_modulo(index_val, length);

                        ia.storage_ = direct_storage{elem_off};
                    },
                    [&](auto&)
                    {
                        pool element_pool = get_pool(t);
                        size_t dst_off = get_memory_size(element_pool);
                        push_sentinel(element_pool);

                        ia.storage_ = indirect_storage{dst_off};
                    }
                },
                ia.index_->wrapped_);
            },
            [&] (comprehension&)
            {
                translation_error(ew.loc_, "Array comprehensions not yet supported in layout");
            },
            [&] (const auto&)
            {
                translation_error(ew.loc_, "Unsupported expression type in layout");
            }
        },
        ew.wrapped_);
    };

    for (auto& decl : prog_ast.declarations_)
    {
        layout_expr(layout_expr, decl.value_);
    }
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
    
    //layout_exprs(prog_ast);
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
