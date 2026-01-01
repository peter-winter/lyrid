#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>

using Catch::Matchers::ContainsSubstring;

#include "parser.hpp"
#include "ast.hpp"
#include "semantic_analyzer.hpp"

using namespace lyrid;


TEST_CASE("Semantic error: non-integer array index", "[semantic]")
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
    sa.analyze(p.get_program());

    REQUIRE(!sa.is_valid());
    const auto& sem_errors = sa.get_errors();
    REQUIRE_FALSE(sem_errors.empty());
    REQUIRE_THAT(sem_errors.back(), ContainsSubstring("Array index must be of type 'int', but got 'float'"));
}

TEST_CASE("Semantic error: mixed types in array construction", "[semantic]")
{
    parser p;
    p.parse("int[] arr = [1, 2.0, 3]");

    const auto& parse_errors = p.get_errors();
    REQUIRE(parse_errors.empty());

    semantic_analyzer sa;
    sa.analyze(p.get_program());

    REQUIRE(!sa.is_valid());
    const auto& sem_errors = sa.get_errors();
    REQUIRE_FALSE(sem_errors.empty());
    REQUIRE_THAT(sem_errors[0], ContainsSubstring("Type mismatch in array construction"));
}

TEST_CASE("Semantic error: nested array in construction", "[semantic]")
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
    REQUIRE_FALSE(sem_errors.empty());
    REQUIRE_THAT(sem_errors.back(), ContainsSubstring("Array construction elements must be scalar types"));
}

// tests/test_semantics.cpp
// Replace all existing array comprehension-related TEST_CASE blocks with the following updated versions

TEST_CASE("Semantic valid: basic array comprehension with literal body", "[semantic]")
{
    parser p;
    p.parse(R"(
int[] src = [1, 2, 3]
int[] res = [|i| in |src| do 42]
)");

    const auto& parse_errors = p.get_errors();
    REQUIRE(parse_errors.empty());

    semantic_analyzer sa;
    sa.analyze(p.get_program());

    REQUIRE(sa.is_valid());
    REQUIRE(sa.get_errors().empty());
}

TEST_CASE("Semantic valid: comprehension with multiple sources using variable from scope", "[semantic]")
{
    parser p;
    p.parse(R"(
int[] ints = [1, 2]
float[] floats = [3.0, 4.0]
float[] res = [|i, f| in |ints, floats| do f]
)");

    const auto& parse_errors = p.get_errors();
    REQUIRE(parse_errors.empty());

    semantic_analyzer sa;
    sa.analyze(p.get_program());

    REQUIRE(sa.is_valid());
    REQUIRE(sa.get_errors().empty());
}

TEST_CASE("Semantic error: source expression is not an array", "[semantic]")
{
    parser p;
    p.parse(R"(
int scalar_val = 42
int[] res = [|i| in |scalar_val| do i]
)");

    const auto& parse_errors = p.get_errors();
    REQUIRE(parse_errors.empty());

    semantic_analyzer sa;
    sa.analyze(p.get_program());

    REQUIRE(!sa.is_valid());
    const auto& sem_errors = sa.get_errors();
    REQUIRE_FALSE(sem_errors.empty());
    REQUIRE_THAT(sem_errors.back(), ContainsSubstring("Source in array comprehension must be an array type"));
}

TEST_CASE("Semantic error: 'do' expression is not scalar", "[semantic]")
{
    parser p;
    p.parse(R"(
int[] src = [1, 2, 3]
int[] inner = [4, 5]
int[] res = [|i| in |src| do inner]
)");

    const auto& parse_errors = p.get_errors();
    REQUIRE(parse_errors.empty());

    semantic_analyzer sa;
    sa.analyze(p.get_program());

    REQUIRE(!sa.is_valid());
    const auto& sem_errors = sa.get_errors();
    REQUIRE_FALSE(sem_errors.empty());
    REQUIRE_THAT(sem_errors.back(), ContainsSubstring("'do' expression in array comprehension must be a scalar type"));
}

TEST_CASE("Semantic error: duplicate variable names in comprehension", "[semantic]")
{
    parser p;
    p.parse(R"(
int[] src1 = [1, 2]
int[] src2 = [3, 4]
int[] res = [|x, x| in |src1, src2| do x]
)");

    const auto& parse_errors = p.get_errors();
    REQUIRE(parse_errors.empty());

    semantic_analyzer sa;
    sa.analyze(p.get_program());

    REQUIRE(!sa.is_valid());
    const auto& sem_errors = sa.get_errors();
    REQUIRE_FALSE(sem_errors.empty());
    REQUIRE_THAT(sem_errors.back(), ContainsSubstring("Duplicate variable"));
}

TEST_CASE("Semantic error: comprehension infers mismatched array element type", "[semantic]")
{
    parser p;
    p.parse(R"(
int[] src = [1, 2, 3]
float[] res = [|i| in |src| do 42]
)");

    const auto& parse_errors = p.get_errors();
    REQUIRE(parse_errors.empty());

    semantic_analyzer sa;
    sa.analyze(p.get_program());

    REQUIRE(!sa.is_valid());
    const auto& sem_errors = sa.get_errors();
    REQUIRE_FALSE(sem_errors.empty());
    REQUIRE_THAT(sem_errors.back(), ContainsSubstring("Type mismatch in declaration"));
    REQUIRE_THAT(sem_errors.back(), ContainsSubstring("declared as 'float[]' but expression has type 'int[]'"));
}

