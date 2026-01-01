#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>

using Catch::Matchers::ContainsSubstring;

#include "parser.hpp"
#include "ast.hpp"

using namespace lyrid;

TEST_CASE("Empty input is valid", "[valid]")
{
    parser p;
    auto opt = p.parse("");

    REQUIRE(opt.has_value());
    const program& prog = *opt;

    REQUIRE(prog.is_valid());
    REQUIRE(prog.declarations_.empty());
}

TEST_CASE("Single int declaration", "[valid]")
{
    parser p;
    auto opt = p.parse("int x = 42");

    REQUIRE(opt.has_value());
    const program& prog = *opt;

    REQUIRE(prog.is_valid());
    REQUIRE(prog.declarations_.size() == 1);

    const declaration& decl = prog.declarations_[0];
    REQUIRE(decl.type_ == type::int_scalar);
    REQUIRE(decl.name_ == "x");
    REQUIRE(std::holds_alternative<int32_t>(decl.value_.wrapped_));
    REQUIRE(std::get<int32_t>(decl.value_.wrapped_) == 42);
}

TEST_CASE("Function call with arguments", "[valid]")
{
    parser p;
    auto opt = p.parse("int result = foo(1, bar, 3.0)");

    REQUIRE(opt.has_value());
    const program& prog = *opt;

    REQUIRE(prog.is_valid());
    REQUIRE(prog.declarations_.size() == 1);

    const declaration& decl = prog.declarations_[0];
    REQUIRE(decl.type_ == type::int_scalar);
    REQUIRE(std::holds_alternative<f_call>(decl.value_.wrapped_));

    const f_call& call = std::get<f_call>(decl.value_.wrapped_);
    REQUIRE(call.name_ == "foo");
    REQUIRE(call.args_.size() == 3);

    REQUIRE(std::holds_alternative<int32_t>(call.args_[0].wrapped_));
    REQUIRE(std::get<int32_t>(call.args_[0].wrapped_) == 1);

    REQUIRE(std::holds_alternative<id>(call.args_[1].wrapped_));
    REQUIRE(std::get<id>(call.args_[1].wrapped_) == "bar");

    REQUIRE(std::holds_alternative<float>(call.args_[2].wrapped_));
    REQUIRE(std::get<float>(call.args_[2].wrapped_) == 3.0f);
}

TEST_CASE("Array literals and array types", "[valid]")
{
    parser p;
    auto opt = p.parse(R"(
int[] int_arr = [1, 2, 3]
float[] float_arr = [1.0, 2.0, .5]
)");

    REQUIRE(opt.has_value());
    const program& prog = *opt;

    REQUIRE(prog.is_valid());
    REQUIRE(prog.declarations_.size() == 2);

    REQUIRE(prog.declarations_[0].type_ == type::int_array);
    REQUIRE(std::holds_alternative<int_array>(prog.declarations_[0].value_.wrapped_));
    const int_array& iarr = std::get<int_array>(prog.declarations_[0].value_.wrapped_);
    REQUIRE(iarr.values_ == std::vector<int32_t>{1, 2, 3});

    REQUIRE(prog.declarations_[1].type_ == type::float_array);
    REQUIRE(std::holds_alternative<float_array>(prog.declarations_[1].value_.wrapped_));
    const float_array& farr = std::get<float_array>(prog.declarations_[1].value_.wrapped_);
    REQUIRE(farr.values_.size() == 3);
    REQUIRE(farr.values_[2] == 0.5f);
}

TEST_CASE("Multiple declarations", "[valid]")
{
    parser p;
    auto opt = p.parse(R"(
int a = 10
float b = 20.0
int c = foo()
)");

    REQUIRE(opt.has_value());
    const program& prog = *opt;

    REQUIRE(prog.is_valid());
    REQUIRE(prog.declarations_.size() == 3);
}

TEST_CASE("Empty array literal causes syntax error", "[invalid]")
{
    parser p;
    auto opt = p.parse("int[] arr = []");

    REQUIRE_FALSE(opt.has_value());

    const auto& errors = p.get_errors();
    REQUIRE(errors.size() == 1);
    REQUIRE_THAT(errors[0], ContainsSubstring("Line 1"));
    REQUIRE_THAT(errors[0], ContainsSubstring("Empty array literals are not allowed"));
}

TEST_CASE("Extra characters after expression", "[invalid]")
{
    parser p;
    auto opt = p.parse("int x = 42 extra");

    REQUIRE_FALSE(opt.has_value());

    const auto& errors = p.get_errors();
    REQUIRE(errors.size() == 1);
    REQUIRE_THAT(errors[0], ContainsSubstring("Line 1"));
    REQUIRE_THAT(errors[0], ContainsSubstring("Extra characters after expression"));
}

TEST_CASE("Unexpected token (unknown type)", "[invalid]")
{
    parser p;
    auto opt = p.parse("unknown_type x = 1");

    REQUIRE_FALSE(opt.has_value());

    const auto& errors = p.get_errors();
    REQUIRE(errors.size() == 1);
    REQUIRE_THAT(errors[0], ContainsSubstring("Line 1"));
    REQUIRE_THAT(errors[0], ContainsSubstring("Unknown type"));
}

TEST_CASE("Multiple errors across lines", "[invalid]")
{
    parser p;
    auto opt = p.parse(R"(
int[] bad_array = []
int valid = 10
unknown_type bad = foo()
int another = 20
)");

    REQUIRE_FALSE(opt.has_value());

    const auto& errors = p.get_errors();
    REQUIRE(errors.size() == 2);
    REQUIRE_THAT(errors[0], ContainsSubstring("Line 2"));
    REQUIRE_THAT(errors[0], ContainsSubstring("Empty array literals are not allowed"));
    REQUIRE_THAT(errors[1], ContainsSubstring("Line 4"));
    REQUIRE_THAT(errors[1], ContainsSubstring("Unknown type"));
}

TEST_CASE("Multi-line error reporting with recovery", "[invalid]")
{
    parser p;
    auto opt = p.parse(R"(
int valid = 10
unknown_type bad = 1
int another = foo()
)");

    REQUIRE_FALSE(opt.has_value());

    const auto& errors = p.get_errors();
    REQUIRE(errors.size() == 1);
    REQUIRE_THAT(errors[0], ContainsSubstring("Line 3"));
    REQUIRE_THAT(errors[0], ContainsSubstring("Unknown type"));
}
