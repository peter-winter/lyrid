#include <stdlib.h>
#include "lyrid/ast.h"

static void
free_expr(lyrid_expr *e)
{
    if (e == NULL)
    {
        return;
    }

    switch (e->kind)
    {
        case LYRID_EXPR_VARIABLE:
            free(e->var_name);
            break;

        case LYRID_EXPR_ARRAY_LITERAL:
            for (size_t i = 0; i < e->array.count; i++)
            {
                free_expr(e->array.items[i]);
            }
            free(e->array.items);
            break;

        case LYRID_EXPR_FUNC_CALL:
            free(e->func_call.name);
            for (size_t i = 0; i < e->func_call.arg_count; i++)
            {
                free_expr(e->func_call.args[i]);
            }
            free(e->func_call.args);
            break;

        default:
            /* Literals have no allocated substructures */
            break;
    }

    free(e);
}

void
lyrid_ast_free(lyrid_program *prog)
{
    if (prog == NULL)
    {
        return;
    }

    /* Free error message if present */
    free(prog->error_message);

    /* Free statements only if valid (to avoid partial ASTs) */
    if (prog->is_valid)
    {
        for (size_t i = 0; i < prog->count; i++)
        {
            lyrid_assignment *assign = prog->statements[i];
            free(assign->var_name);
            free_expr(assign->value);
            free(assign);
        }
        free(prog->statements);
    }

    free(prog);
}
