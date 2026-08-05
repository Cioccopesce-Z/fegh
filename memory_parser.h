#ifndef MEMORY_PARSER_H 
#define MEMORY_PARSER_H

#include "loader.h"
#include <stdio.h>
#include <string.h>

#define true  1
#define false 0


__uintmax_t calcuate_dimesion_in_byte_for_a_scope_not_declaring(size_t base, size_t end);

#endif