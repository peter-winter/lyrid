#include <stdlib.h>
#include "lyrid/ast_builder.h"

lyrid_expr *
lyrid_build_variable_expr(char *var_name)
{
    lyrid_expr *e = malloc(sizeof(lyrid_expr));
    e->kind = LYRID_EXPR_VARIABLE;
    e->var_name = var_name;
    return e;
}

lyrid_expr *
lyrid_build_int_literal_expr(long long value)
{
    lyrid_expr *e = malloc(sizeof(lyrid_expr));
    e->kind = LYRID_EXPR_INT_LITERAL;
    e->int_value = value;
    return e;
}

lyrid_expr *
lyrid_build_float_literal_expr(double value)
{
    lyrid_expr *e = malloc(sizeof(lyrid_expr));
    e->kind = LYRID_EXPR_FLOAT_LITERAL;
    e->float_value = value;
    return e;
}

lyrid_expr *
lyrid_build_array_literal_expr(lyrid_array_literal *array)
{
    lyrid_expr *e = malloc(sizeof(lyrid_expr));
    e->kind = LYRID_EXPR_ARRAY_LITERAL;
    e->array = *array;
    free(array);
    return e;
}

lyrid_expr *
lyrid_build_func_call_expr(char *func_name, lyrid_func_call *call_info)
{
    lyrid_expr *e = malloc(sizeof(lyrid_expr));
    e->kind = LYRID_EXPR_FUNC_CALL;
    e->func_call = *call_info;
    e->func_call.name = func_name;
    free(call_info);
    return e;
}

lyrid_array_literal *
lyrid_build_array_literal_start(lyrid_expr *first_item)
{
    lyrid_array_literal *array = malloc(sizeof(lyrid_array_literal));
    array->count = 1;
    array->items = malloc(sizeof(lyrid_expr *));
    array->items[0] = first_item;
    return array;
}

void
lyrid_extend_array_literal(lyrid_array_literal *array, lyrid_expr *new_item)
{
    array->count++;
    array->items = realloc(array->items, array->count * sizeof(lyrid_expr *));
    array->items[array->count - 1] = new_item;
}

lyrid_func_call *
lyrid_build_func_call_start(void)
{
    lyrid_func_call *call = malloc(sizeof(lyrid_func_call));
    call->arg_count = 0;
    call->args = NULL;
    call->name = NULL;
    return call;
}

void
lyrid_extend_func_call_args(lyrid_func_call *call, lyrid_expr *new_arg)
{
    call->arg_count++;
    call->args = realloc(call->args, call->arg_count * sizeof(lyrid_expr *));
    call->args[call->arg_count - 1] = new_arg;
}

void
lyrid_set_func_call_name(lyrid_func_call *call, char *func_name)
{
    call->name = func_name;
}

lyrid_assignment *
lyrid_build_assignment(lyrid_type type, char *var_name, lyrid_expr *value)
{
    lyrid_assignment *assign = malloc(sizeof(lyrid_assignment));
    assign->has_explicit_type = true;
    assign->type = type;
    assign->var_name = var_name;
    assign->value = value;
    return assign;
}

lyrid_program *
lyrid_build_program_start(void)
{
    lyrid_program *prog = malloc(sizeof(lyrid_program));
    prog->count = 0;
    prog->statements = NULL;
    return prog;
}

void
lyrid_append_assignment_to_program(lyrid_program *prog, lyrid_assignment *assign)
{
    prog->count++;
    prog->statements = realloc(prog->statements, prog->count * sizeof(lyrid_assignment *));
    prog->statements[prog->count - 1] = assign;
}
