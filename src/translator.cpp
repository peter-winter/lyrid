#include "translator.hpp"
#include "utility.hpp"

#include <span>
#include <utility>

namespace lyrid
{

using namespace assembly;

translator::reg_file translator::get_scalar_reg_file(scalar_type t)
{
    return std::visit(
        overloaded
        {
            [](int_scalar_type) { return reg_file::i_scalar; },
            [](float_scalar_type) { return reg_file::f_scalar; }
        },
        t
    );  
}

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

translator::reg_file translator::to_scalar_reg_file(reg_file span_reg_file)
{
    switch (span_reg_file)
    {
        case reg_file::i_span:
            return reg_file::i_scalar;
        case reg_file::f_span:
            return reg_file::f_scalar;
    }
    std::unreachable();
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
        
    auto get_spans = [&] (scalar_type st, memory_type mt) -> array_spans&
    {
        return std::visit(
            overloaded
            {
                [&](int_scalar_type) -> array_spans& { return program_.get_int_array_spans(mt); },
                [&](float_scalar_type) -> array_spans& { return program_.get_float_array_spans(mt); }
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
                spans.emplace_back(span{offset, len});
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

    program_.const_int_memory_.reserve(should_be_more_than_enough_constants);
    program_.const_float_memory_.reserve(should_be_more_than_enough_constants);
    
    for (auto& decl : prog.declarations_)
        analyze_memory(analyze_memory, decl.value_);

    auto finalize_const_spans = [&]<typename T>(
        const std::vector<T>& memory,
        const array_spans& spans,
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
            ++counts[std::to_underlying(rf)];
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
    decl_regs_.reserve(should_be_more_than_enough_registers);

    next_reg_.fill(global_max_args_);

    for (size_t i = 0; i < prog.declarations_.size(); ++i)
    {
        const auto& decl = prog.declarations_[i];
        reg_file rf = get_reg_file(decl.type_);
        size_t reg_idx = next_reg_[std::to_underlying(rf)]++;
        decl_regs_.emplace_back(rf, reg_idx);

        emit_assignment(decl.value_, rf, reg_idx);
    }
}

void translator::emit_mov_x_reg_const(reg_file file, reg_index dst_reg_idx, const_index src_const_idx)
{
    switch (file)
    {
        case reg_file::i_scalar:
            program_.instructions_.emplace_back(mov_i_reg_const{dst_reg_idx, src_const_idx});
            return;
        case reg_file::f_scalar:
            program_.instructions_.emplace_back(mov_f_reg_const{dst_reg_idx, src_const_idx});
            return;
        case reg_file::i_span:
            program_.instructions_.emplace_back(mov_is_reg_const{dst_reg_idx, src_const_idx});
            return;
        case reg_file::f_span:
            program_.instructions_.emplace_back(mov_fs_reg_const{dst_reg_idx, src_const_idx});
            return;
    }
    std::unreachable();
}

void translator::emit_mov_x_reg_reg(reg_file file, reg_index dst_reg_idx, reg_index src_reg_idx)
{
    switch (file)
    {
        case reg_file::i_scalar:
            program_.instructions_.emplace_back(mov_i_reg_reg{dst_reg_idx, src_reg_idx});
            return;
        case reg_file::f_scalar:
            program_.instructions_.emplace_back(mov_f_reg_reg{dst_reg_idx, src_reg_idx});
            return;
        case reg_file::i_span:
            program_.instructions_.emplace_back(mov_is_reg_reg{dst_reg_idx, src_reg_idx});
            return;
        case reg_file::f_span:
            program_.instructions_.emplace_back(mov_fs_reg_reg{dst_reg_idx, src_reg_idx});
            return;
    }
    std::unreachable();
}

void translator::emit_mov_x_reg_mut(reg_file file, reg_index dst_reg_idx, span_index span_idx)
{
    switch (file)
    {
        case reg_file::i_span:
            program_.instructions_.emplace_back(mov_is_reg_mut{dst_reg_idx, span_idx});
            return;
        case reg_file::f_span:
            program_.instructions_.emplace_back(mov_fs_reg_mut{dst_reg_idx, span_idx});
            return;
    }
    std::unreachable();
}

void translator::emit_store_x_const(reg_file file, span_index span_idx, size_t offset, const_index src_const_idx)
{
    switch (file)
    {
        case reg_file::i_scalar:
            program_.instructions_.emplace_back(store_i_const{span_idx, offset, src_const_idx});
            return;
        case reg_file::f_scalar:
            program_.instructions_.emplace_back(store_f_const{span_idx, offset, src_const_idx});
            return;
    }
    std::unreachable();
}

void translator::emit_store_x_reg(reg_file file, span_index span_idx, size_t offset, reg_index src_reg_idx)
{
    switch (file)
    {
        case reg_file::i_scalar:
            program_.instructions_.emplace_back(store_i_reg{span_idx, offset, src_reg_idx});
            return;
        case reg_file::f_scalar:
            program_.instructions_.emplace_back(store_f_reg{span_idx, offset, src_reg_idx});
            return;
    }
    std::unreachable();
}

void translator::emit_load_x_reg_const(reg_file file, span_index span_idx, size_t offset, reg_index dst_reg_idx)
{
    switch (file)
    {
        case reg_file::i_scalar:
            program_.instructions_.emplace_back(load_i_reg_const{span_idx, offset, dst_reg_idx});
            return;
        case reg_file::f_scalar:
            program_.instructions_.emplace_back(load_f_reg_const{span_idx, offset, dst_reg_idx});
            return;
    }
    std::unreachable();
}

void translator::emit_load_x_reg_mut(reg_file file, span_index span_idx, size_t offset, reg_index dst_reg_idx)
{
    switch (file)
    {
        case reg_file::i_scalar:
            program_.instructions_.emplace_back(load_i_reg_mut{span_idx, offset, dst_reg_idx});
            return;
        case reg_file::f_scalar:
            program_.instructions_.emplace_back(load_f_reg_mut{span_idx, offset, dst_reg_idx});
            return;
    }
    std::unreachable();
}

void translator::emit_load_x_mut_const(reg_file file, span_index dst_span_idx, size_t dst_offset, span_index src_span_idx, size_t src_offset)
{
    switch (file)
    {
        case reg_file::i_scalar:
            program_.instructions_.emplace_back(load_i_mut_const{dst_span_idx, dst_offset, src_span_idx, src_offset});
            return;
        case reg_file::f_scalar:
            program_.instructions_.emplace_back(load_f_mut_const{dst_span_idx, dst_offset, src_span_idx, src_offset});
            return;
    }
    std::unreachable();
}

void translator::emit_load_x_mut_mut(reg_file file, span_index dst_span_idx, size_t dst_offset, span_index src_span_idx, size_t src_offset)
{
    switch (file)
    {
        case reg_file::i_scalar:
            program_.instructions_.emplace_back(load_i_mut_mut{dst_span_idx, dst_offset, src_span_idx, src_offset});
            return;
        case reg_file::f_scalar:
            program_.instructions_.emplace_back(load_f_mut_mut{dst_span_idx, dst_offset, src_span_idx, src_offset});
            return;
    }
    std::unreachable();
}

void translator::emit_call_reg(function_index proto_idx, reg_file res_reg_file, reg_index res_reg_idx)
{
    switch (res_reg_file)
    {
        case reg_file::i_scalar:
            program_.instructions_.emplace_back(call_i_reg{proto_idx, res_reg_idx});
            return;
        case reg_file::f_scalar:
            program_.instructions_.emplace_back(call_f_reg{proto_idx, res_reg_idx});
            return;
        case reg_file::i_span:
            program_.instructions_.emplace_back(call_is_reg{proto_idx, res_reg_idx});
            return;
        case reg_file::f_span:
            program_.instructions_.emplace_back(call_fs_reg{proto_idx, res_reg_idx});
            return;
    }
    std::unreachable();
}

void translator::emit_call_mut(function_index proto_idx, reg_file res_reg_file, span_index span_idx, size_t offset)
{
    switch (res_reg_file)
    {
        case reg_file::i_scalar:
            program_.instructions_.emplace_back(call_i_mut{proto_idx, span_idx, offset});
            return;
        case reg_file::f_scalar:
            program_.instructions_.emplace_back(call_f_mut{proto_idx, span_idx, offset});
            return;
    }
    std::unreachable();
}

void translator::emit_call_arguments(const ast::f_call& call, const prototype& proto)
{
    if (call.is_flat_)
    {
        std::array<size_t, 4> arg_pos{0, 0, 0, 0};

        for (size_t a = 0; a < call.args_.size(); ++a)
        {
            type arg_t = proto.arg_types_[a];
            reg_file arg_reg_file = get_reg_file(arg_t);
            size_t arg_reg_idx = arg_pos[std::to_underlying(arg_reg_file)]++;
            emit_assignment(call.args_[a], arg_reg_file, arg_reg_idx);
        }
    }
    else
    {
        std::vector<std::pair<reg_file, size_t>> temp_regs;
        temp_regs.reserve(call.args_.size());

        // Evaluate left-to-right into high temporaries
        for (size_t a = 0; a < call.args_.size(); ++a)
        {
            type arg_t = proto.arg_types_[a];
            reg_file arg_reg_file = get_reg_file(arg_t);
            size_t temp_idx = next_reg_[std::to_underlying(arg_reg_file)]++;
            emit_assignment(call.args_[a], arg_reg_file, temp_idx);
            temp_regs.emplace_back(arg_reg_file, temp_idx);
        }

        // Move temporaries to low argument positions (in prototype order)
        std::array<size_t, 4> arg_pos{0, 0, 0, 0};
        for (size_t a = 0; a < call.args_.size(); ++a)
        {
            auto [src_file, src_idx] = temp_regs[a];
            size_t dst_idx = arg_pos[std::to_underlying(src_file)]++;
            emit_mov_x_reg_reg(src_file, dst_idx, src_idx);
        }
    }
}

std::optional<size_t> translator::emit_mutable_store(const ast::expr_wrapper& ew, reg_file target_scalar_reg_file, span_index mem_idx, size_t off, std::optional<size_t> temp_reg_idx)
{
    if (!expect(ew.inferred_type_.has_value(), ew.loc_, "Missing inferred type for expression in mutable array element"))
        return temp_reg_idx;
        
    if (!expect(get_reg_file(*ew.inferred_type_) == target_scalar_reg_file, ew.loc_, "Expression type does not match target register file in mutable array element"))
        return temp_reg_idx;
                    
    return std::visit(
        overloaded
        {
            [&] (const ast::int_scalar& lit) -> std::optional<size_t>
            {
                if (!expect(lit.const_memory_idx_.has_value(), ew.loc_, "Missing hoisted constant index for integer literal in mutable array element"))
                    return temp_reg_idx;
                emit_store_x_const(target_scalar_reg_file, mem_idx, off, *lit.const_memory_idx_);
                return temp_reg_idx;
            },
            [&] (const ast::float_scalar& lit) -> std::optional<size_t>
            {
                if (!expect(lit.const_memory_idx_.has_value(), ew.loc_, "Missing hoisted constant index for float literal in mutable array element"))
                    return temp_reg_idx;
                emit_store_x_const(target_scalar_reg_file, mem_idx, off, *lit.const_memory_idx_);
                return temp_reg_idx;
            },
            [&] (const ast::symbol_ref& ref) -> std::optional<size_t>
            {
                if (!expect(ref.declaration_idx_.has_value(), ew.loc_, "Unresolved symbol reference in mutable array element"))
                    return temp_reg_idx;

                auto [src_reg_file, src_reg_idx] = decl_regs_[*ref.declaration_idx_];
                
                if (!expect(src_reg_file == target_scalar_reg_file, ew.loc_, "Register file mismatch with symbol reference in mutable array element"))
                    return temp_reg_idx;
                    
                emit_store_x_reg(target_scalar_reg_file, mem_idx, off, src_reg_idx);
                return temp_reg_idx;
            },
            [&] (const ast::f_call& call) -> std::optional<size_t>
            {
                if (!expect(call.fn_.proto_idx_.has_value(), ew.loc_, "Unresolved function name in function call within mutable array element"))
                    return temp_reg_idx;

                size_t proto_idx = *call.fn_.proto_idx_;

                if (!expect(proto_idx < functions_.size(), ew.loc_, "Invalid prototype index in function call within mutable array element"))
                    return temp_reg_idx;

                const prototype& proto = functions_[proto_idx].proto_;
                reg_file res_reg_file = get_reg_file(proto.return_type_);

                if (!expect(res_reg_file == target_scalar_reg_file, ew.loc_, "Function return type does not match target scalar register file in mutable array element"))
                    return temp_reg_idx;

                emit_call_arguments(call, proto);
                emit_call_mut(proto_idx, target_scalar_reg_file, mem_idx, off);
                return temp_reg_idx;
            },
            [&] (const auto&) -> std::optional<size_t>
            {
                // index_access should work here, 
                // array comprehension and array construction should not reach here (both not scalar)
                
                if (!temp_reg_idx.has_value())
                    temp_reg_idx = next_reg_[std::to_underlying(target_scalar_reg_file)]++;
                    
                emit_assignment(ew, target_scalar_reg_file, *temp_reg_idx);
                emit_store_x_reg(target_scalar_reg_file, mem_idx, off, *temp_reg_idx);
                return temp_reg_idx;
            }
        },
        ew.wrapped_
    );
}

void translator::emit_assignment(const ast::expr_wrapper& ew, reg_file target_reg_file, size_t target_reg_idx)
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

                emit_mov_x_reg_const(target_reg_file, target_reg_idx, *lit.const_memory_idx_);
            },
            [&] (const ast::float_scalar& lit)
            {
                if (!expect(lit.const_memory_idx_.has_value(), ew.loc_, "Missing hoisted constant index for float literal"))
                    return;

                emit_mov_x_reg_const(target_reg_file, target_reg_idx, *lit.const_memory_idx_);
            },
            [&] (const ast::array_construction& ac)
            {
                if (!expect(ac.memory_annotation_.has_value(), ew.loc_, "Array construction has no associated memory span"))
                    return;
                    
                const memory_span_annotation& annot = *ac.memory_annotation_;
                span_index mem_idx = annot.idx_;
                memory_type mt = annot.type_;
                                
                if (mt == memory_type::mem_const)
                {
                    emit_mov_x_reg_const(target_reg_file, target_reg_idx, mem_idx);
                    return;
                }
                else
                {
                    std::optional<size_t> temp_reg_idx;

                    for (size_t off = 0; off < ac.elements_.size(); ++off)
                    {
                        const ast::expr_wrapper& elem = ac.elements_[off];
                        reg_file scalar_rf = to_scalar_reg_file(target_reg_file);
                        temp_reg_idx = emit_mutable_store(elem, scalar_rf, mem_idx, off, temp_reg_idx);
                    }

                    emit_mov_x_reg_mut(target_reg_file, target_reg_idx, mem_idx);
                }
            },
            [&] (const ast::symbol_ref& ref)
            {
                if (!expect(ref.declaration_idx_.has_value(), ew.loc_, "Unresolved symbol reference"))
                    return;

                auto [src_reg_file, src_reg_idx] = decl_regs_[*ref.declaration_idx_];

                if (!expect(src_reg_file == target_reg_file, ew.loc_, "Register file mismatch in symbol reference"))
                    return;

                emit_mov_x_reg_reg(target_reg_file, target_reg_idx, src_reg_idx);
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

                emit_call_arguments(call, proto);
                emit_call_reg(proto_idx, res_reg_file, target_reg_idx);
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
    program_ = program{};

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
