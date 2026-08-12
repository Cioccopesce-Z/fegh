# fegh

A C runtime (C99/C11, no C++) for a custom interpreted language, using
`.fgh` source files. The project is split into two parts that share
the same underlying memory model:

1. **Self-describing memory** (`mem.c`/`mem.h`): every variable is a
   record written directly into a single buffer (`memory`), carrying
   its own metadata (scope, size, value length, method list) — no
   separate symbol table is needed to read it back.
2. **Compile-time-offset frames** (`frame.c`/`frame.h`): an
   alternative/complementary model, meant for function calls, where a
   scope is a template of a size known at compile time, instantiated
   at runtime by moving two cursors (`base`/`end`) — no runtime symbol
   lookup.

Alongside these sits the **source script analysis** layer
(`loader.c`/`loader.h` + `memory_parser.c`/`memory_parser.h`), which
reads a `.fgh` file, splits it into logical lines, recognizes scopes
(functions, if, while, for, etc.) and the `let` declarations inside
them, and builds a variable table (name/size/rank/repetition) ready to
be used by the rest of the interpreter.

`fegh.c` holds a demo/test `main()`: it exercises the `mem.c` API by
hand, then loads and processes `test.fgh` through
`loader.c`/`memory_parser.c`.

---

## File layout

| File                  | Content |
|------------------------|-----------|
| `mem.h` / `mem.c`      | Self-describing memory model: allocation, variable declaration/update, metadata reads, array/matrix indexing. |
| `frame.h` / `frame.c`  | Compile-time-offset frame model: `frame_enter`/`frame_exit`, argument passing (`push_argument`/`pull_argument`), return values (`reserve_return_slot`/`write_return`/`read_return`). |
| `loader.h` / `loader.c`| Loads the `.fgh` file into logical lines (`script[]`) and recognizes scopes (`scope_table[]`: functions, if/else/while/for/during, macros, `C` blocks). |
| `memory_parser.h` / `memory_parser.c` | Parses `let` declarations found inside scopes into a variable table (`var_table[]`) with name, cell size, value, method, rank and repetition (array/matrix/tensor). |
| `sintax_keyword.h`     | Syntax keywords and separators (`let`, `!`, `if`, `else`, `esleif`, `#`) and the array/matrix indexing formula. |
| `fegh.c`               | Demo `main()`: exercises `mem.c` by hand, then loads and processes `test.fgh`. |
| `test.fgh`             | Example script used by `fegh.c`. |

---

## The self-describing memory model (`mem.c`)

Every variable is written in sequence inside the single global buffer
`memory` (allocated by `begin()`), as a record made of fields whose
byte width is configurable (via the global variables `byte_for_scope`,
`byte_for_dim`, `byte_for_vleng`, `byte_for_method_lenght`, to be set
**before** calling `begin()`):

```
[scope?] [dim] [vleng] [value...] [method_lenght?] [method...]
```

- **scope**: which scope the variable belongs to (optional, present
  only if `byte_for_scope > 0`).
- **dim**: absolute address of the first byte right after this
  record — lets you jump from one variable to the next without
  needing to know its content.
- **vleng**: how many bytes the `value` occupies.
- **value**: the actual payload, `vleng` bytes.
- **method_lenght / method**: optional, a method list attached to the
  variable.

`initialize_variable()` is the central function: it declares a new
variable (appended at the first free address, `memory_cursor`) or
updates an existing one (by passing `use_address` + the address). The
size of `value` and of `method` is fixed at the first declaration and
can never change afterward (subsequent
`update_value_of_variable_from_address()` calls must fit inside it).

The `get_*` functions read the various fields of a record starting
from its address, with no need for an external symbol table: the
address plus the `byte_for_*` constants are enough to correctly parse
the raw bytes.

### Arrays, matrices, tensors

`resolve_array_index_from_normal_sintax()` resolves the absolute
address of a cell inside an array/matrix/tensor, starting from the
base address of the data struct (`Xa`), the size of each axis
(`repetition[]`) and the requested indices, following the row-major
notation described in `sintax_keyword.h`:

