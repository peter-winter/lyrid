#include "translator.hpp"
#include "assembly.hpp"
#include "test_instruction_helper.hpp"

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
    require<mov_i_reg_const>(0, ins, 0, 0);
}

TEST_CASE("Codegen: float literal assignment", "[translator][codegen]")
{
    translator t;
    t.translate("float y = 3.14");
    REQUIRE(t.is_valid());

    const auto& ins = t.get_program().instructions_;
    REQUIRE(ins.size() == 1);
    require<mov_f_reg_const>(0, ins, 0, 0);
}

TEST_CASE("Codegen: constant array literal", "[translator][codegen]")
{
    translator t;
    t.translate("int[] a = [10, 20, 30]");
    REQUIRE(t.is_valid());

    const auto& ins = t.get_program().instructions_;
    REQUIRE(ins.size() == 1);
    require<mov_is_reg_const>(0, ins, 0, 0);
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
    require<mov_i_reg_const>(0, ins, 0, 0);
    require<mov_i_reg_reg>(1, ins, 1, 0);
}

TEST_CASE("Codegen: simple function call (no args)", "[translator][codegen]")
{
    translator t;
    t.register_function("foo", {}, {}, int_scalar_type{});

    t.translate("int res = foo()");
    REQUIRE(t.is_valid());

    const auto& ins = t.get_program().instructions_;
    REQUIRE(ins.size() == 1);
    require<call_i_reg>(0, ins, 0, 0);
}

TEST_CASE("Codegen: function call with literal arguments", "[translator][codegen]")
{
    translator t;
    t.register_function("process", {int_scalar_type{}, float_scalar_type{}}, {"i", "f"}, int_scalar_type{});

    t.translate("int res = process(42, 2.71)");
    REQUIRE(t.is_valid());

    const auto& ins = t.get_program().instructions_;
    REQUIRE(ins.size() == 3);

    require<mov_i_reg_const>(0, ins, 0, 0);  // left arg 42
    require<mov_f_reg_const>(1, ins, 0, 0);  // right arg 2.71
    require<call_i_reg>(2, ins, 0, 1);       // res in reg 1 (offset 1)
}

TEST_CASE("Codegen: float array literal constant", "[translator][codegen]")
{
    translator t;
    t.translate("float[] fa = [1.0, 2.5, 3.14]");
    REQUIRE(t.is_valid());

    const auto& ins = t.get_program().instructions_;
    REQUIRE(ins.size() == 1);
    require<mov_fs_reg_const>(0, ins, 0, 0);
}

TEST_CASE("Codegen: float span variable copy", "[translator][codegen]")
{
    translator t;
    t.translate(R"(
float[] a = [0.1, 0.2, 0.3]
float[] b = a
)");
    REQUIRE(t.is_valid());

    const auto& ins = t.get_program().instructions_;
    REQUIRE(ins.size() == 2);
    require<mov_fs_reg_const>(0, ins, 0, 0);
    require<mov_fs_reg_reg>(1, ins, 1, 0);  // b in f_span reg 1 copies from a in reg 0
}

TEST_CASE("Codegen: function returning float scalar", "[translator][codegen]")
{
    translator t;
    t.register_function("get_float", {}, {}, float_scalar_type{});

    t.translate("float val = get_float()");
    REQUIRE(t.is_valid());

    const auto& ins = t.get_program().instructions_;
    REQUIRE(ins.size() == 1);
    require<call_f_reg>(0, ins, 0, 0);
}

TEST_CASE("Codegen: function returning int array", "[translator][codegen]")
{
    translator t;
    t.register_function("generate_ints", {}, {}, array_type{int_scalar_type{}});

    t.translate("int[] arr = generate_ints()");
    REQUIRE(t.is_valid());

    const auto& ins = t.get_program().instructions_;
    REQUIRE(ins.size() == 1);
    require<call_is_reg>(0, ins, 0, 0);
}

TEST_CASE("Codegen: function returning float array", "[translator][codegen]")
{
    translator t;
    t.register_function("generate_floats", {}, {}, array_type{float_scalar_type{}});

    t.translate("float[] arr = generate_floats()");
    REQUIRE(t.is_valid());

    const auto& ins = t.get_program().instructions_;
    REQUIRE(ins.size() == 1);
    require<call_fs_reg>(0, ins, 0, 0);
}

