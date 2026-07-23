# Size-Based Memory Model (C prototype)

This is a small experimental prototype exploring an alternative way to manage
variables in a manually allocated memory buffer, in C.

The core idea is to reason about memory in terms of **size**, rather than
**type**. Instead of declaring a variable as `int`, `char`, etc., a variable
is described purely by how many bytes it needs to hold its value. The
allocator doesn't know or care what a value represents, only how big it is.

## Status

This is a skeleton, not a library. It's a working proof of concept meant to
show how a size-based memory record could be laid out and manipulated by
hand, not a production-ready allocator. There is no parser, no language, no
public API — just a single `main.c` demonstrating the mechanism.

## How it works

Variables are stored sequentially in a single flat `__uint8_t *memory`
buffer, allocated once via `begin()`. Each variable is represented as a
contiguous record with the following layout:

```
[ scope (2B) ][ dim / next address (2B) ][ value length (1B) ][ value (N bytes) ][ optional: method length (2B) + method address(es) ]
```

- **scope** — identifies which scope the variable belongs to (e.g. global vs
  main scope).
- **dim** — the address where the next variable's record begins. This turns
  the buffer into a de facto linked structure without pointers, since each
  record knows where the following one starts.
- **value length** — how many bytes are reserved for the value itself. This
  is decided once, at first declaration, either explicitly or automatically
  (based on the smallest number of bytes needed to represent the initial
  value).
- **value** — the actual bytes of the value.
- **method section (optional)** — a variable can optionally carry a
  reference to a "method": an address pointing into a list of predefined
  routines (resolved at compile time) that can be applied to that variable.
  This is scaffolding for future array/object-like behavior more than a
  finished feature.

Variables are placed one after another using a bump-pointer style allocator:
a static `end` cursor tracks where the next free slot starts, and each call
to `initialize_variable` either appends a new variable there or, if an
address is passed explicitly, overwrites/reassigns an existing one.

A key design decision: once a variable's size is set, it never grows. If you
reassign a variable with a value that needs more bytes than originally
reserved, the operation fails instead of silently reallocating or shifting
memory. This keeps the layout predictable and avoids the complexity of
moving every subsequent record whenever one variable grows.

## Why bother with this

Reasoning about memory this way is mostly useful as a learning exercise: it
forces you to think about what a "variable" actually is at the byte level,
stripped of any language-level type system. It's also a rough sketch of how
a very small interpreted language might lay out its own variables in memory
without leaning on the host language's type system — deciding, essentially,
"this thing needs N bytes" and building everything else (scoping, method
dispatch, arrays) on top of that single primitive.

Arrays, for instance, are meant to reuse the exact same record structure:
a vector of 5 cells of 2 bytes each is just a variable whose value length is
`cells * cell_size`, with the value bytes conceptually grouped into `cells`
chunks of `cell_size` bytes when read back. No separate array type is
needed.

## Design goals (not yet implemented)

The current code is a minimal demonstration of the record layout and the
bump allocator. The intended design, once built out, goes further:

- **Growable memory, not a fixed cap.** Because every address in this model
  is an offset into `memory` rather than a raw C pointer, the underlying
  buffer can be grown with `realloc` without invalidating any address
  already stored anywhere. This is one of the main reasons offsets were
  chosen over pointers in the first place: the buffer is free to move in
  physical memory as it grows, since nothing outside `begin()` ever holds a
  real pointer into it.
- **Scope exit reclaims memory by moving `end` back.** When a scope is
  exited, the variables declared inside it are dropped from the list and
  `end` is simply restored to the value it had before entering that scope.
  No memory is copied or shifted — going out of scope is just moving a
  cursor backward.
- **Separate address space for globals.** Variables that must outlive their
  scope (globals) don't live in the same region as local variables. At
  compile time, the maximum memory needed for locals is computed by
  counting the declarations in the scope with the most declarations, which
  fixes a ceiling address. Global variables are placed above that ceiling.
  This means exiting a scope never has to shift or touch the globals — only
  the local `end` cursor moves, since locals and globals never share
  address space.
- **Field sizes are parameters, not hardcoded limits.** `byte_for_dim`,
  `byte_for_scope`, `byte_for_method_lenght` and similar constants are
  meant to be tunable. The addressable space of a given deployment is a
  configuration choice, not a hard ceiling baked into the record format.

## Caveats

- No error recovery beyond printed messages yet — errors are logged but not
  propagated to a caller in any structured way. This still needs work.
- Some parts of the allocation logic are not yet optimized; performance
  hasn't been a priority while the design is still being figured out.
- Method support currently allows only a single method per variable.
- No real symbol table yet — declarations are just calls to
  `initialize_variable` from `main()`.
- The growable-buffer, scope-reclaim, and global/local address split
  described above are design decisions, not yet reflected in this code —
  the current `main.c` still uses a single fixed-size buffer with no
  reclaim.

This is a work in progress, kept intentionally minimal while the underlying
model is still being figured out.
