#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <math.h>
#include <string.h>

#define to_declare 20
#define byte_for_dim 2
#define byte_for_scope 0
#define byte_for_method_lenght 2
#define byte_for_vleng 1

#define tru 1
#define fal 0
#define auto fal
#define up 16
#define down 4

#define global_scope 0
#define main_scope 1

__uint8_t *memory;

size_t memory_cursor = 0;

//0 code_scope    <---- address pt1
//1 code_scope    <---- address pt2

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

size_t get_lenght_in_byte_from_address(size_t address){

    return (size_t)memory[address + byte_for_dim + byte_for_scope];

}

size_t get_current_scope_by_address(size_t address){

    size_t scope = 0;

    for (int i = 0; i < byte_for_scope; i++) {
        scope |= (size_t)memory[address + i] << (8 * i);
    }

    return scope;

}

size_t get_value_of_variable(size_t address){

    __uint8_t lenght_of_value = memory[address + byte_for_scope +byte_for_dim];

    size_t value = 0;

    for (int i = 1; i <= lenght_of_value; i++) {
        value |= (size_t)memory[address + byte_for_scope + byte_for_dim + i] << (8 * (i-1));
    }

    return value;
}

//FUNCTION TO WRITE

void shift_all_memory(int direction, int number_of_shifts, int use_carry,size_t memory_lenght_in_byte){

    size_t i = 0;

    if(direction == up){

        __uint8_t carry = 0;


        for(i = 0; i < number_of_shifts; i++)
            for(i = 0; i < memory_lenght_in_byte; i++){

                memory_cursor -= number_of_shifts;

                if(use_carry){
                    carry = memory[0];
                    memory[memory_cursor-i] = carry; 
                }

                memory[i] = memory[i+1];

            }
        
    }

    else if(direction == down){
        for(i = 0; i < number_of_shifts; i++)
            for(i = 0; i < memory_lenght_in_byte; i++)
                if(i-1 == -1) memory[i] = memory[i-1];
                    else memory[i] = memory[i-1];
            
    }

    
}

void add_method_to_variable(){

}


