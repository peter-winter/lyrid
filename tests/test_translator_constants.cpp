// tests/test_translator_constants.cpp
#include <catch2/catch_test_macros.hpp>

#include "translator.hpp"

#include <vector>
#include <span>
#include <cstdint>

using namespace lyrid;

template<typename T>
void assert_span_points_to(
    const std::span<const T>& span,
    const std::vector<T>& memory,
    size_t expected_offset,
    size_t expected_size)
{
    REQUIRE(span.size() == expected_size);
    REQUIRE(span.data() == memory.data() + expected_offset);

    // Verify contiguity by checking element addresses
    for (size_t i = 0; i < expected_size; ++i)
    {
        REQUIRE(&span[i] == memory.data() + expected_offset + i);
    }
}

TEST_CASE("Translator constants: simple int scalar", "[translator][constants]")
{
    translator t;
    t.translate("int x = 42");

    const auto& errors = t.get_errors();
    REQUIRE(errors.empty());

    const auto& int_mem = t.get_const_int_memory();
    REQUIRE(int_mem == std::vector<int64_t>{42});

    const auto& float_mem = t.get_const_float_memory();
    REQUIRE(float_mem.empty());

    const auto& int_spans = t.get_const_int_array_spans();
    REQUIRE(int_spans.empty());

    const auto& float_spans = t.get_const_float_array_spans();
    REQUIRE(float_spans.empty());
}

TEST_CASE("Translator constants: simple float scalar", "[translator][constants]")
{
    translator t;
    t.translate("float x = 3.14");

    const auto& errors = t.get_errors();
    REQUIRE(errors.empty());

    const auto& int_mem = t.get_const_int_memory();
    REQUIRE(int_mem.empty());

    const auto& float_mem = t.get_const_float_memory();
    REQUIRE(float_mem == std::vector<double>{3.14});

    const auto& int_spans = t.get_const_int_array_spans();
    REQUIRE(int_spans.empty());

    const auto& float_spans = t.get_const_float_array_spans();
    REQUIRE(float_spans.empty());
}

TEST_CASE("Translator constants: simple int array literal", "[translator][constants]")
{
    translator t;
    t.translate("int[] arr = [10, 20, 30]");

    const auto& errors = t.get_errors();
    REQUIRE(errors.empty());

    const auto& int_mem = t.get_const_int_memory();
    REQUIRE(int_mem == std::vector<int64_t>{10, 20, 30});

    const auto& int_spans = t.get_const_int_array_spans();
    REQUIRE(int_spans.size() == 1);
    assert_span_points_to(int_spans[0], int_mem, 0, 3);

    const auto& float_spans = t.get_const_float_array_spans();
    REQUIRE(float_spans.empty());
}

TEST_CASE("Translator constants: multiple int arrays of different lengths", "[translator][constants]")
{
    translator t;
    t.translate(R"(
int[] a = [1, 2]
int[] b = [3]
int[] c = [4, 5, 6, 7]
)");

    const auto& errors = t.get_errors();
    REQUIRE(errors.empty());

    const auto& int_mem = t.get_const_int_memory();
    REQUIRE(int_mem == std::vector<int64_t>{1, 2, 3, 4, 5, 6, 7});

    const auto& int_spans = t.get_const_int_array_spans();
    REQUIRE(int_spans.size() == 3);
    assert_span_points_to(int_spans[0], int_mem, 0, 2);
    assert_span_points_to(int_spans[1], int_mem, 2, 1);
    assert_span_points_to(int_spans[2], int_mem, 3, 4);

    const auto& float_spans = t.get_const_float_array_spans();
    REQUIRE(float_spans.empty());
}

TEST_CASE("Translator constants: mixed scalars and arrays (int and float)", "[translator][constants]")
{
    translator t;
    t.translate(R"(
int s1 = -100
int[] ia = [1, 2, 3]
float f1 = 4.0
float[] fa = [5.0, 6.0]
int s2 = 7
int[] ib = [8, 9]
)");

    const auto& errors = t.get_errors();
    REQUIRE(errors.empty());

    const auto& int_mem = t.get_const_int_memory();
    REQUIRE(int_mem == std::vector<int64_t>{-100, 1, 2, 3, 7, 8, 9});

    const auto& float_mem = t.get_const_float_memory();
    REQUIRE(float_mem == std::vector<double>{4.0, 5.0, 6.0});

    const auto& int_spans = t.get_const_int_array_spans();
    REQUIRE(int_spans.size() == 2);
    assert_span_points_to(int_spans[0], int_mem, 1, 3);
    assert_span_points_to(int_spans[1], int_mem, 5, 2);

    const auto& float_spans = t.get_const_float_array_spans();
    REQUIRE(float_spans.size() == 1);
    assert_span_points_to(float_spans[0], float_mem, 1, 2);
}

