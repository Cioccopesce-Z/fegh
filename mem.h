#ifndef MEM_H
#define MEM_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#define tru 1
#define fal 0
#define auto fal
#define up 16
#define down 4
#define debug fal

#define global_scope 0
#define main_scope 1

/* configurazione della struttura memoria: variabili, non define.
   vanno DEFINITE (con un valore) nel file che fa il main, PRIMA di
   chiamare begin() o qualunque altra funzione della libreria. */
extern size_t to_declare;
extern size_t byte_for_lenght_of_the_lenght_of_the_scope;
extern size_t byte_for_scope_code;
extern size_t byte_for_scope;
extern size_t byte_for_dim;
extern size_t byte_for_vleng;
extern size_t byte_for_method_lenght;

/* memoria vera e propria: stessa cosa, dichiarata qui, definita nel main. */
extern __uint8_t *memory;
extern size_t memory_cursor;

void begin(size_t byte_to_allocate);
size_t bytes_needed(__uintmax_t value);

// FUNCTION TO READ
size_t get_lenght_in_byte_of_value_from_address(size_t address);
size_t get_scope_of_variable_from_address(size_t address);
size_t get_value_of_variable(size_t address);
size_t get_variable_struct_end_index_from_address(size_t address);
size_t get_methodlist_lenght_in_byte_from_address(size_t address);
size_t get_methodlist_of_variable_from_address(size_t address);
size_t get_value_of_address(size_t address);
size_t get_direct_lenght_in_byte_of_variable_struct(size_t address);

//function to parse and get the correct address of an array by their index and declaration mark (value,repetition)
size_t resolve_array_index_from_normal_sintax(size_t address_of_data_struct,size_t repetition[], 
                                        size_t value_of_index[], size_t rank);

// FUNCTION TO WRITE
size_t resolve_value_lenght(size_t existing_lenght, size_t requested_lenght, __uintmax_t value);
size_t resolve_method_lenght(size_t existing_lenght, size_t method_address);
void write_value(size_t start, size_t lenght_in_byte, __uintmax_t value);
void write_methodlist(size_t record_end, size_t method_lenght, size_t method_address);

size_t initialize_variable(int use_scope, size_t scope_address,
                            int use_address, size_t address_offset,
                            size_t lenght_in_byte_of_value, __uintmax_t value,
                            int use_method, size_t method_address);

void set_scope_start_end(int is_start, int auto_set_code_for_scope, int scope_code);

void update_value_of_variable_from_address(size_t address, __uintmax_t value);
void update_method_of_variable_from_address(size_t address);

size_t declare_simple_array(size_t byte_dim, size_t value, size_t methodlist, size_t repetition);
size_t declare_simple_variable(size_t byte_dim, size_t value, size_t methodlist);

#endif