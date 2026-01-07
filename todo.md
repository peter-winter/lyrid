# Lyrid Translator Overhaul: Memory Layout and Instruction Production Plans

## Overview

The redesigned translator adopts a fully static, register-free memory model to exploit the language's single-assignment semantics, fixed array lengths (known at translation time), and absence of user-defined control flow. All storage is pre-allocated in two contiguous pools, enabling predictable offsets and minimal runtime overhead. Execution consists of a linear sequence of scalar moves, indirect scalar loads, and external function calls, with results written directly to pre-allocated locations.

The translation process comprises two primary phases:
- Memory Layout Phase: Compute fixed offsets for all values and annotate the AST accordingly.
- Instruction Production Phase: Emit instructions in post-order, referencing allocated offsets.

## Memory Pools

Two separate, contiguous buffers form the program's memory image:

- int_memory: A vector of int64_t containing all integer scalars, all integer array elements, and all span metadata (pairs of int64_t: data offset into the target pool + fixed length), regardless of whether the span describes an integer or float array.
- float_memory: A vector of double containing all float scalars and all float array elements.

No padding or alignment is required. Pools are sized exactly to fit allocated data, with literals hoisted directly and unknown slots (awaiting function results or indirect loads) initialized with sentinels for debugging.

Spans are uniformly represented as two consecutive int64_t values in int_memory. Span metadata (data offset and length) is fully resolved and hoisted during the layout phase and remains immutable at runtime. External functions write only to pre-allocated data buffers.

The pool for data interpretation is derived from the expression's inferred_type_.

## Storage Representation

- Scalars: Direct offset into the derived pool (int_memory for int, float_memory for float).
- Arrays: Span metadata offset in int_memory (pointing to the offset + length pair). The data buffer starts at the loaded data offset in the target pool.
- Annotations: Embedded in AST nodes producing values:
  - Literals: Direct scalar offset in pool-specific field.
  - Array constructions/comprehensions: span_offset_in_int_memory_.
  - Function calls: result_storage_ (scalar_offset or span_offset wrapper).
  - Index access: storage_ (direct_storage or indirect_storage with offset).
  - Declarations and references: Derived from initializer annotations via recursive resolution.

Array references are handled by propagating the same span offset during layout; no runtime duplication occurs.

## Memory Layout Phase

The layout phase is implemented as a single function `layout_exprs(ast::program& prog_ast)` containing recursive lambdas for mutual recursion between layout and resolution.

- Full traversal allocates result slots for all value-producing subexpressions.
- Literals are hoisted directly (standalone or into array buffer positions).
- Function calls allocate return slots (scalar or array buffer + immutable span) after laying out arguments.
- Array constructions allocate buffer + immutable span, hoisting literal elements directly and allocating separate result slots for non-literals.
- Index access allocates base and index, resolves span, and annotates storage_ (direct elem_off for static literal index; indirect temp slot for dynamic).
- Symbol references are resolved recursively via declaration lookup.
- Sentinels mark runtime-filled slots.

The phase produces a complete, fixed-size memory image with all offsets resolved and annotations set.

## Instruction Production Phase (Approach)

The instruction production phase will mirror the layout phase structure, using similar recursive lambdas to traverse the AST post-layout.

1. Traverse declarations and expressions in post-order.
2. For each value-producing node:
   - If result is hoisted literal: No instruction.
   - If non-hoisted placement needed (e.g., non-literal array element): Emit move_scalar from subexpression result to buffer position.
   - For function calls: Emit call with:
     - Input args_ referencing argument result offsets/scalars/spans.
     - Mandatory return_ referencing pre-allocated result storage.
   - For dynamic index access: Emit move_scalar_indirect using base span offset, index result offset, element pool, and temp dst offset.
   - Static index access: Direct reference (no instruction).

The resulting program will contain only necessary scalar moves, indirect loads, and calls, executed sequentially by the virtual machine.

## Handling Specific Constructs

- Nested Calls: Arguments laid out first; results referenced directly in outer call.
- Mixed Known/Unknown Arrays: Literals hoisted; runtime results moved into buffer.
- Comprehensions: Future implementation (treated as array constructions).
- Index Access:
  - Static (literal index chain): Direct element offset.
  - Dynamic: Indirect load to temp slot (optional move to final position).
- Array Producers: Calls returning arrays allocate dedicated buffer + immutable span; function fills buffer only.
- Consumers: Reference same span offset directly.

## Planned Future Work

- Implement instruction emission phase with recursive traversal.
- Add dead move elimination and direct placement optimizations.
- Support comprehensions (generate moves/calls for element computation).
- Implement virtual machine to execute the generated program.
- Add optional runtime bounds checking for indirect loads.
- Extend to support mutable stores if language evolves.

This design ensures deterministic, efficient code generation while supporting the language's constraints. Implementation is proceeding incrementally, with memory layout complete and instruction phase next.