TEST_CASE("Translator constants: literal array as comprehension source", "[translator][constants]")
{
    translator t;
    t.translate("int[] res = [|x| in |[10, 20, 30]| do x]");

    const auto& errors = t.get_errors();
    //REQUIRE(errors.empty());

    const auto& int_mem = t.get_const_int_memory();
    REQUIRE(int_mem == std::vector<int64_t>{10, 20, 30});

    const auto& int_spans = t.get_const_int_array_spans();
    REQUIRE(int_spans.size() == 1);
    assert_span_points_to(int_spans[0], int_mem, 0, 3);
}

TEST_CASE("Translator constants: scalar literal in comprehension do", "[translator][constants]")
{
    translator t;
    t.translate("int[] res = [|x| in |[1, 2, 3]| do 42]");

    const auto& errors = t.get_errors();
    //REQUIRE(errors.empty());

    const auto& int_mem = t.get_const_int_memory();
    REQUIRE(int_mem == std::vector<int64_t>{1, 2, 3, 42});

    const auto& int_spans = t.get_const_int_array_spans();
    REQUIRE(int_spans.size() == 1);
    assert_span_points_to(int_spans[0], int_mem, 0, 3);
}

TEST_CASE("Translator constants: multiple literal array sources in comprehension", "[translator][constants]")
{
    translator t;
    t.translate("int[] res = [|x, y| in |[1, 2], [3, 4]| do x]");

    const auto& errors = t.get_errors();
    //REQUIRE(errors.empty());

    const auto& int_mem = t.get_const_int_memory();
    REQUIRE(int_mem == std::vector<int64_t>{1, 2, 3, 4});

    const auto& int_spans = t.get_const_int_array_spans();
    REQUIRE(int_spans.size() == 2);
    assert_span_points_to(int_spans[0], int_mem, 0, 2);
    assert_span_points_to(int_spans[1], int_mem, 2, 2);
}

TEST_CASE("Translator constants: float array and comprehension with float scalar in do", "[translator][constants]")
{
    translator t;
    t.translate(R"(
float[] src = [1.0, 2.0]
float[] res = [|x| in |src| do 3.0]
)");

    const auto& errors = t.get_errors();
    //REQUIRE(errors.empty());

    const auto& float_mem = t.get_const_float_memory();
    REQUIRE(float_mem == std::vector<double>{1.0, 2.0, 3.0});

    const auto& float_spans = t.get_const_float_array_spans();
    REQUIRE(float_spans.size() == 1);
    assert_span_points_to(float_spans[0], float_mem, 0, 2);
}

TEST_CASE("Translator constants: function call returning scalar (no literals)", "[translator][constants]")
{
    translator t;
    t.register_function("get_scalar", {}, {}, type::int_scalar);

    t.translate("int x = get_scalar()");

    const auto& errors = t.get_errors();
    REQUIRE(errors.empty());

    const auto& int_mem = t.get_const_int_memory();
    REQUIRE(int_mem.empty());

    const auto& float_mem = t.get_const_float_memory();
    REQUIRE(float_mem.empty());

    const auto& int_spans = t.get_const_int_array_spans();
    REQUIRE(int_spans.empty());

    const auto& float_spans = t.get_const_float_array_spans();
    REQUIRE(float_spans.empty());
}

TEST_CASE("Translator constants: function call returning array (no literals)", "[translator][constants]")
{
    translator t;
    t.register_function("get_array", {}, {}, type::float_array);

    t.translate("float[] arr = get_array()");

    const auto& errors = t.get_errors();
    REQUIRE(errors.empty());

    const auto& int_mem = t.get_const_int_memory();
    REQUIRE(int_mem.empty());

    const auto& float_mem = t.get_const_float_memory();
    REQUIRE(float_mem.empty());

    const auto& int_spans = t.get_const_int_array_spans();
    REQUIRE(int_spans.empty());

    const auto& float_spans = t.get_const_float_array_spans();
    REQUIRE(float_spans.empty());
}

