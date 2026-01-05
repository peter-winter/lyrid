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
    REQUIRE(!call.fn_.proto_idx_.has_value());
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
    sa.register_function_prototype("foo", {int_scalar_type{}}, {"value"}, float_scalar_type{});

    program& prog = p.get_program();
    sa.analyze(prog);

    REQUIRE(!sa.is_valid());
    const auto& sem_errors = sa.get_errors();
    REQUIRE(sem_errors.size() == 1);
    REQUIRE(sem_errors[0] == "Error [2, 15]: Type mismatch for 'value' in call to 'foo': expected 'int' but got 'float'");

    const auto& call = std::get<f_call>(prog.declarations_[0].value_.wrapped_);
    REQUIRE(call.fn_.proto_idx_.has_value());
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
    sa.register_function_prototype("foo", {}, {}, int_scalar_type{});

    program& prog = p.get_program();
    sa.analyze(prog);

    REQUIRE(!sa.is_valid());
    const auto& sem_errors = sa.get_errors();
    REQUIRE(sem_errors.size() == 1);
    REQUIRE(sem_errors[0] == "Error [2, 11]: Type mismatch in declaration of 'x': declared as 'float' but expression has type 'int'");

    const auto& call = std::get<f_call>(prog.declarations_[0].value_.wrapped_);
    REQUIRE(call.fn_.proto_idx_.has_value());
}

TEST_CASE("Semantic error: redeclaration of variable", "[semantics][error]")
{
    parser p;
    p.parse(R"(
int x = 10
int x = 20
)");

    const auto& parse_errors = p.get_errors();
    REQUIRE(parse_errors.empty());

    semantic_analyzer sa;
    program& prog = p.get_program();
    sa.analyze(prog);

    REQUIRE(!sa.is_valid());
    const auto& sem_errors = sa.get_errors();
    REQUIRE(sem_errors.size() == 1);
    REQUIRE(sem_errors[0] == "Error [3, 5]: Redeclaration of variable 'x'");
}

TEST_CASE("Semantic error: reference to undefined variable", "[semantics][error]")
{
    parser p;
    p.parse("int y = x");

    const auto& parse_errors = p.get_errors();
    REQUIRE(parse_errors.empty());

    semantic_analyzer sa;
    program& prog = p.get_program();
    sa.analyze(prog);

    REQUIRE(!sa.is_valid());
    const auto& sem_errors = sa.get_errors();
    REQUIRE(sem_errors.size() == 1);
    REQUIRE(sem_errors[0] == "Error [1, 9]: Undefined variable 'x'");
}

TEST_CASE("Semantic error: incorrect number of arguments in function call", "[semantics][error]")
{
    parser p;
    p.parse("int res = foo(1, 2.0)");

    const auto& parse_errors = p.get_errors();
    REQUIRE(parse_errors.empty());

    semantic_analyzer sa;
    sa.register_function_prototype("foo", {int_scalar_type{}}, {"a"}, int_scalar_type{});

    program& prog = p.get_program();
    sa.analyze(prog);

    REQUIRE(!sa.is_valid());
    const auto& sem_errors = sa.get_errors();
    REQUIRE(sem_errors.size() == 1);
    REQUIRE(sem_errors[0] == "Error [1, 11]: Incorrect number of arguments in call to 'foo': expected 1 but provided 2");
}

TEST_CASE("Semantic error: indexing non-array type", "[semantics][error]")
{
    parser p;
    p.parse(R"(
int scalar = 42
int x = scalar[0]
)");

    const auto& parse_errors = p.get_errors();
    REQUIRE(parse_errors.empty());

    semantic_analyzer sa;
    program& prog = p.get_program();
    sa.analyze(prog);

    REQUIRE(!sa.is_valid());
    const auto& sem_errors = sa.get_errors();
    REQUIRE(sem_errors.size() == 1);
    REQUIRE(sem_errors[0] == "Error [3, 9]: Indexing applied to non-array type 'int'");
}

TEST_CASE("Semantic error: comprehension source expression not an array", "[semantics][error]")
{
    parser p;
    p.parse(R"(
int scalar = 42
int[] res = [|i| in |scalar| do i]
)");

    const auto& parse_errors = p.get_errors();
    REQUIRE(parse_errors.empty());

    semantic_analyzer sa;
    program& prog = p.get_program();
    sa.analyze(prog);

    REQUIRE(!sa.is_valid());
    const auto& sem_errors = sa.get_errors();
    REQUIRE(sem_errors.size() == 1);
    REQUIRE(sem_errors[0] == "Error [3, 22]: Source in array comprehension must be an array type, got 'int'");
}

TEST_CASE("Semantic error: comprehension 'do' expression not scalar", "[semantics][error]")
{
    parser p;
    p.parse(R"(
int[] src = [1, 2]
int[] inner = [|j| in |src| do j]
int[] res = [|i| in |src| do inner]
)");

    const auto& parse_errors = p.get_errors();
    REQUIRE(parse_errors.empty());

    semantic_analyzer sa;
    program& prog = p.get_program();
    sa.analyze(prog);

    REQUIRE(!sa.is_valid());
    const auto& sem_errors = sa.get_errors();
    REQUIRE(sem_errors.size() == 1);
    REQUIRE(sem_errors[0] == "Error [4, 30]: 'do' expression in array comprehension must be a scalar type, got 'int[]'");
}

TEST_CASE("Semantic error: duplicate variable in comprehension", "[semantics][error]")
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
    program& prog = p.get_program();
    sa.analyze(prog);

    REQUIRE(!sa.is_valid());
    const auto& sem_errors = sa.get_errors();
    REQUIRE(sem_errors.size() == 1);
    REQUIRE(sem_errors[0] == "Error [4, 18]: Duplicate variable 'x' in array comprehension");
}
