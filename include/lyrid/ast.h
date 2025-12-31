#ifndef LYRID_AST_H
#define LYRID_AST_H

#include <stddef.h>
#include <stdbool.h>

typedef enum {
    LYRID_TYPE_INT,
    LYRID_TYPE_FLOAT,
    LYRID_TYPE_INT_ARRAY,
    LYRID_TYPE_FLOAT_ARRAY
} lyrid_type;

typedef struct lyrid_expr lyrid_expr;

typedef struct {
    lyrid_expr **items;
    size_t count;
} lyrid_array_literal;

typedef struct {
    char *name;
    lyrid_expr **args;
    size_t arg_count;
} lyrid_func_call;

typedef enum {
    LYRID_EXPR_VARIABLE,
    LYRID_EXPR_INT_LITERAL,
    LYRID_EXPR_FLOAT_LITERAL,
    LYRID_EXPR_ARRAY_LITERAL,
    LYRID_EXPR_FUNC_CALL
} lyrid_expr_kind;

struct lyrid_expr {
    lyrid_expr_kind kind;
    union {
        char *var_name;
        long long int_value;
        double float_value;
        lyrid_array_literal array;
        lyrid_func_call func_call;
    };
};

typedef struct {
    bool has_explicit_type;
    lyrid_type type;          // Used only if has_explicit_type is true
    char *var_name;
    lyrid_expr *value;
} lyrid_assignment;

typedef struct {
    bool is_valid;
    char *error_message;      // Owned by this struct; NULL if valid
    lyrid_assignment **statements;
    size_t count;
} lyrid_program;

void lyrid_ast_free(lyrid_program *prog);

#endif // LYRID_AST_H
