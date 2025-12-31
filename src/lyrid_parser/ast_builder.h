#ifndef LYRID_AST_BUILDER_H
#define LYRID_AST_BUILDER_H

#include "lyrid/ast.h"

lyrid_expr *lyrid_build_variable_expr(char *var_name);
lyrid_expr *lyrid_build_int_literal_expr(long long value);
lyrid_expr *lyrid_build_float_literal_expr(double value);
lyrid_expr *lyrid_build_array_literal_expr(lyrid_array_literal *array);
lyrid_expr *lyrid_build_func_call_expr(char *func_name, lyrid_func_call *call_info);

lyrid_array_literal *lyrid_build_array_literal_start(lyrid_expr *first_item);
void lyrid_extend_array_literal(lyrid_array_literal *array, lyrid_expr *new_item);

lyrid_func_call *lyrid_build_func_call_start(void);
void lyrid_extend_func_call_args(lyrid_func_call *call, lyrid_expr *new_arg);
void lyrid_set_func_call_name(lyrid_func_call *call, char *func_name);

lyrid_assignment *lyrid_build_assignment(lyrid_type type, char *var_name, lyrid_expr *value);

lyrid_program *lyrid_build_program_start(void);
void lyrid_append_assignment_to_program(lyrid_program *prog, lyrid_assignment *assign);

#endif // LYRID_AST_BUILDER_H
