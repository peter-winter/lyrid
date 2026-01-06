#include "translator.hpp"
#include "assembly.hpp"
#include "test_instruction_helper.hpp"

#include <catch2/catch_test_macros.hpp>

using namespace lyrid;
using namespace lyrid::assembly;

TEST_CASE("Codegen: mutable int array with variable elements only", "[translator][codegen][mutable_array]")
{
    translator t;
    t.translate(R"(
int x = 42
int y = 100
int[] a = [x, y]
)");

    REQUIRE(t.is_valid());

    const auto& ins = t.get_program().instructions_;
    const auto& const_int = t.get_const_int_memory();

    REQUIRE(const_int.size() == 2);
    REQUIRE(const_int[0] == 42);
    REQUIRE(const_int[1] == 100);

    REQUIRE(ins.size() == 5);
    require<mov_i_reg_const>(0, ins, 0, 0);  // decl x: i_scalar reg 0 <- const 0
    require<mov_i_reg_const>(1, ins, 1, 1);  // decl y: i_scalar reg 1 <- const 1
    require<mov_i_mut_reg>(2, ins, 0, 0, 0);  // a[0] = x (from i_scalar reg 0)
    require<mov_i_mut_reg>(3, ins, 0, 1, 1);  // a[1] = y (from i_scalar reg 1)
    require<mov_is_reg_mut>(4, ins, 0, 0);  // decl a: i_span reg 0 <- mutable span 0

    REQUIRE(t.get_mutable_int_memory_size() == 2);

    const auto& mut_spans = t.get_int_array_spans(memory_type::mem_mutable);
    REQUIRE(mut_spans.size() == 1);
    REQUIRE(mut_spans[0].offset_ == 0);
    REQUIRE(mut_spans[0].len_ == 2);
}

TEST_CASE("Codegen: mutable int array with mixed literals and variables", "[translator][codegen][mutable_array]")
{
    translator t;
    t.translate(R"(
int x = 5
int[] arr = [1, x, 3]
)");

    REQUIRE(t.is_valid());

    const auto& ins = t.get_program().instructions_;
    const auto& const_int = t.get_const_int_memory();

    REQUIRE(const_int.size() == 3);
    REQUIRE(const_int[0] == 5);
    REQUIRE(const_int[1] == 1);
    REQUIRE(const_int[2] == 3);

    REQUIRE(ins.size() == 5);
    require<mov_i_reg_const>(0, ins, 0, 0);  // decl x: i_scalar reg 0 <- const 0
    require<mov_i_mut_const>(1, ins, 0, 0, 1);  // arr[0] = 1
    require<mov_i_mut_reg>(2, ins, 0, 1, 0);  // arr[1] = x
    require<mov_i_mut_const>(3, ins, 0, 2, 2);  // arr[2] = 3
    require<mov_is_reg_mut>(4, ins, 0, 0);  // decl arr: i_span reg 0 <- mutable span 0

    REQUIRE(t.get_mutable_int_memory_size() == 3);

    const auto& mut_spans = t.get_int_array_spans(memory_type::mem_mutable);
    REQUIRE(mut_spans.size() == 1);
    REQUIRE(mut_spans[0].offset_ == 0);
    REQUIRE(mut_spans[0].len_ == 3);
}

TEST_CASE("Codegen: mutable int array with function call element (direct store, with register reservation)", "[translator][codegen][mutable_array]")
{
    translator t;
    t.register_function("twice", {int_scalar_type{}}, {"v"}, int_scalar_type{});

    t.translate(R"(
int base = 10
int[] arr = [twice(base), base]
)");

    REQUIRE(t.is_valid());

    const auto& ins = t.get_program().instructions_;
    const auto& const_int = t.get_const_int_memory();

    REQUIRE(const_int.size() == 1);
    REQUIRE(const_int[0] == 10);

    REQUIRE(ins.size() == 5);
    require<mov_i_reg_const>(0, ins, 1, 0);  // decl base: i_scalar reg 1 <- const 0 (reservation=1)
    require<mov_i_reg_reg>(1, ins, 0, 1);  // arg setup: move base to arg pos 0
    require<call_i_mut>(2, ins, 0, 0, 0);  // call twice → store directly into arr[0]
    require<mov_i_mut_reg>(3, ins, 0, 1, 1);  // arr[1] = base (from decl reg 1)
    require<mov_is_reg_mut>(4, ins, 1, 0);  // decl arr: i_span reg 1 <- mutable span 0

    REQUIRE(t.get_mutable_int_memory_size() == 2);

    const auto& mut_spans = t.get_int_array_spans(memory_type::mem_mutable);
    REQUIRE(mut_spans.size() == 1);
    REQUIRE(mut_spans[0].offset_ == 0);
    REQUIRE(mut_spans[0].len_ == 2);
}

TEST_CASE("Codegen: multiple mutable int arrays (contiguous packing)", "[translator][codegen][mutable_array]")
{
    translator t;
    t.translate(R"(
int p = 1
int q = 2
int r = 3
int[] a = [p, q]
int[] b = [r, 4]
)");

    REQUIRE(t.is_valid());

    const auto& const_int = t.get_const_int_memory();

    REQUIRE(const_int.size() == 4);
    REQUIRE(const_int[0] == 1);
    REQUIRE(const_int[1] == 2);
    REQUIRE(const_int[2] == 3);
    REQUIRE(const_int[3] == 4);

    REQUIRE(t.get_mutable_int_memory_size() == 4);

    const auto& mut_spans = t.get_int_array_spans(memory_type::mem_mutable);
    REQUIRE(mut_spans.size() == 2);
    REQUIRE(mut_spans[0].offset_ == 0);
    REQUIRE(mut_spans[0].len_ == 2);
    REQUIRE(mut_spans[1].offset_ == 2);
    REQUIRE(mut_spans[1].len_ == 2);
}

TEST_CASE("Codegen: mutable float array with mixed literals and variables", "[translator][codegen][mutable_array]")
{
    translator t;
    t.translate(R"(
float x = 5.5
float[] arr = [1.0, x, 3.0]
)");

    REQUIRE(t.is_valid());

    const auto& ins = t.get_program().instructions_;
    const auto& const_float = t.get_const_float_memory();

    REQUIRE(const_float.size() == 3);
    REQUIRE(const_float[0] == 5.5);
    REQUIRE(const_float[1] == 1.0);
    REQUIRE(const_float[2] == 3.0);

    REQUIRE(ins.size() == 5);
    require<mov_f_reg_const>(0, ins, 0, 0);  // decl x
    require<mov_f_mut_const>(1, ins, 0, 0, 1);  // arr[0]
    require<mov_f_mut_reg>(2, ins, 0, 1, 0);  // arr[1]
    require<mov_f_mut_const>(3, ins, 0, 2, 2);  // arr[2]
    require<mov_fs_reg_mut>(4, ins, 0, 0);  // decl arr

    REQUIRE(t.get_mutable_float_memory_size() == 3);

    const auto& mut_spans = t.get_float_array_spans(memory_type::mem_mutable);
    REQUIRE(mut_spans.size() == 1);
    REQUIRE(mut_spans[0].offset_ == 0);
    REQUIRE(mut_spans[0].len_ == 3);
}