```
:((Xa + o) + idx[i..rank] * repetition[i+1..rank] * $a)
```

where `$a` is the size in bytes of a single cell. No magic numbers:
rank, dimensions and stride are always read from metadata, never
hard-coded.

---

## The frame model (`frame.c`)

An alternative geared toward function calls: a scope is not a series
of dynamically allocated records, but a **template** of a size known
at compile time (`scope_size`), instantiated at runtime by moving two
cursors:

- `base` → start address of the active scope
- `end`  → first free byte (stack pointer)

Every variable/argument inside the scope is therefore resolved at
compile time as `base + offset`; there is no runtime symbol lookup.
Isolation between frames is guaranteed statically (by the code
generator, not by a runtime check).

Typical calling convention:

```c
size_t target_base = frame_prepare_call(scope_size_leggi);

push_argument(target_base, 0, 1, 'c');
push_argument(target_base, 1, 1, 'i');
/* ... */

size_t prev_base, prev_end;
frame_enter(target_base, scope_size_leggi, &prev_base, &prev_end);

    /* callee scope body: pull_argument(offset, byte_lenght) */

frame_exit(prev_base, prev_end);
```

The return value instead lives in the **caller's** frame (so it
survives the callee's `frame_exit`), resolved with
`reserve_return_slot()` before `frame_enter`, and written/read with
`write_return()`/`read_return()`.

See the comments at the top of `frame.h` for the full explanation and
a complete example.

---

## Loading and parsing a script (`loader.c` + `memory_parser.c`)

1. `load_script(filename)` reads the `.fgh` file character by
   character and splits it into logical lines inside `script[]`: `{`
   and `}` always close off a line of their own, `;` and `\n` end the
   current statement. Every line is stripped of stray spaces/tabs
   (`clean_line`/`remove_spaces`) — **spacing in the source is never
   reliable and must never be assumed** by the rest of the parsing.

2. `build_scope_signatures()` scans `script[]` and, for every line
   that opens a scope (`{`), builds a `sign_of_scope` entry in
   `scope_table[]`: type (`scope`/function, `if`, `else`, `esleif`,
   `while`, `during`, `for`, `#` macro, `C` block), opening and
   closing line, and — if it's a function — its name and argument
   list.

3. `create_metadata_for_var_struct_in_a_scope(start, end)`
   (`memory_parser.c`) scans a scope's lines looking for `let`
   declarations (both in the body and in function arguments) and
   passes them to `return_dimension_in_byte_of_var_struct()`, which
   tokenizes them on `:` and passes them to `parse_let()`, which
   finally appends them to `var_table[]` as a `var_data_struct` (name,
   cell size, value, method, rank, repetition).

Format of a `let` declaration (see `parse_let` in `memory_parser.c`
for the exact token layout):

```
let name:cell_size:[repetition...]:method:value
```

Example (from `test.fgh`, a `[3][3][2]` matrix of 1-byte cells,
method=199, initial value 8 in every cell):

```
scope main() {
    let matr :1:3:3:2:199:8;
}
```

---

## General project conventions

- No C++, no magic numbers: rank, dimensions and stride are always
  read from metadata at runtime, never hard-coded in the code.
- Spacing in `.fgh` source is never reliable: all parsing explicitly
  strips spaces before interpreting a line.
- The size of a variable's `value` and `method` is fixed at first
  declaration and can never be resized afterward.

## Error handling

All of the engine's runtime errors (memory overflow, a value that
doesn't fit in the reserved space, out-of-bounds address,
out-of-memory, malformed `.fgh` declarations, an unopenable script
file, etc.) are now reported consistently on `stderr` (no longer mixed
in with normal output on `stdout`), in the format:

```
[ERROR] <file>:<line> in <function>(): <description> [+ any relevant values]
```

so you can immediately trace back to the exact point in the source
that raised the error just by reading the message — especially useful
since the program's normal output can be very verbose (e.g. during a
memory or `var_table` dump).

---

## Build

No dedicated build system is included in this file set; a plain gcc
command is enough, e.g.:

```sh
gcc -o fegh fegh.c frame.c loader.c mem.c memory_parser.c
./fegh
```

