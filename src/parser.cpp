#include "parser.hpp"
#include "utility.hpp"

#include <cctype>
#include <limits>
#include <stdexcept>

namespace lyrid
{

const std::vector<std::string>& parser::get_errors() const
{
    return errors_;
}
    
std::optional<program> parser::parse(const std::string& source)
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

        return std::nullopt;
    }

    return prog_;
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

        args.emplace_back(expr_wrapper{*e});
    }
    while (match(','));

    expect(')', "Expected ')' to close argument list");

    return args;
}

std::optional<expr> parser::parse_array_literal()
{
    if (!match('['))
    {
        return {};
    }

    skip_horizontal_whitespace();

    if (match(']'))
    {
        error("Empty array literals are not allowed");

        return {};
    }

    auto first = parse_number();

    if (!first)
    {
        error("Array literals must contain only numeric constants");

        return {};
    }

    auto parse_scalar_array = [&]<typename Sc>(Sc sc) -> std::optional<expr>
    {
        using Ar = typename to_array_t<Sc>::type;
        typename Ar::vector_type values { std::get<Sc>(*first).value_ };

        while (match(','))
        {
            auto val = parse_number();

            if (!val || !std::holds_alternative<Sc>(*val))
            {
                error("Type mismatch in array literal");

                return {};
            }

            values.push_back(std::get<Sc>(*val).value_);
        }

        expect(']', "Expected ']' to close array literal");

        return expr(Ar{std::move(values)});
    };
    
    return std::visit(
        overloaded
        {
            [&](int_scalar sc) -> std::optional<expr> { return parse_scalar_array(sc); },
            [&](float_scalar sc) -> std::optional<expr> { return parse_scalar_array(sc); },
            [](auto) -> std::optional<expr> { return {}; }
        },
        *first
    );
}

std::optional<expr> parser::parse_function_call()
{
    size_t saved_pos = pos_;
    size_t saved_line = line_;

    std::string name = parse_identifier();

    if (name.empty())
    {
        return {};
    }

    if (!match('('))
    {
        pos_ = saved_pos;
        line_ = saved_line;

        return {};
    }

    auto args = parse_arg_list();

    if (!args)
    {
        return {};
    }

    return expr(f_call{std::move(name), std::move(*args)});
}

std::optional<expr> parser::parse_primary()
{
    auto num = parse_number();

    if (num)
    {
        return num;
    }

    std::string name = parse_identifier();

    if (!name.empty())
    {
        return expr(id{std::move(name)});
    }

    return parse_array_literal();
}

std::optional<expr> parser::parse_expr()
{
    auto call = parse_function_call();

    if (call)
    {
        return call;
    }

    return parse_primary();
}

bool parser::parse_declaration()
{
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

    prog_.declarations_.emplace_back(declaration{*decl_type, std::move(name), expr_wrapper{*value}});

    return true;
}

}
