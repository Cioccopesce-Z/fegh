#include <stddef.h>
#include <stdio.h>
#include "mem.h"
#include "loader.h"
#include "memory_parser.h"

//nonn possono essere dichiarate variabili a runtime, ma hanno un comportamento tipo static, dichiarate una volta sola
// e poi riassegnate



/* memoria vera e propria: allocata da begin(), dichiarata qui perche'
   e' il file-main che decide dimensioni e vive per tutta l'esecuzione. */
__uint8_t *memory;
size_t memory_cursor = 0;

/* configurazione della struttura: modificabile qui prima di begin(). */
size_t to_declare = 35;
size_t byte_for_lenght_of_the_lenght_of_the_scope = 1;
size_t byte_for_scope_code = 1;

size_t byte_for_scope = 0;
size_t byte_for_dim = 1;
size_t byte_for_vleng = 1;
size_t byte_for_method_lenght = 1;

int main(){

    begin(to_declare);

    printf("%p\n",memory);
    printf("\n");

    set_scope_start_end(tru,auto,fal);

    int idx = initialize_variable(tru,main_scope,fal,auto,auto,28,tru,59);
    update_value_of_variable_from_address(idx, 87);

    int ix = initialize_variable(tru,main_scope,fal,auto,auto,30,tru,55);
    update_value_of_variable_from_address(ix, 32);
    int id = initialize_variable(tru,main_scope,fal,auto,auto,31,tru,55);
    int ids = initialize_variable(tru,main_scope,fal,auto,auto,78,tru,55);

    set_scope_start_end(fal,auto,fal);

    printf("current value %lu-87 %lu-32 %lu-31 %lu-78\n\n",get_value_of_variable(idx),
    get_value_of_variable(ix),
    get_value_of_variable(id),
    get_value_of_variable(ids));

    printf("methodlist of belonging %lu-59 %lu-55 %lu-55 %lu-55\n\n",
    get_methodlist_of_variable_from_address(idx),
    get_methodlist_of_variable_from_address(ix),
    get_methodlist_of_variable_from_address(id),
    get_methodlist_of_variable_from_address(ids));

    printf("direct end index of variable struct in byte %lu-7 %lu-12 %lu-17 %lu-22\n\n",
    get_variable_struct_end_index_from_address(idx),
    get_variable_struct_end_index_from_address(ix),
    get_variable_struct_end_index_from_address(id),
    get_variable_struct_end_index_from_address(ids));

    printf("direct lenght in byte of var struct %lu-5 %lu-5 %lu-5 %lu-5\n\n",
    get_direct_lenght_in_byte_of_variable_struct(idx),
    get_direct_lenght_in_byte_of_variable_struct(ix),
    get_direct_lenght_in_byte_of_variable_struct(id),
    get_direct_lenght_in_byte_of_variable_struct(ids));
    

    printf("\n");
    printf("address direct value <%lu-21>\n",get_value_of_address(1));
    printf("\n");

    printf("\n");
    printf("crs position; <%lu>\n",memory_cursor);
    printf("\n");

    for (size_t i = 0; i < to_declare; i++){
        printf("line(%zu) %ld: %u\n",i, (long)(memory + i), memory[i]);
    }



    /*SCRIPT ANALIZATION PART*/

    printf("\n\n");

    load_script("test.fgh");

    printscript();

    build_scope_signatures();

    printf("Scope trovati: %zu\n\n", scope_count);

    for(size_t i = 0; i < scope_count; i++){

        printf("----------------------------------------\n");
        printf("Nome      : %s\n", scope_table[i].name);
        printf("Funzione  : %s\n", scope_table[i].is_function ? "SI" : "NO");
        printf("Inizio    : %d\n", scope_table[i].start_line);
        printf("Fine      : %d\n", scope_table[i].end_line);

        if(scope_table[i].arg_count){
            printf("Argomenti:\n");
            for(size_t j = 0; j < scope_table[i].arg_count; j++)
                printf("   %s\n", scope_table[i].args[j]);
        }

        printf("\n");
    }

    printf("\n");
    printf("\n");

    for(size_t i = 0; i < scope_count; i++){

        printf("\n\n%lu' scope \n",i);

        create_metadata_for_var_struct_in_a_scope(scope_table[i].start_line, scope_table[i].end_line);
    }

    stampa_contenuto_var_table();

    free_scope_signatures();

    free_script();

    /*IT IS STILL MEMORY MANAGEMENTE BUT MORE IN A READING WAY*/
    size_t bs[] ={3,3}; //repetition getted from mem_parse.c  in this case this is = to [3][3]
    size_t bss[] = {0, 8}; //getted form the script or where you are trying to access
                                  //in this system [2][2] = [0][8] in both you access the last cells 

    printf("%lu 42\n",resolve_array_index_from_normal_sintax(2,bs,
                                            bss,2)
                                        );
}