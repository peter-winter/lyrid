#include "translator.hpp"
#include "utility.hpp"

#include <utility>
#include <limits>
#include <cmath>

namespace lyrid
{

using namespace assembly;
using namespace ast;


pool translator::get_scalar_pool(scalar_type t)
{
    return std::visit(
        overloaded
        {
            [](int_scalar_type) -> pool { return int_pool{}; },
            [](float_scalar_type) -> pool { return float_pool{}; }
        },
        t
    );  
}

pool translator::get_pool(type t)
{
    return std::visit(
        overloaded
        {
            [](int_scalar_type) -> pool { return int_pool{}; },
            [](float_scalar_type) -> pool { return float_pool{}; },
            [](array_type ar) -> pool { return get_scalar_pool(ar.sc_); }
        },
        t
    );
}

bool translator::expect(bool condition, const source_location& loc, const std::string& message)
{
    if (!condition)
        translation_error(loc, message);

    return condition;
}

void translator::translation_error(const source_location& loc, const std::string& message)
{
    std::string prefix = "Translation error [" + std::to_string(loc.line_) + ":" +
                         std::to_string(loc.column_) + "]: ";
    errors_.emplace_back(prefix + message);
}

size_t translator::get_memory_size(pool p) const
{
    return std::visit(overloaded
    {
        [&](int_pool) { return program_.int_memory_.size(); },
        [&](float_pool) { return program_.float_memory_.size(); }
    },
    p);
}

void translator::translate(const std::string& source)
{
    errors_.clear();

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

    ast::program& prog_ast = p.get_program();
    sa.analyze(prog_ast);

    const auto& sem_errors = sa.get_errors();
    if (!sem_errors.empty())
    {
        errors_ = sem_errors;
        return;
    }
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
