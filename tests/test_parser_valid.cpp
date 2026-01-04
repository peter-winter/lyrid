#include <catch2/catch_test_macros.hpp>

#include "parser.hpp"
#include "ast.hpp"

using namespace lyrid;
using namespace lyrid::ast;

TEST_CASE("Empty input is valid", "[parser][valid]")
{
    parser p;
    p.parse("");

    const program& prog = p.get_program();
    const auto& errors = p.get_errors();

    REQUIRE(errors.empty());
    REQUIRE(prog.is_valid());
    REQUIRE(prog.declarations_.empty());
}

TEST_CASE("Single int declaration", "[parser][valid]")
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

TEST_CASE("Function call with arguments", "[parser][valid]")
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
    REQUIRE(call.fn_.ident_.value_ == "foo");
    REQUIRE(call.args_.size() == 3);

    REQUIRE(std::holds_alternative<int_scalar>(call.args_[0].wrapped_));
    REQUIRE(std::get<int_scalar>(call.args_[0].wrapped_).value_ == 1);

    REQUIRE(std::holds_alternative<symbol_ref>(call.args_[1].wrapped_));
    REQUIRE(std::get<symbol_ref>(call.args_[1].wrapped_).ident_.value_ == "bar");

    REQUIRE(std::holds_alternative<float_scalar>(call.args_[2].wrapped_));
    REQUIRE(std::get<float_scalar>(call.args_[2].wrapped_).value_ == 3.0);
}

TEST_CASE("Array construction with literals", "[parser][valid]")
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

TEST_CASE("Multiple declarations", "[parser][valid]")
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

TEST_CASE("Nested function calls in arguments", "[parser][valid]")
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

    REQUIRE(outer.fn_.ident_.value_ == "foo");
    REQUIRE(outer.args_.size() == 3);

    REQUIRE(std::holds_alternative<int_scalar>(outer.args_[0].wrapped_));
    REQUIRE(std::get<int_scalar>(outer.args_[0].wrapped_).value_ == 1);

    REQUIRE(std::holds_alternative<f_call>(outer.args_[1].wrapped_));
    const f_call& inner = std::get<f_call>(outer.args_[1].wrapped_);
    REQUIRE(inner.fn_.ident_.value_ == "bar");
    REQUIRE(inner.args_.size() == 2);

    REQUIRE(std::holds_alternative<int_scalar>(inner.args_[0].wrapped_));
    REQUIRE(std::get<int_scalar>(inner.args_[0].wrapped_).value_ == 2);

    REQUIRE(std::holds_alternative<float_scalar>(inner.args_[1].wrapped_));
    REQUIRE(std::get<float_scalar>(inner.args_[1].wrapped_).value_ == 3.0);

    REQUIRE(std::holds_alternative<symbol_ref>(outer.args_[2].wrapped_));
    REQUIRE(std::get<symbol_ref>(outer.args_[2].wrapped_).ident_.value_ == "baz");
}

TEST_CASE("Array comprehension: basic single-variable with literal body", "[parser][valid]")
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
    REQUIRE(std::holds_alternative<symbol_ref>(fc.in_exprs_[0].wrapped_));
    REQUIRE(std::get<symbol_ref>(fc.in_exprs_[0].wrapped_).ident_.value_ == "src");

    REQUIRE(std::holds_alternative<int_scalar>(fc.do_expr_->wrapped_));
    REQUIRE(std::get<int_scalar>(fc.do_expr_->wrapped_).value_ == 42);
}

