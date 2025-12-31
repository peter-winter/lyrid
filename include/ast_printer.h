#ifndef LYRID_AST_PRINTER_H
#define LYRID_AST_PRINTER_H

#include "lyrid/ast.h"

/* All functions return a newly allocated string that the caller must free. */

char *lyrid_print_type(lyrid_type type);
char *lyrid_print_expr(const lyrid_expr *expr);
char *lyrid_print_assignment(const lyrid_assignment *assign);
char *lyrid_print_program(const lyrid_program *prog);

#endif // LYRID_AST_PRINTER_H
