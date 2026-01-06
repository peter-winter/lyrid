# Lyrid Translator Overhaul: Memory Layout and Instruction Production Plans

## Overview

The redesigned translator adopts a fully static, register-free memory model to exploit the language's single-assignment semantics, fixed array lengths (known at translation time), and absence of user-defined control flow. All storage is pre-allocated in two contiguous pools, enabling predictable offsets and minimal runtime overhead. Execution consists of a linear sequence of memory moves and external function calls, with results written directly to pre-allocated locations.

The translation process comprises two primary phases:
- Memory Layout Phase: Compute fixed offsets for all values and annotate the AST accordingly.
- Instruction Production Phase: Emit instructions in post-order, referencing allocated offsets.

## Memory Pools

Two separate, contiguous buffers form the program's memory image:

- int_memory: A vector of int64_t containing all integer scalars, all integer array elements, and all span metadata (pairs of int64_t: data offset into the target pool + fixed length), regardless of whether the span describes an integer or float array.
- float_memory: A vector of double containing all float scalars and all float array elements.

No padding or alignment is required. Pools are sized exactly to fit allocated data, with literals hoisted directly and unknown slots (awaiting function results) initialized with sentinels for debugging.

Spans are uniformly represented as two consecutive int64_t values in int_memory. The pool for data interpretation is derived from the expression's inferred_type_.

Span metadata (data offset and length) is fully resolved and hoisted during the layout phase and remains immutable at runtime. External functions that return arrays write only to the pre-allocated data buffer; they do not modify span metadata.

## Storage Representation

- Scalars: Direct offset into the derived pool (int_memory for int, float_memory for float).
- Arrays: Span metadata offset in int_memory (pointing to the offset + length pair). The data buffer starts at the loaded data offset in the target pool.
- Annotations: Embedded in AST nodes producing values:
  - Literals: Direct scalar offset in pool-specific field.
  - Array constructions/comprehensions: span_offset_in_int_memory_.
  - Function calls: result_storage_ (scalar_offset or span_offset wrapper).
  - Index access: scalar_offset_in_memory_ (static case) or handled via indirect instruction (dynamic case).
  - Declarations and references: Derived from initializer annotations.

Array references (including copies such as `b = a`) are handled by propagating the same span offset during layout; no runtime duplication of metadata occurs.

## Memory Layout Phase

Performed via recursive traversal of the AST (post-order to ensure subexpressions are allocated first):

1. Allocate Scalars:
   - For literal scalars: Append to the appropriate pool and record offset in the node.
   - For function call results (scalar return): Reserve a slot in the derived pool and record offset.

2. Allocate Arrays:
   - Compute fixed length (from type or deduction).
   - Reserve contiguous data buffer in the target pool.
   - Reserve span pair in int_memory: first slot for data offset (set to buffer start), second for length (hoisted from type).
   - Record span_offset_in_int_memory_ in the producing node.
   - For constructions with known elements: Hoist literals directly into buffer positions.
   - For partial unknowns: Initialize known positions; sentinel-fill others.

3. Resolve Static Indexing:
   - If base span and index literal are resolved: Compute direct element offset and annotate scalar_offset_in_memory_.

4. Sharing:
   - Array reference copies and uses propagate the same span offset (no new allocation).

The phase produces a complete, fixed-size memory image with all offsets and immutable span metadata resolved.

## Instruction Production Phase

Instructions are emitted in post-order (innermost expressions first) to ensure results are available before use:

1. Literals and Fully Static Expressions:
   - No instructions emitted (values hoisted during layout).

2. Copies/Moves:
   - Emit move_scalar for scalar copies to new slots (when not fully shared).

3. Function Calls:
   - Prepare arguments: Reference allocated offsets/scalars/spans directly.
   - Emit call with input args_ (derived from subexpression annotations) and mandatory return_ pointing to pre-allocated result storage.
   - Nested calls: Inner results referenced directly as arguments (no intermediate moves).
   - Array-returning calls: External function fills only the pre-allocated data buffer; span metadata is already complete.

4. Array Initialization with Runtime Elements:
   - Emit move_scalar to place call results into buffer positions (if not direct).

5. Indexing:
   - Static cases: Treated as direct offset reference (no instruction).
   - Dynamic cases: Emit move_scalar_indirect.

The resulting program contains only necessary moves and calls, executed sequentially by the virtual machine.

## Handling Specific Constructs

- Nested Calls: Each call receives a pre-allocated result slot. Inner results are passed by direct offset reference to outer calls.
- Mixed Known/Unknown Arrays: Known elements hoisted; runtime results placed via moves into reserved positions.
- Comprehensions: Treated as array constructions with computed length (longest source); elements filled via generated moves/calls (future implementation).
- Index Access:
  - Static Case (index is literal or fully resolvable): Resolved during layout to a direct scalar offset (scalar_offset_in_memory_); no runtime instruction needed.
  - Dynamic Case (index from runtime value, e.g., call result): Base span is always static (pre-allocated data offset + length). Emit a move_scalar_indirect instruction:
    struct move_scalar_indirect
    {
        pool pool_;                          // Target data pool (int_pool or float_pool)
        size_t span_offset_in_int_memory_;   // Offset to span pair (data_offset + length)
        size_t index_offset_in_int_memory_;  // Offset to runtime index scalar (int64_t)
        size_t dst_offset_;                  // Destination scalar offset in the specified pool
    };
    - Runtime behavior: Load data offset from span, load index value, compute address (data_offset + index), copy element to dst_offset.
    - Pool is encoded statically during emission for assembly self-sufficiency.
    - Bounds checking optional (using loaded length).
    - The instruction is fully self-contained; the virtual machine requires no AST or type information to execute it correctly.
- Array Producers and Consumers:
  - Array-returning function calls are array producers: a dedicated buffer and span pair are pre-allocated for each such call.
  - The span metadata (data offset and length) is fully hoisted and immutable.
  - The external function writes only element values into the buffer.
  - All consumers (declarations, outer call arguments, index bases, comprehension sources) reference the same pre-allocated span offset directly via static propagation.
  - No runtime span metadata copying occurs.

## Planned Optimizations

- Dead move elimination: Remove redundant scalar copies (e.g., fully static initializers).
- Direct placement: Inline call results into array buffers where possible.
- Sharing propagation: Maximize reference reuse for scalars and arrays.

This design ensures deterministic, efficient code generation while supporting the language's constraints, including controlled dynamic indexing via a single indirect move instruction. Implementation will proceed incrementally, beginning with layout for literals and simple declarations.
