#include "sintax_keyword.h"
#include "memory_parser.h"
#include "loader.h"
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

var_data_struct *var_table = NULL;
size_t var_table_count = 0;
size_t var_table_capacity = 0;




/* aggiunge una var_data_struct alla tabella globale, con realloc a raddoppio */
static void push_var(var_data_struct v){
    if (var_table_count == var_table_capacity) {
        var_table_capacity = var_table_capacity ? var_table_capacity * 2 : 8;
        var_data_struct *tmp = realloc(var_table, var_table_capacity * sizeof(*var_table));
        if (!tmp) { fprintf(stderr, "OOM push_var\n"); exit(1); }
        var_table = tmp;
    }
    var_table[var_table_count++] = v;
}

/*
 * parse_let - riceve i token GIA' splittati sul ":" (li splitta e li passa
 * return_dimension_in_byte_of_var_struct, vedi sotto).
 *
 * Layout dei token per una dichiarazione "DIVIDER_KEY nome:dim:[rip...]:method:value":
 *   tokens[0]              -> nome
 *   tokens[1]               -> dimensione in byte di UNA cella
 *   tokens[2..count-3]     -> ripetizioni (0 o piu': array/matrice/tensore)
 *   tokens[count-2]        -> method_list_code
 *   tokens[count-1]        -> value
 *
 * Esempi (vedi walkthrough sopra per i conti):
 *   arr:1:2:8:55   -> dim=1, repetition=[2],   method=8, value=55
 *   matr:1:2:4:7:99 -> dim=1, repetition=[2,4], method=7, value=99
 */
void parse_let(char **tokens, size_t token_count){

    if (token_count < 3) {
        fprintf(stderr, "SYSTEM ERROR: parse_let - dichiarazione incompleta (%zu token)\n", token_count);
        exit(0);
    }

    var_data_struct v = {0};

    /* nome: copia perche' tokens[0] punta dentro la stringa originale,
       che strtok ha modificato e che potrebbe essere liberata/riusata dopo */
    v.name = malloc(strlen(tokens[0]) + 1);
    if (!v.name) { fprintf(stderr, "OOM parse_let name\n"); exit(1); }
    strcpy(v.name, tokens[0]+sizeof(DECLARE_KEY)-1);

    v.method_list_code = strtoull(tokens[token_count - 2], NULL, 10);
    v.value             = strtoull(tokens[token_count - 1], NULL, 10);

    /* mid_count = quanti token stanno tra il nome e (method,value)          */
    /* include SEMPRE la dimensione (tokens[1]) + eventuali ripetizioni      */
    size_t mid_count = token_count - 3;

    if (mid_count == 0) {
        fprintf(stderr, "SYSTEM ERROR: parse_let - manca la dimensione in byte per '%s'\n", v.name);
        free(v.name);
        exit(0);
    }

    v.dimension = strtoull(tokens[1], NULL, 10);   /* sempre il primo dopo il nome */
    v.rank = mid_count - 1;                          /* tutto il resto e' repetition */

    v.repetition = v.rank ? malloc(v.rank * sizeof(size_t)) : NULL;
    if (v.rank && !v.repetition) { fprintf(stderr, "OOM parse_let repetition\n"); exit(1); }

    for (size_t k = 0; k < v.rank; k++)
        v.repetition[k] = strtoull(tokens[2 + k], NULL, 10);

    push_var(v);
}

/*
 * return_dimension_in_byte_of_var_struct - collezionatore di token.
 * Spezza la stringa su ":" con strtok (come faceva l'originale), accumula
 * i pezzi in un array dinamico (buffer con realloc a raddoppio, come volevi
 * tu), e SOLO alla fine chiama parse_let() una volta con tutti i token.
 * Prima invece parse_let veniva chiamata dentro il while, un token alla
 * volta -> impossibile capire chi fosse il penultimo/ultimo senza aver
 * gia' visto tutta la stringa.
 */
size_t return_dimension_in_byte_of_var_struct(char *start_to_parse_let_instruction_position){

    if (start_to_parse_let_instruction_position == NULL) {
        fprintf(stderr, "SYSTEM ERROR: return_dimension_in_byte_of_var_struct chiamata con argomento NULL, ABORT\n");
        exit(0);
    }

    char **tokens = NULL;
    size_t count = 0, cap = 0;

    char *tok = strtok(start_to_parse_let_instruction_position, ":");
    while (tok) {
        if (count == cap) {
            cap = cap ? cap * 2 : 4;
            char **tmp = realloc(tokens, cap * sizeof(char*));
            if (!tmp) { fprintf(stderr, "OOM tokenize\n"); exit(1); }
            tokens = tmp;
        }
        tokens[count++] = tok;
        tok = strtok(NULL, ":");
    }

    parse_let(tokens, count);
    free(tokens);   /* libera solo l'array di puntatori, non le stringhe:
                        quelle vivono dentro il buffer originale, e il nome
                        e' gia' stato copiato in v.name dentro parse_let */
    return 0;
}

/*
 * stampa_var_table - dump ordinato di tutta la var_table per debug.
 */