`fegh.c` expects to find `test.fgh` in the current directory.


# Size-Based Memory Model

A small experimental memory model based on **size-oriented storage instead of
type-oriented storage**.

The project explores how variables, scopes, arrays, and methods can be
represented directly inside a raw byte buffer without relying on the host
language type system.

The prototype is written in C, but the final goal is a small compiled language
where the compiler builds the complete memory layout before execution.

The fundamental idea is:

> A variable is not a type. A variable is a memory record with a known size.

The runtime only needs to know:
- where a record starts;
- how large it is;
- where its value bytes are located.

Everything else can be resolved before execution.

---

# Project Status

This repository currently contains a **minimal C proof of concept**.

It is not:
- a general purpose allocator;
- a C replacement;
- a production memory manager;
- a complete programming language.

The current implementation demonstrates:

- manual memory layout;
- variable records;
- automatic size calculation;
- fixed-size reassignment;
- scope metadata;
- method placeholders;
- sequential memory allocation.

The complete language design extends this prototype with:

- compile-time memory mapping;
- Memory Print generation;
- AST bytecode generation;
- resolved addresses;
- runtime scope instantiation.

---

# Core Memory Model

Memory is represented as a single flat byte buffer:

```c
uint8_t *memory;
```

The buffer is allocated once:

```c
begin(size);
```

and every variable is stored sequentially inside it.

The allocator does not know whether a value is:

- an integer;
- a character;
- an object;
- an array;
- another structure.

It only knows:

```
how many bytes are required
```

---

# Variable Record Format

Every variable is stored as a contiguous record:

```
[ scope ][ dim ][ value length ][ value ][ optional methods ]
```

Default layout:

```
[ scope (2B) ]
[ dim (2B) ]
[ value length (1B) ]
[ value (N bytes) ]
[ method length (2B) ]
[ method address ]
```

---

## Scope

The scope field identifies the owner of the variable.

Example:

```
0 = global scope
1 = main scope
2 = function scope
```

The current prototype supports scope metadata, while the final compiler will
generate scope identifiers automatically.

---

## Dim

`dim` is the most important field.

It stores the address where the next record begins.

Example:

```
address 0

[ variable A ]
dim = 10


address 10

[ variable B ]
```

Because records are contiguous:

```
next address = current address + record size
```

`dim` can therefore be interpreted as:

- next record address;
- record end address;
- traversal offset;
- array stride.

This allows the memory buffer to behave like a linked structure without using
real pointers.

---

## Value Length

The value length stores how many bytes belong to the actual value.

Example:

```
[ length = 2 ][ value ][ value ]
```

means the variable owns two bytes of data.

The size is fixed after declaration.

---

## Value

The value is stored directly as raw bytes.

Example:

```
511
```

requires:

```
11111111 00000001
```

and therefore occupies two bytes.

The memory system does not interpret these bytes.

Interpretation belongs to the language layer.

---

# Automatic Value Sizing

If the user does not specify the size of a variable, the minimum required size
is calculated automatically.

Example:

```
5
```

requires:

```
1 byte
```

while:

```
511
```

requires:

```
2 bytes
```

The function responsible for this calculation determines the smallest number of
bytes needed to represent the value.

---

# Immutable Record Size

Once created, a variable record cannot grow.

Example:

```
let a = 5
```

creates:

```
value size = 1 byte
```

Later:

```
a = 500
```

fails because:

```
500 requires 2 bytes
```

The allocator never:

- moves following variables;
- reallocates individual records;
- changes existing addresses.

This guarantees address stability.

---

# Memory Cursor

The prototype uses a bump-pointer style allocator.

A cursor tracks the first free byte:

```c
size_t memory_cursor;
```

Creating a variable:

```
start = memory_cursor

write record

memory_cursor = record_end
```

No searching is required.

No fragmentation is created.

---

# Updating Existing Variables

The initialization function supports two modes:

## New variable

The record is appended:

```
start = memory_cursor
```

## Existing address

The record is overwritten:

```
start = provided address
```

The original allocation position is preserved.

This allows direct memory manipulation while keeping the layout stable.

---

