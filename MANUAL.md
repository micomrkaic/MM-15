# MM-15 manual
## RPN Calculator for real and complex scalars and matrices.
A stack-based Reverse Polish Notation (RPN) calculator written in C. Supports real and complex numbers, strings, and matrices (real and complex).

## Quick Start -- Entry rules
- All inputs are case sensitive.
- Enter complex numbers as in: (1,3) or (-1.2e-4, 0.7e2)
- Enter matrices as [#rows #cols $ matrix_elements]. 
- Entries can be complex; #rows and #cols are integers.
- Read matrix from file as [#rows, #cols, "filename"] where #cols and #rows are integers and "filename" is the name of the file (must be in quotes).
- Inline matrices entered using J-like syntax [#rows #cols $ list or entries]
- Recall previous commands with up-arrow
- Comments: `\` or `#` to the end of the line
- Execution tokens: `@name` pushes a token for a command or word (for `integrate`, `fzero`, `exec`, ...)
- Registers: `value n sto`, `n rcl` (HP order)
- Strings: `"text"` with `\"`, `\\`, `\n`, `\t` escapes

## Features

- ✅ Real and complex number support
- ✅ Entry by means of GNU Readline with TAB completion and full history support
- ✅ String operations:  concatenate, upper<->lower case, string length, evaluate strings
- ✅ Real and complex matrices using [GSL](https://www.gnu.org/software/gsl/)
- ✅ Stack-based computation with visitor traversal
- ✅ User-defined functions and variables
- ✅ Registers for storage
- ✅ Math functions: trigonometric, exponential, logarithmic, hyp[erbolic, special functions...
- ✅ Matrix functionality: addition, multiplication, inversion, division
- ✅ Special matrices: identity, constant, random, Gaussian random
- ✅ Linear algebra: inverse, determinant, eigenvalues, SVD, pseudo inverse, cholesky
- ✅ Matrix statistics: means, sums, and variances by rows or columns
- ✅ GNU Readline support for command history and editing
- ✅ Normal pdf, cdf, quantiles
- ✅ Polynomials: zeros and evaluations
- ✅ Numerical integration (tanh-sinh), root finding and minimisation (Brent), derivatives (Ridders), sums and products over any word, via `@name` execution tokens
- ✅ Statistics (colmean, rowmean,...); regression; normpdf, normcdf, rand, nrand
- ✅ Constants: pi, e, gravity
- ✅ Date functions: difference between two dates, date + days, day of the week
- ✅ User defined words with FORTH syntax, compiled at definition time, with `if/else/then`, `begin/until`, `begin/while/repeat`, `do/loop/+loop`
- ✅ Predefined (interpreted) macros, including  NPV, IRR, ... Get the full list with `listmacros`.
- ✅ Programmability a la HP-41C with labels, jumps, and subroutines (programs in files)
- ✅ Atomic errors: a failing token rolls the stack back; batch mode with exit status

## Build Instructions

```
make            # bin/release/mm_15
make test       # under AddressSanitizer
make install    # ~/.local (PREFIX=/usr/local for system-wide); then: man mm_15
```

## Requirements
- C compiler (gcc or clang, C17 standard with limited POSIX extensions)
- GNU make
- GNU readline (libreadline-dev)
- GNU Scientific Library (libgsl-dev)

## Future additions and improvements
- Nicer printing wiwth a build in pager
- Built in model estimation: OLS, GLS, GMM, ML, etc. (Though this could be implemented already by users with programming features.)

# Annex I: Command reference

The complete list of commands with stack pictures, descriptions and
examples is generated from the program itself and kept in
[COMMANDS.md](COMMANDS.md) (`make docs` regenerates it; `listfcns` prints
the names by category and `"name" usage` describes one). The man page
(`man mm_15`) covers input syntax, words, control flow, numerical methods
and programs.
