#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "mem.h"

void begin(size_t byte_to_allocate){

    memory = malloc(byte_to_allocate);
    memset(memory,0,byte_to_allocate);

}

size_t bytes_needed(__uintmax_t value){
    size_t n = 1;
    while (value >> (8 * n)) n++;
    return n;
}

// FUNCTION TO READ

size_t get_lenght_in_byte_of_value_from_address(size_t address){

    size_t v_lenght_in_byte = 0;

    for (size_t i = 0; i < byte_for_vleng; i++) {
        v_lenght_in_byte |= (size_t)memory[address + byte_for_scope + byte_for_dim +  i] << (8 * i);
    }

    return v_lenght_in_byte;

}

size_t get_scope_of_variable_from_address(size_t address){

    size_t scope = 0;

    for (size_t i = 0; i < byte_for_scope; i++) {
        scope |= (size_t)memory[address + i] << (8 * i);
    }

    return scope;

}

size_t get_value_of_variable(size_t address){

    size_t lenght_of_value = get_lenght_in_byte_of_value_from_address(address);
    if(debug) printf("get lenght of value in byte already written: %lu\n", lenght_of_value);

    size_t value_start = address + byte_for_scope + byte_for_dim + byte_for_vleng;

    size_t value = 0;
    for (size_t k = 0; k < lenght_of_value; k++) {
        value |= (size_t)memory[value_start + k] << (8 * k);
    }

    return value;
}

size_t get_variable_struct_dimension_from_address(size_t address){

    size_t struct_dim = 0;

    for (size_t i = 0; i < byte_for_dim; i++) {
        struct_dim |= (size_t)memory[address + byte_for_scope + i] 
                                                            << (8 * i);
    }

    return struct_dim;

}

size_t get_methodlist_lenght_in_byte_from_address(size_t address){
    if(address >= to_declare){
        printf("errore: indirizzo %lu fuori dai limiti (%zu) in get_methodlist_lenght_in_byte_from_address\n", address, to_declare);
        return 0;
    }
    size_t lenght_of_value = get_lenght_in_byte_of_value_from_address(address);
    size_t value_end = address + byte_for_scope + byte_for_dim + byte_for_vleng + lenght_of_value;

    if(get_variable_struct_dimension_from_address(address) <= value_end){
        return 0; // nessun metodo presente, non un indirizzo
    }

    size_t lenght_in_byte_of_method_lenght = 0;
    for (size_t k = 0; k < byte_for_method_lenght; k++) {
        lenght_in_byte_of_method_lenght |= (size_t)memory[value_end + k] << (8 * k);
    }
    return lenght_in_byte_of_method_lenght;
}

size_t get_methodlist_of_variable_from_address(size_t address){

    size_t method_list_lenght_in_byte = get_methodlist_lenght_in_byte_from_address(address);
    size_t lenght_of_value = get_lenght_in_byte_of_value_from_address(address);

    size_t method_start = address + byte_for_scope + byte_for_dim + byte_for_vleng
                                + lenght_of_value + byte_for_method_lenght;

    size_t methodlist_code = 0;
    for (size_t k = 0; k < method_list_lenght_in_byte; k++) {
        methodlist_code |= (size_t)memory[method_start + k] << (8 * k);
    }

    return methodlist_code;
}


size_t get_value_of_address(size_t address){
    return (size_t)memory[address];
}

//FUNCTION TO WRITE

/* calcola la lenght in byte finale del value.
   se esiste gia' una lenght per questo slot, resta quella (fissata alla
   dichiarazione): errore se il nuovo value non ci sta.
   altrimenti: se richiesta = 0 -> autodimensionata su value,
   errore se richiesta < byte necessari per value. */
size_t resolve_value_lenght(size_t existing_lenght, size_t requested_lenght, __uintmax_t value){
    size_t needed = bytes_needed(value);

    if(existing_lenght > 0){
        if(needed > existing_lenght){
            printf("error: il valore %lu richiede %lu byte, ma la variabile ne ha solo %lu\n",
                    (size_t)value, needed, existing_lenght);
            return 0;
        }
        return existing_lenght;
    }

    if(requested_lenght == 0) requested_lenght = needed;

    if(requested_lenght < needed){
        printf("error: the requested lenght of: %lubyte is to little to fit the value: %lu \nare needed at least: %lubyte \n\n",
                requested_lenght, value, needed);
        return 0;
    }

    return requested_lenght;
}

size_t resolve_method_lenght(size_t existing_lenght, size_t method_address){
    size_t needed = bytes_needed(method_address);

    if(existing_lenght > 0){
        if(needed > existing_lenght){
            printf("error: il metodo %lu richiede %lu byte, ma sono gia' stati allocati solo %lu\n",
                    method_address, needed, existing_lenght);
            return 0;
        }
        return existing_lenght;
    }

    if(needed > byte_for_method_lenght){
        printf("errore: l'indirizzo del metodo (%lu) richiede %lu byte, ma ne sono disponibili solo %zu\n",
                    method_address, needed, byte_for_method_lenght);
        return 0;
    }

    return needed;
}

void write_value(size_t start, size_t lenght_in_byte, __uintmax_t value){
    for(size_t i = 0; i < byte_for_vleng; i++){
        memory[start + byte_for_scope + byte_for_dim + i] = (__uint8_t)(lenght_in_byte >> (8 * i));
    }

    size_t value_start = start + byte_for_scope + byte_for_dim + byte_for_vleng;
    for(size_t k = 0; k < lenght_in_byte; k++){
        memory[value_start + k] = (__uint8_t)(value >> (8 * k));
    }
}

