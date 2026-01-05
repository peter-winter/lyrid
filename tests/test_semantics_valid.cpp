// tests/test_semantics_valid.cpp
#include <catch2/catch_test_macros.hpp>

#include "parser.hpp"
#include "ast.hpp"
#include "semantic_analyzer.hpp"

using namespace lyrid;
using namespace lyrid::ast;

TEST_CASE("Semantic valid: basic array comprehension with literal body", "[semantics][valid]")
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
    REQUIRE(decl_res.value_.inferred_type_.value() == array_type{int_scalar_type{}});

    const auto& comp = std::get<comprehension>(decl_res.value_.wrapped_);
    REQUIRE(comp.in_exprs_[0].inferred_type_.has_value());
    REQUIRE(comp.in_exprs_[0].inferred_type_.value() == array_type{int_scalar_type{}});
    REQUIRE(comp.do_expr_->inferred_type_.has_value());
    REQUIRE(comp.do_expr_->inferred_type_.value() == int_scalar_type{});

    const auto& src_ref = std::get<symbol_ref>(comp.in_exprs_[0].wrapped_);
    REQUIRE(src_ref.declaration_idx_.has_value());
    REQUIRE(*src_ref.declaration_idx_ == 0);
}

TEST_CASE("Semantic valid: comprehension with multiple sources using variable from scope", "[semantics][valid]")
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
    REQUIRE(decl_res.value_.inferred_type_.value() == array_type{float_scalar_type{}});

    const auto& comp = std::get<comprehension>(decl_res.value_.wrapped_);
    REQUIRE(comp.in_exprs_[0].inferred_type_.has_value());
    REQUIRE(comp.in_exprs_[0].inferred_type_.value() == array_type{int_scalar_type{}});
    REQUIRE(comp.in_exprs_[1].inferred_type_.has_value());
    REQUIRE(comp.in_exprs_[1].inferred_type_.value() == array_type{float_scalar_type{}});
    REQUIRE(comp.do_expr_->inferred_type_.has_value());
    REQUIRE(comp.do_expr_->inferred_type_.value() == float_scalar_type{});

    const auto& ints_ref = std::get<symbol_ref>(comp.in_exprs_[0].wrapped_);
    REQUIRE(ints_ref.declaration_idx_.has_value());
    REQUIRE(*ints_ref.declaration_idx_ == 0);

    const auto& floats_ref = std::get<symbol_ref>(comp.in_exprs_[1].wrapped_);
    REQUIRE(floats_ref.declaration_idx_.has_value());
    REQUIRE(*floats_ref.declaration_idx_ == 1);

    const auto& f_ref = std::get<symbol_ref>(comp.do_expr_->wrapped_);
    REQUIRE(!f_ref.declaration_idx_.has_value());
}

TEST_CASE("Semantic valid: simple function call with registered prototype", "[semantics][valid]")
{
    parser p;
    p.parse(R"(
int x = foo(42)
)");

    const auto& parse_errors = p.get_errors();
    REQUIRE(parse_errors.empty());

    semantic_analyzer sa;
    sa.register_function_prototype("foo", {int_scalar_type{}}, {"arg"}, int_scalar_type{});

    program& prog = p.get_program();
    sa.analyze(prog);

    REQUIRE(sa.is_valid());
    REQUIRE(sa.get_errors().empty());

    const auto& decl_x = prog.declarations_[0];
    REQUIRE(decl_x.value_.inferred_type_.has_value());
    REQUIRE(decl_x.value_.inferred_type_.value() == int_scalar_type{});

    const auto& call = std::get<f_call>(decl_x.value_.wrapped_);
    REQUIRE(call.args_[0].inferred_type_.has_value());
    REQUIRE(call.args_[0].inferred_type_.value() == int_scalar_type{});

    REQUIRE(call.fn_.proto_idx_.has_value());
    REQUIRE(*call.fn_.proto_idx_ == 0);
}

TEST_CASE("Semantic valid: nested function calls with registered prototypes", "[semantics][valid]")
{
    parser p;
    p.parse(R"(
int y = bar(foo(1, 2.0))
)");

    const auto& parse_errors = p.get_errors();
    REQUIRE(parse_errors.empty());

    semantic_analyzer sa;
    sa.register_function_prototype("foo", {int_scalar_type{}, float_scalar_type{}}, {"a", "b"}, int_scalar_type{});
    sa.register_function_prototype("bar", {int_scalar_type{}}, {"x"}, int_scalar_type{});

    program& prog = p.get_program();
    sa.analyze(prog);

    REQUIRE(sa.is_valid());
    REQUIRE(sa.get_errors().empty());

    const auto& decl_y = prog.declarations_[0];
    REQUIRE(decl_y.value_.inferred_type_.has_value());
    REQUIRE(decl_y.value_.inferred_type_.value() == int_scalar_type{});

    const auto& outer_call = std::get<f_call>(decl_y.value_.wrapped_);
    const auto& inner_call = std::get<f_call>(outer_call.args_[0].wrapped_);

    REQUIRE(inner_call.fn_.proto_idx_.has_value());
    REQUIRE(*inner_call.fn_.proto_idx_ == 0);
    REQUIRE(outer_call.fn_.proto_idx_.has_value());
    REQUIRE(*outer_call.fn_.proto_idx_ == 1);
}

