#include "memory_parser.h"
#include "loader.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


/*charateristic sta per tutti i numeri che vengono dopo il nome e prima di = in modo da poterli interpretare come
dimesnione in byte, repetion, repetition, finche is_last non e' vero che allora l'ultimo inviato
diventa la lista dei metodi appartenente, se non viene assegnato allora si non si includono metodi*/
size_t parse_let(char *name, int is_last){

    return 0;
}


size_t return_dimension_in_byte_of_var_struct(char *start_to_parse_let_instruction_position){


    if(start_to_parse_let_instruction_position!= NULL){
        
        char *tok = strtok(start_to_parse_let_instruction_position, ":");
        
        while(tok){
            printf("%s  ",tok);
            parse_let(tok,false);
            tok = strtok(NULL, ":");
        }
        printf("\n");

    }
    else{
        printf("SYSTEM ERROR: return_dimension_in_byte_of_var_struct has been called with a null argument, ABORT\n");
        exit(0);
    }
    return 0;

}

__uintmax_t calcuate_dimesion_in_byte_for_a_scope_not_declaring(size_t base_line_in_script, size_t end_line_in_script){


    for(size_t i = base_line_in_script; i < end_line_in_script; i++){

        for (size_t s = 0; s < scope_count; s++) {

            if (scope_table[s].start_line == i &&
                scope_table[s].start_line != base_line_in_script) {

                i = scope_table[s].end_line;
                break;
            }
        }

        char *let_position = NULL;

        if((let_position = strstr(script[i], "let"))) 
        
                    printf("\nline with let init [%lu]: %s\n",i,script[i]);
        
        else{
            continue;
        }
        
            


        

        size_t dimension_in_byte_of_var_struct = 0;

        printf("let - %c%c%c\n",*let_position,*(let_position+1),*(let_position+2)); //test for alligment

        /*check se parsare gli argomenti di una funzione in base se contengono o no let*/

        size_t function_index = line_belong_to_any_function_declaration(i);

        if(  function_index != -1 ){

            for (size_t j = 0; j < scope_table[function_index].arg_count; j++) {
                
                return_dimension_in_byte_of_var_struct(scope_table[function_index].args[j]);
            }

        }
        else 
            return_dimension_in_byte_of_var_struct(let_position);

        
    }

    return -1;
}