#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define to_declare 35   
#define byte_for_dim 1
#define byte_for_scope 0
#define byte_for_method_lenght 1
#define byte_for_vleng 1
#define byte_for_lenght_of_the_lenght_of_the_scope 1
#define byte_for_scope_code 1

#define tru 1
#define fal 0
#define auto fal
#define up 16
#define down 4
#define debug fal

#define global_scope 0
#define main_scope 1

__uint8_t *memory;

size_t memory_cursor = 0;

//0 lenght_in_byte_for_scope_dimension
//1 lenght_of_the_scope or next free idx

//(eventual now disabled) code_scope    <---- address pt1
//(eventual now disabled) code_scope    <---- address pt2
//2 var dim       <---- sum eq address end line (eg 10)
//3 var dim       <---- sum eq address end line (eg 10)
//4 lenght in byte
//5 value
//6 value
//7 method_lenght in byte
//8 value
//9 value



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

    for (int i = 0; i < byte_for_vleng; i++) {
        v_lenght_in_byte |= (size_t)memory[address + byte_for_scope + byte_for_dim +  i] << (8 * i);
    }

    return v_lenght_in_byte;

}

size_t get_current_scope_from_address(size_t address){

    size_t scope = 0;

    for (int i = 0; i < byte_for_scope; i++) {
        scope |= (size_t)memory[address + i] << (8 * i);
    }

    return scope;

}

size_t get_value_of_variable(size_t address){

    __uint8_t lenght_of_value = get_lenght_in_byte_of_value_from_address(address);
    if(debug) printf("get lenght of value in byte already written: %d\n", lenght_of_value);

    size_t value = 0;

    for (int i = 1; i <= lenght_of_value; i++) {
        value |= (size_t)memory[address + byte_for_scope + byte_for_dim + i] << (8 * (i-1));
    }

    return value;
}

size_t get_method_lenght_in_byte_from_address(size_t address){


    __uint8_t lenght_of_value = get_lenght_in_byte_of_value_from_address(address);

    size_t lenght_in_byte_of_method_lenght = 0;

    for (int i = 1; i <= byte_for_method_lenght; i++) {
        lenght_in_byte_of_method_lenght |= (size_t)memory[address + byte_for_scope + byte_for_dim + lenght_of_value + i] 
                                                            << (8 * (i-1));
    }

    return lenght_in_byte_of_method_lenght;

}

//FUNCTION TO WRITE

void shift_all_memory(int direction, int number_of_shifts, int use_carry, size_t memory_length_in_byte) {
    for (int s = 0; s < number_of_shifts; s++) {
        if (direction == up) {
            __uint8_t carry_out = memory[0];
            for (size_t i = 0; i < memory_length_in_byte - 1; i++)
                memory[i] = memory[i + 1];
            if (use_carry) memory[memory_length_in_byte - 1] = carry_out;
            memory_cursor--;
        } 
        else if (direction == down) {
            __uint8_t carry_out = memory[memory_length_in_byte - 1];
            for (size_t i = memory_length_in_byte - 1; i > 0; i--)
                memory[i] = memory[i - 1];
            if (use_carry) memory[0] = carry_out;
            memory_cursor++;
        }
    }
}

void add_method_to_variable(size_t address){

}


