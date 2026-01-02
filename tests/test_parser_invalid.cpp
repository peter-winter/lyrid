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

TEST_CASE("Invalid number literal - just dot", "[parser][invalid]")
{
    parser p;
    p.parse("int x = .");

    const auto& errors = p.get_errors();
    REQUIRE(errors.size() == 1);
    REQUIRE(errors[0] == "Error [1, 9]: Invalid number literal: no digits");
}

TEST_CASE("Invalid float literal - extra dot", "[parser][invalid]")
{
    parser p;
    p.parse("int x = .4.3");

    const auto& errors = p.get_errors();
    REQUIRE(errors.size() == 1);
    REQUIRE(errors[0] == "Error [1, 11]: Extra characters after expression");
}

TEST_CASE("Invalid float literal - two dots in a row", "[parser][invalid]")
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
    p.parse("int x = 9223372036854775808");     // 2^63

    const auto& errors = p.get_errors();
    REQUIRE(errors.size() == 1);
    REQUIRE(errors[0] == "Error [1, 9]: Invalid integer literal");
}

TEST_CASE("Invalid integer literal: negative overflow", "[parser][invalid]")
{
    parser p;
    p.parse("int x = -9223372036854775809");     // -2^63

    const auto& errors = p.get_errors();
    REQUIRE(errors.size() == 1);
    REQUIRE(errors[0] == "Error [1, 9]: Invalid integer literal");
}

TEST_CASE("Invalid scientific notation: missing exponent digits", "[parser][invalid]")
{
    parser p;
    p.parse("float x = 1e");

    const auto& errors = p.get_errors();
    REQUIRE(errors.size() == 1);
    REQUIRE(errors[0] == "Error [1, 11]: Invalid number literal: exponent has no digits");
}

TEST_CASE("Invalid scientific notation: exponent with sign but no digits (positive)", "[parser][invalid]")
{
    parser p;
    p.parse("float x = 1e+");

    const auto& errors = p.get_errors();
    REQUIRE(errors.size() == 1);
    REQUIRE(errors[0] == "Error [1, 11]: Invalid number literal: exponent has no digits");
}

TEST_CASE("Invalid scientific notation: exponent with sign but no digits (negative)", "[parser][invalid]")
{
    parser p;
    p.parse("float x = 1.5e-");

    const auto& errors = p.get_errors();
    REQUIRE(errors.size() == 1);
    REQUIRE(errors[0] == "Error [1, 11]: Invalid number literal: exponent has no digits");
}

TEST_CASE("Invalid scientific notation: multiple exponents", "[parser][invalid]")
{
    parser p;
    p.parse("float x = 1e2e3");

    const auto& errors = p.get_errors();
    REQUIRE(errors.size() == 1);
    REQUIRE(errors[0] == "Error [1, 14]: Extra characters after expression");
}

TEST_CASE("Invalid float literal: very large exponent", "[parser][invalid]")
{
    parser p;
    p.parse("float x = 1.0e1000");

    const auto& errors = p.get_errors();
    REQUIRE(errors.size() == 1);
    REQUIRE(errors[0] == "Error [1, 11]: Invalid float literal");
}

TEST_CASE("Invalid float literal: very small exponent", "[parser][valid]")
{
    parser p;
    p.parse("float x = 1e-400");

    const auto& errors = p.get_errors();
    REQUIRE(errors.size() == 1);
    REQUIRE(errors[0] == "Error [1, 11]: Invalid float literal");
}

TEST_CASE("Invalid declaration: missing type keyword, starts with literal (triggers generic error)", "[parser][invalid]")
{
    parser p;
    p.parse("42 = x");

    const auto& errors = p.get_errors();
    REQUIRE(errors.size() == 1);
    REQUIRE(errors[0] == "Error [1, 1]: Invalid declaration");

    const auto& prog = p.get_program();
    REQUIRE(prog.declarations_.empty());
}

TEST_CASE("Invalid declaration: missing type keyword, starts with punctuation (triggers generic error)", "[parser][invalid]")
{
    parser p;
    p.parse("= 42");

    const auto& errors = p.get_errors();
    REQUIRE(errors.size() == 1);
    REQUIRE(errors[0] == "Error [1, 1]: Invalid declaration");

    const auto& prog = p.get_program();
    REQUIRE(prog.declarations_.empty());
}

TEST_CASE("Parser recovery: skips malformed declaration line (generic error branch)", "[parser][invalid]")
{
    parser p;
    p.parse(R"(
42 = x
int y = 10
int z = 20
)");

    const auto& errors = p.get_errors();
    REQUIRE(errors.size() == 1);
    REQUIRE(errors[0] == "Error [2, 1]: Invalid declaration");

    const auto& prog = p.get_program();
    REQUIRE(prog.declarations_.empty());
}

TEST_CASE("Empty array literal is invalid", "[parser][invalid]")
{
    parser p;
    p.parse("int[] a = []");

    const auto& errors = p.get_errors();
    REQUIRE(errors.size() == 1);
    REQUIRE(errors[0] == "Error [1, 11]: Array construction cannot be empty; expected elements or a comprehension");

    const auto& prog = p.get_program();
    REQUIRE(prog.declarations_.empty());
}

TEST_CASE("Empty array literal with internal whitespace is invalid", "[parser][invalid]")
{
    parser p;
    p.parse("int[] a = [   ]");

    const auto& errors = p.get_errors();
    REQUIRE(errors.size() == 1);
    REQUIRE(errors[0] == "Error [1, 11]: Array construction cannot be empty; expected elements or a comprehension");

    const auto& prog = p.get_program();
    REQUIRE(prog.declarations_.empty());
}
