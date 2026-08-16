#include "free.h"

void free_memory_buffer(void){
    free(memory);
    memory = NULL;
    memory_cursor = 0;
}

void free_var_table(void){
    for(size_t i = 0; i < var_table_count; i++){
        free(var_table[i].name);
        free(var_table[i].repetition);
    }
    free(var_table);
    var_table = NULL;
    var_table_count = 0;
    var_table_capacity = 0;
}

void free_scope_memory_fingerprints(void){
    for(size_t i = 0; i < scope_count; i++)
        free(scope_memory[i]);
    free(scope_memory);
    free(scope_dim);
    scope_memory = NULL;
    scope_dim = NULL;
}

void free_all_fegh_resources(void){
    free_scope_memory_fingerprints();  /* PRIMA: usa scope_count */
    free_var_table();
    free_scope_signatures();           /* azzera scope_table/scope_count */
    free_script();
    free_memory_buffer();
}
