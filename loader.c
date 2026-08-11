#include "sintax_keyword.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "loader.h"

char **script = NULL;
char **dirt_script_used_for_debugging_by_user;
size_t script_size = 0;

sign_of_scope *scope_table = NULL;
size_t scope_count = 0;
static size_t scope_capacity = 0;

int starts_with(const char *str, const char *prefix) {
    return strncmp(str, prefix, strlen(prefix)) == 0;
}

/* ============================ LOADING ============================ */

static char *clean_line(char *line){
    char *out = malloc(1);
    size_t len = 0;

    while (*line == ' ' || *line == '\t')
        line++;

    while (*line){
        if (*line != '\n' && *line != '\r' && *line != '\t'){
            out = realloc(out, len + 2);
            out[len++] = *line;
        }
        line++;
    }

    while (len && out[len - 1] == ' ')
        len--;

    

    out[len] = '\0';

    remove_spaces(out);

    return out;
}

void load_script(const char *filename){
    FILE *fp = fopen(filename, "r");
    if (!fp)
        return;

    char *line = NULL;
    size_t len = 0;
    int c;

    while ((c = fgetc(fp)) != EOF) {

        if (c == '{' || c == '}') {

            line = realloc(line, len + 2);
            line[len++] = (char)c;
            line[len] = '\0';

            char *clean = clean_line(line);

            if (*clean) {
                script = realloc(script, sizeof(char*) * (script_size + 1));
                script[script_size++] = clean;
            } else {
                free(clean);
            }

            free(line);
            line = NULL;
            len = 0;

            continue;
        }

        if (c == ';' || c == '\n') {

            if (line) {
                line[len] = '\0';

                char *clean = clean_line(line);

                if (*clean) {
                    script = realloc(script, sizeof(char*) * (script_size + 1));
                    script[script_size++] = clean;
                } else {
                    free(clean);
                }

                free(line);
                line = NULL;
                len = 0;
            }

            continue;
        }

        line = realloc(line, len + 2);
        line[len++] = (char)c;
        line[len] = '\0';
    }

    if (line){
        char *clean = clean_line(line);
        if (*clean){
            script = realloc(script, sizeof(char *) * (script_size + 1));
            script[script_size++] = clean;
        }
        else
            free(clean);

        free(line);
    }

    fclose(fp);
}

void free_script(void){
    for (size_t i = 0; i < script_size; i++)
        free(script[i]);

    free(script);
    script = NULL;
    script_size = 0;
}



/* ======================== SCOPE SIGNATURES ======================== */

/*
* remove_spaces
* -------------
* Rimuove OGNI spazio/tab dalla stringa 's', in place, ovunque si trovino.
* clean_line() ripulisce solo i bordi della riga, non gli spazi interni
* (es "let numero_uno: 1" o "od  somma(" arrivano qui intatti), quindi
* il parsing di nomi/argomenti non deve mai fare affidamento sugli spazi.
*/
static void remove_spaces(char *s){
    char *w = s;
    int in_string = 0;

    for (char *r = s; *r; r++) {

        if (*r == '"') {
            in_string = !in_string;
            *w++ = *r;
            continue;
        }

        if (in_string || (*r != ' ' && *r != '\t'))
            *w++ = *r;
    }

    *w = '\0';
}

/*
* find_scope_end
* --------------
* Dato l'indice di riga in cui si apre uno scope (contiene almeno una '{'),
* scandisce in avanti contando graffe aperte/chiuse finché la profondità
* torna a 0.
*
* start -> riga di apertura dello scope
* return-> riga (index) in cui lo scope si chiude, -1 se non trovata
*/
static int find_scope_end(size_t start){
    int depth = 0;
    for(size_t i = start; i < script_size; i++){
        if(strchr(script[i], '{'))
            depth++;
        if(strchr(script[i], '}')){
            depth--;
            if(depth == 0)
                return (int)i;
        }
    }
    return -1;
}

/*
* add_argument
* ------------
* Aggiunge un argomento (nome + dimensione in byte) alla lista args di uno
* scope, espandendo l'array se serve (raddoppio della capacità).
*
* scope -> scope a cui aggiungere l'argomento
* name  -> nome dell'argomento, già ripulito da spazi (viene duplicato)
* size  -> dimensione in byte dell'argomento
*/
static void add_argument(sign_of_scope *scope, const char *text){
    if(scope->arg_count == scope->arg_capacity){
        scope->arg_capacity = scope->arg_capacity ? scope->arg_capacity * 2 : 8;
        scope->args = realloc(scope->args,
                            scope->arg_capacity * sizeof(char));
    }
    scope->args[scope->arg_count] = strdup(text);
    scope->arg_count++;
}