void write_methodlist(size_t record_end, size_t method_lenght, size_t method_address){
    for(size_t i = 0; i < byte_for_method_lenght; i++){
        memory[record_end + i] = (__uint8_t)(method_lenght >> (8 * i));
    }
    for(size_t i = 0; i < method_lenght; i++){
        memory[record_end + byte_for_method_lenght + i] = (__uint8_t)(method_address >> (8 * i));
    }
}

/*con l'utilizzo di address si possono andare a modificare i valori dei metadati di una
variabile nello specifico altrimenti con address = fal si incoda la variabile da dichiarare
al primo indirizzo disponibile ritornato dalla funzione stessa
....................................................................
se lenght_in_byte è = 0 allora viene assegnata autonomamente la dimensione
mentre viene specificato con use_method se ci sono da aggiungere metodi alla variabile.
nota: sia la dimensione del value che quella del method vengono fissate alla
prima dichiarazione e non sono piu' ridimensionabili in seguito.*/
size_t initialize_variable(int use_scope, size_t scope_address,
                            int use_address, size_t address_offset,  
                            size_t lenght_in_byte_of_value, __uintmax_t value,
                            int use_method, size_t method_address){

    static size_t backup_end = 0;

    size_t start = use_address ? address_offset : memory_cursor;
    if(use_address) backup_end = memory_cursor;

    printf("%lu starting from line\n", start);

    if(start >= to_declare){
        printf("error: cannot declare other variable risk of overflow\ntry to increase the capacity of %zu\n", to_declare);
        printf("normally you shouldnt encounter this error since the declaration of the scope is automated to fit all the variable\n\n");
        return start;
    }

    size_t old_v_lenght = get_lenght_in_byte_of_value_from_address(start);
    size_t existing_method_lenght = get_methodlist_lenght_in_byte_from_address(start);

    size_t new_v_lenght = resolve_value_lenght(old_v_lenght, lenght_in_byte_of_value, value);
    if(new_v_lenght == 0) return start; // errore gia' stampato

    if(start + byte_for_scope + byte_for_dim + new_v_lenght >= to_declare){
        printf("error: cannot declare other variable risk of overflow\n"
            "try to increase the capacity of %zu\n", to_declare);
        return start;
    }
    printf("%lubyte for \"%lu\" numero di celle value da utilizzare\n", new_v_lenght, value);

    write_value(start, new_v_lenght, value);

    size_t record_end = start + byte_for_scope + byte_for_dim + byte_for_vleng + new_v_lenght;

    if(!use_method && existing_method_lenght){
        // preservo il metodo gia' presente
        record_end += byte_for_method_lenght + existing_method_lenght;
        memory_cursor = use_address ? backup_end : record_end;
        printf("line to %lu continue from(no method, existing method preserved)\n\n", record_end);
    }
    else{
        size_t method_lenght = resolve_method_lenght(existing_method_lenght, method_address);
        if(method_lenght == 0) return start; // errore gia' stampato

        printf("%lubyte richiesti per salvare il codice del metodo \n", method_lenght);

        write_methodlist(record_end, method_lenght, method_address);

        record_end += byte_for_method_lenght + method_lenght;
        memory_cursor = use_address ? backup_end : record_end;
        printf("%lu continue from address(yes method)\n\n", record_end);
    }

    // risolvo dim — SEMPRE con record_end, mai col cursore globale
    for(size_t i = 0; i < byte_for_dim; i++){
        memory[start + byte_for_scope + i] = (__uint8_t)(record_end >> (8 * i));
    }

    return start;
}

void set_scope_start_end(int is_start, int auto_set_code_for_scope, int scope_code){
    static size_t scope_start = 0;
    static size_t length_address = 0;
    static int scope_counter = 0;

    if (auto_set_code_for_scope)
        scope_code = scope_counter++;
    else
        scope_code++;

    if (is_start){

        scope_start = memory_cursor;

        for (size_t i = 0; i < byte_for_scope_code; i++){

            memory[memory_cursor + i] =
                (__uint8_t)(scope_code >> (8 * i));
        }
        memory_cursor += byte_for_scope_code;

        length_address = memory_cursor;

        for (size_t i = 0; i < byte_for_lenght_of_the_lenght_of_the_scope; i++){
            memory[memory_cursor + i] = 0;
        }
        memory_cursor += byte_for_lenght_of_the_lenght_of_the_scope;
    }
    else{
        size_t scope_length = memory_cursor - scope_start - byte_for_scope_code;

        size_t bytes = bytes_needed(scope_length);

        if (bytes > byte_for_lenght_of_the_lenght_of_the_scope){

            printf("Errore: %zu byte necessari per salvare la lunghezza dello scope.\n", bytes);
            return;
        }

        for (size_t i = 0; i < bytes; i++){

            memory[length_address + i] =
                (__uint8_t)(scope_length >> (8 * i));
        }
    }
}

void update_value_of_variable_from_address(size_t address, __uintmax_t value){

    if(byte_for_scope > 0)
        initialize_variable(tru,get_scope_of_variable_from_address(address)
                                    ,tru, address,auto,
                                value,fal,auto);
    else
        initialize_variable(fal,auto,tru,
                        address,auto,
                            value,fal,auto);
}

void update_method_of_variable_from_address(size_t address){

}


size_t declare_simple_variable(size_t byte_dim, size_t value, size_t methodlist){

    int use_method = fal;
    if(methodlist == fal) use_method = fal; else use_method = tru;

    return  initialize_variable(fal,fal,fal,auto,
                        byte_dim,value,use_method,value);
    

}