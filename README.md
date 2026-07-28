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