/*con l'utilizzo di address si possono andare a modificare i valori dei metadati di una
variabile nello specifico altrimenti con address = fal si incoda la variabile da dichiarare
al primo indirizzo disponibile ritornato dalla funzione stessa
....................................................................
se lenght_in_byte è = 0 allora viene assegnata autonomamente la dimensione
mentre viene specificato con use_method se ci sono da aggiungere metodi alla variabile*/
size_t initialize_variable(int use_scope, size_t scope_address,
                            int use_address, size_t address_offset,  
                            size_t lenght_in_byte, __uintmax_t value,
                            int use_method, size_t method_address){

    size_t start = 0;                            
    static size_t end = 0;
    static size_t backup_end = 0;

    if (use_address){ start = address_offset; backup_end = end;} 
    else start = end ;
    printf("%lu starting from line\n",start);

    if(start >= to_declare){
        printf("error: cannot declare other variable risk of overflow\ntry to increase the capacity of %d\n", to_declare);
        printf("normally you shouldnt encounter this error since the declaration of the scope is automated to fit all the variable\n\n");
        return start;
    }

    size_t old_lenght_in_byte = get_lenght_in_byte_from_address(start);
                                
    //la quantita di byte destinati a i valori è gia assegnata
    if (old_lenght_in_byte > 0) {
        // riassegnazione: la dimensione resta quella dichiarata all'origine, non cresce mai
        if (bytes_needed(value) > old_lenght_in_byte) {
            printf("error: il valore %lu richiede %lu byte, ma la variabile ne ha solo %lu\n",
                (size_t)value, bytes_needed(value), old_lenght_in_byte);
            return start;
        }
        lenght_in_byte = old_lenght_in_byte;   // mantieni fissa la dimensione originale
    }
    else {
        // prima dichiarazione
        if (lenght_in_byte == 0) {
            lenght_in_byte = bytes_needed(value);
        }
        else if (lenght_in_byte < bytes_needed(value)) {
            printf("error: the requested lenght of: %lubyte is to little to fit the value: %lu \nare needed at least: %lubyte \n\n",
                    lenght_in_byte,value,bytes_needed(value));
            return start;
        }
    }
    printf("%lubyte for \"%lu\" numero di celle value da utilizzare\n",lenght_in_byte,value);


    if(start + byte_for_scope + byte_for_dim + lenght_in_byte >= to_declare){
        printf("error: cannot declare other variable risk of overflow\n"
            "try to increase the capacity of %d\n", to_declare);
        return start;
    }

    //risolvo scope
    for(size_t i = 0; i < byte_for_scope; i++){
        memory[start + i] = (__uint8_t)(scope_address >> (8 * i));      
    }

    //se sono gia state utilizzate le celle e si sta cambiando valore alla var
    //va re inizializzara tutto lo scope dei value a 0 prima di riassegnare
    if(old_lenght_in_byte > 0){
        for(size_t i = 1; i <= old_lenght_in_byte; i++){
            memory[start + byte_for_dim + byte_for_scope + i] = 0;
        }
    }

    //altrimenti usa direttamente lenght_in_byte
    memory[start + byte_for_dim + byte_for_scope] = lenght_in_byte;

    //vengono gia assegnati i valori nelle rispettive celle
    for(size_t i = 1; i <= lenght_in_byte; i++){
        memory[start + byte_for_dim + byte_for_scope + i] = (__uint8_t)(value >> (8 * (i-1)));
    }

    //valore di dim escludendo la presenza di metodi
    end = start + byte_for_scope + byte_for_dim + lenght_in_byte;

    //se non ci sono metodi assegna il valore di dim e poi ritorna l'indirizzo di start
    if(!use_method){

        if(use_address) end = backup_end;
        memory_cursor = end;
        printf("line to %lu continue from\n\n",end);
    }

    //vanno aggiunti i method
    else{

        size_t method_lenght = bytes_needed(method_address);
        if(method_lenght > byte_for_method_lenght){
            printf("error: the address of the method (%lu) is to big to be saved in %d \n", 
                        method_lenght,byte_for_method_lenght);
        }
        printf("%lubyte richiesti per salvare il codice del metodo \n", method_lenght);

        for(int i = 0; i < byte_for_method_lenght; i++){
            memory[start + byte_for_scope + byte_for_dim + byte_for_vleng + lenght_in_byte + i]   
                                 = (__uint8_t)(method_lenght >> (8 * i));
        }
        
        for(int i = 0; i < method_lenght; i++){

            memory[end + byte_for_method_lenght + i]   =   (__uint8_t)(method_address >> (8 * i));
    
        }


        //aggiorno i end con la lunghezza dei metodi
        end += method_lenght + byte_for_method_lenght;
        if(use_address) end = backup_end;
        memory_cursor = end;
        printf("%lu continue from address\n\n",end);
        

    }

    //risolvo dim
    for(int i = 0; i < byte_for_dim; i++){
        memory[start + byte_for_scope + i]   = (__uint8_t)(end >> (8 * i));
    }

    return start ;
   
}

int starts_with(const char *str, const char *prefix) {
    return strncmp(str, prefix, strlen(prefix)) == 0;
}


int main(){
    
    begin(to_declare);

    printf("%p\n",memory);
    printf("\n");
    
    initialize_variable(tru,main_scope,fal,auto,auto,4,tru,256);
        
    printf("\n");
    printf("crs position; <%lu>\n",memory_cursor);
    printf("\n");

    for (int i = 0; i < to_declare; i++){        
        printf("line(%d) %ld: %u\n",i, (long)(memory + i), memory[i]);
    }

    printf("\n\n");
    shift_all_memory(up,1,tru,to_declare);


    for (int i = 0; i < to_declare; i++){        
        printf("line(%d) %ld: %u\n",i, (long)(memory + i), memory[i]);
    }

}