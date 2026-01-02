// tests/test_parser.cpp
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>

using Catch::Matchers::ContainsSubstring;

#include "parser.hpp"
#include "ast.hpp"
#include "semantic_analyzer.hpp"

using namespace lyrid;
using namespace lyrid::ast;

TEST_CASE("Empty input is valid", "[valid]")
{
    parser p;
    p.parse("");

    const program& prog = p.get_program();
    const auto& errors = p.get_errors();

    REQUIRE(errors.empty());
    REQUIRE(prog.is_valid());
    REQUIRE(prog.declarations_.empty());
}

TEST_CASE("Single int declaration", "[valid]")
{
    parser p;
    p.parse("int x = 42");

    const program& prog = p.get_program();
    const auto& errors = p.get_errors();

    REQUIRE(errors.empty());
    REQUIRE(prog.is_valid());
    REQUIRE(prog.declarations_.size() == 1);

    const declaration& decl = prog.declarations_[0];
    REQUIRE(decl.type_ == type::int_scalar);
    REQUIRE(decl.name_.value_ == "x");
    REQUIRE(std::holds_alternative<int_scalar>(decl.value_.wrapped_));
    REQUIRE(std::get<int_scalar>(decl.value_.wrapped_).value_ == 42);
}

TEST_CASE("Function call with arguments", "[valid]")
{
    parser p;
    p.parse("int result = foo(1, bar, 3.0)");

    const program& prog = p.get_program();
    const auto& errors = p.get_errors();

    REQUIRE(errors.empty());
    REQUIRE(prog.is_valid());
    REQUIRE(prog.declarations_.size() == 1);

    const declaration& decl = prog.declarations_[0];
    REQUIRE(decl.type_ == type::int_scalar);
    REQUIRE(std::holds_alternative<f_call>(decl.value_.wrapped_));

    const f_call& call = std::get<f_call>(decl.value_.wrapped_);
    REQUIRE(call.name_.value_ == "foo");
    REQUIRE(call.args_.size() == 3);

    REQUIRE(std::holds_alternative<int_scalar>(call.args_[0].wrapped_));
    REQUIRE(std::get<int_scalar>(call.args_[0].wrapped_).value_ == 1);

    REQUIRE(std::holds_alternative<identifier>(call.args_[1].wrapped_));
    REQUIRE(std::get<identifier>(call.args_[1].wrapped_).value_ == "bar");

    REQUIRE(std::holds_alternative<float_scalar>(call.args_[2].wrapped_));
    REQUIRE(std::get<float_scalar>(call.args_[2].wrapped_).value_ == 3.0);
}

TEST_CASE("Array construction with literals", "[valid]")
{
    parser p;
    p.parse(R"(
int[] int_arr = [1, 2, 3]
float[] float_arr = [1.0, 2.0, .5]
)");

    const program& prog = p.get_program();
    const auto& errors = p.get_errors();

    REQUIRE(errors.empty());
    REQUIRE(prog.declarations_.size() == 2);

    const declaration& decl0 = prog.declarations_[0];
    REQUIRE(decl0.type_ == type::int_array);
    REQUIRE(std::holds_alternative<array_construction>(decl0.value_.wrapped_));
    const array_construction& arr0 = std::get<array_construction>(decl0.value_.wrapped_);
    REQUIRE(arr0.elements_.size() == 3);
    REQUIRE(std::holds_alternative<int_scalar>(arr0.elements_[0].wrapped_));
    REQUIRE(std::get<int_scalar>(arr0.elements_[0].wrapped_).value_ == 1);
    REQUIRE(std::get<int_scalar>(arr0.elements_[1].wrapped_).value_ == 2);
    REQUIRE(std::get<int_scalar>(arr0.elements_[2].wrapped_).value_ == 3);

    const declaration& decl1 = prog.declarations_[1];
    REQUIRE(decl1.type_ == type::float_array);
    REQUIRE(std::holds_alternative<array_construction>(decl1.value_.wrapped_));
    const array_construction& arr1 = std::get<array_construction>(decl1.value_.wrapped_);
    REQUIRE(arr1.elements_.size() == 3);
    REQUIRE(std::holds_alternative<float_scalar>(arr1.elements_[0].wrapped_));
    REQUIRE(std::get<float_scalar>(arr1.elements_[0].wrapped_).value_ == 1.0);
    REQUIRE(std::get<float_scalar>(arr1.elements_[1].wrapped_).value_ == 2.0);
    REQUIRE(std::get<float_scalar>(arr1.elements_[2].wrapped_).value_ == 0.5);
}