TEST_CASE("Codegen: nested function calls", "[translator][codegen]")
{
    translator t;
    t.register_function("inner", {int_scalar_type{}}, {"v"}, float_scalar_type{});
    t.register_function("outer", {float_scalar_type{}, int_scalar_type{}}, {"f", "i"}, int_scalar_type{});

    t.translate("int result = outer(inner(5), 10)");
    REQUIRE(t.is_valid());

    const auto& ins = t.get_program().instructions_;
    REQUIRE(ins.size() == 6);

    // global_max_args_ = 1 → low register 0 reserved in each file, declarations/temporaries start at index 1
    // Outer call is non-flat → general path (left-to-right temporaries for args, direct result write)
    // Inner call is flat → direct low-arg emission, direct result write to its temporary target

    require<mov_i_reg_const>(0, ins, 0, 0);   // inner arg: 5 → i_scalar low arg pos 0 (const index 0)
    require<call_f_reg>(1, ins, 0, 1);    // inner() → f_scalar temporary 1 (function index 0)
    require<mov_i_reg_const>(2, ins, 2, 1);   // outer right arg: 10 → i_scalar temporary 2 (const index 1)
    require<mov_f_reg_reg>(3, ins, 0, 1);     // move left temporary (inner result f_scalar 1) → f_scalar arg pos 0
    require<mov_i_reg_reg>(4, ins, 0, 2);     // move right temporary (10 i_scalar 2) → i_scalar arg pos 0
    require<call_i_reg>(5, ins, 1, 1);    // outer() → i_scalar declaration register 1 (function index 1)
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

    require<mov_i_reg_const>(0, ins, 1, 0);      // base = 100 (i_scalar reg 1, offset 1)
    require<mov_is_reg_const>(1, ins, 1, 0);     // data = [1,2] (i_span reg 1)
    require<mov_i_reg_reg>(2, ins, 0, 1);        // copy base (reg 1) to arg pos 0
    require<call_i_reg>(3, ins, 0, 2);       // result in i_scalar reg 2
}

TEST_CASE("Codegen: mixed scalars and arrays with declarations and calls", "[translator][codegen]")
{
    translator t;
    t.register_function("process_mixed", 
        {int_scalar_type{}, float_scalar_type{}, array_type{int_scalar_type{}}, array_type{float_scalar_type{}}},
        {"i", "f", "is", "fs"},
        array_type{int_scalar_type{}});

    t.register_function("get_float_scalar", {}, {}, float_scalar_type{});

    t.translate(R"(
int base_int = 100
float base_float = 2.5
int[] int_data = [10, 20, 30, 40]
float[] float_data = [0.1, 0.2, 0.3]
float intermediate = get_float_scalar()
int[] result = process_mixed(base_int, intermediate, int_data, float_data)
)");

    REQUIRE(t.is_valid());

    const auto& ins = t.get_program().instructions_;
    REQUIRE(ins.size() == 10);

    // global_max_args_ = 1 → low register 0 reserved in each file
    // Declaration registers start at index 1 in each relevant file

    // Declarations in source order
    require<mov_i_reg_const>(0, ins, 1, 0);   // base_int = 100 → i_scalar reg 1
    require<mov_f_reg_const>(1, ins, 1, 0);   // base_float = 2.5 → f_scalar reg 1
    require<mov_is_reg_const>(2, ins, 1, 0);  // int_data literal → i_span reg 1
    require<mov_fs_reg_const>(3, ins, 1, 0);  // float_data literal → f_span reg 1

    // Intermediate flat call (function index 1, no arguments)
    require<call_f_reg>(4, ins, 1, 2);    // get_float_scalar() → f_scalar reg 2

    // Flat process_mixed (function index 0): arguments emitted left-to-right directly into low positions
    require<mov_i_reg_reg>(5, ins, 0, 1);     // arg0: base_int (reg 1) → i_scalar arg pos 0
    require<mov_f_reg_reg>(6, ins, 0, 2);     // arg1: intermediate (reg 2) → f_scalar arg pos 0
    require<mov_is_reg_reg>(7, ins, 0, 1);    // arg2: int_data (reg 1) → i_span arg pos 0
    require<mov_fs_reg_reg>(8, ins, 0, 1);    // arg3: float_data (reg 1) → f_span arg pos 0

    // Final call returning int[] (direct write to declaration register)
    require<call_is_reg>(9, ins, 0, 2);   // process_mixed → i_span reg 2
}

