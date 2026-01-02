#pragma once

#include "ast.hpp"
#include "type.hpp"

#include <string>
#include <vector>
#include <optional>

namespace lyrid
{
    
class parser
{
public:
    parser() = default;

    void parse(const std::string& source);

    const std::vector<std::string>& get_errors() const;

    const ast::program& get_program() const;
    ast::program& get_program();
    
private:
    std::string input_;
    size_t pos_ = 0;
    size_t line_ = 1;
    size_t column_ = 1;
    ast::program prog_;
    std::vector<std::string> errors_;

    char peek() const;
    char advance();
    void skip_whitespace();
    void skip_horizontal_whitespace();

    bool match(char expected);
    void expect(char expected, const std::string& message);

    void error(const ast::source_location& loc, const std::string& message);

    std::optional<std::pair<std::string, ast::source_location>> parse_identifier();

    std::optional<ast::expr_wrapper> parse_number();

    std::optional<type> parse_type();

    std::optional<std::vector<ast::expr_wrapper>> parse_arg_list();

    std::optional<std::vector<ast::expr_wrapper>> parse_literal_array_construction();
    
    std::optional<ast::comprehension> parse_array_comprehension();
    
    std::optional<ast::expr_wrapper> parse_array_construction();

    std::optional<ast::expr_wrapper> parse_primary();

    std::optional<ast::expr_wrapper> parse_expr();

    std::optional<ast::expr_wrapper> try_parse_index_access(ast::expr_wrapper&& base);

    bool parse_declaration();
};

}
