#include "parser.hpp"
#include "utility.hpp"

#include <cctype>
#include <limits>
#include <stdexcept>
#include <memory>

namespace lyrid
{

using namespace ast;

const std::vector<std::string>& parser::get_errors() const
{
    return errors_;
}

const program& parser::get_program() const
{
    return prog_;
}

ast::program& parser::get_program()
{
    return prog_;
}

void parser::parse(const std::string& source)
{
    input_ = source;
    pos_ = 0;
    line_ = 1;
    column_ = 1;
    prog_ = program{};
    errors_.clear();

    skip_whitespace();

    while (peek() != '\0')
    {
        size_t decl_start_line = line_;
        size_t decl_start_column = column_;
        size_t start_errors = errors_.size();

        bool success = parse_declaration();

        if (!success)
        {
            if (errors_.size() == start_errors)
            {
                source_location generic_loc{decl_start_line, decl_start_column};
                error(generic_loc, "Invalid declaration");
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
            column_ = 1;
        }
        else
        {
            ++column_;
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
        source_location loc{line_, column_};
        error(loc, message);
    }
}

void parser::error(const source_location& loc, const std::string& message)
{
    std::string prefix = "Error [" + std::to_string(loc.line_) + ", " +
                         std::to_string(loc.column_) + "]: ";
    errors_.emplace_back(prefix + message);
}

std::optional<std::pair<std::string, source_location>> parser::parse_identifier()
{
    skip_horizontal_whitespace();

    if (!std::isalpha(peek()) && peek() != '_')
    {
        return {};
    }

    size_t start_line = line_;
    size_t start_column = column_;

    advance();

    while (std::isalnum(peek()) || peek() == '_')
    {
        advance();
    }

    source_location loc{start_line, start_column};
    std::string identifier = input_.substr(pos_ - (column_ - start_column), column_ - start_column);

    return {{std::move(identifier), loc}};
}

std::optional<expr_wrapper> parser::parse_number()
{
    skip_horizontal_whitespace();

    size_t start_line = line_;
    size_t start_column = column_;

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
        if (std::isdigit(static_cast<unsigned char>(c)))
        {
            has_digit = true;
            break;
        }
    }

    if (!has_digit)
    {
        source_location loc{start_line, start_column};
        error(loc, "Invalid number literal");
        return {};
    }

    bool is_float = str.find('.') != std::string::npos;

    source_location loc{start_line, start_column};

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
            double val = std::stod(str);
            return expr_wrapper(expr(float_scalar{val}), loc);
        }
        catch (...)
        {
            error(loc, "Invalid float literal");
            return {};
        }
    }
    else
    {
        try
        {
            int64_t val = std::stoll(str);
            return expr_wrapper(expr(int_scalar{val}), loc);
        }
        catch (...)
        {
            error(loc, "Invalid integer literal");
            return {};
        }
    }
}

std::optional<type> parser::parse_type()
{
    auto kw_opt = parse_identifier();

    if (!kw_opt)
    {
        return {};
    }

    std::string kw = std::move(kw_opt->first);
    source_location kw_loc = kw_opt->second;

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

    error(kw_loc, "Unknown type '" + kw + "'; expected 'int' or 'float'");
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
            source_location loc{line_, column_};
            error(loc, "Expected expression in argument list");
            return {};
        }

        args.emplace_back(std::move(*e));
    }
    while (match(','));

    expect(')', "Expected ')' to close argument list");

    return args;
}

std::optional<std::vector<expr_wrapper>> parser::parse_literal_array_construction()
{
    std::vector<expr_wrapper> elements;

    auto first_opt = parse_expr();
    if (!first_opt)
    {
        source_location loc{line_, column_};
        error(loc, "Expected expression in array literal");
        return {};
    }
    elements.emplace_back(std::move(*first_opt));

    while (true)
    {
        skip_horizontal_whitespace();
        if (!match(','))
        {
            break;
        }
        auto elem_opt = parse_expr();
        if (!elem_opt)
        {
            source_location loc{line_, column_};
            error(loc, "Expected expression after ',' in array literal");
            return {};
        }
        elements.emplace_back(std::move(*elem_opt));
    }

    return elements;
}

