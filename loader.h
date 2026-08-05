#ifndef LOADER_H
#define LOADER_H


#include <stdio.h>
#include <stdlib.h>

/* ============================ SCRIPT ============================ */

extern char **script;
extern size_t script_size;

void load_script(const char *filename);
void printscript();
/*remove spaces from a line directly modifyng it*/
static void remove_spaces(char *s);
void free_script(void);

/*return trueif the line pointed by the line_index in script is a declaration of a function*/
size_t line_belong_to_any_function_declaration(size_t line_absolute_index_of_the_line);

/* ======================== SCOPE SIGNATURES ======================== */



/*
 * sign_of_scope
 * -------------
 * Firma di uno scope trovato nello script (funzione od, if, while, for, ecc).
 *
 * name        -> nome/tipo dello scope ("od", "if", "while", nome funzione, ecc)
 * start_line  -> riga (indice 0-based in script[]) di apertura scope
 * end_line    -> riga di chiusura scope (-1 se non trovata, graffe non bilanciate)
 * is_function -> 1 se lo scope è una funzione (od o __start), 0 altrimenti
 * args        -> array di argomenti (solo per funzioni od, vuoto altrimenti)
 * arg_count   -> numero di argomenti effettivamente presenti in args
 * arg_capacity-> capacità allocata di args (uso interno, per il resize)
 */
typedef struct {
    char *name;
    int start_line;
    int end_line;
    int is_function;
    char **args;
    size_t arg_count;
    size_t arg_capacity;
} sign_of_scope;

extern sign_of_scope *scope_table;
extern size_t scope_count;

/* Analizza script[] (già caricato da load_script) e popola scope_table. */
void build_scope_signatures(void);

/* Libera tutta la memoria allocata da build_scope_signatures. */
void free_scope_signatures(void);

#endif