TEST_CASE("Array comprehension: multiple variables and sources with call in body", "[parser][valid]")
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
    REQUIRE(std::holds_alternative<symbol_ref>(fc.in_exprs_[0].wrapped_));
    REQUIRE(std::get<symbol_ref>(fc.in_exprs_[0].wrapped_).ident_.value_ == "ints");
    REQUIRE(std::holds_alternative<symbol_ref>(fc.in_exprs_[1].wrapped_));
    REQUIRE(std::get<symbol_ref>(fc.in_exprs_[1].wrapped_).ident_.value_ == "floats");

    REQUIRE(std::holds_alternative<f_call>(fc.do_expr_->wrapped_));
    const f_call& call = std::get<f_call>(fc.do_expr_->wrapped_);
    REQUIRE(call.fn_.ident_.value_ == "foo");
    REQUIRE(call.args_.size() == 2);
    REQUIRE(std::holds_alternative<symbol_ref>(call.args_[0].wrapped_));
    REQUIRE(std::get<symbol_ref>(call.args_[0].wrapped_).ident_.value_ == "i");
    REQUIRE(std::holds_alternative<symbol_ref>(call.args_[1].wrapped_));
    REQUIRE(std::get<symbol_ref>(call.args_[1].wrapped_).ident_.value_ == "f");
}

TEST_CASE("Array indexing on identifier", "[parser][valid]")
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

    REQUIRE(std::holds_alternative<symbol_ref>(ia.base_->wrapped_));
    REQUIRE(std::get<symbol_ref>(ia.base_->wrapped_).ident_.value_ == "arr");

    REQUIRE(std::holds_alternative<symbol_ref>(ia.index_->wrapped_));
    REQUIRE(std::get<symbol_ref>(ia.index_->wrapped_).ident_.value_ == "idx");
}

TEST_CASE("Array indexing on function call", "[parser][valid]")
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

    REQUIRE(call.fn_.ident_.value_ == "foo");
    REQUIRE(call.args_.size() == 2);
}

TEST_CASE("Array indexing on construction", "[parser][valid]")
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

TEST_CASE("Array comprehension: indexing on comprehension", "[parser][valid]")
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
    REQUIRE(std::holds_alternative<symbol_ref>(fc.in_exprs_[0].wrapped_));
    REQUIRE(std::get<symbol_ref>(fc.in_exprs_[0].wrapped_).ident_.value_ == "src");
    REQUIRE(std::holds_alternative<symbol_ref>(fc.do_expr_->wrapped_));
    REQUIRE(std::get<symbol_ref>(fc.do_expr_->wrapped_).ident_.value_ == "i");

    REQUIRE(std::holds_alternative<int_scalar>(ia.index_->wrapped_));
    REQUIRE(std::get<int_scalar>(ia.index_->wrapped_).value_ == 1);
}

TEST_CASE("Nested array comprehension", "[parser][valid]")
{
    parser p;
    p.parse(R"(
int[] a = [1,2,3]
int[] b = [4,5,6]
int[] res = [|i| in |a| do [|j| in |b| do foo(i, j)]]
)");

    const program& prog = p.get_program();
    const auto& errors = p.get_errors();

    REQUIRE(errors.empty());
    REQUIRE(prog.declarations_.size() == 3);

    const declaration& decl_res = prog.declarations_[2];
    REQUIRE(std::holds_alternative<comprehension>(decl_res.value_.wrapped_));
    const comprehension& outer = std::get<comprehension>(decl_res.value_.wrapped_);

    REQUIRE(std::holds_alternative<comprehension>(outer.do_expr_->wrapped_));
    const comprehension& inner = std::get<comprehension>(outer.do_expr_->wrapped_);

    REQUIRE(inner.variables_[0].value_ == "j");
    REQUIRE(std::holds_alternative<symbol_ref>(inner.in_exprs_[0].wrapped_));
    REQUIRE(std::get<symbol_ref>(inner.in_exprs_[0].wrapped_).ident_.value_ == "b");

    REQUIRE(std::holds_alternative<f_call>(inner.do_expr_->wrapped_));
    const f_call& call = std::get<f_call>(inner.do_expr_->wrapped_);
    REQUIRE(call.fn_.ident_.value_ == "foo");
    REQUIRE(std::holds_alternative<symbol_ref>(call.args_[0].wrapped_));
    REQUIRE(std::get<symbol_ref>(call.args_[0].wrapped_).ident_.value_ == "i");
    REQUIRE(std::holds_alternative<symbol_ref>(call.args_[1].wrapped_));
    REQUIRE(std::get<symbol_ref>(call.args_[1].wrapped_).ident_.value_ == "j");
}