std::optional<comprehension> parser::parse_array_comprehension()
{
    skip_horizontal_whitespace();

    size_t comp_start_line = line_;
    size_t comp_start_column = column_;

    if (!match('|'))
    {
        error({comp_start_line, comp_start_column}, "Expected '|' to start variable list in array comprehension");
        return {};
    }

    source_location comp_loc{comp_start_line, comp_start_column};  // Points to the first '|'

    std::vector<identifier> variables;

    skip_horizontal_whitespace();

    if (match('|'))
    {
        error(comp_loc, "Array comprehension must have at least one variable");
        return {};
    }

    do
    {
        auto var_opt = parse_identifier();
        if (!var_opt)
        {
            source_location loc{line_, column_};
            error(loc, "Expected identifier in variable list");
            return {};
        }

        variables.emplace_back(identifier{std::move(var_opt->first), var_opt->second});

        skip_horizontal_whitespace();
    } while (match(','));

    if (!match('|'))
    {
        source_location loc{line_, column_};
        error(loc, "Expected '|' to close variable list in array comprehension");
        return {};
    }

    skip_horizontal_whitespace();
    auto in_opt = parse_identifier();
    if (!in_opt || in_opt->first != "in")
    {
        source_location loc = in_opt ? in_opt->second : source_location{line_, column_};
        error(comp_loc, "Expected 'in' after variable list in array comprehension");
        return {};
    }

    skip_horizontal_whitespace();
    if (!match('|'))
    {
        source_location loc{line_, column_};
        error(loc, "Expected '|' to start source expression list in array comprehension");
        return {};
    }

    std::vector<expr_wrapper> in_exprs;
    skip_horizontal_whitespace();

    auto src_opt = parse_expr();
    if (!src_opt)
    {
        source_location loc{line_, column_};
        error(loc, "Expected source expression in 'in' clause");
        return {};
    }
    in_exprs.emplace_back(std::move(*src_opt));

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
            source_location loc{line_, column_};
            error(loc, "Expected source expression after ',' in 'in' clause");
            return {};
        }
        in_exprs.emplace_back(std::move(*src_opt));
    }

    if (in_exprs.size() != variables.size())
    {
        error(comp_loc,
              "Number of variables (" + std::to_string(variables.size()) +
              ") and source expressions (" + std::to_string(in_exprs.size()) +
              ") must match in array comprehension");
        return {};
    }

    if (!match('|'))
    {
        source_location loc{line_, column_};
        error(loc, "Expected '|' to close source expression list in array comprehension");
        return {};
    }

    skip_horizontal_whitespace();
    auto do_opt = parse_identifier();
    if (!do_opt || do_opt->first != "do")
    {
        source_location loc = do_opt ? do_opt->second : source_location{line_, column_};
        error(comp_loc, "Expected 'do' after source list in array comprehension");
        return {};
    }

    skip_horizontal_whitespace();
    auto body_opt = parse_expr();
    if (!body_opt)
    {
        source_location loc{line_, column_};
        error(loc, "Expected expression after 'do' in array comprehension");
        return {};
    }

    return comprehension{
        std::move(variables),
        std::move(in_exprs),
        std::make_unique<expr_wrapper>(std::move(*body_opt))
    };
}