TEST_CASE("Semantic valid: simple function call with registered prototype", "[semantic]")
{
    parser p;
    p.parse(R"(
int x = foo(42)
)");

    const auto& parse_errors = p.get_errors();
    REQUIRE(parse_errors.empty());

    semantic_analyzer sa;
    sa.register_function_prototype("foo", {type::int_scalar}, {"arg"}, type::int_scalar);

    sa.analyze(p.get_program());

    REQUIRE(sa.is_valid());
    REQUIRE(sa.get_errors().empty());

    const auto& symbols = sa.get_symbols();
    REQUIRE(symbols.at("x") == type::int_scalar);
}

TEST_CASE("Semantic valid: nested function calls with registered prototypes", "[semantic]")
{
    parser p;
    p.parse(R"(
int y = bar(foo(1, 2.0))
)");

    const auto& parse_errors = p.get_errors();
    REQUIRE(parse_errors.empty());

    semantic_analyzer sa;
    sa.register_function_prototype("foo", {type::int_scalar, type::float_scalar}, {"a", "b"}, type::int_scalar);
    sa.register_function_prototype("bar", {type::int_scalar}, {"x"}, type::int_scalar);

    sa.analyze(p.get_program());

    REQUIRE(sa.is_valid());
    REQUIRE(sa.get_errors().empty());
}

TEST_CASE("Semantic valid: function call returning array used in declaration", "[semantic]")
{
    parser p;
    p.parse(R"(
int[] arr = create_int_array(5)
)");

    const auto& parse_errors = p.get_errors();
    REQUIRE(parse_errors.empty());

    semantic_analyzer sa;
    sa.register_function_prototype("create_int_array", {type::int_scalar}, {"size"}, type::int_array);

    sa.analyze(p.get_program());

    REQUIRE(sa.is_valid());
    REQUIRE(sa.get_errors().empty());
}

TEST_CASE("Semantic valid: function call in array comprehension 'do' expression", "[semantic]")
{
    parser p;
    p.parse(R"(
int[] src = [1, 2, 3]
float[] res = [|i| in |src| do scale(i)]
)");

    const auto& parse_errors = p.get_errors();
    REQUIRE(parse_errors.empty());

    semantic_analyzer sa;
    sa.register_function_prototype("scale", {type::int_scalar}, {"val"}, type::float_scalar);

    sa.analyze(p.get_program());

    REQUIRE(sa.is_valid());
    REQUIRE(sa.get_errors().empty());
}

TEST_CASE("Semantic error: call to undefined function", "[semantic]")
{
    parser p;
    p.parse(R"(
int x = unknown_func(42)
)");

    const auto& parse_errors = p.get_errors();
    REQUIRE(parse_errors.empty());

    semantic_analyzer sa;
    // No prototype registered
    sa.analyze(p.get_program());

    REQUIRE(!sa.is_valid());
    const auto& sem_errors = sa.get_errors();
    REQUIRE_FALSE(sem_errors.empty());
    REQUIRE_THAT(sem_errors[0], ContainsSubstring("Call to undefined function 'unknown_func'"));
}

TEST_CASE("Semantic error: incorrect number of arguments in call", "[semantic]")
{
    parser p;
    p.parse(R"(
int x = foo(1, 2.0)
)");

    const auto& parse_errors = p.get_errors();
    REQUIRE(parse_errors.empty());

    semantic_analyzer sa;
    sa.register_function_prototype("foo", {type::int_scalar}, {"a"}, type::int_scalar);

    sa.analyze(p.get_program());

    REQUIRE(!sa.is_valid());
    const auto& sem_errors = sa.get_errors();
    REQUIRE_FALSE(sem_errors.empty());
    REQUIRE_THAT(sem_errors[0], ContainsSubstring("Incorrect number of arguments"));
    REQUIRE_THAT(sem_errors[0], ContainsSubstring("expected 1 but provided 2"));
}

TEST_CASE("Semantic error: argument type mismatch with named parameter in error", "[semantic]")
{
    parser p;
    p.parse(R"(
float x = foo(1.0)
)");

    const auto& parse_errors = p.get_errors();
    REQUIRE(parse_errors.empty());

    semantic_analyzer sa;
    sa.register_function_prototype("foo", {type::int_scalar}, {"value"}, type::float_scalar);

    sa.analyze(p.get_program());

    REQUIRE(!sa.is_valid());
    const auto& sem_errors = sa.get_errors();
    REQUIRE_FALSE(sem_errors.empty());
    REQUIRE_THAT(sem_errors[0], ContainsSubstring("Type mismatch for 'value'"));
    REQUIRE_THAT(sem_errors[0], ContainsSubstring("expected 'int' but got 'float'"));
}

TEST_CASE("Semantic error: return type mismatch in declaration", "[semantic]")
{
    parser p;
    p.parse(R"(
float x = foo()
)");

    const auto& parse_errors = p.get_errors();
    REQUIRE(parse_errors.empty());

    semantic_analyzer sa;
    sa.register_function_prototype("foo", {}, {}, type::int_scalar);

    sa.analyze(p.get_program());

    REQUIRE(!sa.is_valid());
    const auto& sem_errors = sa.get_errors();
    REQUIRE_FALSE(sem_errors.empty());
    REQUIRE_THAT(sem_errors[0], ContainsSubstring("Type mismatch in declaration"));
    REQUIRE_THAT(sem_errors[0], ContainsSubstring("declared as 'float' but expression has type 'int'"));
}
