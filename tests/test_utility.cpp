#include <catch2/catch_test_macros.hpp>

#include "utility.hpp"

#include <cstdint>

using namespace lyrid;


TEST_CASE("normalized_modulo", "[normalized_modulo]")
{
    SECTION("Basic positive indices (len = 10)")
    {
        REQUIRE(normalized_modulo(0, 10) == 0);
        REQUIRE(normalized_modulo(5, 10) == 5);
        REQUIRE(normalized_modulo(9, 10) == 9);
        REQUIRE(normalized_modulo(10, 10) == 0);
        REQUIRE(normalized_modulo(11, 10) == 1);
        REQUIRE(normalized_modulo(25, 10) == 5);
    }

    SECTION("Specified negative indexing cases (len = 10)")
    {
        REQUIRE(normalized_modulo(-1, 10) == 9);   // len - 1
        REQUIRE(normalized_modulo(-2, 10) == 8);   // len - 2
        REQUIRE(normalized_modulo(-9, 10) == 1);
        REQUIRE(normalized_modulo(-10, 10) == 0);  // -len → 0
        REQUIRE(normalized_modulo(-11, 10) == 9);  // -len-1 → len - 1
        REQUIRE(normalized_modulo(-12, 10) == 8);
    }

    SECTION("Wrapping beyond one full cycle (len = 10)")
    {
        REQUIRE(normalized_modulo(-21, 10) == 9);  // -2*len -1 → len - 1
        REQUIRE(normalized_modulo(-20, 10) == 0);  // -2*len → 0
        REQUIRE(normalized_modulo(20, 10) == 0);   // 2*len → 0
        REQUIRE(normalized_modulo(21, 10) == 1);
    }

    SECTION("Edge case: len = 1")
    {
        REQUIRE(normalized_modulo(0, 1) == 0);
        REQUIRE(normalized_modulo(1, 1) == 0);
        REQUIRE(normalized_modulo(100, 1) == 0);
        REQUIRE(normalized_modulo(-1, 1) == 0);
        REQUIRE(normalized_modulo(-2, 1) == 0);
        REQUIRE(normalized_modulo(-100, 1) == 0);
    }

    SECTION("Different length (len = 7)")
    {
        REQUIRE(normalized_modulo(0, 7) == 0);
        REQUIRE(normalized_modulo(6, 7) == 6);
        REQUIRE(normalized_modulo(7, 7) == 0);
        REQUIRE(normalized_modulo(-1, 7) == 6);
        REQUIRE(normalized_modulo(-7, 7) == 0);
        REQUIRE(normalized_modulo(-8, 7) == 6);
    }

    SECTION("Large indices (len = 10)")
    {
        REQUIRE(normalized_modulo(1000000000000LL, 10) == 0);
        REQUIRE(normalized_modulo(-1000000000001LL, 10) == 9);
        REQUIRE(normalized_modulo(9223372036854775807LL, 10) == 7);  // INT64_MAX % 10
        REQUIRE(normalized_modulo(-9223372036854775807LL, 10) == 3); // -9223372036854775807 % 10 → -7 → 3
    }
}
