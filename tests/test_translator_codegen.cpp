// tests/test_translator_codegen.cpp

#include "translator.hpp"
#include "assembly.hpp"

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>

using namespace lyrid;

namespace
{

void require_mov_i_const(size_t idx, const assembly::instruction& i, size_t dst, size_t src_const)
{
    INFO("Instruction " << idx);
    REQUIRE(std::holds_alternative<assembly::mov_i_reg_const>(i));
    const auto& m = std::get<assembly::mov_i_reg_const>(i);
    REQUIRE(m.dst_ == dst);
    REQUIRE(m.src_ == src_const);
}

void require_mov_f_const(size_t idx, const assembly::instruction& i, size_t dst, size_t src_const)
{
    INFO("Instruction " << idx);
    REQUIRE(std::holds_alternative<assembly::mov_f_reg_const>(i));
    const auto& m = std::get<assembly::mov_f_reg_const>(i);
    REQUIRE(m.dst_ == dst);
    REQUIRE(m.src_ == src_const);
}

void require_smov_i_const(size_t idx, const assembly::instruction& i, size_t dst, size_t src_const)
{
    INFO("Instruction " << idx);
    REQUIRE(std::holds_alternative<assembly::smov_i_reg_const>(i));
    const auto& m = std::get<assembly::smov_i_reg_const>(i);
    REQUIRE(m.dst_ == dst);
    REQUIRE(m.src_ == src_const);
}

void require_smov_f_const(size_t idx, const assembly::instruction& i, size_t dst, size_t src_const)
{
    INFO("Instruction " << idx);
    REQUIRE(std::holds_alternative<assembly::smov_f_reg_const>(i));
    const auto& m = std::get<assembly::smov_f_reg_const>(i);
    REQUIRE(m.dst_ == dst);
    REQUIRE(m.src_ == src_const);
}

void require_mov_i_reg(size_t idx, const assembly::instruction& i, size_t dst, size_t src)
{
    INFO("Instruction " << idx);
    REQUIRE(std::holds_alternative<assembly::mov_i_reg_reg>(i));
    const auto& m = std::get<assembly::mov_i_reg_reg>(i);
    REQUIRE(m.dst_ == dst);
    REQUIRE(m.src_ == src);
}

void require_mov_f_reg(size_t idx, const assembly::instruction& i, size_t dst, size_t src)
{
    INFO("Instruction " << idx);
    REQUIRE(std::holds_alternative<assembly::mov_f_reg_reg>(i));
    const auto& m = std::get<assembly::mov_f_reg_reg>(i);
    REQUIRE(m.dst_ == dst);
    REQUIRE(m.src_ == src);
}

void require_smov_i_reg(size_t idx, const assembly::instruction& i, size_t dst, size_t src)
{
    INFO("Instruction " << idx);
    REQUIRE(std::holds_alternative<assembly::smov_i_reg_reg>(i));
    const auto& m = std::get<assembly::smov_i_reg_reg>(i);
    REQUIRE(m.dst_ == dst);
    REQUIRE(m.src_ == src);
}

void require_smov_f_reg(size_t idx, const assembly::instruction& i, size_t dst, size_t src)
{
    INFO("Instruction " << idx);
    REQUIRE(std::holds_alternative<assembly::smov_f_reg_reg>(i));
    const auto& m = std::get<assembly::smov_f_reg_reg>(i);
    REQUIRE(m.dst_ == dst);
    REQUIRE(m.src_ == src);
}

void require_call_i(size_t idx, const assembly::instruction& i, size_t fid, size_t res)
{
    INFO("Instruction " << idx);
    REQUIRE(std::holds_alternative<assembly::call_i>(i));
    const auto& c = std::get<assembly::call_i>(i);
    REQUIRE(c.id_ == fid);
    REQUIRE(c.res_ == res);
}

void require_call_f(size_t idx, const assembly::instruction& i, size_t fid, size_t res)
{
    INFO("Instruction " << idx);
    REQUIRE(std::holds_alternative<assembly::call_f>(i));
    const auto& c = std::get<assembly::call_f>(i);
    REQUIRE(c.id_ == fid);
    REQUIRE(c.res_ == res);
}

void require_scall_i(size_t idx, const assembly::instruction& i, size_t fid, size_t res)
{
    INFO("Instruction " << idx);
    REQUIRE(std::holds_alternative<assembly::scall_i>(i));
    const auto& c = std::get<assembly::scall_i>(i);
    REQUIRE(c.id_ == fid);
    REQUIRE(c.res_ == res);
}

void require_scall_f(size_t idx, const assembly::instruction& i, size_t fid, size_t res)
{
    INFO("Instruction " << idx);
    REQUIRE(std::holds_alternative<assembly::scall_f>(i));
    const auto& c = std::get<assembly::scall_f>(i);
    REQUIRE(c.id_ == fid);
    REQUIRE(c.res_ == res);
}

}

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
    require_smov_i_const(0, ins[0], 0, 0);
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
    t.register_function("foo", {}, {}, type::int_scalar);

    t.translate("int res = foo()");
    REQUIRE(t.is_valid());

    const auto& ins = t.get_program().instructions_;
    REQUIRE(ins.size() == 1);
    require_call_i(0, ins[0], 0, 0);
}

