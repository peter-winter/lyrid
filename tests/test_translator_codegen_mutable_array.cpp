#include "translator.hpp"
#include "assembly.hpp"
#include "test_instruction_helpers.hpp"

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>

using namespace lyrid;

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