TEST_CASE("Multiple declarations", "[valid]")
{
    parser p;
    p.parse(R"(
int a = 10
float b = 20.0
int c = foo()
)");

    const program& prog = p.get_program();
    const auto& errors = p.get_errors();

    REQUIRE(errors.empty());
    REQUIRE(prog.is_valid());
    REQUIRE(prog.declarations_.size() == 3);
}

TEST_CASE("Nested function calls in arguments", "[valid]")
{
    parser p;
    p.parse("int result = foo(1, bar(2, 3.0), baz)");

    const program& prog = p.get_program();
    const auto& errors = p.get_errors();

    REQUIRE(errors.empty());
    REQUIRE(prog.is_valid());
    REQUIRE(prog.declarations_.size() == 1);

    const declaration& decl = prog.declarations_[0];
    REQUIRE(decl.type_ == type::int_scalar);
    REQUIRE(decl.name_.value_ == "result");

    REQUIRE(std::holds_alternative<f_call>(decl.value_.wrapped_));
    const f_call& outer = std::get<f_call>(decl.value_.wrapped_);

    REQUIRE(outer.name_.value_ == "foo");
    REQUIRE(outer.args_.size() == 3);

    REQUIRE(std::holds_alternative<int_scalar>(outer.args_[0].wrapped_));
    REQUIRE(std::get<int_scalar>(outer.args_[0].wrapped_).value_ == 1);

    REQUIRE(std::holds_alternative<f_call>(outer.args_[1].wrapped_));
    const f_call& inner = std::get<f_call>(outer.args_[1].wrapped_);
    REQUIRE(inner.name_.value_ == "bar");
    REQUIRE(inner.args_.size() == 2);

    REQUIRE(std::holds_alternative<int_scalar>(inner.args_[0].wrapped_));
    REQUIRE(std::get<int_scalar>(inner.args_[0].wrapped_).value_ == 2);

    REQUIRE(std::holds_alternative<float_scalar>(inner.args_[1].wrapped_));
    REQUIRE(std::get<float_scalar>(inner.args_[1].wrapped_).value_ == 3.0);

    REQUIRE(std::holds_alternative<identifier>(outer.args_[2].wrapped_));
    REQUIRE(std::get<identifier>(outer.args_[2].wrapped_).value_ == "baz");
}

TEST_CASE("Array comprehension: basic single-variable with literal body", "[valid]")
{
    parser p;
    p.parse(R"(
int[] src = [1, 2, 3]
int[] res = [|i| in |src| do 42]
)");

    const program& prog = p.get_program();
    const auto& errors = p.get_errors();

    REQUIRE(errors.empty());
    REQUIRE(prog.is_valid());
    REQUIRE(prog.declarations_.size() == 2);

    const declaration& decl = prog.declarations_[1];
    REQUIRE(decl.type_ == type::int_array);
    REQUIRE(decl.name_.value_ == "res");
    REQUIRE(std::holds_alternative<comprehension>(decl.value_.wrapped_));

    const comprehension& fc = std::get<comprehension>(decl.value_.wrapped_);
    REQUIRE(fc.variables_.size() == 1);
    REQUIRE(fc.variables_[0].value_ == "i");
    REQUIRE(fc.in_exprs_.size() == 1);
    REQUIRE(std::holds_alternative<identifier>(fc.in_exprs_[0].wrapped_));
    REQUIRE(std::get<identifier>(fc.in_exprs_[0].wrapped_).value_ == "src");

    REQUIRE(std::holds_alternative<int_scalar>(fc.do_expr_->wrapped_));
    REQUIRE(std::get<int_scalar>(fc.do_expr_->wrapped_).value_ == 42);
}

