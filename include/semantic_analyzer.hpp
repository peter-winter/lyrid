#pragma once

#include "ast.hpp"
#include "utility.hpp"

#include <map>
#include <optional>
#include <string>
#include <vector>

namespace lyrid
{

class semantic_analyzer
{
public:
    struct prototype
    {
        std::vector<type> arg_types;
        std::vector<std::string> arg_names;
        type return_type;
    };

    void register_function_prototype(
        const std::string& name,
        std::vector<type> arg_types,
        std::vector<std::string> arg_names,
        type return_type);

    void analyze(const program& prog);

    bool is_valid() const { return errors_.empty(); }

    const std::vector<std::string>& get_errors() const { return errors_; }

    const std::map<std::string, type>& get_symbols() const { return symbols_; }

private:
    std::string type_to_string(type t) const;

    std::optional<type> infer_expression_type(
        const expr& e,
        const std::map<std::string, type>& symbols,
        size_t decl_line);

    std::map<std::string, prototype> functions_;
    std::vector<std::string> errors_;
    std::map<std::string, type> symbols_;
};

}