std::optional<expr_wrapper> parser::parse_array_construction()
{
    size_t start_line = line_;
    size_t start_column = column_;

    if (!match('['))
    {
        return {};
    }

    skip_horizontal_whitespace();

    if (match(']'))
    {
        source_location loc{start_line, start_column};
        error(loc, "Array construction cannot be empty; expected elements or a comprehension");
        return {};
    }

    size_t save_pos = pos_;
    size_t save_line = line_;
    size_t save_column = column_;

    skip_horizontal_whitespace();

    bool is_comprehension = match('|');

    pos_ = save_pos;
    line_ = save_line;
    column_ = save_column;

    source_location array_loc{start_line, start_column};

    if (is_comprehension)
    {
        auto comp_opt = parse_array_comprehension();
        if (!comp_opt)
        {
            while (peek() != ']' && peek() != '\0')
            {
                advance();
            }
            match(']');
            return {};
        }

        skip_horizontal_whitespace();
        source_location close_loc{line_, column_};
        if (!match(']'))
        {
            error(close_loc, "Expected ']' to close array comprehension");
            return {};
        }

        return expr_wrapper(expr(std::move(*comp_opt)), array_loc);
    }
    else
    {
        auto lit_opt = parse_literal_array_construction();
        if (!lit_opt)
        {
            while (peek() != ']' && peek() != '\0')
            {
                advance();
            }
            match(']');
            return {};
        }

        skip_horizontal_whitespace();
        source_location close_loc{line_, column_};
        if (!match(']'))
        {
            error(close_loc, "Expected ']' to close array literal");
            return {};
        }

        return expr_wrapper(expr(array_construction{std::move(*lit_opt)}), array_loc);
    }
}

std::optional<expr_wrapper> parser::parse_primary()
{
    skip_horizontal_whitespace();

    size_t primary_start_line = line_;
    size_t primary_start_column = column_;

    auto num_opt = parse_number();
    if (num_opt)
    {
        return std::move(*num_opt);
    }

    auto arr_opt = parse_array_construction();
    if (arr_opt)
    {
        return std::move(*arr_opt);
    }

    auto id_opt = parse_identifier();
    if (id_opt)
    {
        identifier ident{std::move(id_opt->first), id_opt->second};

        if (match('('))
        {
            auto args_opt = parse_arg_list();
            if (!args_opt)
            {
                return {};
            }

            source_location call_loc{primary_start_line, primary_start_column};
            f_call call{std::move(ident), std::move(*args_opt)};
            return expr_wrapper(expr(std::move(call)), call_loc);
        }

        return expr_wrapper(expr(std::move(ident)), ident.loc_);
    }

    return {};
}

std::optional<expr_wrapper> parser::try_parse_index_access(expr_wrapper&& base)
{
    if (!match('['))
    {
        return {};
    }

    auto index_opt = parse_expr();
    if (!index_opt)
    {
        source_location loc{line_, column_};
        error(loc, "Expected index expression");
        return {};
    }

    if (!match(']'))
    {
        source_location loc{line_, column_};
        error(loc, "Expected ']' after index expression");
        return {};
    }

    source_location whole_loc{base.loc_.line_, base.loc_.column_};

    index_access acc{
        std::make_unique<expr_wrapper>(std::move(base)),
        std::make_unique<expr_wrapper>(std::move(*index_opt))
    };

    return expr_wrapper(expr(std::move(acc)), whole_loc);
}

std::optional<expr_wrapper> parser::parse_expr()
{
    auto prim_opt = parse_primary();
    if (!prim_opt)
    {
        return {};
    }

    auto current = std::move(*prim_opt);

    auto indexed_opt = try_parse_index_access(std::move(current));
    if (indexed_opt)
    {
        return std::move(*indexed_opt);
    }

    return current;
}

bool parser::parse_declaration()
{
    size_t decl_start_line = line_;
    size_t decl_start_column = column_;

    auto decl_type_opt = parse_type();
    if (!decl_type_opt)
    {
        return false;
    }

    auto name_opt = parse_identifier();
    if (!name_opt)
    {
        source_location loc{line_, column_};
        error(loc, "Expected identifier after type");
        return false;
    }
    identifier name{std::move(name_opt->first), name_opt->second};

    if (!match('='))
    {
        source_location loc{line_, column_};
        error(loc, "Expected '=' after identifier");
        return false;
    }

    auto value_opt = parse_expr();
    if (!value_opt)
    {
        return false;
    }

    skip_horizontal_whitespace();

    if (peek() != '\0' && peek() != '\n')
    {
        source_location extra_loc{line_, column_};
        error(extra_loc, "Extra characters after expression");
        return false;
    }

    source_location decl_loc{decl_start_line, decl_start_column};

    prog_.declarations_.emplace_back(declaration{
        *decl_type_opt,
        std::move(name),
        std::move(*value_opt),
        decl_loc
    });

    return true;
}

}
