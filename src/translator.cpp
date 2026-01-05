#include "translator.hpp"
#include "utility.hpp"

#include <span>
#include <utility>

namespace lyrid
{

translator::reg_file translator::get_span_reg_file(scalar_type t)
{
    return std::visit(
        overloaded
        {
            [](int_scalar_type) { return reg_file::i_span; },
            [](float_scalar_type) { return reg_file::f_span; }
        },
        t
    );  
}

translator::reg_file translator::get_reg_file(type t)
{
    return std::visit(
        overloaded
        {
            [](int_scalar_type) { return reg_file::i_scalar; },
            [](float_scalar_type) { return reg_file::f_scalar; },
            [](array_type ar) { return get_span_reg_file(ar.sc_); }
        },
        t
    );
}

void translator::prepare_memory_model(ast::program& prog)
{
    auto get_const_pool_offset = [&] (scalar_type st) -> size_t
    {
        return std::visit(
            overloaded
            {
                [&](int_scalar_type) { return program_.const_int_memory_.size(); },
                [&](float_scalar_type) { return program_.const_float_memory_.size(); }
            },
            st
        );
    };
    
    auto get_mutable_pool_size = [&] (scalar_type st) -> size_t&
    {
        return std::visit(
            overloaded
            {
                [&](int_scalar_type) -> size_t& { return program_.mutable_int_memory_size_; },
                [&](float_scalar_type) -> size_t& { return program_.mutable_float_memory_size_; }
            },
            st
        );
    };
        
    auto get_spans = [&] (scalar_type st, memory_type mt) -> assembly::array_spans&
    {
        return std::visit(
            overloaded
            {
                [&](int_scalar_type) -> assembly::array_spans& { return program_.get_int_array_spans(mt); },
                [&](float_scalar_type) -> assembly::array_spans& { return program_.get_float_array_spans(mt); }
            },
            st
        );
    };

    auto analyze_memory = [&](auto&& self, ast::expr_wrapper& ew) -> bool
    {
        if (!expect(ew.inferred_type_.has_value(), ew.loc_, "Missing inferred type for expression"))
            return false;
        
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
                if (ac.elements_.empty())
                    return false;

                array_type arr_type = std::get<array_type>(ew.inferred_type_.value());
                
                size_t len = ac.elements_.size();
    
                size_t const_pool_offset = get_const_pool_offset(arr_type.sc_);
                size_t& mutable_pool_size = get_mutable_pool_size(arr_type.sc_);
                
                bool all_const = true;
                for (auto& elem : ac.elements_)
                    if (!self(self, elem))
                        all_const = false;
                
                memory_type mem_type = all_const ? memory_type::mem_const : memory_type::mem_mutable;
                auto& spans = get_spans(arr_type.sc_, mem_type);
                size_t idx = spans.size();
                size_t offset = all_const ? const_pool_offset : mutable_pool_size;
                spans.emplace_back(assembly::span{offset, len});
                ac.memory_annotation_ = memory_span_annotation{ mem_type, idx };
                
                if (!all_const)
                    mutable_pool_size += len;
                
                return false;
            },
            [&](ast::f_call& call) -> bool
            {
                for (auto& arg : call.args_)
                    self(self, arg);
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
            [&](const auto&) -> bool
            {
                return false;
            }
        }, ew.wrapped_);
    };

    program_.const_int_memory_.reserve(256);
    program_.const_float_memory_.reserve(256);
    
    for (auto& decl : prog.declarations_)
        analyze_memory(analyze_memory, decl.value_);

    auto finalize_const_spans = [&]<typename T>(
        const std::vector<T>& memory,
        const assembly::array_spans& spans,
        std::vector<std::span<const T>>& memory_spans)
    {
        memory_spans.reserve(spans.size());
        for (const auto& s : spans)
        {
            memory_spans.emplace_back(memory.data() + s.offset_, s.len_);
        }
    };
    
    finalize_const_spans(program_.const_int_memory_, program_.get_int_array_spans(memory_type::mem_const), program_.const_int_array_memory_spans_);
    finalize_const_spans(program_.const_float_memory_, program_.get_float_array_spans(memory_type::mem_const), program_.const_float_array_memory_spans_);
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
            max_per_file[i] = std::max(max_per_file[i], counts[i]);
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
            program_.instructions_.emplace_back(assembly::mov_is_reg_const{dst_reg_idx, src_const_idx});
            return;
        case reg_file::f_span:
            program_.instructions_.emplace_back(assembly::mov_fs_reg_const{dst_reg_idx, src_const_idx});
            return;
    }
    std::unreachable();
}