TEST_CASE("Translator constants: index access with function calls (no literals)", "[translator][constants]")
{
    translator t;
    t.register_function("get_arr", {}, {}, type::int_array);
    t.register_function("get_idx", {}, {}, type::int_scalar);

    t.translate(R"(
int[] arr = get_arr()
int idx = get_idx()
int val = arr[idx]
)");

    const auto& errors = t.get_errors();
    //REQUIRE(errors.empty());

    const auto& int_mem = t.get_const_int_memory();
    REQUIRE(int_mem.empty());

    const auto& float_mem = t.get_const_float_memory();
    REQUIRE(float_mem.empty());

    const auto& int_spans = t.get_const_int_array_spans();
    REQUIRE(int_spans.empty());

    const auto& float_spans = t.get_const_float_array_spans();
    REQUIRE(float_spans.empty());
}

TEST_CASE("Translator constants: array comprehension with function calls (no literals)", "[translator][constants]")
{
    translator t;
    t.register_function("get_src", {}, {}, type::int_array);
    t.register_function("transform", {type::int_scalar}, {"v"}, type::int_scalar);

    t.translate(R"(
int[] src = get_src()
int[] res = [|i| in |src| do transform(i)]
)");

    const auto& errors = t.get_errors();
    //REQUIRE(errors.empty());

    const auto& int_mem = t.get_const_int_memory();
    REQUIRE(int_mem.empty());

    const auto& float_mem = t.get_const_float_memory();
    REQUIRE(float_mem.empty());

    const auto& int_spans = t.get_const_int_array_spans();
    REQUIRE(int_spans.empty());

    const auto& float_spans = t.get_const_float_array_spans();
    REQUIRE(float_spans.empty());
}

TEST_CASE("Translator constants: mixed constructs (function calls, index access, symbol references) - no literals at all", "[translator][constants]")
{
    translator t;
    t.register_function("make_array", {}, {}, type::float_array);
    t.register_function("make_index", {}, {}, type::int_scalar);
    t.register_function("process", {type::float_scalar}, {"v"}, type::float_scalar);

    t.translate(R"(
float[] data = make_array()
int idx = make_index()
float value = data[idx]
float[] result = [|x| in |data| do process(x)]
float copied = value
)");

    const auto& errors = t.get_errors();
    //REQUIRE(errors.empty());

    const auto& int_mem = t.get_const_int_memory();
    REQUIRE(int_mem.empty());

    const auto& float_mem = t.get_const_float_memory();
    REQUIRE(float_mem.empty());

    const auto& int_spans = t.get_const_int_array_spans();
    REQUIRE(int_spans.empty());

    const auto& float_spans = t.get_const_float_array_spans();
    REQUIRE(float_spans.empty());
}

TEST_CASE("Translator constants: empty program (no declarations, no constants)", "[translator][constants]")
{
    translator t;
    t.translate("");

    const auto& errors = t.get_errors();
    REQUIRE(errors.empty());

    const auto& int_mem = t.get_const_int_memory();
    REQUIRE(int_mem.empty());

    const auto& float_mem = t.get_const_float_memory();
    REQUIRE(float_mem.empty());

    const auto& int_spans = t.get_const_int_array_spans();
    REQUIRE(int_spans.empty());

    const auto& float_spans = t.get_const_float_array_spans();
    REQUIRE(float_spans.empty());
}

TEST_CASE("Translator constants: function call with literal scalar arguments", "[translator][constants]")
{
    translator t;
    t.register_function("foo", {type::int_scalar, type::float_scalar}, {"i", "f"}, type::int_scalar);

    t.translate("int res = foo(42, 3.14)");

    const auto& errors = t.get_errors();
    REQUIRE(errors.empty());

    const auto& int_mem = t.get_const_int_memory();
    REQUIRE(int_mem == std::vector<int64_t>{42});

    const auto& float_mem = t.get_const_float_memory();
    REQUIRE(float_mem == std::vector<double>{3.14});

    const auto& int_spans = t.get_const_int_array_spans();
    REQUIRE(int_spans.empty());

    const auto& float_spans = t.get_const_float_array_spans();
    REQUIRE(float_spans.empty());
}