TEST_CASE("Semantic valid: function call returning array used in declaration", "[semantics][valid]")
{
    parser p;
    p.parse(R"(
int[] arr = create_int_array(5)
)");

    const auto& parse_errors = p.get_errors();
    REQUIRE(parse_errors.empty());

    semantic_analyzer sa;
    sa.register_function_prototype("create_int_array", {int_scalar_type{}}, {"size"}, array_type{int_scalar_type{}});

    program& prog = p.get_program();
    sa.analyze(prog);

    REQUIRE(sa.is_valid());
    REQUIRE(sa.get_errors().empty());

    const auto& decl_arr = prog.declarations_[0];
    REQUIRE(decl_arr.value_.inferred_type_.has_value());
    REQUIRE(decl_arr.value_.inferred_type_.value() == array_type{int_scalar_type{}});

    const auto& call = std::get<f_call>(decl_arr.value_.wrapped_);
    REQUIRE(call.fn_.proto_idx_.has_value());
    REQUIRE(*call.fn_.proto_idx_ == 0);
}

TEST_CASE("Semantic valid: function call in array comprehension 'do' expression", "[semantics][valid]")
{
    parser p;
    p.parse(R"(
int[] src = [1, 2, 3]
float[] res = [|i| in |src| do scale(i)]
)");

    const auto& parse_errors = p.get_errors();
    REQUIRE(parse_errors.empty());

    semantic_analyzer sa;
    sa.register_function_prototype("scale", {int_scalar_type{}}, {"val"}, float_scalar_type{});

    program& prog = p.get_program();
    sa.analyze(prog);

    REQUIRE(sa.is_valid());
    REQUIRE(sa.get_errors().empty());

    const auto& decl_res = prog.declarations_[1];
    REQUIRE(decl_res.value_.inferred_type_.has_value());
    REQUIRE(decl_res.value_.inferred_type_.value() == array_type{float_scalar_type{}});

    const auto& comp = std::get<comprehension>(decl_res.value_.wrapped_);
    const auto& call = std::get<f_call>(comp.do_expr_->wrapped_);

    const auto& src_ref = std::get<symbol_ref>(comp.in_exprs_[0].wrapped_);
    REQUIRE(src_ref.declaration_idx_.has_value());
    REQUIRE(*src_ref.declaration_idx_ == 0);

    const auto& i_ref = std::get<symbol_ref>(call.args_[0].wrapped_);
    REQUIRE(!i_ref.declaration_idx_.has_value());

    REQUIRE(call.fn_.proto_idx_.has_value());
    REQUIRE(*call.fn_.proto_idx_ == 0);
}

TEST_CASE("Resolution: top-level variable reference", "[semantics][valid]")
{
    parser p;
    p.parse(R"(
int a = 10
int b = a
int c = b
)");

    const auto& parse_errors = p.get_errors();
    REQUIRE(parse_errors.empty());

    semantic_analyzer sa;
    program& prog = p.get_program();
    sa.analyze(prog);

    REQUIRE(sa.is_valid());

    const auto& decl_b = prog.declarations_[1];
    const auto& ref_a = std::get<symbol_ref>(decl_b.value_.wrapped_);
    REQUIRE(ref_a.declaration_idx_.has_value());
    REQUIRE(*ref_a.declaration_idx_ == 0);

    const auto& decl_c = prog.declarations_[2];
    const auto& ref_b = std::get<symbol_ref>(decl_c.value_.wrapped_);
    REQUIRE(ref_b.declaration_idx_.has_value());
    REQUIRE(*ref_b.declaration_idx_ == 1);
}

TEST_CASE("Shadowing: local shadows global", "[semantics][valid]")
{
    parser p;
    p.parse(R"(
int x = 10
int [] src = [1,2,3]
int[] res = [|x| in |src| do x]
)");

    const auto& parse_errors = p.get_errors();
    REQUIRE(parse_errors.empty());

    semantic_analyzer sa;
    program& prog = p.get_program();
    sa.analyze(prog);

    REQUIRE(sa.is_valid());

    const auto& comp = std::get<comprehension>(prog.declarations_[2].value_.wrapped_);
    const auto& x_ref = std::get<symbol_ref>(comp.do_expr_->wrapped_);
    REQUIRE(!x_ref.declaration_idx_.has_value());  // local x shadows global x
}

