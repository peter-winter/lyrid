#include "semantic_analyzer.hpp"

#include <stdexcept>

namespace lyrid
{

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
    const expr& e,
    const std::map<std::string, type>& symbols,
    size_t decl_line)
{
    return std::visit(overloaded
    {
        [&](const int_scalar&) -> std::optional<type>
        {
            return type::int_scalar;
        },
        [&](const float_scalar&) -> std::optional<type>
        {
            return type::float_scalar;
        },
        [&](const id& i) -> std::optional<type>
        {
            auto it = symbols.find(i);
            if (it == symbols.end())
            {
                errors_.emplace_back("Line " + std::to_string(decl_line) + ": Undefined variable '" + i + "'");
                return {};
            }
            return it->second;
        },
        [&](const f_call& call) -> std::optional<type>
        {
            auto fit = functions_.find(call.name_);
            if (fit == functions_.end())
            {
                errors_.emplace_back("Line " + std::to_string(decl_line) + ": Call to undefined function '" + call.name_ + "'");
                return {};
            }

            const prototype& proto = fit->second;

            if (call.args_.size() != proto.arg_types.size())
            {
                errors_.emplace_back(
                    "Line " + std::to_string(decl_line) + ": Incorrect number of arguments in call to '" + call.name_ +
                    "': expected " + std::to_string(proto.arg_types.size()) +
                    " but provided " + std::to_string(call.args_.size()));
                return {};
            }

            for (size_t i = 0; i < call.args_.size(); ++i)
            {
                std::optional<type> arg_type = infer_expression_type(call.args_[i].wrapped_, symbols, decl_line);
                if (!arg_type)
                {
                    return {};
                }

                if (*arg_type != proto.arg_types[i])
                {
                    std::string arg_name = proto.arg_names[i].empty()
                        ? "argument " + std::to_string(i + 1)
                        : "'" + proto.arg_names[i] + "'";

                    errors_.emplace_back(
                        "Line " + std::to_string(decl_line) + ": Type mismatch for " + arg_name + " in call to '" + call.name_ +
                        "': expected " + type_to_string(proto.arg_types[i]) +
                        " but got " + type_to_string(*arg_type));
                }
            }

            return proto.return_type;
        },
        [&](const index_access& acc) -> std::optional<type>
        {
            std::optional<type> base_type = infer_expression_type(acc.base_->wrapped_, symbols, decl_line);

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
                errors_.emplace_back(
                    "Line " + std::to_string(decl_line) +
                    ": Indexing applied to non-array type '" + type_to_string(*base_type) + "'");

                return {};
            }

            std::optional<type> index_type = infer_expression_type(acc.index_->wrapped_, symbols, decl_line);

            if (!index_type)
            {
                return {};
            }

            if (*index_type != type::int_scalar)
            {
                errors_.emplace_back(
                    "Line " + std::to_string(decl_line) +
                    ": Array index must be of type 'int', but got '" + type_to_string(*index_type) + "'");

                return {};
            }

            return element_type;
        },
        [&](const array_construction& ac) -> std::optional<type>
        {
            if (ac.elements_.empty())
            {
                errors_.emplace_back("Line " + std::to_string(decl_line) + ": Empty array construction is not allowed");
                return {};
            }

            std::optional<type> elem_type;

            for (const auto& elem : ac.elements_)
            {
                std::optional<type> t_opt = infer_expression_type(elem.wrapped_, symbols, decl_line);

                if (!t_opt)
                {
                    return {};
                }

                type t = *t_opt;

                if (t != type::int_scalar && t != type::float_scalar)
                {
                    errors_.emplace_back(
                        "Line " + std::to_string(decl_line) +
                        ": Array construction elements must be scalar types, but got '" + type_to_string(t) + "'");

                    return {};
                }

                if (!elem_type)
                {
                    elem_type = t;
                }
                else if (t != *elem_type)
                {
                    errors_.emplace_back(
                        "Line " + std::to_string(decl_line) +
                        ": Type mismatch in array construction: expected '" + type_to_string(*elem_type) +
                        "' but got '" + type_to_string(t) + "'");

                    return {};
                }
            }

            return (*elem_type == type::int_scalar) ? type::int_array : type::float_array;
        }
    }, e);
}

void semantic_analyzer::analyze(const program& prog)
{
    errors_.clear();
    symbols_.clear();

    for (const auto& decl : prog.declarations_)
    {
        if (symbols_.contains(decl.name_))
        {
            errors_.emplace_back("Line " + std::to_string(decl.line_number_) + ": Redeclaration of variable '" + decl.name_ + "'");
        }

        std::optional<type> expr_type = infer_expression_type(decl.value_, symbols_, decl.line_number_);

        if (expr_type && *expr_type != decl.type_)
        {
            errors_.emplace_back(
                "Line " + std::to_string(decl.line_number_) + ": Type mismatch in declaration of '" + decl.name_ +
                "': declared as " + type_to_string(decl.type_) +
                " but expression has type " + type_to_string(*expr_type));
        }

        symbols_[decl.name_] = decl.type_;
    }
}


}