void stampa_var_table(void){
    printf("\n=== VAR TABLE (%zu variabili) ===\n", var_table_count);
    for (size_t i = 0; i < var_table_count; i++) {
        var_data_struct *v = &var_table[i];
        printf("[%zu] nome=%-15s dim=%zu byte  method=%zu  value=%zu  rank=%zu  repetition=[",
               i, v->name, v->dimension, v->method_list_code, v->value, v->rank);
        for (size_t d = 0; d < v->rank; d++) {
            printf("%zu", v->repetition[d]);
            if (d + 1 < v->rank) printf(",");
        }
        printf("]\n");
    }
    printf("=== fine dump ===\n\n");
}

/*
 * calcola_offset_indicizzazione - la "parte di indicizzazione" che
 * chiedevi, separata dall'indirizzo base (XA).
 *
 * Row-major puro: nessun magic number, "quattro" non e' mai scritto a
 * mano, e' sempre letto da v->repetition[d]. Con rank=1 collassa
 * esattamente al caso array (offset = indici[0]). Con rank=2 (matr[2][4])
 * diventa offset = indici[0]*4 + indici[1], dove il "4" e' v->repetition[1]
 * preso dalla struct, non un letterale.
 */
size_t calcola_offset_indicizzazione(var_data_struct *v, size_t *indici){
    size_t offset = 0;
    for (size_t d = 0; d < v->rank; d++)
        offset = offset * v->repetition[d] + indici[d];
    return offset;
}

/*
 * indirizzo_cella - indirizzo finale di una cella.
 *   XA        = indirizzo dell'array/struct principale (base address)
 *   dollaro($) = v->dimension, costante fissa: la dimensione in byte di
 *                UNA singola variabile della struct (dichiarare una
 *                matrice = dichiarare N variabili tutte uguali)
 */
size_t indirizzo_cella(size_t XA, var_data_struct *v, size_t *indici){
    return XA + calcola_offset_indicizzazione(v, indici) * v->dimension;
}


__uintmax_t create_metadata_for_var_struct_in_a_scope(size_t base_line_in_script, size_t end_line_in_script){

    for (size_t i = base_line_in_script; i < end_line_in_script; i++) {

        for (size_t s = 0; s < scope_count; s++) {
            if (scope_table[s].start_line == i &&
                scope_table[s].start_line != base_line_in_script) {
                i = scope_table[s].end_line;
                break;
            }
        }

        char *let_position = NULL;

        if ((let_position = strstr(script[i], "let")))
            printf("\nline with DECLARATION_KEY init [%lu]: %s\n", i, script[i]);
        else
            continue;

        printf("DECLARATION_KEY - %c%c%c\n", *let_position, *(let_position+1), *(let_position+2));

        size_t function_index = line_belong_to_any_function_declaration(i);

        if (function_index != (size_t)-1) {
            for (size_t j = 0; j < scope_table[function_index].arg_count; j++)
                return_dimension_in_byte_of_var_struct(scope_table[function_index].args[j]);
        } else {
            return_dimension_in_byte_of_var_struct(let_position);
        }
    }

    return -1;
}

/* stampa ricorsiva degli indici e dei valori usata da un'altra stampa non consigliato*/
static void stampa_var_recursive(
    var_data_struct *v,
    size_t livello,
    size_t *indici,
    size_t *counter
){
    if (livello == v->rank) {

        size_t offset = calcola_offset_indicizzazione(v, indici);
        size_t byte_offset = offset * v->dimension;

        printf("    ");

        if (v->rank == 0) {
            printf("%s = %zu", v->name, v->value);
        }
        else {
            printf("%s[", v->name);

            for (size_t i = 0; i < v->rank; i++) {
                printf("%zu", indici[i]);
                if (i + 1 < v->rank)
                    printf(",");
            }

            printf("] = %zu", v->value);
        }

        printf("  (offset=%zu celle, byte=%zu)\n",
               offset,
               byte_offset);

        (*counter)++;
        return;
    }


    for (size_t i = 0; i < v->repetition[livello]; i++) {
        indici[livello] = i;

        stampa_var_recursive(
            v,
            livello + 1,
            indici,
            counter
        );
    }
}

/*stampa per debug*/
void stampa_contenuto_var_table(void)
{
    printf("\n========== CONTENUTO VAR TABLE ==========\n");

    for (size_t i = 0; i < var_table_count; i++) {

        var_data_struct *v = &var_table[i];

        printf("\n[%zu] %s\n", i, v->name);

        printf(" dimensione cella : %zu byte\n", v->dimension);
        printf(" method           : %zu\n", v->method_list_code);
        printf(" value base       : %zu\n", v->value);
        printf(" rank             : %zu\n", v->rank);


        if (v->rank > 0) {
            printf(" dimensioni       : [");

            for (size_t d = 0; d < v->rank; d++) {
                printf("%zu", v->repetition[d]);

                if (d + 1 < v->rank)
                    printf(",");
            }

            printf("]\n");
        }
        else {
            printf(" dimensioni       : scalare\n");
        }


        size_t counter = 0;


        if (v->rank == 0) {

            printf("    %s = %zu (offset=0 celle, byte=0)\n",
                   v->name,
                   v->value);

        }
        else {

            size_t *indici = malloc(sizeof(size_t) * v->rank);

            if (!indici) {
                fprintf(stderr,
                        "OOM stampa_contenuto_var_table\n");
                exit(1);
            }


            stampa_var_recursive(
                v,
                0,
                indici,
                &counter
            );


            free(indici);
        }


        printf(" totale celle: %zu\n", 
               v->rank ? counter : 1);
    }


    printf("\n========== FINE VAR TABLE ==========\n\n");
}