/*con l'utilizzo di address si possono andare a modificare i valori dei metadati di una
variabile nello specifico altrimenti con address = fal si incoda la variabile da dichiarare
al primo indirizzo disponibile ritornato dalla funzione stessa
....................................................................
se lenght_in_byte è = 0 allora viene assegnata autonomamente la dimensione
mentre viene specificato con use_method se ci sono da aggiungere metodi alla variabile*/
size_t initialize_variable(int use_scope, size_t scope_address,
                            int use_address, size_t address_offset,  
                            size_t lenght_in_byte_of_value, __uintmax_t value,
                            int use_method, size_t method_address){

    size_t start = 0;
    static size_t backup_end = 0;

    if (use_address){ start = address_offset; backup_end = memory_cursor; }
    else start = memory_cursor;
    printf("%lu starting from line\n", start);

    if(start >= to_declare){
        printf("error: cannot declare other variable risk of overflow\ntry to increase the capacity of %d\n", to_declare);
        printf("normally you shouldnt encounter this error since the declaration of the scope is automated to fit all the variable\n\n");
        return start;
    }

    size_t old_lenght_in_byte = get_lenght_in_byte_of_value_from_address(start);

    if (old_lenght_in_byte > 0) {
        if (bytes_needed(value) > old_lenght_in_byte) {
            printf("error: il valore %lu richiede %lu byte, ma la variabile ne ha solo %lu\n",
                (size_t)value, bytes_needed(value), old_lenght_in_byte);
            return start;
        }
        lenght_in_byte_of_value = old_lenght_in_byte;
    }
    else {
        if (lenght_in_byte_of_value == 0) {
            lenght_in_byte_of_value = bytes_needed(value);
        }
        else if (lenght_in_byte_of_value < bytes_needed(value)) {
            printf("error: the requested lenght of: %lubyte is to little to fit the value: %lu \nare needed at least: %lubyte \n\n",
                    lenght_in_byte_of_value, value, bytes_needed(value));
            return start;
        }
    }
    printf("%lubyte for \"%lu\" numero di celle value da utilizzare\n", lenght_in_byte_of_value, value);

    if(start + byte_for_scope + byte_for_dim + lenght_in_byte_of_value >= to_declare){
        printf("error: cannot declare other variable risk of overflow\n"
            "try to increase the capacity of %d\n", to_declare);
        return start;
    }

    for(size_t i = 0; i < byte_for_scope; i++){
        memory[start + i] = (__uint8_t)(scope_address >> (8 * i));
    }

    if(old_lenght_in_byte > 0){
        for(size_t i = 1; i <= old_lenght_in_byte; i++){
            memory[start + byte_for_dim + byte_for_scope + i] = 0;
        }
    }

    memory[start + byte_for_dim + byte_for_scope] = lenght_in_byte_of_value;

    for(size_t i = 1; i <= lenght_in_byte_of_value; i++){
        memory[start + byte_for_dim + byte_for_scope + i] = (__uint8_t)(value >> (8 * (i-1)));
    }

    size_t record_end = start + byte_for_scope + byte_for_dim + byte_for_vleng + lenght_in_byte_of_value;

    if(!use_method){
        memory_cursor = use_address ? backup_end : record_end;
        printf("line to %lu continue from\n\n", record_end);
    }
    else{
        size_t method_lenght = bytes_needed(method_address);
        if(method_lenght > byte_for_method_lenght){
            printf("error: the address of the method (%lu) is to big to be saved in %d \n",
                        method_lenght, byte_for_method_lenght);
        }
        printf("%lubyte richiesti per salvare il codice del metodo \n", method_lenght);

        for(int i = 0; i < byte_for_method_lenght; i++){
            memory[record_end + i] = (__uint8_t)(method_lenght >> (8 * i));
        }

        for(int i = 0; i < method_lenght; i++){
            memory[record_end + byte_for_method_lenght + i] = (__uint8_t)(method_address >> (8 * i));
        }

        record_end += byte_for_method_lenght + method_lenght;
        memory_cursor = use_address ? backup_end : record_end;
        printf("%lu continue from address\n\n", record_end);
    }

    // risolvo dim — SEMPRE con record_end, mai col cursore globale
    for(int i = 0; i < byte_for_dim; i++){
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

    if (is_start)
    {
        scope_start = memory_cursor;

        /* codice dello scope */
        for (int i = 0; i < byte_for_scope_code; i++)
        {
            memory[memory_cursor + i] =
                (__uint8_t)(scope_code >> (8 * i));
        }
        memory_cursor += byte_for_scope_code;

        /* salvo dove dovrà essere scritta la lunghezza */
        length_address = memory_cursor;

        /* placeholder */
        for (int i = 0; i < byte_for_lenght_of_the_lenght_of_the_scope; i++)
        {
            memory[memory_cursor + i] = 0;
        }
        memory_cursor += byte_for_lenght_of_the_lenght_of_the_scope;
    }
    else
    {
        /* lunghezza dello scope (escluso il codice scope) */
        size_t scope_length = memory_cursor - scope_start - byte_for_scope_code;

        size_t bytes = bytes_needed(scope_length);

        /* se il campo è troppo piccolo segnalo errore */
        if (bytes > byte_for_lenght_of_the_lenght_of_the_scope)
        {
            printf("Errore: %zu byte necessari per salvare la lunghezza dello scope.\n", bytes);
            return;
        }

        for (size_t i = 0; i < bytes; i++)
        {
            memory[length_address + i] =
                (__uint8_t)(scope_length >> (8 * i));
        }
    }
}

size_t get_value_of_address(size_t address){
    return (size_t)memory[address];
}


int main(){
    
    begin(to_declare);

    printf("%p\n",memory);
    printf("\n");

    set_scope_start_end(tru,auto,fal);
    
    int idx = initialize_variable(tru,main_scope,fal,auto,auto,28,tru,59);
    int ix = initialize_variable(tru,main_scope,fal,auto,auto,30,tru,55);
    int id = initialize_variable(tru,main_scope,fal,auto,auto,31,tru,55);
    int ids = initialize_variable(tru,main_scope,fal,auto,auto,78,tru,55);

    set_scope_start_end(fal,auto,fal);

    printf("%lu-28 %lu-30 %lu-31 %lu-78\n",get_value_of_variable(idx),
                                    get_value_of_variable(ix),
                                    get_value_of_variable(id),
                                    get_value_of_variable(ids));
        
    printf("\n");
    printf("crs position; <%lu>\n",memory_cursor);
    printf("\n");

    for (int i = 0; i < to_declare; i++){        
        printf("line(%d) %ld: %u\n",i, (long)(memory + i), memory[i]);
    }

}