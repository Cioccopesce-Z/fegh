#include <stdio.h>
#include "mem.h"

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
size_t byte_for_vleng = 2;
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

    printf("methodlist of belonging %lu-59 %lu-55 %lu-55 %lu-55\n",
                get_methodlist_of_variable_from_address(idx),
                get_methodlist_of_variable_from_address(ix),
                get_methodlist_of_variable_from_address(id),
                get_methodlist_of_variable_from_address(ids));


    printf("\n");
    printf("address direct value <%lu-25>\n",get_value_of_address(1));
    printf("\n");


    printf("\n");
    printf("crs position; <%lu>\n",memory_cursor);
    printf("\n");

    for (size_t i = 0; i < to_declare; i++){
        printf("line(%zu) %ld: %u\n",i, (long)(memory + i), memory[i]);
    }

}