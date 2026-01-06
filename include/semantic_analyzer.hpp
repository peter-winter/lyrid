#pragma once

#include "ast.hpp"
#include "type.hpp"
#include "utility.hpp"
#include "prototype.hpp"

#include <map>
#include <optional>
#include <string>
#include <vector>

namespace lyrid
{

class semantic_analyzer
{
public:
    void register_function_prototype(
        const std::string& name,
        std::vector<type> arg_types,
        std::vector<std::string> arg_names,
        type return_type);

    void analyze(ast::program& prog);

    bool is_valid() const { return errors_.empty(); }

    const std::vector<std::string>& get_errors() const { return errors_; }

private:
    void process_array_type_in_function_prototype(type t, const std::string& name);
    
    struct scope_entry
    {
        std::optional<size_t> decl_index_;
        type var_type_;
    };
    
    std::string type_to_string(type t) const;
    std::string scalar_type_to_string(scalar_type t) const;

    void error(const ast::source_location& loc, const std::string& message);
    
    std::vector<prototype> prototypes_;
    std::map<std::string, size_t> prototype_map_;

    std::vector<std::string> errors_;
};

}
