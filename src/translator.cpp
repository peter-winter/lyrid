#include "translator.hpp"
#include "utility.hpp"

#include <span>
#include <utility>

namespace lyrid
{

translator::reg_file translator::get_reg_file(type t)
{
    switch (t)
    {
        case type::int_scalar:  return reg_file::i_scalar;
        case type::float_scalar:return reg_file::f_scalar;
        case type::int_array:   return reg_file::i_span;
        case type::float_array: return reg_file::f_span;
    }
    std::unreachable();
}

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

                return valid;
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

    program_.const_int_memory_size_ = program_.const_int_memory_.size();
    program_.const_float_memory_size_ = program_.const_float_memory_.size();
}

bool translator::expect(bool condition, const ast::source_location& loc, const std::string& message)
{
    if (!condition)
        translation_error(loc, message);

    return condition;
}

void translator::translation_error(const ast::source_location& loc, const std::string& message)
{
    std::string prefix = "Translation error [" + std::to_string(loc.line_) + ":" +
                         std::to_string(loc.column_) + "]: ";
    errors_.emplace_back(prefix + message);
}

void translator::compute_global_max_args()
{
    std::array<size_t, 4> max_per_file{0, 0, 0, 0};

    for (const auto& f : functions_)
    {
        std::array<size_t, 4> counts{0, 0, 0, 0};
        for (type at : f.proto_.arg_types_)
        {
            reg_file rf = get_reg_file(at);
            ++counts[static_cast<size_t>(rf)];
        }
        for (size_t i = 0; i < 4; ++i)
        {
            max_per_file[i] = std::max(max_per_file[i], counts[i]);
        }
    }

    global_max_args_ = 0;
    for (size_t m : max_per_file)
        global_max_args_ = std::max(global_max_args_, m);
}

void translator::emit_declarations(const ast::program& prog)
{
    decl_regs_.clear();
    decl_regs_.reserve(prog.declarations_.size());

    std::array<size_t, 4> next_reg{};
    next_reg.fill(global_max_args_);

    for (size_t i = 0; i < prog.declarations_.size(); ++i)
    {
        const auto& decl = prog.declarations_[i];
        reg_file reg_file = get_reg_file(decl.type_);
        size_t reg_idx = next_reg[static_cast<size_t>(reg_file)]++;
        decl_regs_.emplace_back(reg_file, reg_idx);

        emit_expr(decl.value_, reg_file, reg_idx);
    }
}

void translator::emit_move_const_src(reg_file file, size_t dst_reg_idx, size_t src_const_idx)
{
    switch (file)
    {
        case reg_file::i_scalar:
            program_.instructions_.emplace_back(assembly::mov_i_reg_const{dst_reg_idx, src_const_idx});
            return;
        case reg_file::f_scalar:
            program_.instructions_.emplace_back(assembly::mov_f_reg_const{dst_reg_idx, src_const_idx});
            return;
        case reg_file::i_span:
            program_.instructions_.emplace_back(assembly::smov_i_reg_const{dst_reg_idx, src_const_idx});
            return;
        case reg_file::f_span:
            program_.instructions_.emplace_back(assembly::smov_f_reg_const{dst_reg_idx, src_const_idx});
            return;
    }
    std::unreachable();
}

void translator::emit_move_reg_src(reg_file file, size_t dst_reg_idx, size_t src_reg_idx)
{
    bool is_scalar = (file == reg_file::i_scalar || file == reg_file::f_scalar);
    bool is_int    = (file == reg_file::i_scalar || file == reg_file::i_span);

    if (is_scalar)
    {
        if (is_int)
            program_.instructions_.emplace_back(assembly::mov_i_reg_reg{dst_reg_idx, src_reg_idx});
        else
            program_.instructions_.emplace_back(assembly::mov_f_reg_reg{dst_reg_idx, src_reg_idx});
    }
    else
    {
        if (is_int)
            program_.instructions_.emplace_back(assembly::smov_i_reg_reg{dst_reg_idx, src_reg_idx});
        else
            program_.instructions_.emplace_back(assembly::smov_f_reg_reg{dst_reg_idx, src_reg_idx});
    }
}

