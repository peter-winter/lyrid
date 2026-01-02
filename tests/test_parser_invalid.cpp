// tests/test_parser_invalid.cpp
#include <catch2/catch_test_macros.hpp>

#include "parser.hpp"

using namespace lyrid;

TEST_CASE("Array comprehension: mismatched variable/source counts", "[parser][invalid]")
{
    parser p;
    p.parse("int[] res = [|x, y| in |src| do 42]");

    const auto& errors = p.get_errors();
    REQUIRE(errors.size() == 1);
    REQUIRE(errors[0] == "Error [1, 14]: Number of variables (2) and source expressions (1) must match in array comprehension");
}

TEST_CASE("Array comprehension: missing 'in' keyword", "[parser][invalid]")
{
    parser p;
    p.parse("int[] res = [|x| foo |src| do 42]");

    const auto& errors = p.get_errors();
    REQUIRE(errors.size() == 1);
    REQUIRE(errors[0] == "Error [1, 14]: Expected 'in' after variable list in array comprehension");
}

TEST_CASE("Array comprehension: missing 'do' keyword", "[parser][invalid]")
{
    parser p;
    p.parse("int[] res = [|x| in |src| 42]");

    const auto& errors = p.get_errors();
    REQUIRE(errors.size() == 1);
    REQUIRE(errors[0] == "Error [1, 14]: Expected 'do' after source list in array comprehension");
}

TEST_CASE("Array comprehension: no variables", "[parser][invalid]")
{
    parser p;
    p.parse("int[] res = [|| in |src| do 42]");

    const auto& errors = p.get_errors();
    REQUIRE(errors.size() == 1);
    REQUIRE(errors[0] == "Error [1, 14]: Array comprehension must have at least one variable");
}

TEST_CASE("Extra characters after expression", "[parser][invalid]")
{
    parser p;
    p.parse("int x = 42 extra");

    const auto& errors = p.get_errors();
    REQUIRE(errors.size() == 1);
    REQUIRE(errors[0] == "Error [1, 12]: Extra characters after expression");
}

TEST_CASE("Unexpected token (unknown type)", "[parser][invalid]")
{
    parser p;
    p.parse("unknown_type x = 1");

    const auto& errors = p.get_errors();
    REQUIRE(errors.size() == 1);
    REQUIRE(errors[0] == "Error [1, 1]: Unknown type 'unknown_type'; expected 'int' or 'float'");
}

TEST_CASE("Chained indexing causes syntax error", "[parser][invalid]")
{
    parser p;
    p.parse("int x = arr[0][1]");

    const auto& errors = p.get_errors();
    REQUIRE(errors.size() == 1);
    REQUIRE(errors[0] == "Error [1, 15]: Extra characters after expression");
}

TEST_CASE("Missing closing bracket in index", "[parser][invalid]")
{
    parser p;
    p.parse("int x = arr[0");

    const auto& errors = p.get_errors();
    REQUIRE(errors.size() == 1);
    REQUIRE(errors[0] == "Error [1, 14]: Expected ']' after index expression");
}

TEST_CASE("Missing identifier after type in declaration", "[parser][invalid]")
{
    parser p;
    p.parse("int = 4");

    const auto& errors = p.get_errors();
    REQUIRE(errors.size() == 1);
    REQUIRE(errors[0] == "Error [1, 5]: Expected identifier after type");
}

TEST_CASE("Missing = after identifier in declaration", "[parser][invalid]")
{
    parser p;
    p.parse("int x 4");

    const auto& errors = p.get_errors();
    REQUIRE(errors.size() == 1);
    REQUIRE(errors[0] == "Error [1, 7]: Expected '=' after identifier");
}

TEST_CASE("Float scalar - extra dot", "[parser][invalid]")
{
    parser p;
    p.parse("int x = .4.3");

    const auto& errors = p.get_errors();
    REQUIRE(errors.size() == 1);
    REQUIRE(errors[0] == "Error [1, 11]: Extra characters after expression");
}

TEST_CASE("Float scalar - two dots in a row", "[parser][invalid]")
{
    parser p;
    p.parse("float x = 1..2");

    const auto& errors = p.get_errors();
    REQUIRE(errors.size() == 1);
    REQUIRE(errors[0] == "Error [1, 13]: Extra characters after expression");
}

TEST_CASE("Invalid integer literal: overflow", "[parser][invalid]")
{
    parser p;
    p.parse("int x = 999999999999999999999999999999999999999");

    const auto& errors = p.get_errors();
    REQUIRE(errors.size() == 1);
    REQUIRE(errors[0].find("Invalid integer literal") != std::string::npos);
}

TEST_CASE("Invalid float literal: overflow", "[parser][invalid]")
{
    parser p;
    p.parse("int x = 999999999999999999999999999999999999999");

    const auto& errors = p.get_errors();
    REQUIRE(errors.size() == 1);
    REQUIRE(errors[0].find("Invalid integer literal") != std::string::npos);
}

