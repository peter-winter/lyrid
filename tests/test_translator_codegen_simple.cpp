#include "translator.hpp"
#include "assembly.hpp"
#include "test_instruction_helpers.hpp"

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>

using namespace lyrid;


TEST_CASE("Codegen: empty program", "[translator][codegen]")
{
    translator t;
    t.translate("");
    REQUIRE(t.is_valid());
    REQUIRE(t.get_program().instructions_.empty());
}

TEST_CASE("Codegen: scalar literal assignment", "[translator][codegen]")
{
    translator t;
    t.translate("int x = 42");
    REQUIRE(t.is_valid());

    const auto& ins = t.get_program().instructions_;
    REQUIRE(ins.size() == 1);
    require_mov_i_const(0, ins[0], 0, 0);
}

TEST_CASE("Codegen: float literal assignment", "[translator][codegen]")
{
    translator t;
    t.translate("float y = 3.14");
    REQUIRE(t.is_valid());

    const auto& ins = t.get_program().instructions_;
    REQUIRE(ins.size() == 1);
    require_mov_f_const(0, ins[0], 0, 0);
}

TEST_CASE("Codegen: constant array literal", "[translator][codegen]")
{
    translator t;
    t.translate("int[] a = [10, 20, 30]");
    REQUIRE(t.is_valid());

    const auto& ins = t.get_program().instructions_;
    REQUIRE(ins.size() == 1);
    require_mov_is_const(0, ins[0], 0, 0);
}

TEST_CASE("Codegen: variable copy", "[translator][codegen]")
{
    translator t;
    t.translate(R"(
int x = 100
int y = x
)");
    REQUIRE(t.is_valid());

    const auto& ins = t.get_program().instructions_;
    REQUIRE(ins.size() == 2);
    require_mov_i_const(0, ins[0], 0, 0);
    require_mov_i_reg(1, ins[1], 1, 0);
}

TEST_CASE("Codegen: simple function call (no args)", "[translator][codegen]")
{
    translator t;
    t.register_function("foo", {}, {}, int_scalar_type{});

    t.translate("int res = foo()");
    REQUIRE(t.is_valid());

    const auto& ins = t.get_program().instructions_;
    REQUIRE(ins.size() == 1);
    require_call_i_reg(0, ins[0], 0, 0);
}

TEST_CASE("Codegen: function call with literal arguments", "[translator][codegen]")
{
    translator t;
    t.register_function("process", {int_scalar_type{}, float_scalar_type{}}, {"i", "f"}, int_scalar_type{});

    t.translate("int res = process(42, 2.71)");
    REQUIRE(t.is_valid());

    const auto& ins = t.get_program().instructions_;
    REQUIRE(ins.size() == 3);

    require_mov_f_const(0, ins[0], 0, 0);  // right arg 2.71
    require_mov_i_const(1, ins[1], 0, 0);  // left arg 42
    require_call_i_reg(2, ins[2], 0, 1);       // res in reg 1 (offset 1)
}

TEST_CASE("Codegen: nested function calls", "[translator][codegen]")
{
    translator t;
    t.register_function("inner", {int_scalar_type{}}, {"v"}, float_scalar_type{});
    t.register_function("outer", {float_scalar_type{}, int_scalar_type{}}, {"f", "i"}, int_scalar_type{});

    t.translate("int result = outer(inner(5), 10)");
    REQUIRE(t.is_valid());

    const auto& ins = t.get_program().instructions_;
    REQUIRE(ins.size() == 4);

    require_mov_i_const(0, ins[0], 0, 1);   // right arg 10 from index 1
    require_mov_i_const(1, ins[1], 0, 0);   // inner arg 5 from index 0
    require_call_f_reg(2, ins[2], 0, 0);        // inner result in f_scalar 0
    require_call_i_reg(3, ins[3], 1, 1);        // outer result in i_scalar 1
}

TEST_CASE("Codegen: index access unsupported", "[translator][codegen]")
{
    translator t;
    t.translate(R"(
int[] arr = [1,2,3]
int v = arr[0]
)");
    REQUIRE_FALSE(t.is_valid());

    const auto& errors = t.get_errors();
    REQUIRE_FALSE(errors.empty());
    REQUIRE_THAT(errors[0], Catch::Matchers::ContainsSubstring("not yet supported"));
}

TEST_CASE("Codegen: comprehension unsupported", "[translator][codegen]")
{
    translator t;
    t.translate("int[] res = [|i| in |[1,2,3]| do i]");
    REQUIRE_FALSE(t.is_valid());

    const auto& errors = t.get_errors();
    REQUIRE_FALSE(errors.empty());
    REQUIRE_THAT(errors[0], Catch::Matchers::ContainsSubstring("not yet supported"));
}

TEST_CASE("Codegen: mixed declarations and calls", "[translator][codegen]")
{
    translator t;
    t.register_function("transform", {int_scalar_type{}}, {"v"}, int_scalar_type{});

    t.translate(R"(
int base = 100
int[] data = [1, 2]
int result = transform(base)
)");
    REQUIRE(t.is_valid());

    const auto& ins = t.get_program().instructions_;
    REQUIRE(ins.size() == 4);

    require_mov_i_const(0, ins[0], 1, 0);      // base = 100 (i_scalar reg 1, offset 1)
    require_mov_is_const(1, ins[1], 1, 0);     // data = [1,2] (i_span reg 1)
    require_mov_i_reg(2, ins[2], 0, 1);        // copy base (reg 1) to arg pos 0
    require_call_i_reg(3, ins[3], 0, 2);       // result in i_scalar reg 2
}