TEST_CASE("Codegen: function call with literal arguments", "[translator][codegen]")
{
    translator t;
    t.register_function("process", {type::int_scalar, type::float_scalar}, {"i", "f"}, type::int_scalar);

    t.translate("int res = process(42, 2.71)");
    REQUIRE(t.is_valid());

    const auto& ins = t.get_program().instructions_;
    REQUIRE(ins.size() == 3);

    require_mov_f_const(0, ins[0], 0, 0);  // right arg 2.71
    require_mov_i_const(1, ins[1], 0, 0);  // left arg 42
    require_call_i(2, ins[2], 0, 1);       // res in reg 1 (offset 1)
}

TEST_CASE("Codegen: nested function calls", "[translator][codegen]")
{
    translator t;
    t.register_function("inner", {type::int_scalar}, {"v"}, type::float_scalar);
    t.register_function("outer", {type::float_scalar, type::int_scalar}, {"f", "i"}, type::int_scalar);

    t.translate("int result = outer(inner(5), 10)");
    REQUIRE(t.is_valid());

    const auto& ins = t.get_program().instructions_;
    REQUIRE(ins.size() == 4);

    require_mov_i_const(0, ins[0], 0, 1);   // right arg 10 from index 1
    require_mov_i_const(1, ins[1], 0, 0);   // inner arg 5 from index 0
    require_call_f(2, ins[2], 0, 0);        // inner result in f_scalar 0
    require_call_i(3, ins[3], 1, 1);        // outer result in i_scalar 1
}

TEST_CASE("Codegen: non-constant array construction error", "[translator][codegen]")
{
    translator t;
    t.translate(R"(
int x = 42
int[] a = [x]
)");
    REQUIRE_FALSE(t.is_valid());

    const auto& errors = t.get_errors();
    REQUIRE_FALSE(errors.empty());
    REQUIRE_THAT(errors[0], Catch::Matchers::ContainsSubstring("not yet supported"));
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
    t.register_function("transform", {type::int_scalar}, {"v"}, type::int_scalar);

    t.translate(R"(
int base = 100
int[] data = [1, 2]
int result = transform(base)
)");
    REQUIRE(t.is_valid());

    const auto& ins = t.get_program().instructions_;
    REQUIRE(ins.size() == 4);

    require_mov_i_const(0, ins[0], 1, 0);      // base = 100 (i_scalar reg 1, offset 1)
    require_smov_i_const(1, ins[1], 1, 0);     // data = [1,2] (i_span reg 1)
    require_mov_i_reg(2, ins[2], 0, 1);        // copy base (reg 1) to arg pos 0
    require_call_i(3, ins[3], 0, 2);           // result in i_scalar reg 2
}