void translator::emit_move_reg_src(reg_file file, size_t dst_reg_idx, size_t src_reg_idx)
{
    bool is_scalar = (file == reg_file::i_scalar || file == reg_file::f_scalar);
    bool is_int    = (file == reg_file::i_scalar || file == reg_file::i_span);

    if (is_scalar)
        if (is_int)
            program_.instructions_.emplace_back(assembly::mov_i_reg_reg{dst_reg_idx, src_reg_idx});
        else
            program_.instructions_.emplace_back(assembly::mov_f_reg_reg{dst_reg_idx, src_reg_idx});
    else
        if (is_int)
            program_.instructions_.emplace_back(assembly::mov_is_reg_reg{dst_reg_idx, src_reg_idx});
        else
            program_.instructions_.emplace_back(assembly::mov_fs_reg_reg{dst_reg_idx, src_reg_idx});
}

void translator::emit_call(size_t proto_idx, reg_file res_reg_file, size_t res_reg_idx)
{
    bool returns_scalar = (res_reg_file == reg_file::i_scalar || res_reg_file == reg_file::f_scalar);
    bool is_int         = (res_reg_file == reg_file::i_scalar || res_reg_file == reg_file::i_span);

    if (returns_scalar)
        if (is_int)
            program_.instructions_.emplace_back(assembly::call_i{proto_idx, res_reg_idx});
        else
            program_.instructions_.emplace_back(assembly::call_f{proto_idx, res_reg_idx});
    else
        if (is_int)
            program_.instructions_.emplace_back(assembly::scall_i{proto_idx, res_reg_idx});
        else
            program_.instructions_.emplace_back(assembly::scall_f{proto_idx, res_reg_idx});
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
            [&] (const ast::int_scalar& lit)
            {
                if (!expect(lit.const_memory_idx_.has_value(), ew.loc_, "Missing hoisted constant index for integer literal"))
                    return;

                emit_move_const_src(target_reg_file, target_reg_idx, *lit.const_memory_idx_);
            },
            [&] (const ast::float_scalar& lit)
            {
                if (!expect(lit.const_memory_idx_.has_value(), ew.loc_, "Missing hoisted constant index for float literal"))
                    return;

                emit_move_const_src(target_reg_file, target_reg_idx, *lit.const_memory_idx_);
            },
            [&] (const ast::array_construction& ac)
            {
                if (!expect(ac.memory_annotation_.has_value(), ew.loc_, "Array construction has no associated memory span"))
                    return;
                    
                if (ac.memory_annotation_.value().type_ == memory_type::mem_const)
                {
                    emit_move_const_src(target_reg_file, target_reg_idx, ac.memory_annotation_.value().idx_);
                    return;
                }

                translation_error(ew.loc_, "Mutable array constructions are not yet supported in code generation");
            },
            [&] (const ast::symbol_ref& ref)
            {
                if (!expect(ref.declaration_idx_.has_value(), ew.loc_, "Internal error: unresolved symbol reference"))
                    return;

                auto [src_reg_file, src_reg_idx] = decl_regs_[*ref.declaration_idx_];

                if (!expect(src_reg_file == target_reg_file, ew.loc_, "Internal error: register file mismatch in symbol reference"))
                    return;

                emit_move_reg_src(target_reg_file, target_reg_idx, src_reg_idx);
            },
            [&] (const ast::f_call& call)
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
            [&] (const ast::index_access&)
            {
                translation_error(ew.loc_, "Index access not yet supported in code generation");
            },
            [&] (const ast::comprehension&)
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
        sa.register_function_prototype(func.name_, func.proto_.arg_types_, func.proto_.arg_names_, func.proto_.return_type_);

    ast::program& prog = p.get_program();
    sa.analyze(prog);

    const auto& sem_errors = sa.get_errors();
    if (!sem_errors.empty())
    {
        errors_ = sem_errors;
        return;
    }

    prepare_memory_model(prog);

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
