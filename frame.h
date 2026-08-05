#ifndef FRAME_H
#define FRAME_H

#include "mem.h"

/* ============================================================
   MODELLO DI FRAME A OFFSET COMPILE-TIME
   ============================================================

   Questo modulo implementa lo strato "runtime scope instantiation"
   descritto nel design: uno scope non e' una serie di record allocati
   dinamicamente (come fa initialize_variable), ma un TEMPLATE di
   dimensione nota a compile-time (scope_size), instanziato a runtime
   spostando due soli cursori:

       base -> indirizzo di partenza dello scope attivo
       end  -> primo byte libero (stack pointer)

   entrata:  base = end ; end += scope_size
   uscita :  end  = previous_end ; base = previous_base

   Ogni variabile/argomento dentro lo scope e' quindi risolta a
   compile-time come "base + offset" (indirizzo x+y del design doc).
   Non esiste symbol lookup a runtime e non esiste una "pull": un
   parametro e' semplicemente una variabile locale il cui valore e'
   gia' stato scritto dal chiamante PRIMA della entrata nello scope.

   Isolamento tra frame: garantito staticamente, non a runtime. Il
   compilatore non genera mai, per il corpo di uno scope, offset fuori
   dal range [0, scope_size) di quello scope. Questo modulo si limita
   a fornire le primitive; il rispetto dei confini e' responsabilita'
   di chi genera gli offset (il compilatore/generatore di bytecode).

   Ricorsione: ogni entrata in scope produce un target_base diverso
   (perche' end e' sempre avanzato), quindi chiamate annidate/ricorsive
   ottengono automaticamente frame diversi. La sequenza (non annidata)
   di chiamate riusa invece lo stesso spazio, perche' frame_exit riporta
   end indietro prima della chiamata successiva.

   ------------------------------------------------------------
   CONVENZIONE DI CHIAMATA (esempio con leggi(let str:1:5 ! let lenght:1))
   ------------------------------------------------------------

       // --- lato chiamante ---
       size_t target_base = frame_prepare_call(scope_size_leggi);

       push_argument(target_base, 0, 1, 'c');
       push_argument(target_base, 1, 1, 'i');
       push_argument(target_base, 2, 1, 'a');
       push_argument(target_base, 3, 1, 'o');
       push_argument(target_base, 5, 1, 4);      // lenght

       size_t prev_base, prev_end;
       frame_enter(target_base, scope_size_leggi, &prev_base, &prev_end);

           // --- lato chiamato (dentro lo scope di leggi) ---
           // str[0] == pull_argument(0, 1);
           // str[1] == pull_argument(1, 1);
           // lenght == pull_argument(5, 1);

       frame_exit(prev_base, prev_end);

   ------------------------------------------------------------
   VALORE DI RITORNO
   ------------------------------------------------------------

   Lo slot di ritorno vive nel frame del CHIAMANTE (sopravvive quindi
   a frame_exit del chiamato), a un offset riservato dal compilatore
   dentro lo scope_size del chiamante stesso, una per ogni call-site:

       size_t ret_addr = reserve_return_slot(ret_offset); // = base + ret_offset, PRIMA della call

       // il chiamato scrive il ritorno usando l'indirizzo assoluto
       // ricevuto (va passato come argomento speciale, es. offset 0):
       write_return(ret_addr, byte_lenght, value);

       // dopo frame_exit, il chiamante legge:
       __uintmax_t result = read_return(ret_addr, byte_lenght);
*/

extern size_t base;   /* indirizzo di partenza del frame attivo   */
extern size_t end;    /* primo byte libero (stack pointer)        */

/* Inizializza base/end. Da chiamare una sola volta dopo begin(),
   con start_base tipicamente = fine dell'area globale. */
void frame_init(size_t start_base);

/* Calcola (senza modificare base/end) l'indirizzo che uno scope
   chiamato AVRA' come proprio 'base' una volta entrato: e' semplicemente     restituisce la base da qui inizia il nuovo
   'end' corrente. Va chiamata subito prima di scrivere gli argomenti         scope ovvero da dovescrivere le nuove variabili
   con push_argument, cosi' che il chiamante sappia dove scriverli. */
size_t frame_prepare_call(size_t scope_size);

/* Entra nello scope: salva base/end correnti (li restituisce al
   chiamante via i puntatori) e sposta base/end sul nuovo frame.
   target_base deve essere il valore ottenuto da frame_prepare_call,
   invariato nel frattempo (nessun'altra push/call in mezzo).
   Ritorna target_base per comodita', o (size_t)-1 in caso di overflow. */
size_t frame_enter(size_t target_base, size_t scope_size,
                    size_t *previous_base, size_t *previous_end);

/* Esce dallo scope: ripristina base/end ai valori salvati da frame_enter.
   Tutto cio' che il frame uscente aveva scritto resta nei byte ma torna
   fuori dal range valido (memoria "morta" ma innocua, coerente col
   resto del modello a bump allocation). */
void frame_exit(size_t previous_base, size_t previous_end);

/* Primitive di lettura/scrittura RAW: nessun metadato (niente scope/
   dim/vlength/method), solo i byte del valore a partire da 'address'.
   Adatte a un modello dove dimensione e posizione sono gia' risolte
   a compile-time: non serve auto-descrizione a runtime. */
void  frame_write(size_t address, size_t byte_lenght, __uintmax_t value);
__uintmax_t frame_read(size_t address, size_t byte_lenght);

/* Risolve un offset compile-time nel frame ATTIVO in un indirizzo    per offset si intende l'indirizzo della variabile
   assoluto: base + offset. E' la 'y' del modello x+y con x = base.   o il gia citato address partendo pero da 0*/
size_t frame_address(size_t offset);

/* push_argument: scrive un argomento nel frame che sta per essere
   aperto (target_base = risultato di frame_prepare_call), quindi PRIMA       target_base mi viene dato da frame_prepare_call
   di frame_enter, mentre lo scope attivo e' ancora quello del chiamante.     si puo usare byte_needed pertrovare i byte di value*/
void push_argument(size_t target_base, size_t offset,
                    size_t byte_lenght, __uintmax_t value); 

/* pull_argument: legge un argomento/variabile locale del frame
   ATTUALMENTE attivo (quindi da usare dopo frame_enter, dentro il
   corpo dello scope chiamato). E' solo zucchero su frame_read +
   frame_address: non esiste una vera operazione di "pull" a runtime,
   il valore e' gia' li' perche' scritto dal chiamante prima della call. offset e' address*/
__uintmax_t pull_argument(size_t offset, size_t byte_lenght);

/* reserve_return_slot: risolve l'offset (riservato dal compilatore
   dentro lo scope del CHIAMANTE) in un indirizzo assoluto, usando
   il base corrente. Va chiamata mentre il frame attivo e' ancora
   quello del chiamante, PRIMA di frame_enter sul chiamato, cosi' che
   l'indirizzo possa essere passato al chiamato (es. come argomento
   nascosto scritto con push_argument). */
size_t reserve_return_slot(size_t offset);

/* write_return / read_return: wrapper semantici su frame_write/read
   che operano sull'indirizzo assoluto gia' risolto da
   reserve_return_slot (quindi validi anche a cavallo di frame_exit,
   perche' puntano nel frame del chiamante, non in quello uscente). */
void write_return(size_t return_address, size_t byte_lenght, __uintmax_t value);
__uintmax_t read_return(size_t return_address, size_t byte_lenght);

#endif
