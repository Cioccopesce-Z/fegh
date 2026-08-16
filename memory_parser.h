#ifndef MEMORY_PARSER_H 
#define MEMORY_PARSER_H

#include <stdio.h>
#include <string.h>

#define true  1
#define false 0


__uintmax_t create_metadata_for_var_struct_in_a_scope(size_t base, size_t end);

/*stampa contenuto struct delle variabile con metadati a fini di debug*/
void stampa_contenuto_var_table(void);
void stampa_var_table(void);

void create_memory_fingerprint_of_all_scope(/*taking script & scope_table*/);

extern __uint8_t **scope_memory;   /* scope_memory[i] = bytes del fingerprint dello scope i */
extern size_t      *scope_dim;      /* scope_dim[i]    = lunghezza in byte di scope_memory[i] */

void stampa_fingerprint_di_tutti_gli_scope(void);


/* -------------------------------------------------------------------- */
/* struttura di una variabile parsata da una dichiarazione "let"        */
/* niente più limiti fissi: nome e repetition sono malloc'd             */
/* -------------------------------------------------------------------- */
typedef struct {
    char   *name;              /* malloc'd, nome della variabile           */
    size_t  dimension;         /* dimensione in byte di UNA cella          */
    size_t  value;             /* valore iniziale                          */
    size_t  method_list_code;  /* id della method list                     */
    size_t *repetition;        /* dimensioni: [] scalare, [n] array,       */
                                /* [n,m] matrice, ecc. (malloc'd)           */
    size_t  rank;               /* numero di entry in repetition            */
} var_data_struct;

extern size_t var_table_count;
extern size_t var_table_capacity;
extern var_data_struct *var_table;

#endif