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

    functions_[name] = { std::move(arg_types), std::move(arg_names), return_type };
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

std::optional<type> semantic_analyzer::infer_expression_type(
    expr_wrapper& wrapper,
    const std::map<std::string, type>& symbols)
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
        [&](identifier& i) -> std::optional<type>
        {
            auto it = symbols.find(i.value_);
            if (it == symbols.end())
            {
                error(i.loc_, "Undefined variable '" + i.value_ + "'");
                return {};
            }
            return wrapper.inferred_type_ = it->second;
        },
        [&](f_call& call) -> std::optional<type>
        {
            auto fit = functions_.find(call.name_.value_);
            if (fit == functions_.end())
            {
                error(call.name_.loc_, "Call to undefined function '" + call.name_.value_ + "'");
                return {};
            }

            const prototype& proto = fit->second;

            if (call.args_.size() != proto.arg_types_.size())
            {
                error(wrapper.loc_,
                      "Incorrect number of arguments in call to '" + call.name_.value_ +
                      "': expected " + std::to_string(proto.arg_types_.size()) +
                      " but provided " + std::to_string(call.args_.size()));
                return {};
            }

            for (size_t i = 0; i < call.args_.size(); ++i)
            {
                std::optional<type> arg_type = infer_expression_type(call.args_[i], symbols);
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
                          "Type mismatch for " + arg_name + " in call to '" + call.name_.value_ +
                          "': expected '" + type_to_string(proto.arg_types_[i]) +
                          "' but got '" + type_to_string(*arg_type) + "'");
                    return {};
                }
            }
            return wrapper.inferred_type_ = proto.return_type_;
        },
        [&](index_access& acc) -> std::optional<type>
        {
            std::optional<type> base_type = infer_expression_type(*acc.base_, symbols);

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

            std::optional<type> index_type = infer_expression_type(*acc.index_, symbols);

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
                std::optional<type> t_opt = infer_expression_type(elem, symbols);

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
        [&](comprehension& fc) -> std::optional<type>
        {
            size_t n = fc.variables_.size();

            if (n == 0 || n != fc.in_exprs_.size())
            {
                error(wrapper.loc_, "Invalid array comprehension (mismatched variables/sources)");
                return {};
            }

            std::vector<type> elem_types;
            for (auto& src : fc.in_exprs_)
            {
                std::optional<type> src_type = infer_expression_type(src, symbols);
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
            for (const auto& v : fc.variables_)
            {
                if (!seen.insert(v.value_).second)
                {
                    error(v.loc_, "Duplicate variable '" + v.value_ + "' in array comprehension");
                    return {};
                }
            }

            std::map<std::string, type> local_symbols;
            for (size_t i = 0; i < n; ++i)
            {
                local_symbols[fc.variables_[i].value_] = elem_types[i];
            }

            auto combined_symbols = symbols;
            combined_symbols.insert(local_symbols.begin(), local_symbols.end());

            std::optional<type> body_type = infer_expression_type(*fc.do_expr_, combined_symbols);
            if (!body_type) 
                return {};

            if (*body_type != type::int_scalar && *body_type != type::float_scalar)
            {
                error(fc.do_expr_->loc_,
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
    symbols_.clear();

    for (auto& decl : prog.declarations_)
    {
        if (symbols_.contains(decl.name_.value_))
        {
            error(decl.name_.loc_, "Redeclaration of variable '" + decl.name_.value_ + "'");
        }

        std::optional<type> expr_type = infer_expression_type(decl.value_, symbols_);

        if (expr_type && *expr_type != decl.type_)
        {
            error(decl.value_.loc_,
                  "Type mismatch in declaration of '" + decl.name_.value_ +
                  "': declared as '" + type_to_string(decl.type_) +
                  "' but expression has type '" + type_to_string(*expr_type) + "'");
        }

        symbols_[decl.name_.value_] = decl.type_;
    }
}


}