TEST_CASE("Translator constants: nested function calls with literal arguments", "[translator][constants]")
{
    translator t;
    t.register_function("inner", {type::int_scalar, type::float_scalar}, {"a", "b"}, type::float_scalar);
    t.register_function("outer", {type::float_scalar, type::int_scalar}, {"x", "y"}, type::int_scalar);

    t.translate("int res = outer(inner(10, 2.5), 100)");

    const auto& errors = t.get_errors();
    REQUIRE(errors.empty());

    const auto& int_mem = t.get_const_int_memory();
    REQUIRE(int_mem == std::vector<int64_t>{10, 100});

    const auto& float_mem = t.get_const_float_memory();
    REQUIRE(float_mem == std::vector<double>{2.5});

    const auto& int_spans = t.get_const_int_array_spans();
    REQUIRE(int_spans.empty());

    const auto& float_spans = t.get_const_float_array_spans();
    REQUIRE(float_spans.empty());
}

TEST_CASE("Translator constants: function call with literal array argument", "[translator][constants]")
{
    translator t;
    t.register_function("process", {type::float_array}, {"arr"}, type::float_scalar);

    t.translate("float res = process([1.0, 2.0, 3.0])");

    const auto& errors = t.get_errors();
    REQUIRE(errors.empty());

    const auto& int_mem = t.get_const_int_memory();
    REQUIRE(int_mem.empty());

    const auto& float_mem = t.get_const_float_memory();
    REQUIRE(float_mem == std::vector<double>{1.0, 2.0, 3.0});

    const auto& float_spans = t.get_const_float_array_spans();
    REQUIRE(float_spans.size() == 1);
    assert_span_points_to(float_spans[0], float_mem, 0, 3);
}

TEST_CASE("Translator constants: index access with literal index on symbol reference", "[translator][constants]")
{
    translator t;
    t.register_function("get_arr", {}, {}, type::int_array);

    t.translate(R"(
int[] arr = get_arr()
int val = arr[7]
)");

    const auto& errors = t.get_errors();
    //REQUIRE(errors.empty());

    const auto& int_mem = t.get_const_int_memory();
    REQUIRE(int_mem == std::vector<int64_t>{7});

    const auto& float_mem = t.get_const_float_memory();
    REQUIRE(float_mem.empty());

    const auto& int_spans = t.get_const_int_array_spans();
    REQUIRE(int_spans.empty());

    const auto& float_spans = t.get_const_float_array_spans();
    REQUIRE(float_spans.empty());
}

TEST_CASE("Translator constants: index access on literal array with literal index", "[translator][constants]")
{
    translator t;
    t.translate("int val = [10, 20, 30][2]");

    const auto& errors = t.get_errors();
    //REQUIRE(errors.empty());

    const auto& int_mem = t.get_const_int_memory();
    REQUIRE(int_mem == std::vector<int64_t>{10, 20, 30, 2});

    const auto& float_mem = t.get_const_float_memory();
    REQUIRE(float_mem.empty());

    const auto& int_spans = t.get_const_int_array_spans();
    REQUIRE(int_spans.size() == 1);
    assert_span_points_to(int_spans[0], int_mem, 0, 3);
}

TEST_CASE("Translator constants: index access on function call returning array with literal index", "[translator][constants]")
{
    translator t;
    t.register_function("get_arr", {}, {}, type::float_array);

    t.translate("float val = get_arr()[1]");

    const auto& errors = t.get_errors();
    //REQUIRE(errors.empty());

    const auto& int_mem = t.get_const_int_memory();
    REQUIRE(int_mem == std::vector<int64_t>{1});

    const auto& float_mem = t.get_const_float_memory();
    REQUIRE(float_mem.empty());

    const auto& int_spans = t.get_const_int_array_spans();
    REQUIRE(int_spans.empty());

    const auto& float_spans = t.get_const_float_array_spans();
    REQUIRE(float_spans.empty());
}

TEST_CASE("Translator constants: mixed - function call containing literal array and index access on literal array", "[translator][constants]")
{
    translator t;
    t.register_function("process", {type::int_array, type::int_scalar}, {"arr", "idx"}, type::int_scalar);

    t.translate(R"(
int[] src = [100, 200]
int res = process([1, 2, 3, 4], src[1])
)");

    const auto& errors = t.get_errors();
    //REQUIRE(errors.empty());

    const auto& int_mem = t.get_const_int_memory();
    REQUIRE(int_mem == std::vector<int64_t>{100, 200, 1, 2, 3, 4, 1});

    const auto& float_mem = t.get_const_float_memory();
    REQUIRE(float_mem.empty());

    const auto& int_spans = t.get_const_int_array_spans();
    REQUIRE(int_spans.size() == 2);
    assert_span_points_to(int_spans[0], int_mem, 0, 2);  // src = [100, 200]
    assert_span_points_to(int_spans[1], int_mem, 2, 4);  // [1, 2, 3, 4]
}
