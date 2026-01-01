#include "parser.hpp"
#include "utility.hpp"

#include <cctype>
#include <limits>
#include <stdexcept>
#include <memory>

namespace lyrid
{

const std::vector<std::string>& parser::get_errors() const
{
    return errors_;
}

const program& parser::get_program() const
{
    return prog_;
}

void parser::parse(const std::string& source)
{
    input_ = source;
    pos_ = 0;
    line_ = 1;
    prog_ = program{};
    errors_.clear();

    skip_whitespace();

    while (peek() != '\0')
    {
        size_t decl_start_line = line_;
        size_t start_errors = errors_.size();

        bool success = parse_declaration();

        if (!success)
        {
            if (errors_.size() == start_errors)
            {
                errors_.emplace_back("Line " + std::to_string(decl_start_line) + ": Invalid declaration");
            }

            // Recovery: skip to the end of the current line
            while (peek() != '\0' && peek() != '\n')
            {
                advance();
            }

            if (peek() == '\n')
            {
                advance();
            }
        }

        skip_whitespace();
    }

    if (!errors_.empty())
    {
        prog_.declarations_.clear();
    }
}

char parser::peek() const
{
    if (pos_ < input_.size())
    {
        return input_[pos_];
    }

    return '\0';
}

char parser::advance()
{
    char c = peek();

    if (c != '\0')
    {
        ++pos_;

        if (c == '\n')
        {
            ++line_;
        }
    }

    return c;
}

void parser::skip_whitespace()
{
    while (true)
    {
        char c = peek();

        if (!std::isspace(static_cast<unsigned char>(c)))
        {
            break;
        }

        advance();
    }
}

void parser::skip_horizontal_whitespace()
{
    while (true)
    {
        char c = peek();

        if (c != ' ' && c != '\t' && c != '\r')
        {
            break;
        }

        advance();
    }
}

bool parser::match(char expected)
{
    skip_horizontal_whitespace();

    if (peek() == expected)
    {
        advance();

        return true;
    }

    return false;
}

void parser::expect(char expected, const std::string& message)
{
    if (!match(expected))
    {
        error(message);
    }
}

void parser::error(const std::string& message)
{
    errors_.emplace_back("Line " + std::to_string(line_) + ": " + message);
}

std::string parser::parse_identifier()
{
    skip_horizontal_whitespace();

    if (!std::isalpha(peek()) && peek() != '_')
    {
        return "";
    }

    size_t start = pos_;

    advance();

    while (std::isalnum(peek()) || peek() == '_')
    {
        advance();
    }

    return input_.substr(start, pos_ - start);
}

std::optional<expr> parser::parse_number()
{
    skip_horizontal_whitespace();

    std::string str;

    if (peek() == '.')
    {
        str += advance();
    }

    while (std::isdigit(peek()))
    {
        str += advance();
    }

    if (peek() == '.')
    {
        str += advance();

        while (std::isdigit(peek()))
        {
            str += advance();
        }
    }

    if (str.empty())
    {
        return {};
    }

    bool has_digit = false;

    for (char c : str)
    {
        if (std::isdigit(c))
        {
            has_digit = true;

            break;
        }
    }

    if (!has_digit)
    {
        return {};
    }

    bool is_float = str.find('.') != std::string::npos;

    if (is_float)
    {
        if (str.front() == '.')
        {
            str = "0" + str;
        }

        if (str.back() == '.')
        {
            str += "0";
        }

        try
        {
            return expr(float_scalar{std::stod(str)});
        }
        catch (...)
        {
            error("Invalid float literal");

            return {};
        }
    }
    else
    {
        try
        {
            int64_t val = std::stoll(str);
            return expr(int_scalar{val});
        }
        catch (...)
        {
            error("Invalid integer literal");

            return {};
        }
    }
}

std::optional<type> parser::parse_type()
{
    skip_horizontal_whitespace();

    std::string kw = parse_identifier();

    bool is_array = match('[');

    if (is_array)
    {
        expect(']', "Expected ']' after '[' in array type");
    }

    if (kw == "int")
    {
        return is_array ? type::int_array : type::int_scalar;
    }

    if (kw == "float")
    {
        return is_array ? type::float_array : type::float_scalar;
    }

    if (!kw.empty())
    {
        error("Unknown type '" + kw + "'; expected 'int' or 'float'");
    }

    return {};
}

std::optional<std::vector<expr_wrapper>> parser::parse_arg_list()
{
    std::vector<expr_wrapper> args;

    skip_horizontal_whitespace();

    if (match(')'))
    {
        return args;
    }

    do
    {
        auto e = parse_expr();

        if (!e)
        {
            error("Expected expression in argument list");

            return {};
        }

        args.emplace_back(expr_wrapper{std::move(*e)});
    }
    while (match(','));

    expect(')', "Expected ')' to close argument list");

    return args;
}

std::optional<array_construction> parser::parse_literal_array_construction()
{
    std::vector<expr_wrapper> elements;

    auto first_opt = parse_expr();
    if (!first_opt)
    {
        error("Expected expression in array literal");
        return {};
    }
    elements.emplace_back(expr_wrapper{std::move(*first_opt)});

    while (true)
    {
        skip_horizontal_whitespace();
        if (!match(','))
        {
            break;
        }
        skip_horizontal_whitespace();
        auto elem_opt = parse_expr();
        if (!elem_opt)
        {
            error("Expected expression after ',' in array literal");
            return {};
        }
        elements.emplace_back(expr_wrapper{std::move(*elem_opt)});
    }

    return array_construction{std::move(elements)};
}

std::optional<comprehension> parser::parse_array_comprehension()
{
    // Expect opening '|' for variables
    skip_horizontal_whitespace();
    if (!match('|'))
    {
        error("Expected '|' to start variable list in array comprehension");
        return {};
    }

    // Parse variables (comma-separated identifiers, at least one)
    std::vector<std::string> variables;
    skip_horizontal_whitespace();

    // Check for empty variable list early
    if (match('|'))
    {
        error("Array comprehension must have at least one variable");
        return {};
    }

    do
    {
        std::string var = parse_identifier();
        if (var.empty())
        {
            error("Expected identifier in variable list");
            return {};
        }
        variables.push_back(std::move(var));

        skip_horizontal_whitespace();
    } while (match(','));

    // Expect closing '|' for variables
    if (!match('|'))
    {
        error("Expected '|' to close variable list in array comprehension");
        return {};
    }

    // Expect 'in' keyword
    skip_horizontal_whitespace();
    std::string in_kw = parse_identifier();
    if (in_kw != "in")
    {
        error("Expected 'in' after variable list in array comprehension");
        return {};
    }

    // Expect opening '|' for sources
    skip_horizontal_whitespace();
    if (!match('|'))
    {
        error("Expected '|' to start source expression list in array comprehension");
        return {};
    }

    // Parse source expressions (comma-separated, count must match variables)
    std::vector<expr_wrapper> in_exprs;
    skip_horizontal_whitespace();

    // Require at least one source expression (mismatch will be caught below anyway)
    auto src_opt = parse_expr();
    if (!src_opt)
    {
        error("Expected source expression in 'in' clause");
        return {};
    }
    in_exprs.emplace_back(expr_wrapper{std::move(*src_opt)});

    while (true)
    {
        skip_horizontal_whitespace();
        if (!match(','))
        {
            break;
        }
        skip_horizontal_whitespace();
        src_opt = parse_expr();
        if (!src_opt)
        {
            error("Expected source expression after ',' in 'in' clause");
            return {};
        }
        in_exprs.emplace_back(expr_wrapper{std::move(*src_opt)});
    }

    // Check count mismatch (syntax error as originally required)
    if (in_exprs.size() != variables.size())
    {
        error("Number of variables (" + std::to_string(variables.size()) +
              ") and source expressions (" + std::to_string(in_exprs.size()) +
              ") must match in array comprehension");
        return {};
    }

    // Expect closing '|' for sources
    skip_horizontal_whitespace();
    if (!match('|'))
    {
        error("Expected '|' to close source expression list in array comprehension");
        return {};
    }

    // Expect 'do' keyword
    skip_horizontal_whitespace();
    std::string do_kw = parse_identifier();
    if (do_kw != "do")
    {
        error("Expected 'do' after source list in array comprehension");
        return {};
    }

    // Parse body expression
    skip_horizontal_whitespace();
    auto body_opt = parse_expr();
    if (!body_opt)
    {
        error("Expected expression after 'do' in array comprehension");
        return {};
    }

    auto do_wrapper = std::make_unique<expr_wrapper>(expr_wrapper{std::move(*body_opt)});

    return comprehension{
        std::move(variables),
        std::move(in_exprs),
        std::move(do_wrapper)
    };
}

std::optional<expr> parser::parse_array_construction()
{
    if (!match('['))
    {
        return {};
    }

    skip_horizontal_whitespace();

    // Disallow empty array
    if (match(']'))
    {
        error("Array construction cannot be empty; expected elements or a comprehension");
        return {};
    }

    // Distinguish comprehension (starts with '|') from literal array
    size_t save_pos = pos_;
    size_t save_line = line_;
    skip_horizontal_whitespace();

    if (match('|'))
    {
        // Comprehension path
        pos_ = save_pos;   // rollback the matched '|'
        line_ = save_line;

        auto comp_opt = parse_array_comprehension();
        if (!comp_opt)
        {
            // Recovery: skip to ']'
            while (peek() != ']' && peek() != '\0' && peek() != '\n')
            {
                advance();
            }
            match(']');
            return {};
        }

        skip_horizontal_whitespace();
        expect(']', "Expected ']' to close array comprehension");

        return expr(std::move(*comp_opt));
    }
    else
    {
        // Literal array path
        pos_ = save_pos;
        line_ = save_line;

        auto lit_opt = parse_literal_array_construction();
        if (!lit_opt)
        {
            // Recovery
            while (peek() != ']' && peek() != '\0' && peek() != '\n')
            {
                advance();
            }
            match(']');
            return {};
        }

        skip_horizontal_whitespace();
        expect(']', "Expected ']' to close array literal");

        return expr(std::move(*lit_opt));
    }
}

std::optional<expr> parser::parse_primary()
{
    skip_horizontal_whitespace();

    auto num = parse_number();
    if (num)
    {
        return num;
    }

    auto arr = parse_array_construction();
    if (arr)
    {
        return arr;
    }

    std::string name = parse_identifier();
    if (name.empty())
    {
        return {};
    }

    if (match('('))
    {
        auto args_opt = parse_arg_list();
        if (!args_opt)
        {
            return {};
        }

        return expr(f_call{std::move(name), std::move(*args_opt)});
    }

    return expr(id{std::move(name)});
}

std::optional<expr> parser::parse_expr()
{
    auto base = parse_primary();

    if (!base)
    {
        return {};
    }

    expr current = std::move(*base);

    if (match('['))
    {
        auto index = parse_expr();

        if (!index)
        {
            error("Expected index expression");

            return {};
        }

        expect(']', "Expected ']' after index expression");

        auto base_wrapper = std::make_unique<expr_wrapper>(expr_wrapper{std::move(current)});
        auto index_wrapper = std::make_unique<expr_wrapper>(expr_wrapper{std::move(*index)});

        return expr(index_access{std::move(base_wrapper), std::move(index_wrapper)});
    }

    return current;
}

bool parser::parse_declaration()
{
    size_t decl_line = line_;  // Capture the starting line of this declaration

    auto decl_type = parse_type();

    if (!decl_type)
    {
        return false;
    }

    std::string name = parse_identifier();

    if (name.empty())
    {
        error("Expected identifier after type");

        return false;
    }

    if (!match('='))
    {
        error("Expected '=' after identifier");

        return false;
    }

    auto value = parse_expr();

    if (!value)
    {
        return false;
    }

    skip_horizontal_whitespace();

    if (peek() != '\0' && peek() != '\n')
    {
        error("Extra characters after expression");

        return false;
    }

    prog_.declarations_.emplace_back(declaration{*decl_type, std::move(name), std::move(*value), decl_line});

    return true;
}

}