TEST_CASE("Array comprehension: multiple variables and sources with call in body", "[valid]")
{
    parser p;
    p.parse(R"(
int[] ints = [1, 2]
float[] floats = [3.0, 4.0]
float[] res = [|i, f| in |ints, floats| do foo(i, f)]
)");

    const program& prog = p.get_program();
    const auto& errors = p.get_errors();

    REQUIRE(errors.empty());
    REQUIRE(prog.is_valid());
    REQUIRE(prog.declarations_.size() == 3);

    const declaration& decl = prog.declarations_[2];
    REQUIRE(decl.type_ == type::float_array);
    REQUIRE(decl.name_.value_ == "res");
    REQUIRE(std::holds_alternative<comprehension>(decl.value_.wrapped_));

    const comprehension& fc = std::get<comprehension>(decl.value_.wrapped_);
    REQUIRE(fc.variables_.size() == 2);
    REQUIRE(fc.variables_[0].value_ == "i");
    REQUIRE(fc.variables_[1].value_ == "f");
    REQUIRE(fc.in_exprs_.size() == 2);
    REQUIRE(std::holds_alternative<identifier>(fc.in_exprs_[0].wrapped_));
    REQUIRE(std::get<identifier>(fc.in_exprs_[0].wrapped_).value_ == "ints");
    REQUIRE(std::holds_alternative<identifier>(fc.in_exprs_[1].wrapped_));
    REQUIRE(std::get<identifier>(fc.in_exprs_[1].wrapped_).value_ == "floats");

    REQUIRE(std::holds_alternative<f_call>(fc.do_expr_->wrapped_));
    const f_call& call = std::get<f_call>(fc.do_expr_->wrapped_);
    REQUIRE(call.name_.value_ == "foo");
    REQUIRE(call.args_.size() == 2);
    REQUIRE(std::holds_alternative<identifier>(call.args_[0].wrapped_));
    REQUIRE(std::get<identifier>(call.args_[0].wrapped_).value_ == "i");
    REQUIRE(std::holds_alternative<identifier>(call.args_[1].wrapped_));
    REQUIRE(std::get<identifier>(call.args_[1].wrapped_).value_ == "f");
}

TEST_CASE("Array comprehension: mismatched variable/source counts", "[invalid]")
{
    parser p;
    p.parse("int[] res = [|x, y| in |src| do 42]");

    const auto& errors = p.get_errors();
    REQUIRE(errors.size() == 1);
    REQUIRE(errors[0] == "Error [1, 14]: Number of variables (2) and source expressions (1) must match in array comprehension");
}

TEST_CASE("Array comprehension: missing 'in' keyword", "[invalid]")
{
    parser p;
    p.parse("int[] res = [|x| foo |src| do 42]");

    const auto& errors = p.get_errors();
    REQUIRE(errors.size() == 1);
    REQUIRE(errors[0] == "Error [1, 14]: Expected 'in' after variable list in array comprehension");
}

TEST_CASE("Array comprehension: missing 'do' keyword", "[invalid]")
{
    parser p;
    p.parse("int[] res = [|x| in |src| 42]");

    const auto& errors = p.get_errors();
    REQUIRE(errors.size() == 1);
    REQUIRE(errors[0] == "Error [1, 14]: Expected 'do' after source list in array comprehension");
}

TEST_CASE("Array comprehension: no variables", "[invalid]")
{
    parser p;
    p.parse("int[] res = [|| in |src| do 42]");

    const auto& errors = p.get_errors();
    REQUIRE(errors.size() == 1);
    REQUIRE(errors[0] == "Error [1, 14]: Array comprehension must have at least one variable");
}

TEST_CASE("Extra characters after expression", "[invalid]")
{
    parser p;
    p.parse("int x = 42 extra");

    const auto& errors = p.get_errors();
    REQUIRE(errors.size() == 1);
    REQUIRE(errors[0] == "Error [1, 12]: Extra characters after expression");
}

TEST_CASE("Unexpected token (unknown type)", "[invalid]")
{
    parser p;
    p.parse("unknown_type x = 1");

    const auto& errors = p.get_errors();
    REQUIRE(errors.size() == 1);
    REQUIRE(errors[0] == "Error [1, 1]: Unknown type 'unknown_type'; expected 'int' or 'float'");
}

TEST_CASE("Array indexing on identifier", "[valid]")
{
    parser p;
    p.parse(R"(
float[] arr = [1.0, 2.0, 3.0]
int idx = 1
float x = arr[idx]
)");

    const program& prog = p.get_program();
    const auto& errors = p.get_errors();

    REQUIRE(errors.empty());
    REQUIRE(prog.declarations_.size() == 3);

    const declaration& decl = prog.declarations_[2];
    REQUIRE(decl.type_ == type::float_scalar);
    REQUIRE(std::holds_alternative<index_access>(decl.value_.wrapped_));

    const index_access& ia = std::get<index_access>(decl.value_.wrapped_);

    REQUIRE(std::holds_alternative<identifier>(ia.base_->wrapped_));
    REQUIRE(std::get<identifier>(ia.base_->wrapped_).value_ == "arr");

    REQUIRE(std::holds_alternative<identifier>(ia.index_->wrapped_));
    REQUIRE(std::get<identifier>(ia.index_->wrapped_).value_ == "idx");
}

