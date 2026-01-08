#pragma once

#include "parser.hpp"
#include "semantic_analyzer.hpp"
#include "prototype.hpp"
#include "type.hpp"
#include "vm.hpp"

#include <string>
#include <vector>
#include <utility>
#include <limits>

namespace lyrid
{

struct function
{
    std::string name_;
    prototype proto_;
};

class translator
{
public:
    translator() = default;

    void translate(const std::string& source);

    void register_function(
        std::string name,
        std::vector<type> arg_types,
        std::vector<std::string> arg_names,
        type return_type);

    const std::vector<std::string>& get_errors() const { return errors_; }

    bool is_valid() const { return errors_.empty(); }
    
private:
    void translation_error(const ast::source_location& loc, const std::string& message);
    bool expect(bool condition, const ast::source_location& loc, const std::string& message);

    std::vector<std::string> errors_;
    std::vector<function> functions_;
    vm_program program_;
};

}
