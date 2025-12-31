#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "lyrid/ast_printer.h"

/* Internal string builder for dynamic concatenation */
typedef struct {
    char *buf;
    size_t len;
    size_t cap;
} lyrid_string_builder;

static void
lyrid_string_builder_init(lyrid_string_builder *sb)
{
    sb->buf = NULL;
    sb->len = 0;
    sb->cap = 0;
}

static void
lyrid_string_builder_append(lyrid_string_builder *sb, const char *str)
{
    size_t str_len = strlen(str);

    /* Grow capacity if necessary */
    if (sb->len + str_len + 1 > sb->cap)
    {
        size_t new_cap = (sb->cap == 0) ? 64 : sb->cap * 2;
        while (sb->len + str_len + 1 > new_cap)
        {
            new_cap *= 2;
        }
        sb->buf = realloc(sb->buf, new_cap);
        sb->cap = new_cap;
    }

    memcpy(sb->buf + sb->len, str, str_len);
    sb->len += str_len;
    sb->buf[sb->len] = '\0';
}

static char *
lyrid_string_builder_detach(lyrid_string_builder *sb)
{
    /* Return the buffer and reset the builder */
    char *result = sb->buf;
    sb->buf = NULL;
    sb->len = 0;
    sb->cap = 0;
    return result;
}

static void
lyrid_string_builder_free(lyrid_string_builder *sb)
{
    free(sb->buf);
    sb->buf = NULL;
    sb->len = 0;
    sb->cap = 0;
}

char *
lyrid_print_type(lyrid_type type)
{
    switch (type)
    {
        case LYRID_TYPE_INT:         return strdup("int");
        case LYRID_TYPE_FLOAT:       return strdup("float");
        case LYRID_TYPE_INT_ARRAY:   return strdup("int[]");
        case LYRID_TYPE_FLOAT_ARRAY: return strdup("float[]");
    }
    return NULL; /* unreachable */
}

char *
lyrid_print_expr(const lyrid_expr *e)
{
    if (e == NULL) return NULL;

    lyrid_string_builder sb;
    lyrid_string_builder_init(&sb);

    switch (e->kind)
    {
        case LYRID_EXPR_VARIABLE:
            lyrid_string_builder_append(&sb, e->var_name);
            break;

        case LYRID_EXPR_INT_LITERAL:
        {
            char tmp[32];
            snprintf(tmp, sizeof(tmp), "%lld", e->int_value);
            lyrid_string_builder_append(&sb, tmp);
            break;
        }

        case LYRID_EXPR_FLOAT_LITERAL:
        {
            char tmp[64];
            snprintf(tmp, sizeof(tmp), "%.15g", e->float_value);
            lyrid_string_builder_append(&sb, tmp);
            break;
        }

        case LYRID_EXPR_ARRAY_LITERAL:
        {
            lyrid_string_builder_append(&sb, "[");
            for (size_t i = 0; i < e->array.count; i++)
            {
                char *item = lyrid_print_expr(e->array.items[i]);
                lyrid_string_builder_append(&sb, item);
                free(item);
                if (i + 1 < e->array.count)
                {
                    lyrid_string_builder_append(&sb, ",");
                }
            }
            lyrid_string_builder_append(&sb, "]");
            break;
        }

        case LYRID_EXPR_FUNC_CALL:
        {
            lyrid_string_builder_append(&sb, e->func_call.name);
            lyrid_string_builder_append(&sb, "(");
            for (size_t i = 0; i < e->func_call.arg_count; i++)
            {
                char *arg = lyrid_print_expr(e->func_call.args[i]);
                lyrid_string_builder_append(&sb, arg);
                free(arg);
                if (i + 1 < e->func_call.arg_count)
                {
                    lyrid_string_builder_append(&sb, ",");
                }
            }
            lyrid_string_builder_append(&sb, ")");
            break;
        }
    }

    return lyrid_string_builder_detach(&sb);
}

char *
lyrid_print_assignment(const lyrid_assignment *assign)
{
    if (assign == NULL) return NULL;

    lyrid_string_builder sb;
    lyrid_string_builder_init(&sb);

    char *type_str = lyrid_print_type(assign->type);
    char *expr_str = lyrid_print_expr(assign->value);

    lyrid_string_builder_append(&sb, type_str);
    lyrid_string_builder_append(&sb, assign->var_name);
    lyrid_string_builder_append(&sb, "=");
    lyrid_string_builder_append(&sb, expr_str);
    lyrid_string_builder_append(&sb, "\n");

    free(type_str);
    free(expr_str);

    return lyrid_string_builder_detach(&sb);
}

char *
lyrid_print_program(const lyrid_program *prog)
{
    if (prog == NULL || !prog->is_valid) return NULL;

    lyrid_string_builder sb;
    lyrid_string_builder_init(&sb);

    for (size_t i = 0; i < prog->count; i++)
    {
        char *stmt = lyrid_print_assignment(prog->statements[i]);
        lyrid_string_builder_append(&sb, stmt);
        free(stmt);
    }

    return lyrid_string_builder_detach(&sb);
}
