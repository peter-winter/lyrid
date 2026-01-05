# Lyrid Language Reference

## Overview
Lyrid is a simple, expression-oriented language for defining global variables initialized with constant expressions involving scalars, array literals, function calls, indexing, and array comprehensions. The language has no statements, control flow, or built-in arithmetic operators—computations are performed via external function calls.

All declarations are global. Functions are not defined in Lyrid itself; they are provided externally and may perform arbitrary computations, including allocation and return of arrays.

## Types
- Scalar types:
  - `int` — 64-bit integer
  - `float` — double-precision floating point
- Array types:
  - `int[]` — array of `int`
  - `float[]` — array of `float`

Arrays are passed and assigned by reference (no deep copying). Array creation occurs via array literals, comprehensions, or external functions that return arrays.

## Syntax Overview
A program consists of zero or more declarations, each on its own line:

```
type identifier = expression
```

Declarations are terminated by end-of-line (no semicolon required).

## Declarations
```bnf
declaration ::= type identifier '=' expression
```

Example:
```
int[] result = map(add, numbers, deltas)
float avg = divide(sum, count)
```

The expression type must exactly match the declared type.

## Expressions
Supported expressions:
- Integer literals
- Float literals
- Variable references
- Function calls
- Array indexing
- Array literals
- Array comprehensions

No operator expressions (e.g., no `+`, `-`, `*`, `/`).

### Literals
- Integer: `-123`, `0`, `456`
- Float: `-1.23`, `4.56e-7`, `0.0`

### Variable References
An identifier referring to a previously declared variable.

### Function Calls
```bnf
function_call ::= identifier '(' arg_list? ')'
arg_list      ::= expression (',' expression)*
```

Arguments are passed by value for scalars and by reference for arrays. The function must be registered externally with a matching prototype (argument count, types, and return type).

### Array Indexing
```bnf
indexing ::= expression '[' expression ']'
```

Base must be an array type; index must be `int`. Result type is the element scalar type.

### Array Literals
```bnf
array_literal ::= '[' expression_list? ']'
expression_list ::= expression (',' expression)*
```

All elements must have the same scalar type (`int` or `float`). Empty literals are forbidden.

Example:
```
int[] vals = [1, 2, 3, 4]
float[] coeffs = [0.1, 0.5, -2.0]
```

### Array Comprehensions
```bnf
comprehension ::= '[' '|' var_list '|' 'in' '|' source_list '|' 'do' expression ']'

var_list     ::= identifier (',' identifier)*
source_list  ::= expression (',' expression)*
```

- Number of variables must equal number of source expressions.
- Each source expression must be an array.
- Variables are bound to corresponding elements of the source arrays (nested loops, outermost source corresponds to first variable).
- Variables must be unique within the comprehension.
- The `do` expression must yield a scalar.
- Result is a new array of the corresponding array type.

Examples:
```
int[] squares = [ |x| in |numbers| do mul(x, x) ]
int[] products = [ |a, b| in |xs, ys| do mul(a, b) ]
```

## Array Semantics
- Array literals produce constant arrays known at translation time.
- Arrays from comprehensions or function returns may have runtime-determined sizes.
- Assignment and parameter passing transfer references (no copy).
- Indexing is supported in parsing and semantic analysis but not yet in code generation.
- Comprehensions are not yet supported in code generation.

## Restrictions
- No empty array literals.
- No mixed-type array elements.
- No arithmetic or logical operators.
- All variables are global and immutable (single declaration per name).
- Redeclaration of a name is an error.
- External function prototypes must be registered before translation.