TEST_CASE("Array indexing on function call", "[valid]")
{
    parser p;
    p.parse("int x = foo(1, 2)[0]");

    const program& prog = p.get_program();
    const auto& errors = p.get_errors();

    REQUIRE(errors.empty());
    REQUIRE(prog.declarations_.size() == 1);

    const declaration& decl = prog.declarations_[0];
    REQUIRE(std::holds_alternative<index_access>(decl.value_.wrapped_));

    const index_access& ia = std::get<index_access>(decl.value_.wrapped_);

    REQUIRE(std::holds_alternative<f_call>(ia.base_->wrapped_));
    const f_call& call = std::get<f_call>(ia.base_->wrapped_);

    REQUIRE(call.name_.value_ == "foo");
    REQUIRE(call.args_.size() == 2);
}

TEST_CASE("Array indexing on construction", "[valid]")
{
    parser p;
    p.parse("float x = [1.0, 2.0, 3.0][2]");

    const program& prog = p.get_program();
    const auto& errors = p.get_errors();

    REQUIRE(errors.empty());
    REQUIRE(prog.declarations_.size() == 1);

    const declaration& decl = prog.declarations_[0];
    REQUIRE(std::holds_alternative<index_access>(decl.value_.wrapped_));

    const index_access& ia = std::get<index_access>(decl.value_.wrapped_);

    REQUIRE(std::holds_alternative<array_construction>(ia.base_->wrapped_));
    const array_construction& arr = std::get<array_construction>(ia.base_->wrapped_);
    REQUIRE(arr.elements_.size() == 3);

    REQUIRE(std::holds_alternative<int_scalar>(ia.index_->wrapped_));
    REQUIRE(std::get<int_scalar>(ia.index_->wrapped_).value_ == 2);
}

TEST_CASE("Array comprehension: indexing on comprehension", "[valid]")
{
    parser p;
    p.parse(R"(
int[] src = [10, 20, 30]
int x = [|i| in |src| do i][1]
)");

    const program& prog = p.get_program();
    const auto& errors = p.get_errors();

    REQUIRE(errors.empty());
    REQUIRE(prog.is_valid());
    REQUIRE(prog.declarations_.size() == 2);

    const declaration& decl = prog.declarations_[1];
    REQUIRE(decl.type_ == type::int_scalar);
    REQUIRE(std::holds_alternative<index_access>(decl.value_.wrapped_));

    const index_access& ia = std::get<index_access>(decl.value_.wrapped_);
    REQUIRE(std::holds_alternative<comprehension>(ia.base_->wrapped_));

    const comprehension& fc = std::get<comprehension>(ia.base_->wrapped_);
    REQUIRE(fc.variables_.size() == 1);
    REQUIRE(fc.variables_[0].value_ == "i");
    REQUIRE(std::holds_alternative<identifier>(fc.in_exprs_[0].wrapped_));
    REQUIRE(std::get<identifier>(fc.in_exprs_[0].wrapped_).value_ == "src");
    REQUIRE(std::holds_alternative<identifier>(fc.do_expr_->wrapped_));
    REQUIRE(std::get<identifier>(fc.do_expr_->wrapped_).value_ == "i");

    REQUIRE(std::holds_alternative<int_scalar>(ia.index_->wrapped_));
    REQUIRE(std::get<int_scalar>(ia.index_->wrapped_).value_ == 1);
}

TEST_CASE("Chained indexing causes syntax error", "[invalid]")
{
    parser p;
    p.parse("int x = arr[0][1]");

    const auto& errors = p.get_errors();
    REQUIRE(errors.size() == 1);
    REQUIRE(errors[0] == "Error [1, 15]: Extra characters after expression");
}

TEST_CASE("Missing closing bracket in index", "[invalid]")
{
    parser p;
    p.parse("int x = arr[0");

    const auto& errors = p.get_errors();
    REQUIRE(errors.size() == 1);
    REQUIRE(errors[0] == "Error [1, 14]: Expected ']' after index expression");
}
