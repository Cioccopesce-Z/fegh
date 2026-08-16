#ifndef FREE_H
#define FREE_H

#include "mem.h"
#include "loader.h"
#include "memory_parser.h"

/* libera memory[] (allocato da begin() in mem.c), azzera memory_cursor */
void free_memory_buffer(void);

/* libera var_table: per ogni entry libera name e repetition, poi
   l'array stesso; azzera var_table_count/var_table_capacity */
void free_var_table(void);

/* libera scope_memory[i] per ogni scope, poi scope_memory e scope_dim.
   USA scope_count come limite del ciclo: va chiamata PRIMA di
   free_scope_signatures(), che azzera scope_count. */
void free_scope_memory_fingerprints(void);

/* chiama, nell'ordine corretto, tutte le free sopra + quelle gia'
   esistenti (free_script, free_scope_signatures). Da chiamare una
   sola volta, alla fine del programma, quando script[]/scope_table
   e i fingerprint non servono piu'. */
void free_all_fegh_resources(void);

#endif
