#include "semantic_analyzer.hpp"

#include <stdexcept>
#include <set>

namespace lyrid
{

using namespace ast;

void semantic_analyzer::error(const source_location& loc, const std::string& message)
{
    std::string prefix = "Error [" + std::to_string(loc.line_) + ", " +
                         std::to_string(loc.column_) + "]: ";
    errors_.emplace_back(prefix + message);
}

void semantic_analyzer::register_function_prototype(
    const std::string& name,
    std::vector<type> arg_types,
    std::vector<std::string> arg_names,
    type return_type)
{
    if (arg_types.size() != arg_names.size())
    {
        throw std::invalid_argument(
            "Number of argument types and names must match for function prototype '" + name + "'");
    }

    prototype proto{std::move(arg_types), std::move(arg_names), return_type};
    prototypes_.push_back(std::move(proto));
    prototype_map_[name] = prototypes_.size() - 1;
}

std::string semantic_analyzer::type_to_string(type t) const
{
    switch (t)
    {
        case type::int_scalar:   return "int";
        case type::float_scalar: return "float";
        case type::int_array:    return "int[]";
        case type::float_array:  return "float[]";
    }
    return "unknown";
}

std::optional<type> semantic_analyzer::analyze_expr(
    expr_wrapper& wrapper,
    const std::map<std::string, scope_entry>& current_scope)
{
    return std::visit(overloaded
    {
        [&](int_scalar&) -> std::optional<type>
        {
            return wrapper.inferred_type_ = type::int_scalar;
        },
        [&](float_scalar&) -> std::optional<type>
        {
            return wrapper.inferred_type_ = type::float_scalar;
        },
        [&](symbol_ref& sr) -> std::optional<type>
        {
            const std::string& name = sr.ident_.value_;
            auto it = current_scope.find(name);
            if (it == current_scope.end())
            {
                error(sr.ident_.loc_, "Undefined variable '" + name + "'");
                return {};
            }
            const scope_entry& entry = it->second;
            sr.declaration_idx_ = entry.decl_index_;
            return wrapper.inferred_type_ = entry.var_type_;
        },
        [&](f_call& call) -> std::optional<type>
        {
            const std::string& fname = call.name_.ident_.value_;
            auto pit = prototype_map_.find(fname);
            if (pit == prototype_map_.end())
            {
                error(call.name_.ident_.loc_, "Call to undefined function '" + fname + "'");
                return {};
            }
            call.name_.proto_idx_ = pit->second;
            const prototype& proto = prototypes_[pit->second];

            if (call.args_.size() != proto.arg_types_.size())
            {
                error(wrapper.loc_,
                      "Incorrect number of arguments in call to '" + fname +
                      "': expected " + std::to_string(proto.arg_types_.size()) +
                      " but provided " + std::to_string(call.args_.size()));
                return {};
            }

            for (size_t i = 0; i < call.args_.size(); ++i)
            {
                std::optional<type> arg_type = analyze_expr(call.args_[i], current_scope);
                if (!arg_type)
                {
                    return {};
                }

                if (*arg_type != proto.arg_types_[i])
                {
                    std::string arg_name = proto.arg_names_[i].empty()
                        ? "argument " + std::to_string(i + 1)
                        : "'" + proto.arg_names_[i] + "'";

                    error(call.args_[i].loc_,
                          "Type mismatch for " + arg_name + " in call to '" + fname +
                          "': expected '" + type_to_string(proto.arg_types_[i]) +
                          "' but got '" + type_to_string(*arg_type) + "'");
                    return {};
                }
            }
            return wrapper.inferred_type_ = proto.return_type_;
        },
        [&](index_access& acc) -> std::optional<type>
        {
            std::optional<type> base_type = analyze_expr(*acc.base_, current_scope);
            if (!base_type)
            {
                return {};
            }

            type element_type;
            if (*base_type == type::int_array)
            {
                element_type = type::int_scalar;
            }
            else if (*base_type == type::float_array)
            {
                element_type = type::float_scalar;
            }
            else
            {
                error(acc.base_->loc_, "Indexing applied to non-array type '" + type_to_string(*base_type) + "'");
                return {};
            }

            std::optional<type> index_type = analyze_expr(*acc.index_, current_scope);
            if (!index_type)
            {
                return {};
            }

            if (*index_type != type::int_scalar)
            {
                error(acc.index_->loc_, "Array index must be of type 'int', but got '" + type_to_string(*index_type) + "'");
                return {};
            }

            return wrapper.inferred_type_ = element_type;
        },
        [&](array_construction& ac) -> std::optional<type>
        {
            if (ac.elements_.empty())
            {
                error(wrapper.loc_, "Empty array construction is not allowed");
                return {};
            }

            std::optional<type> elem_type;

            for (auto& elem : ac.elements_)
            {
                std::optional<type> t_opt = analyze_expr(elem, current_scope);
                if (!t_opt)
                {
                    return {};
                }

                type t = *t_opt;

                if (t != type::int_scalar && t != type::float_scalar)
                {
                    error(elem.loc_, "Array construction elements must be scalar types, but got '" + type_to_string(t) + "'");
                    return {};
                }

                if (!elem_type)
                {
                    elem_type = t;
                }
                else if (t != *elem_type)
                {
                    error(elem.loc_,
                          "Type mismatch in array construction: expected '" + type_to_string(*elem_type) +
                          "' but got '" + type_to_string(t) + "'");
                    return {};
                }
            }

            return wrapper.inferred_type_ = ((*elem_type == type::int_scalar) ? type::int_array : type::float_array);
        },
        [&](comprehension& comp) -> std::optional<type>
        {
            size_t n = comp.variables_.size();

            if (n == 0 || n != comp.in_exprs_.size())
            {
                error(wrapper.loc_, "Invalid array comprehension (mismatched variables/sources)");
                return {};
            }

            std::vector<type> elem_types;
            for (auto& src : comp.in_exprs_)
            {
                std::optional<type> src_type = analyze_expr(src, current_scope);
                if (!src_type) return {};

                type elem_t;
                if (*src_type == type::int_array)
                {
                    elem_t = type::int_scalar;
                }
                else if (*src_type == type::float_array)
                {
                    elem_t = type::float_scalar;
                }
                else
                {
                    error(src.loc_,
                          "Source in array comprehension must be an array type, got '" +
                          type_to_string(*src_type) + "'");
                    return {};
                }
                elem_types.push_back(elem_t);
            }

            std::set<std::string> seen;
            for (const auto& v : comp.variables_)
            {
                if (!seen.insert(v.value_).second)
                {
                    error(v.loc_, "Duplicate variable '" + v.value_ + "' in array comprehension");
                    return {};
                }
            }

            auto comp_scope = current_scope;
            for (size_t i = 0; i < n; ++i)
            {
                comp_scope[comp.variables_[i].value_] = {std::nullopt, elem_types[i]};
            }

            std::optional<type> body_type = analyze_expr(*comp.do_expr_, comp_scope);
            if (!body_type) 
                return {};

            if (*body_type != type::int_scalar && *body_type != type::float_scalar)
            {
                error(comp.do_expr_->loc_,
                      "'do' expression in array comprehension must be a scalar type, got '" +
                      type_to_string(*body_type) + "'");
                return {};
            }

            return wrapper.inferred_type_ = ((*body_type == type::int_scalar) ? type::int_array : type::float_array);
        },
    }, wrapper.wrapped_);
}

void semantic_analyzer::analyze(program& prog)
{
    errors_.clear();

    std::map<std::string, scope_entry> scope;

    for (size_t decl_idx = 0; decl_idx < prog.declarations_.size(); ++decl_idx)
    {
        declaration& decl = prog.declarations_[decl_idx];
        const std::string& name = decl.name_.value_;

        if (scope.contains(name))
        {
            error(decl.name_.loc_, "Redeclaration of variable '" + name + "'");
        }

        std::optional<type> expr_type = analyze_expr(decl.value_, scope);

        if (expr_type && *expr_type != decl.type_)
        {
            error(decl.value_.loc_,
                  "Type mismatch in declaration of '" + name +
                  "': declared as '" + type_to_string(decl.type_) +
                  "' but expression has type '" + type_to_string(*expr_type) + "'");
        }

        scope[name] = {decl_idx, decl.type_};
    }
}

}
