#ifndef LOOKUP_H
#define LOOKUP_H

#include <stdio.h>
#include "loader.h"
#include "memory_parser.h"

FILE *open_lookup_table(void);

void write_scope_header_to_lookup_table(FILE *fp, size_t scope_index, sign_of_scope *scope);

void write_variable_to_lookup_table(FILE *fp,size_t idx, var_data_struct *v);

#endif