void translator::emit_call(size_t proto_idx, reg_file res_reg_file, size_t res_reg_idx)
{
    bool returns_scalar = (res_reg_file == reg_file::i_scalar || res_reg_file == reg_file::f_scalar);
    bool is_int         = (res_reg_file == reg_file::i_scalar || res_reg_file == reg_file::i_span);

    if (returns_scalar)
    {
        if (is_int)
            program_.instructions_.emplace_back(assembly::call_i{proto_idx, res_reg_idx});
        else
            program_.instructions_.emplace_back(assembly::call_f{proto_idx, res_reg_idx});
    }
    else
    {
        if (is_int)
            program_.instructions_.emplace_back(assembly::scall_i{proto_idx, res_reg_idx});
        else
            program_.instructions_.emplace_back(assembly::scall_f{proto_idx, res_reg_idx});
    }
}

void translator::emit_expr(const ast::expr_wrapper& ew, reg_file target_reg_file, size_t target_reg_idx)
{
    if (!expect(ew.inferred_type_.has_value(), ew.loc_, "Missing inferred type for expression"))
        return;

    if (!expect(get_reg_file(*ew.inferred_type_) == target_reg_file, ew.loc_, "Expression type does not match target register file"))
        return;

    std::visit(
        overloaded
        {
            [this, target_reg_file, target_reg_idx, &ew](const ast::int_scalar& lit)
            {
                if (!expect(lit.const_memory_idx_.has_value(), ew.loc_, "Missing hoisted constant index for integer literal"))
                    return;

                emit_move_const_src(target_reg_file, target_reg_idx, *lit.const_memory_idx_);
            },
            [this, target_reg_file, target_reg_idx, &ew](const ast::float_scalar& lit)
            {
                if (!expect(lit.const_memory_idx_.has_value(), ew.loc_, "Missing hoisted constant index for float literal"))
                    return;

                emit_move_const_src(target_reg_file, target_reg_idx, *lit.const_memory_idx_);
            },
            [this, target_reg_file, target_reg_idx, &ew](const ast::array_construction& ac)
            {
                if (!expect(ac.const_memory_span_idx_.has_value(), ew.loc_, "Non-constant array construction not yet supported"))
                    return;

                emit_move_const_src(target_reg_file, target_reg_idx, *ac.const_memory_span_idx_);
            },
            [this, target_reg_file, target_reg_idx, &ew](const ast::symbol_ref& ref)
            {
                if (!expect(ref.declaration_idx_.has_value(), ew.loc_, "Internal error: unresolved symbol reference"))
                    return;

                auto [src_reg_file, src_reg_idx] = decl_regs_[*ref.declaration_idx_];

                if (!expect(src_reg_file == target_reg_file, ew.loc_, "Internal error: register file mismatch in symbol reference"))
                    return;

                emit_move_reg_src(target_reg_file, target_reg_idx, src_reg_idx);
            },
            [this, target_reg_file, target_reg_idx, &ew](const ast::f_call& call)
            {
                if (!expect(call.fn_.proto_idx_.has_value(), ew.loc_, "Unresolved function name in call"))
                    return;

                size_t proto_idx = *call.fn_.proto_idx_;

                if (!expect(proto_idx < functions_.size(), ew.loc_, "Invalid prototype index in function call"))
                    return;

                const prototype& proto = functions_[proto_idx].proto_;
                reg_file res_reg_file = get_reg_file(proto.return_type_);

                if (!expect(res_reg_file == target_reg_file, ew.loc_, "Function return type does not match target register file"))
                    return;

                std::array<size_t, 4> arg_pos{0, 0, 0, 0};

                for (ssize_t a = static_cast<ssize_t>(call.args_.size()) - 1; a >= 0; --a)
                {
                    type arg_t = proto.arg_types_[a];
                    reg_file arg_reg_file = get_reg_file(arg_t);
                    size_t arg_reg_idx = arg_pos[static_cast<size_t>(arg_reg_file)]++;
                    emit_expr(call.args_[a], arg_reg_file, arg_reg_idx);
                }

                emit_call(proto_idx, res_reg_file, target_reg_idx);
            },
            [this, &ew](const ast::index_access&)
            {
                translation_error(ew.loc_, "Index access not yet supported in code generation");
            },
            [this, &ew](const ast::comprehension&)
            {
                translation_error(ew.loc_, "Array comprehension not yet supported in code generation");
            }
        },
        ew.wrapped_
    );
}

void translator::translate(const std::string& source)
{
    errors_.clear();
    program_ = assembly::program{};

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

    compute_global_max_args();
    emit_declarations(prog);
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
