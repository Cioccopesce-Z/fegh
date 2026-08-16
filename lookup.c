#include "lookup.h"
#include "sintax_keyword.h"

FILE *open_lookup_table(void){
    return fopen(TABLE_FILENAME, "w");
}

void write_scope_header_to_lookup_table(FILE *fp, size_t scope_index, sign_of_scope *scope){

    //salvo metadata base per lo scope come nome/st/en/func?
    fprintf(fp,"scope %lu %s %d %d %s\n",scope_index,scope->name,
                            scope->start_line, scope->end_line,
                            scope->is_function ? "si" : "no");
}

void write_variable_to_lookup_table(FILE *fp,size_t idx, var_data_struct *v){

    //aggiungo i metadati della variabile appartenente allo scope al file
    fprintf(fp,"    @%lu %s %lu-", idx, v->name, v->dimension);

    //ci concateno a ciascuna variabile la repetition e il rank
    for(size_t repetition = 0; repetition < v->rank; repetition++){
        fprintf(fp,"%lu ",v->repetition[repetition]);
    }
    fprintf(fp, "\n");
}