/*
* parse_single_argument
* ----------------------
* Interpreta un singolo token argomento nella forma "let <nome>: <dimensione>",
* ignorando qualunque spaziatura al suo interno, e lo aggiunge allo scope.
*
* token -> testo del singolo argomento, già isolato dallo split su 'DIVIDER_KEY'
* scope -> scope a cui aggiungere l'argomento risultante
*/
static void parse_single_argument(char *token, sign_of_scope *scope){
    remove_spaces(token);
    if(*token == '\0')
        return;

    add_argument(scope, token);
}

/*
* parse_arguments
* ---------------
* Estrae la lista argomenti dalla riga di dichiarazione di una funzione,
* cioè il contenuto tra '(' e ')', e la spezza sul separatore 'DIVIDER_KEY'.
* Ogni pezzo viene passato a parse_single_argument.
*
* line  -> riga sorgente contenente "od nome( ... ){"
* scope -> scope funzione da riempire con gli argomenti trovati
*/
static void parse_arguments(const char *line, sign_of_scope *scope){
    const char *begin = strchr(line, '(');
    if(!begin)
        return;
    begin++;
    const char *end = strchr(begin, ')');
    if(!end)
        return;

    char *buffer = malloc(end - begin + 1);
    memcpy(buffer, begin, end - begin);
    buffer[end - begin] = '\0';

    char *tok = strtok(buffer, DIVIDER_KEY);
    while(tok){
        parse_single_argument(tok, scope);
        tok = strtok(NULL, DIVIDER_KEY);
    }

    free(buffer);
}

size_t line_belong_to_any_function_declaration(size_t line_absolute_index_of_the_line){

    for(size_t i = 0; i < scope_count; i++){
        if(line_absolute_index_of_the_line == scope_table[i].start_line) return i;
    }
    return -1;
    
}

/*
* parse_scope
* -----------
* Riconosce il tipo di scope che inizia alla riga 'line' e ne popola
* la firma (nome, is_function, argomenti se funzione). Il riconoscimento
* non fa mai affidamento sul numero di spazi presenti nella riga.
*
* line  -> riga di apertura dello scope
* scope -> struct da inizializzare (viene azzerata all'inizio)
*/
static void parse_scope(size_t line, sign_of_scope *scope){
    memset(scope, 0, sizeof(*scope));
    scope->start_line = (int)line;
    scope->end_line = find_scope_end(line);

    const char *l = script[line];

    if (starts_with(l, "scope")) {

        const char *p = l + 5;

        char name[512];
        int i = 0;

        while (*p && *p != '(')
            name[i++] = *p++;

        name[i] = '\0';

        scope->name = strdup(name);
        scope->is_function = 1;
        parse_arguments(l, scope);
        return;
    }
    if(starts_with(l, ELSE_IF_KEY))
        scope->name = strdup(ELSE_IF_KEY);
    else if(starts_with(l, IF_KEY))
        scope->name = strdup(IF_KEY);
    else if(starts_with(l, "while"))
        scope->name = strdup("while");
    else if(starts_with(l, "during"))
        scope->name = strdup("during");
    else if(starts_with(l, "for"))
        scope->name = strdup("for");
    else if(starts_with(l, ELSE_KEY))
        scope->name = strdup(ELSE_KEY);
    else if(starts_with(l, MACRO_INDENT_KEY))
        scope->name = strdup(MACRO_INDENT_KEY);
    else if(starts_with(l, "C"))
        scope->name = strdup("C");
    else
        scope->name = strdup("unknown");

    scope->is_function = 0;

    if(strchr(l, '('))
        parse_arguments(l, scope);
}

void printscript(){
    for(size_t i = 0; i < script_size; i++){

        printf("[%lu]: %s \n",i,script[i]);

    }

    printf("\n");
}

/*
* build_scope_signatures
* -----------------------
* Analizza tutto script[] (popolato da load_script) e costruisce
* scope_table: una firma per ogni riga che apre uno scope ('{').
* Da chiamare dopo load_script() e prima di usare scope_table.
*/
void build_scope_signatures(void){
    scope_count = 0;
    for(size_t i = 0; i < script_size; i++){
        if(!strchr(script[i], '{'))
            continue;
        if(scope_count == scope_capacity){
            scope_capacity = scope_capacity ? scope_capacity * 2 : 8;
            scope_table = realloc(scope_table,
                                scope_capacity * sizeof(sign_of_scope));
        }
        parse_scope(i, &scope_table[scope_count]);
        scope_count++;
    }
}

/*
* free_scope_signatures
* ----------------------
* Libera tutta la memoria allocata per scope_table (nomi, argomenti,
* array argomenti, tabella stessa) e resetta i contatori.
*/
void free_scope_signatures(void){
    for(size_t i = 0; i < scope_count; i++){
        free(scope_table[i].name);
        for(size_t j = 0; j < scope_table[i].arg_count; j++)
            free(scope_table[i].args[j]);
        free(scope_table[i].args);
    }
    free(scope_table);
    scope_table = NULL;
    scope_count = 0;
    scope_capacity = 0;
}