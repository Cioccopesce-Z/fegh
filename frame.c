#include "frame.h"

size_t base = 0;
size_t end  = 0;

void frame_init(size_t start_base){
    base = start_base;
    end  = start_base;
}

size_t frame_prepare_call(size_t scope_size){

    if(end + scope_size >= to_declare){
        printf("error: chiamata di scope rifiutata, overflow imminente\n"
               "richiesti %lu byte, disponibili %lu\n\n",
               scope_size, to_declare - end);
        return end; /* indirizzo comunque restituito, ma la call andra' rifiutata da frame_enter */
    }

    return end; /* diventera' il 'base' del chiamato, una volta entrato */
}

size_t frame_enter(size_t target_base, size_t scope_size,
                    size_t *previous_base, size_t *previous_end){

    if(previous_base) *previous_base = base;
    if(previous_end)  *previous_end  = end;

    if(target_base + scope_size >= to_declare){
        printf("error: cannot enter scope, risk of overflow\n"
               "try to increase the capacity of %zu\n\n", to_declare);
        return (size_t)-1;
    }

    base = target_base;
    end  = target_base + scope_size;

    return target_base;
}

void frame_exit(size_t previous_base, size_t previous_end){
    base = previous_base;
    end  = previous_end;
}

void frame_write(size_t address, size_t byte_lenght, __uintmax_t value){

    if(address + byte_lenght > to_declare){
        printf("error: scrittura fuori dai limiti (%zu) all'indirizzo %lu\n",
                to_declare, address);
        return;
    }

    for(size_t i = 0; i < byte_lenght; i++)
        memory[address + i] = (__uint8_t)(value >> (8 * i));
}

__uintmax_t frame_read(size_t address, size_t byte_lenght){

    if(address + byte_lenght > to_declare){
        printf("error: lettura fuori dai limiti (%zu) all'indirizzo %lu\n",
                to_declare, address);
        return 0;
    }

    __uintmax_t value = 0;
    for(size_t i = 0; i < byte_lenght; i++)
        value |= (__uintmax_t)memory[address + i] << (8 * i);

    return value;
}

size_t frame_address(size_t offset){ //indirizzo nel frame
    return base + offset;
}

void push_argument(size_t target_base, size_t offset,
                    size_t byte_lenght, __uintmax_t value){
    frame_write(target_base + offset, byte_lenght, value);
}

__uintmax_t pull_argument(size_t offset, size_t byte_lenght){
    return frame_read(frame_address(offset), byte_lenght);
}

size_t reserve_return_slot(size_t offset){
    return frame_address(offset);
}

void write_return(size_t return_address, size_t byte_lenght, __uintmax_t value){
    frame_write(return_address, byte_lenght, value);
}

__uintmax_t read_return(size_t return_address, size_t byte_lenght){
    return frame_read(return_address, byte_lenght);
}