TEST_CASE("Int literals - valid values", "[parser][valid]")
{
    parser p;
    p.parse(R"(
float a = 9223372036854775807
float b = -9223372036854775808
float c = -9
float d = 92
float e = 1000
)");

    const auto& errors = p.get_errors();
    REQUIRE(errors.empty());

    const program& prog = p.get_program();
    REQUIRE(prog.declarations_.size() == 5);

    auto check_int = [](const expr_wrapper& ew, int64_t expected)
    {
        REQUIRE(std::holds_alternative<int_scalar>(ew.wrapped_));
        int64_t val = std::get<int_scalar>(ew.wrapped_).value_;
        REQUIRE(val == expected);
    };

    check_int(prog.declarations_[0].value_, std::numeric_limits<int64_t>::max());   // 2^63-1
    check_int(prog.declarations_[1].value_, std::numeric_limits<int64_t>::min());  // -2^63
    check_int(prog.declarations_[2].value_, -9);
    check_int(prog.declarations_[3].value_, 92);
    check_int(prog.declarations_[4].value_, 1000);
}

TEST_CASE("Float literals - valid values", "[parser][valid]")
{
    parser p;
    p.parse(R"(
float a = 1e3
float b = 1.23E+4
float c = .5e-2
float d = 5.E6
float e = 123.e0
float f = 0.0e+0
float g = 1.2E-3
float h = 1.7976931348623157e308
float i = 1.0
float j = -1.2
float k = 34.5
float l = .67
float m = -1.2E-3
)");

    const auto& errors = p.get_errors();
    REQUIRE(errors.empty());

    const program& prog = p.get_program();
    REQUIRE(prog.declarations_.size() == 13);

    auto check_float = [](const expr_wrapper& ew, double expected)
    {
        REQUIRE(std::holds_alternative<float_scalar>(ew.wrapped_));
        double val = std::get<float_scalar>(ew.wrapped_).value_;
        REQUIRE(val == expected);
    };

    check_float(prog.declarations_[0].value_, 1000.0);
    check_float(prog.declarations_[1].value_, 12300.0);
    check_float(prog.declarations_[2].value_, 0.005);
    check_float(prog.declarations_[3].value_, 5000000.0);
    check_float(prog.declarations_[4].value_, 123.0);
    check_float(prog.declarations_[5].value_, 0.0);
    check_float(prog.declarations_[6].value_, 0.0012);
    check_float(prog.declarations_[7].value_, 1.7976931348623157e308);
    check_float(prog.declarations_[8].value_, 1.0);
    check_float(prog.declarations_[9].value_, -1.2);
    check_float(prog.declarations_[10].value_, 34.5);
    check_float(prog.declarations_[11].value_, 0.67);
    check_float(prog.declarations_[12].value_, -0.0012);
}

TEST_CASE("Array literals containing scientific notation floats", "[parser][valid]")
{
    parser p;
    p.parse("float[] arr = [1e0, 2.5E1, .3e-2, 4.E+3]");

    const auto& errors = p.get_errors();
    REQUIRE(errors.empty());

    const program& prog = p.get_program();
    REQUIRE(prog.declarations_.size() == 1);

    const declaration& decl = prog.declarations_[0];
    REQUIRE(decl.type_ == type::float_array);
    REQUIRE(std::holds_alternative<array_construction>(decl.value_.wrapped_));

    const array_construction& arr = std::get<array_construction>(decl.value_.wrapped_);
    REQUIRE(arr.elements_.size() == 4);

    REQUIRE(std::get<float_scalar>(arr.elements_[0].wrapped_).value_ == 1.0);
    REQUIRE(std::get<float_scalar>(arr.elements_[1].wrapped_).value_ == 25.0);
    REQUIRE(std::get<float_scalar>(arr.elements_[2].wrapped_).value_ == 0.003);
    REQUIRE(std::get<float_scalar>(arr.elements_[3].wrapped_).value_ == 4000.0);
}

