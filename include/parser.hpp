#pragma once

#include "ast.hpp"
#include <optional>
#include <string>
#include <vector>

namespace lyrid
{
    
class parser
{
public:
    parser() = default;

    std::optional<program> parse(const std::string& source);

    const std::vector<std::string>& get_errors() const;

private:
    std::string input_;
    size_t pos_ = 0;
    size_t line_ = 1;
    program prog_;
    std::vector<std::string> errors_;

    char peek() const;
    char advance();
    void skip_whitespace();
    void skip_horizontal_whitespace();

    bool match(char expected);
    void expect(char expected, const std::string& message);

    void error(const std::string& message);

    std::string parse_identifier();

    std::optional<expr> parse_number();

    std::optional<type> parse_type();

    std::optional<std::vector<expr_wrapper>> parse_arg_list();

    std::optional<expr> parse_array_literal();

    std::optional<expr> parse_function_call();

    std::optional<expr> parse_primary();

    std::optional<expr> parse_expr();

    bool parse_declaration();
};

}
