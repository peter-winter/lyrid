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
        [&](const int_scalar& lit) -> std::optional<type>
        {
            return type::int_scalar;
        },
        [&](const float_scalar& lit) -> std::optional<type>
        {
            return type::float_scalar;
        },
        [&](const int_array& lit) -> std::optional<type>
        {
            return type::int_array;
        },
        [&](const float_array& lit) -> std::optional<type>
        {
            return type::float_array;
        },
        [&](const id& i) -> std::optional<type>
        {
            auto it = symbols.find(i);
            if (it == symbols.end())
            {
                errors_.emplace_back("Line " + std::to_string(decl_line) + ": Undefined variable '" + i + "'");
                return std::nullopt;
            }
            return it->second;
        },
        [&](const f_call& call) -> std::optional<type>
        {
            auto fit = functions_.find(call.name_);
            if (fit == functions_.end())
            {
                errors_.emplace_back("Line " + std::to_string(decl_line) + ": Call to undefined function '" + call.name_ + "'");
                return std::nullopt;
            }

            const prototype& proto = fit->second;

            if (call.args_.size() != proto.arg_types.size())
            {
                errors_.emplace_back(
                    "Line " + std::to_string(decl_line) + ": Incorrect number of arguments in call to '" + call.name_ +
                    "': expected " + std::to_string(proto.arg_types.size()) +
                    " but provided " + std::to_string(call.args_.size()));
                return std::nullopt;
            }

            for (size_t i = 0; i < call.args_.size(); ++i)
            {
                std::optional<type> arg_type = infer_expression_type(call.args_[i].wrapped_, symbols, decl_line);
                if (!arg_type)
                {
                    return std::nullopt; // Sub-expression error already recorded
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

        std::optional<type> expr_type = infer_expression_type(decl.value_.wrapped_, symbols_, decl.line_number_);

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
