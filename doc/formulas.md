# Formula formatting

Out of laziness to implement an actual formula parser and executor, I came up with my own formula language and interpreter to allow the program to receive different equations to process.

## Buffers and the BufferHead

The system works similarly to brainfuck: it operates on an array of variables, used as buffers. A pointer (the BufferHead) is initialized to point to the first variable. All operations act relative to the BufferHead.

The pointer is moved one variable at a time with the instructions '<' and '>'.

Values can be duplicated to nearby variables with the instructions '[' and ']'.

## Clipboard

In cases where values need to be moved to distant variables without overwritting contents in between, the clipboard is used. It is an auxiliar variable that is not part of the Buffers.

The clipboard can be written to and read from with the instructions ',' and ';' respectively.

## Arguments

In the context of this program, there are two arguments that the function accepts: `y` and `t`. Writting either of these caracters not preceeded by the constant character will result in the saving of the argument value to the current variable.

## Numeric literals

To load literal values into the current variable, the number can be written in plain text.

## Operations

All operations take either one (A) or two (B, A) arguments, these being `B = Buffers\[BufferHead-1\]` and `A = Buffers\[BufferHead\]`.

The currently defined operations are:
Sym | Argument count | Operation
-- | -- | --
\+ | 2 | Addition (A = B + A)
\- | 2 | Subtraction (A = B - A)
\* | 2 | Multiplication (A = B * A)
/ | 2 | Division (A = B / A)
^ | 2 | Power (A = B ^ A)
\{ | 1 | Square root (A = sqrt(A))
\$ | 1 | Logarithm base e (A = log_e(A))
\# | 1 | Absolute (A = \|A\|)
( | 1 | Sine (A = sin(A))
) | 1 | Cosine (A = cos(A))
\\ | 1 | Tangent (A = tan(A))

## Constants

The system also provides access to mathematical constants. To acces their value, prefix the constant letter with `_`. Ex: `_e` = Euler's number.

**Note:** Constante letters are case sensitive.

Letter | Constant
-- | --
e | Euler's number
p | Pi

## Functions

For more advanced operations or for legibility, you can use mathematical functions. They can be called with the following syntax: `=funcName`.

**Note:** Function names are case sensitive.

The currently supported functions are:
Op | Args | funcName | Name(s)
-- | -- | -- | --
V | 1 | sin | Sine
V | 1 | cos | Cosine
V | 1 | tan | Tangent
| 1 | asin | Inverse sine, arcsin, sin⁻¹
| 1 | acos | Inverse cosine, arccos, cos⁻¹
| 1 | atan | Inverse tangent, arctan, tan⁻¹
| 1 | sinh | Hyperbolic sine
| 1 | cosh | Hyperbolic cosine
| 1 | tanh | Hyperbolic tangent
| 1 | asinh | Inverse hyperbolic sine, arsinh, sinh⁻¹
| 1 | acosh | Inverse hyperbolic cosine, arcosh, cosh⁻¹
| 1 | atanh | Inverse hyperbolic tangent, artanh, tanh⁻¹
V | 1 | abs | Absolute value, modulus
V | 2 | pow | Exponentiation
V | 1 | ln | Natural logarithm
| 1 | log | Decimal logarithm, common logarithm
| 1 | log2 | Binary logarithm
| 1 | sign | Sign, signum, sgn
| 1 | ceil | Ceiling
| 1 | floor | Floor
| 1 | round | Round