TEST_CASE("Nested comprehension with outer local usage", "[semantics][valid]")
{
    parser p;
    p.parse(R"(
int[] a = [1,2,3]
int[] b = [4,5,6]
int[] res = [|i| in |a| do bar([|j| in |b| do foo(i, j)])]
)");

    const auto& parse_errors = p.get_errors();
    REQUIRE(parse_errors.empty());

    semantic_analyzer sa;
    // Register prototypes to make the program semantically valid
    sa.register_function_prototype("foo", {int_scalar_type{}, int_scalar_type{}}, {"x", "y"}, int_scalar_type{});
    sa.register_function_prototype("bar", {array_type{int_scalar_type{}}}, {"arr"}, int_scalar_type{});

    program& prog = p.get_program();
    sa.analyze(prog);

    REQUIRE(sa.is_valid());
    REQUIRE(sa.get_errors().empty());

    // res is the third declaration
    const auto& decl_res = prog.declarations_[2];
    REQUIRE(decl_res.value_.inferred_type_.has_value());
    REQUIRE(decl_res.value_.inferred_type_.value() == array_type{int_scalar_type{}});

    // Outer comprehension
    const auto& outer_comp = std::get<comprehension>(decl_res.value_.wrapped_);

    // Outer source 'a' is global (index 0)
    const auto& a_ref = std::get<symbol_ref>(outer_comp.in_exprs_[0].wrapped_);
    REQUIRE(a_ref.declaration_idx_.has_value());
    REQUIRE(*a_ref.declaration_idx_ == 0);

    // Outer 'do' is call to 'bar'
    const auto& bar_call = std::get<f_call>(outer_comp.do_expr_->wrapped_);
    REQUIRE(bar_call.fn_.ident_.value_ == "bar");
    REQUIRE(bar_call.fn_.proto_idx_.has_value());
    REQUIRE(*bar_call.fn_.proto_idx_ == 1);  // bar registered second

    // bar argument is inner comprehension
    const auto& inner_comp = std::get<comprehension>(bar_call.args_[0].wrapped_);

    // Inner source 'b' is global (index 1)
    const auto& b_ref = std::get<symbol_ref>(inner_comp.in_exprs_[0].wrapped_);
    REQUIRE(b_ref.declaration_idx_.has_value());
    REQUIRE(*b_ref.declaration_idx_ == 1);

    // Inner 'do' is call to 'foo'
    const auto& foo_call = std::get<f_call>(inner_comp.do_expr_->wrapped_);
    REQUIRE(foo_call.fn_.ident_.value_ == "foo");
    REQUIRE(foo_call.fn_.proto_idx_.has_value());
    REQUIRE(*foo_call.fn_.proto_idx_ == 0);  // foo registered first

    // foo arguments: i (outer local, nullopt) and j (inner local, nullopt)
    const auto& i_ref = std::get<symbol_ref>(foo_call.args_[0].wrapped_);
    REQUIRE(i_ref.ident_.value_ == "i");
    REQUIRE(!i_ref.declaration_idx_.has_value());

    const auto& j_ref = std::get<symbol_ref>(foo_call.args_[1].wrapped_);
    REQUIRE(j_ref.ident_.value_ == "j");
    REQUIRE(!j_ref.declaration_idx_.has_value());

    // Type checks
    REQUIRE(inner_comp.do_expr_->inferred_type_.has_value());
    REQUIRE(inner_comp.do_expr_->inferred_type_.value() == int_scalar_type{});  // foo returns scalar

    REQUIRE(bar_call.args_[0].inferred_type_.has_value());
    REQUIRE(bar_call.args_[0].inferred_type_.value() == array_type{int_scalar_type{}});  // inner comprehension

    REQUIRE(outer_comp.do_expr_->inferred_type_.has_value());
    REQUIRE(outer_comp.do_expr_->inferred_type_.value() == int_scalar_type{});  // bar returns scalar
}

TEST_CASE("Complex comprehension: indexing local variable", "[semantics][valid]")
{
    parser p;
    p.parse(R"(
int[] src = [10, 20, 30]
int x = src[ [|i| in |src| do i][0] ]
)");

    const auto& parse_errors = p.get_errors();
    REQUIRE(parse_errors.empty());

    semantic_analyzer sa;
    program& prog = p.get_program();
    sa.analyze(prog);

    REQUIRE(sa.is_valid());

    const auto& decl_x = prog.declarations_[1];
    const auto& outer_ia = std::get<index_access>(decl_x.value_.wrapped_);
    const auto& outer_ia_base = std::get<symbol_ref>(outer_ia.base_->wrapped_);
    const auto& inner_ia = std::get<index_access>(outer_ia.index_->wrapped_);

    const auto& comp = std::get<comprehension>(inner_ia.base_->wrapped_);
    
    // inner index 0 is literal
    const auto& inner_index = std::get<int_scalar>(inner_ia.index_->wrapped_);
    REQUIRE(inner_index.value_ == 0);
    
    // i in comprehension do is local
    const auto& i_ref = std::get<symbol_ref>(comp.do_expr_->wrapped_);
    REQUIRE(!i_ref.declaration_idx_.has_value());
}
