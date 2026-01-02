#pragma once

#include "ast.hpp"
#include "type.hpp"
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
        std::vector<type> arg_types_;
        std::vector<std::string> arg_names_;
        type return_type_;
    };

    void register_function_prototype(
        const std::string& name,
        std::vector<type> arg_types,
        std::vector<std::string> arg_names,
        type return_type);

    void analyze(ast::program& prog);

    bool is_valid() const { return errors_.empty(); }

    const std::vector<std::string>& get_errors() const { return errors_; }

    const std::map<std::string, type>& get_symbols() const { return symbols_; }

private:
    std::string type_to_string(type t) const;

    void error(const ast::source_location& loc, const std::string& message);
    
    std::optional<type> infer_expression_type(ast::expr_wrapper& wrapper, const std::map<std::string, type>& symbols);

    std::map<std::string, prototype> functions_;
    std::vector<std::string> errors_;
    std::map<std::string, type> symbols_;
};

}
