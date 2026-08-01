#ifndef LOADER_H
#define LOADER_H

#include <stdio.h>
#include <stdlib.h>

extern char **script;
extern size_t script_size;

void load_script(const char *filename);
void free_script(void);

#endif