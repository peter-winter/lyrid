#ifndef LYRID_PARSER_H
#define LYRID_PARSER_H

#include "lyrid/ast.h"

lyrid_program *lyrid_parse_file(const char *filename);
lyrid_program *lyrid_parse_string(const char *source);

#endif // LYRID_PARSER_H
