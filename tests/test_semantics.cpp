// tests/test_semantics.cpp
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>

#include "parser.hpp"
#include "ast.hpp"
#include "semantic_analyzer.hpp"

using namespace lyrid;
using namespace lyrid::ast;

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
    program& prog = p.get_program();
    sa.analyze(prog);

    REQUIRE(!sa.is_valid());
    const auto& sem_errors = sa.get_errors();
    REQUIRE(sem_errors.size() == 1);
    REQUIRE(sem_errors[0] == "Error [4, 13]: Array index must be of type 'int', but got 'float'");

    const auto& decl_x = prog.declarations_[2];
    REQUIRE(!decl_x.value_.inferred_type_.has_value());  // Root not inferred due to error
    REQUIRE(std::holds_alternative<index_access>(decl_x.value_.wrapped_));
    const auto& ia = std::get<index_access>(decl_x.value_.wrapped_);
    REQUIRE(ia.base_->inferred_type_.has_value());
    REQUIRE(ia.base_->inferred_type_.value() == type::int_array);
    REQUIRE(ia.index_->inferred_type_.has_value());
    REQUIRE(ia.index_->inferred_type_.value() == type::float_scalar);
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
    REQUIRE(sem_errors.size() == 1);
    REQUIRE(sem_errors[0] == "Error [1, 17]: Type mismatch in array construction: expected 'int' but got 'float'");
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
    REQUIRE(sem_errors.size() == 1);
    REQUIRE(sem_errors[0] == "Error [3, 16]: Array construction elements must be scalar types, but got 'int[]'");
}

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
    program& prog = p.get_program();
    sa.analyze(prog);

    REQUIRE(sa.is_valid());
    REQUIRE(sa.get_errors().empty());

    const auto& decl_res = prog.declarations_[1];
    REQUIRE(decl_res.value_.inferred_type_.has_value());
    REQUIRE(decl_res.value_.inferred_type_.value() == type::int_array);

    REQUIRE(std::holds_alternative<comprehension>(decl_res.value_.wrapped_));
    const auto& comp = std::get<comprehension>(decl_res.value_.wrapped_);

    REQUIRE(comp.in_exprs_[0].inferred_type_.has_value());
    REQUIRE(comp.in_exprs_[0].inferred_type_.value() == type::int_array);

    REQUIRE(comp.do_expr_->inferred_type_.has_value());
    REQUIRE(comp.do_expr_->inferred_type_.value() == type::int_scalar);
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
    program& prog = p.get_program();
    sa.analyze(prog);

    REQUIRE(sa.is_valid());
    REQUIRE(sa.get_errors().empty());

    const auto& decl_res = prog.declarations_[2];
    REQUIRE(decl_res.value_.inferred_type_.has_value());
    REQUIRE(decl_res.value_.inferred_type_.value() == type::float_array);

    REQUIRE(std::holds_alternative<comprehension>(decl_res.value_.wrapped_));
    const auto& comp = std::get<comprehension>(decl_res.value_.wrapped_);

    REQUIRE(comp.in_exprs_[0].inferred_type_.has_value());
    REQUIRE(comp.in_exprs_[0].inferred_type_.value() == type::int_array);
    REQUIRE(comp.in_exprs_[1].inferred_type_.has_value());
    REQUIRE(comp.in_exprs_[1].inferred_type_.value() == type::float_array);

    REQUIRE(comp.do_expr_->inferred_type_.has_value());
    REQUIRE(comp.do_expr_->inferred_type_.value() == type::float_scalar);
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

    program& prog = p.get_program();
    sa.analyze(prog);

    REQUIRE(sa.is_valid());
    REQUIRE(sa.get_errors().empty());

    const auto& decl_x = prog.declarations_[0];
    REQUIRE(decl_x.value_.inferred_type_.has_value());
    REQUIRE(decl_x.value_.inferred_type_.value() == type::int_scalar);

    REQUIRE(std::holds_alternative<f_call>(decl_x.value_.wrapped_));
    const auto& call = std::get<f_call>(decl_x.value_.wrapped_);
    REQUIRE(call.args_[0].inferred_type_.has_value());
    REQUIRE(call.args_[0].inferred_type_.value() == type::int_scalar);
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

    program& prog = p.get_program();
    sa.analyze(prog);

    REQUIRE(sa.is_valid());
    REQUIRE(sa.get_errors().empty());

    const auto& decl_y = prog.declarations_[0];
    REQUIRE(decl_y.value_.inferred_type_.has_value());
    REQUIRE(decl_y.value_.inferred_type_.value() == type::int_scalar);

    REQUIRE(std::holds_alternative<f_call>(decl_y.value_.wrapped_));
    const auto& outer_call = std::get<f_call>(decl_y.value_.wrapped_);
    REQUIRE(outer_call.args_[0].inferred_type_.has_value());
    REQUIRE(outer_call.args_[0].inferred_type_.value() == type::int_scalar);

    REQUIRE(std::holds_alternative<f_call>(outer_call.args_[0].wrapped_));
    const auto& inner_call = std::get<f_call>(outer_call.args_[0].wrapped_);
    REQUIRE(inner_call.args_[0].inferred_type_.has_value());
    REQUIRE(inner_call.args_[0].inferred_type_.value() == type::int_scalar);
    REQUIRE(inner_call.args_[1].inferred_type_.has_value());
    REQUIRE(inner_call.args_[1].inferred_type_.value() == type::float_scalar);
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

    program& prog = p.get_program();
    sa.analyze(prog);

    REQUIRE(sa.is_valid());
    REQUIRE(sa.get_errors().empty());

    const auto& decl_arr = prog.declarations_[0];
    REQUIRE(decl_arr.value_.inferred_type_.has_value());
    REQUIRE(decl_arr.value_.inferred_type_.value() == type::int_array);

    REQUIRE(std::holds_alternative<f_call>(decl_arr.value_.wrapped_));
    const auto& call = std::get<f_call>(decl_arr.value_.wrapped_);
    REQUIRE(call.args_[0].inferred_type_.has_value());
    REQUIRE(call.args_[0].inferred_type_.value() == type::int_scalar);
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

    program& prog = p.get_program();
    sa.analyze(prog);

    REQUIRE(sa.is_valid());
    REQUIRE(sa.get_errors().empty());

    const auto& decl_res = prog.declarations_[1];
    REQUIRE(decl_res.value_.inferred_type_.has_value());
    REQUIRE(decl_res.value_.inferred_type_.value() == type::float_array);

    REQUIRE(std::holds_alternative<comprehension>(decl_res.value_.wrapped_));
    const auto& comp = std::get<comprehension>(decl_res.value_.wrapped_);

    REQUIRE(comp.in_exprs_[0].inferred_type_.has_value());
    REQUIRE(comp.in_exprs_[0].inferred_type_.value() == type::int_array);

    REQUIRE(comp.do_expr_->inferred_type_.has_value());
    REQUIRE(comp.do_expr_->inferred_type_.value() == type::float_scalar);

    REQUIRE(std::holds_alternative<f_call>(comp.do_expr_->wrapped_));
    const auto& call = std::get<f_call>(comp.do_expr_->wrapped_);
    REQUIRE(call.args_[0].inferred_type_.has_value());
    REQUIRE(call.args_[0].inferred_type_.value() == type::int_scalar);
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
    program& prog = p.get_program();
    sa.analyze(prog);

    REQUIRE(!sa.is_valid());
    const auto& sem_errors = sa.get_errors();
    REQUIRE(sem_errors.size() == 1);
    REQUIRE(sem_errors[0] == "Error [3, 15]: Type mismatch in declaration of 'res': declared as 'float[]' but expression has type 'int[]'");

    const auto& decl_res = prog.declarations_[1];
    REQUIRE(decl_res.value_.inferred_type_.has_value());
    REQUIRE(decl_res.value_.inferred_type_.value() == type::int_array);  // Inferred correctly, but mismatched declaration
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
    program& prog = p.get_program();
    sa.analyze(prog);

    REQUIRE(!sa.is_valid());
    const auto& sem_errors = sa.get_errors();
    REQUIRE(sem_errors.size() == 1);
    REQUIRE(sem_errors[0] == "Error [2, 9]: Call to undefined function 'unknown_func'");

    const auto& decl_x = prog.declarations_[0];
    REQUIRE(!decl_x.value_.inferred_type_.has_value());  // Root not inferred due to undefined function
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

    program& prog = p.get_program();
    sa.analyze(prog);

    REQUIRE(!sa.is_valid());
    const auto& sem_errors = sa.get_errors();
    REQUIRE(sem_errors.size() == 1);
    REQUIRE(sem_errors[0] == "Error [2, 15]: Type mismatch for 'value' in call to 'foo': expected 'int' but got 'float'");

    const auto& decl_x = prog.declarations_[0];
    REQUIRE(!decl_x.value_.inferred_type_.has_value());  // Root not inferred due to arg mismatch

    REQUIRE(std::holds_alternative<f_call>(decl_x.value_.wrapped_));
    const auto& call = std::get<f_call>(decl_x.value_.wrapped_);
    REQUIRE(call.args_[0].inferred_type_.has_value());
    REQUIRE(call.args_[0].inferred_type_.value() == type::float_scalar);
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

    program& prog = p.get_program();
    sa.analyze(prog);

    REQUIRE(!sa.is_valid());
    const auto& sem_errors = sa.get_errors();
    REQUIRE(sem_errors.size() == 1);
    REQUIRE(sem_errors[0] == "Error [2, 11]: Type mismatch in declaration of 'x': declared as 'float' but expression has type 'int'");

    const auto& decl_x = prog.declarations_[0];
    REQUIRE(decl_x.value_.inferred_type_.has_value());
    REQUIRE(decl_x.value_.inferred_type_.value() == type::int_scalar);  // Inferred return type set before declaration mismatch
}