# Scope Layout

Scopes are stored with a small header:

```
[ scope id ][ scope size ][ variables... ]
```

The prototype can automatically write:

- scope identifier;
- total scope length.

Example:

```
scope 1

size = 30 bytes

variable A
variable B
variable C
```

The final language uses this information as a **scope template**.

---

# Compile-Time Memory Mapping

The final language changes the allocation model.

Variables are not created dynamically during execution.

Instead, the compiler performs:

## Phase 1 — Memory Analysis

The compiler calculates:

- every global variable;
- every scope;
- every variable offset;
- every record size;
- every default value;
- every method reference.

The result is saved as:

```
Memory Print
```

---

## Phase 2 — AST Generation

The source code is converted into bytecode.

All names are replaced by resolved addresses.

Example:

Source:

```
value = 5
```

becomes:

```
copy :(x+6) :5
```

The runtime never performs symbol lookup.

---

# Runtime Scope Instantiation

A scope signature is not an allocated memory block.

It is a template.

Entering a scope:

```
base = end
end += scope_size
```

Leaving:

```
end = previous_end
```

No variable allocation occurs.

No records are individually created.

The whole scope exists because its layout was already calculated.

---

# Recursion

Recursive calls work naturally.

A function has:

```
scope template
```

not a fixed address.

Each call receives a different runtime base:

```
call 1
base = 100


call 2
base = 150
```

Both use the same template but different memory regions.

---

# Global Memory

The global scope is special.

It is:

- created once;
- never released;
- always available.

Local scopes are temporary stack-like regions.

Globals and locals are intended to live in separate address spaces so that
scope destruction never affects permanent data.

---

# Addressing Model

Addresses are represented as:

```
x + y
```

where:

- `x` = runtime scope base;
- `y` = compile-time offset.

Three operations exist:

| Syntax | Meaning |
|-|-|
| `x` | address |
| `'x` | raw byte |
| `:x` | structured value |

Example:

```
x       -> record address
'x      -> dim byte
:x      -> stored value
```

---

# Arrays

Arrays reuse the same variable record.

There is no separate array type.

An array is:

```
one record
+
multiple equally sized value cells
```

Example:

```
5 cells
2 bytes each
```

means:

```
value length = 10 bytes
```

---

## Constant Index

Known indexes are resolved at compile time.

Example:

```
array[5]
```

becomes:

```
base + fixed offset
```

---

## Variable Index

Dynamic indexing uses:

```
array_base + index * stride
```

where:

- array base is known;
- index is read from memory;
- stride comes from the record dimension.

All elements must have identical structure.

This includes attached methods.

---

# Methods

Variables may optionally contain references to methods.

Current prototype:

- supports the storage format;
- stores a method address;
- allows future object-like behavior.

The final compiler resolves methods at compile time.

A variable therefore becomes:

```
data
+
behavior reference
```

without requiring a traditional object system.

---

# Configurable Metadata

The memory format is not fixed.

Parameters such as:

```c
byte_for_dim
byte_for_scope
byte_for_method_length
byte_for_value_length
```

define the size of metadata fields.

Changing them changes the available address space without changing the model.

---

# Design Goals

The final system aims for:

- compile-time memory planning;
- zero runtime symbol resolution;
- deterministic addresses;
- stack-like scope allocation;
- recursive functions;
- compact byte storage;
- pointer-free internal addressing;
- movable memory buffers.

Because addresses are offsets instead of real pointers, the underlying memory
buffer can grow using `realloc()` without invalidating stored addresses.

---

# Current Limitations

The prototype still lacks:

- parser;
- compiler;
- Memory Print generator;
- AST generator;
- virtual machine;
- symbol table;
- error propagation;
- bounds checking;
- multiple methods per variable;
- dynamic arrays.

These are future layers built on top of the memory model.

---

# Philosophy

The project explores a simple question:

> What if memory layout was decided before execution instead of during execution?

By reducing variables to:

```
metadata + bytes
```

and moving complexity into compilation, the runtime can become extremely small.

The goal is not to replace existing languages, but to experiment with a
different foundation where memory itself is the primitive abstraction.
