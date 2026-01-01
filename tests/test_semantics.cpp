#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>

using Catch::Matchers::ContainsSubstring;

#include "parser.hpp"
#include "ast.hpp"
#include "semantic_analyzer.hpp"

using namespace lyrid;


TEST_CASE("Semantic error: non-integer array index", "[semantic]")
{
    parser p;
    p.parse(R"(
int[] arr = [1, 2, 3]
float idx = 0.5
int x = arr[idx]
)");

    const auto& parse_errors = p.get_errors();
    REQUIRE(parse_errors.empty());

    semantic_analyzer sa;
    sa.analyze(p.get_program());

    REQUIRE(!sa.is_valid());
    const auto& sem_errors = sa.get_errors();
    REQUIRE_FALSE(sem_errors.empty());
    REQUIRE_THAT(sem_errors.back(), ContainsSubstring("Array index must be of type 'int', but got 'float'"));
}

TEST_CASE("Semantic error: mixed types in array construction", "[semantic]")
{
    parser p;
    p.parse("int[] arr = [1, 2.0, 3]");

    const auto& parse_errors = p.get_errors();
    REQUIRE(parse_errors.empty());

    semantic_analyzer sa;
    sa.analyze(p.get_program());

    REQUIRE(!sa.is_valid());
    const auto& sem_errors = sa.get_errors();
    REQUIRE_FALSE(sem_errors.empty());
    REQUIRE_THAT(sem_errors[0], ContainsSubstring("Type mismatch in array construction"));
}

TEST_CASE("Semantic error: nested array in construction", "[semantic]")
{
    parser p;
    p.parse(R"(
int[] inner = [1, 2]
int[] outer = [inner]
)");

    const auto& parse_errors = p.get_errors();
    REQUIRE(parse_errors.empty());

    semantic_analyzer sa;
    sa.analyze(p.get_program());

    REQUIRE(!sa.is_valid());
    const auto& sem_errors = sa.get_errors();
    REQUIRE_FALSE(sem_errors.empty());
    REQUIRE_THAT(sem_errors.back(), ContainsSubstring("Array construction elements must be scalar types"));
}
