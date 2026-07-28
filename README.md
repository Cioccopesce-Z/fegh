# Size-Based Memory Model (C Prototype)

This repository contains a small experimental prototype exploring an alternative
memory model based on **record size rather than language types**.

The current implementation is written in C as a proof of concept, but its real
purpose is to validate the memory model that will eventually power a small
compiled/interpreted programming language.

Instead of treating variables as `int`, `char`, `float`, etc., every variable is
represented simply by the number of bytes required to store its value. The
compiler (or, in this prototype, the allocator) only reasons about memory
layout—not about high-level types.

---

# Current Status

This is **not** a library or a general-purpose allocator.

It is a minimal prototype demonstrating how variables can be stored inside a
flat byte buffer using fixed-size records.

The long-term project extends this idea into a compiler and virtual machine
where:

- the entire memory layout is computed at compile time;
- all variable addresses are resolved before execution;
- runtime allocation becomes little more than moving a cursor.

The current C implementation demonstrates only the low-level record layout.

---

# Variable Record

Every variable occupies a contiguous record inside a flat byte buffer.

```
[ scope (2B) ][ dim (2B) ][ value length (1B) ][ value (N bytes) ][ optional methods ]
```

where

- **scope** identifies the owning scope.
- **dim** is the total size of the record.
  It also acts as the stride (or "next record" offset) allowing records to be
  traversed without linked pointers.
- **value length** stores the size of the value section.
- **value** contains the raw bytes representing the stored value.
- **methods (optional)** are compile-time references to predefined routines
  associated with the variable.

The memory model intentionally separates **metadata** from the stored value.

---

# Memory Layout

Variables are stored sequentially inside one flat `uint8_t* memory` buffer.

Allocation uses a simple bump allocator:

```text
base = end;
end += record_size;
```

Records are never moved after creation.

A variable's record size is immutable.

If a reassignment would require more bytes than originally reserved, the write
fails instead of relocating memory.

Keeping record sizes fixed allows every address computed at compile time to
remain valid throughout execution.

---

# Address Model

The future language uses **relative addresses** rather than absolute pointers.

Every address has the form

```
x + y
```

where

- `x` is the runtime base address of the current scope.
- `y` is the compile-time offset inside that scope.

The language defines three addressing operators:

| Notation | Meaning |
|----------|---------|
| `x` | address itself |
| `'x` | raw byte stored at address `x` |
| `:x` | structured value stored in the record |

Example:

```
x       = address
'x      = dim
'(x+1)  = length
:x      = value
```

Since only the scope base changes at runtime, every internal offset remains
constant.

---

# Compile-Time Memory Mapping

The final language does **not** allocate variables one by one while executing.

Instead, the compiler performs two independent tasks.

## 1. Memory Print

The compiler builds the complete memory layout of the program:

- global variables
- every function signature
- every scope signature
- offsets of every variable

The result is saved as a **Memory Print**, containing the byte template of every
scope.

Each scope template already includes:

- record sizes,
- offsets,
- default values,
- attached methods.

## 2. AST Bytecode

Separately, the compiler generates a bytecode where every symbolic reference has
already been replaced with its corresponding relative address.

For example,

```
eta = 5
```

becomes something equivalent to

```
copy :(x+6) :5
```

No name lookup is performed at runtime.

---

# Scope Instantiation

A scope signature is **not** an allocated block of memory.

It is only a template.

Entering a scope performs one operation:

```text
base = current_end;
current_end += scope_size;
```

Leaving the scope simply restores the previous value of `current_end`.

```text
current_end = previous_end;
```

No variable-by-variable allocation occurs.

No heap management is required.

Because every invocation receives a different runtime base address, recursive
calls naturally create independent scope instances while reusing the same
compile-time template.

The global scope (`main`) is instantiated once at startup and never reclaimed.

---

# Arrays

Arrays reuse exactly the same record structure.

An array is simply a value composed of multiple equally sized cells.

```
let values : cell_size : cell_count = ...
```

Constant indexing is resolved entirely at compile time.

```
array[5]
```

becomes a fixed offset.

Variable indexing becomes

```
:( (x+y) + :(index) * 'array )
```

where `'array` is the stride stored inside the record.

All elements of an array must share the same structure (including attached
methods), ensuring a constant stride.

---

# Why This Model?

The goal is to reduce runtime memory management to almost nothing.

Instead of allocating individual variables during execution, the compiler
already knows:

- every variable,
- every record size,
- every scope size,
- every offset,
- every parameter layout.

Runtime execution therefore consists mostly of:

- loading the memory templates,
- instantiating scopes,
- executing bytecode.

No symbol lookup.

No variable allocation.

Almost no address computation.

---

# Future Design Goals

The current prototype demonstrates only the record layout.

The complete implementation is intended to support:

- growable memory using `realloc()` while keeping offsets valid;
- compile-time generated Memory Print files;
- compile-time generated AST bytecode;
- automatic scope instantiation from templates;
- scope destruction by rewinding the allocation cursor;
- separate regions for global and local memory;
- configurable metadata sizes (`scope`, `dim`, etc.);
- compile-time method resolution;
- recursive function calls without special runtime handling.

---

# Current Limitations

The prototype is intentionally minimal.

Currently it lacks:

- a parser;
- a compiler;
- the Memory Print generator;
- the AST bytecode generator;
- a virtual machine;
- a symbol table;
- structured error handling;
- bounds checking;
- multiple methods per variable.

These components belong to the next stage of the project.

The present code exists solely to validate the underlying memory model before
building the compiler and runtime around it.
