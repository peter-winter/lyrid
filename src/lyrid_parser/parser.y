%{
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "lyrid/ast.h"
#include "lyrid/ast_builder.h"

extern int yylineno;
void yyerror(lyrid_program **prog, const char *s);

lyrid_program *parsed_program = NULL;
%}

%parse-param { lyrid_program **prog }

%union {
    char *str;
    long long int_val;
    double float_val;
    lyrid_type type;
    lyrid_expr *expr;
    lyrid_array_literal *array;
    lyrid_func_call *func_call;
}

%token TOK_TYPE_INT TOK_TYPE_FLOAT
%token <str> TOK_ID
%token <int_val> TOK_INT_LITERAL
%token <float_val> TOK_FLOAT_LITERAL

%type <type> type
%type <expr> expr atom
%type <array> array_items
%type <func_call> opt_arg_list

%%

program
    : statements
        {
            parsed_program = lyrid_build_program_start();
            parsed_program->is_valid = true;
            parsed_program->error_message = NULL;
            *prog = parsed_program;
        }
    | program type TOK_ID '=' expr '\n'
        {
            if (parsed_program->is_valid)
            {
                lyrid_assignment *assign = lyrid_build_assignment($2, $3, $5);
                lyrid_append_assignment_to_program(parsed_program, assign);
            }
        }
    | program '\n'
        { /* Allow empty lines */ }
    ;

statements
    : type TOK_ID '=' expr '\n'
    | statements type TOK_ID '=' expr '\n'
    | statements '\n'
        { /* Allow empty lines */ }
    ;

type
    : TOK_TYPE_INT
        { $$ = LYRID_TYPE_INT; }
    | TOK_TYPE_FLOAT
        { $$ = LYRID_TYPE_FLOAT; }
    | TOK_TYPE_INT '[' ']'
        { $$ = LYRID_TYPE_INT_ARRAY; }
    | TOK_TYPE_FLOAT '[' ']'
        { $$ = LYRID_TYPE_FLOAT_ARRAY; }
    ;

expr
    : atom
        { $$ = $1; }
    | TOK_ID '(' opt_arg_list ')'
        {
            lyrid_set_func_call_name($3, $1);
            $$ = lyrid_build_func_call_expr($1, $3);
        }
    ;

atom
    : TOK_ID
        { $$ = lyrid_build_variable_expr($1); }
    | TOK_INT_LITERAL
        { $$ = lyrid_build_int_literal_expr($1); }
    | TOK_FLOAT_LITERAL
        { $$ = lyrid_build_float_literal_expr($1); }
    | '[' array_items ']'
        { $$ = lyrid_build_array_literal_expr($2); }
    | '[' ']'
        { yyerror(prog, "Empty array literal not allowed"); }
    ;

array_items
    : expr
        { $$ = lyrid_build_array_literal_start($1); }
    | array_items ',' expr
        {
            lyrid_extend_array_literal($1, $3);
            $$ = $1;
        }
    ;

opt_arg_list
    : /* empty */
        { $$ = lyrid_build_func_call_start(); }
    | expr
        {
            $$ = lyrid_build_func_call_start();
            lyrid_extend_func_call_args($$, $1);
        }
    | opt_arg_list ',' expr
        {
            lyrid_extend_func_call_args($1, $3);
            $$ = $1;
        }
    ;

%%

void
yyerror(lyrid_program **prog, const char *s)
{
    if (*prog != NULL && (*prog)->is_valid)
    {
        /* Only set the first error; subsequent ones are ignored */
        char error_buf[256];
        snprintf(error_buf, sizeof(error_buf), "Syntax error at line %d: %s", yylineno, s);
        (*prog)->error_message = strdup(error_buf);
        (*prog)->is_valid = false;
    }
}
