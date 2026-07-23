#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <math.h>
#include <string.h>

#define to_declare 20
#define byte_for_dim 2
#define byte_for_scope 2
#define byte_for_method_lenght 2
#define byte_for_vleng 1

#define tru 1
#define fal 0
#define auto fal

#define global_scope 0
#define main_scope 1


__uint8_t *memory;

//0 code_scope    <---- address pt1
//1 code_scope    <---- address pt2
//2 var dim     
//3 var dim     
//4 lenght in byte
//5 value
//6 value
//7 method_lenght in byte
//9 value
//10 value



void begin(size_t byte_to_allocate){

    memory = malloc(byte_to_allocate);
    memset(memory,0,byte_to_allocate);

}



size_t bytes_needed(__uintmax_t value){
    size_t n = 1;
    while (value >> (8 * n)) n++;
    return n;
}

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

/*con l'utilizzo di address si possono andare a modificare i valori dei metadati di una
variabile nello specifico altrimenti con address = fal si incoda la variabile da dichiarare
al primo indirizzo disponibile ritornato dalla funzione stessa
....................................................................
se lenght_in_byte è = 0 allora viene assegnata autonomamente la dimensione
mentre viene specificato con use_method se ci sono da aggiungere metodi alla variabile*/
size_t initialize_variable(int use_address, size_t address_offset, __uintmax_t value, 
                            size_t lenght_in_byte,
                            int use_method, size_t method_address,
                            int use_scope, size_t scope_address){

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
    printf("%lu %lu numero di celle value da utilizzare\n",lenght_in_byte,value);


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

        printf("end_of_data %ld \n",end);
        if(use_address) end = backup_end;
        printf("%lu continue from\n\n",end);
    }

    //vanno aggiunti i method
    else{

        size_t method_lenght = bytes_needed(method_address);
        if(method_lenght > byte_for_method_lenght){
            printf("error: the address of the method (%lu) is to big to be saved in %d \n", 
                        method_lenght,byte_for_method_lenght);
        }
        printf("%lu numero di byte richiesti per salvare il codice del metodo \n", method_lenght);

        for(int i = 0; i < byte_for_method_lenght; i++){
            memory[start + byte_for_scope + byte_for_dim + byte_for_vleng + lenght_in_byte + i]   
                                 = (__uint8_t)(method_lenght >> (8 * i));
        }
        
        for(int i = 0; i < method_lenght; i++){

            memory[end + byte_for_method_lenght + i]   =   (__uint8_t)(method_address >> (8 * i));
    
        }


        //aggiorno i end con la lunghezza dei metodi
        end += method_lenght + byte_for_method_lenght;
        printf("end_of_data %ld \n",end);
        if(use_address) end = backup_end;
        printf("%lu continue from\n\n",end);
        

    }

    //risolvo dim
    for(int i = 0; i < byte_for_dim; i++){
        memory[start + byte_for_scope + i]   = (__uint8_t)(end >> (8 * i));
    }

    return start ;
   
}

size_t read_value_of_variable(size_t address){

    __uint8_t lenght_of_value = memory[address + byte_for_scope +byte_for_dim];

    size_t value = 0;

    for (int i = 1; i <= lenght_of_value; i++) {
        value |= (size_t)memory[address + byte_for_scope + byte_for_dim + i] << (8 * (i-1));
    }

    return value;
}

int main(){
    
    begin(to_declare);



    printf("%p\n",memory);
    printf("\n");
    /*
    
    initialize_variable(fal,0,256,0,fal,0,tru,main_scope);
    initialize_variable(fal,0,612,1,fal,0,tru,main_scope);
    initialize_variable(fal,0,44,0,fal,0,tru,main_scope);
    initialize_variable(fal,0,5,0,fal,0,tru,main_scope);
    initialize_variable(fal,0,81,0,fal,0,tru,main_scope);
    initialize_variable(fal,0,6,0,fal,0,tru,main_scope);

    */
    initialize_variable(fal,auto,4,auto,tru,256,tru,main_scope);
    initialize_variable(fal,auto,96,auto,tru,512,tru,main_scope);
    
        
    printf("\n");

    for (int i = 0; i < to_declare; i++){        
        printf("line(%d) %ld: %u\n",i, (long)(memory + i), memory[i]);
    }
    
    //IL CODICE DEI METODI O ADDRESS FA RIFERIMENTO AD UNA LISTA CREATA A COMPILE TIME
    //DI METODI GIA SCRITTI, AD ESEMPIO SE L'ADDRESS FA RIDERIMENTO ALLA LISTA 210 E LA LISTA 
    //210 CONTIENE APPEND, DOUBLE, SQUARE ECC... QUESTI METODI POTRANNO ESSERE APPLICATI 
    //ALLA VARIABILE
    
    //PER LA GESTIONE DEGLI ARRAY SI USA LA STESSA STRUTTURA NELLA QUALE VENGONO SCRITTI I VALUE
    //IN MODO SPECIALE OVVERO CHE SI SCEGLIE UNA DIMENSIONE PER IL VETTORE IN TERMINI DI CELLE
    //E POI DI DIMENSIONI CELLA: LET ARRAY: 5(NUMERO DI CELLE):2 CHE CORRISPONDE A INT ARRAY[5]
    //POICHE DI 2BYTE E 5 CELLE. QUI SEMPLICEMENTE VIENE CREATA UNA STRUTTURA VAR LA CUI
    //LENGHT È UGUALE A NUMB_CELL*DIM IN MODO CHE LE 10 CELLE VALUE SEMBRINO RAGGRUPPATE IN GRUPPI
    //DA DUE PER FORMARE 5 CELLE DELL'ARRAY QUINDI NELLA TABELLA DEI SIMBOLI SI HA:
    // PER LE VAR:
    //               NOME : IDX
    // PER I VETT:
    //               NOME : NUM_OF_CELL : DIM IN BYTE DELLE CELLE
    //
    //NONOSTANDE VENGANO LETTI IN MODI DIVERSI SI HA LA STESSA STRUTTURA ALLA BASE
}