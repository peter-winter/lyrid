// tests/test_semantics_errors.cpp
#include <catch2/catch_test_macros.hpp>

#include "parser.hpp"
#include "ast.hpp"
#include "semantic_analyzer.hpp"

using namespace lyrid;
using namespace lyrid::ast;

TEST_CASE("Semantic error: non-integer array index", "[semantics][error]")
{
    parser p;
    p.parse(R"(
int[] arr = [1, 2, 3]
float idx = 0.5
int x = arr[idx]
)");

    const auto& parse_errors = p.get_errors();
    REQUIRE(parse_errors.empty());

    semantic_analyzer sa;
    program& prog = p.get_program();
    sa.analyze(prog);

    REQUIRE(!sa.is_valid());
    const auto& sem_errors = sa.get_errors();
    REQUIRE(sem_errors.size() == 1);
    REQUIRE(sem_errors[0] == "Error [4, 13]: Array index must be of type 'int', but got 'float'");
}

TEST_CASE("Semantic error: mixed types in array construction", "[semantics][error]")
{
    parser p;
    p.parse("int[] arr = [1, 2.0, 3]");

    const auto& parse_errors = p.get_errors();
    REQUIRE(parse_errors.empty());

    semantic_analyzer sa;
    sa.analyze(p.get_program());

    REQUIRE(!sa.is_valid());
    const auto& sem_errors = sa.get_errors();
    REQUIRE(sem_errors.size() == 1);
    REQUIRE(sem_errors[0] == "Error [1, 17]: Type mismatch in array construction: expected 'int' but got 'float'");
}

TEST_CASE("Semantic error: nested array in construction", "[semantics][error]")
{
    parser p;
    p.parse(R"(
int[] inner = [1, 2]
int[] outer = [inner]
)");

    const auto& parse_errors = p.get_errors();
    REQUIRE(parse_errors.empty());

    semantic_analyzer sa;
    sa.analyze(p.get_program());

    REQUIRE(!sa.is_valid());
    const auto& sem_errors = sa.get_errors();
    REQUIRE(sem_errors.size() == 1);
    REQUIRE(sem_errors[0] == "Error [3, 16]: Array construction elements must be scalar types, but got 'int[]'");
}

TEST_CASE("Semantic error: comprehension infers mismatched array element type", "[semantics][error]")
{
    parser p;
    p.parse(R"(
int[] src = [1, 2, 3]
float[] res = [|i| in |src| do 42]
)");

    const auto& parse_errors = p.get_errors();
    REQUIRE(parse_errors.empty());

    semantic_analyzer sa;
    program& prog = p.get_program();
    sa.analyze(prog);

    REQUIRE(!sa.is_valid());
    const auto& sem_errors = sa.get_errors();
    REQUIRE(sem_errors.size() == 1);
    REQUIRE(sem_errors[0] == "Error [3, 15]: Type mismatch in declaration of 'res': declared as 'float[]' but expression has type 'int[]'");
}

TEST_CASE("Semantic error: call to undefined function", "[semantics][error]")
{
    parser p;
    p.parse(R"(
int x = unknown_func(42)
)");

    const auto& parse_errors = p.get_errors();
    REQUIRE(parse_errors.empty());

    semantic_analyzer sa;
    program& prog = p.get_program();
    sa.analyze(prog);

    REQUIRE(!sa.is_valid());
    const auto& sem_errors = sa.get_errors();
    REQUIRE(sem_errors.size() == 1);
    REQUIRE(sem_errors[0] == "Error [2, 9]: Call to undefined function 'unknown_func'");

    const auto& call = std::get<f_call>(prog.declarations_[0].value_.wrapped_);
    REQUIRE(!call.name_.proto_idx_.has_value());
}

TEST_CASE("Semantic error: argument type mismatch with named parameter in error", "[semantics][error]")
{
    parser p;
    p.parse(R"(
float x = foo(1.0)
)");

    const auto& parse_errors = p.get_errors();
    REQUIRE(parse_errors.empty());

    semantic_analyzer sa;
    sa.register_function_prototype("foo", {type::int_scalar}, {"value"}, type::float_scalar);

    program& prog = p.get_program();
    sa.analyze(prog);

    REQUIRE(!sa.is_valid());
    const auto& sem_errors = sa.get_errors();
    REQUIRE(sem_errors.size() == 1);
    REQUIRE(sem_errors[0] == "Error [2, 15]: Type mismatch for 'value' in call to 'foo': expected 'int' but got 'float'");

    const auto& call = std::get<f_call>(prog.declarations_[0].value_.wrapped_);
    REQUIRE(call.name_.proto_idx_.has_value());
}

TEST_CASE("Semantic error: return type mismatch in declaration", "[semantics][error]")
{
    parser p;
    p.parse(R"(
float x = foo()
)");

    const auto& parse_errors = p.get_errors();
    REQUIRE(parse_errors.empty());

    semantic_analyzer sa;
    sa.register_function_prototype("foo", {}, {}, type::int_scalar);

    program& prog = p.get_program();
    sa.analyze(prog);

    REQUIRE(!sa.is_valid());
    const auto& sem_errors = sa.get_errors();
    REQUIRE(sem_errors.size() == 1);
    REQUIRE(sem_errors[0] == "Error [2, 11]: Type mismatch in declaration of 'x': declared as 'float' but expression has type 'int'");

    const auto& call = std::get<f_call>(prog.declarations_[0].value_.wrapped_);
    REQUIRE(call.name_.proto_idx_.has_value());
}
