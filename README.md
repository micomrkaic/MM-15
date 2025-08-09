# Mico's toy Calculator--MM-RPN-15
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
- ✅ Numerical integration and zero finding for arbitrary continuous functions on closed intervals
- ✅ Statistics (colmean, rowmean,...); regression; normpdf, normcdf, rand, nrand
- ✅ Constants: pi, e, gravity
- ✅ Date functions: difference between two dates, date + days, day of the week
- ✅ User defined words (commands) by means of FORTH-like syntax
- ✅ Predefined (interpreted) macros, including  NPV, IRR, ... Get the full list with `listmacros`.
- ✅ Programmability a la HP-41C with labels, jumps, and subroutines

## Build Instructions

bash
- git clone https://github.com/micomrkaic/MM-s-Toy-Calculator
- cd MM-s-Toy-calculator
- make

## Requirements
- C compiler (gcc or clang, C17 standard with limited POSIX extensions)
- GNU make
- GNU readline (libreadline-dev)
- GNU Scientific Library (libgsl-dev)

## Future additions and improvements
- Nicer printing wiwth a build in pager
- Built in model estimation: OLS, GLS, GMM, ML, etc. (Though this could be implemented already by users with programming features.)

# Annex I: Full Function List (with Descriptions)

## Mathematics

### Basic Functionality

- `.*` – Elementwise multiplication of two matrices or scalars  
- `./` – Elementwise division  
- `.^` – Elementwise power  
- `ln` – Natural logarithm: `ln(x)`  
- `log` – Base-10 logarithm: `log(x)`  
- `exp` – Exponential: `exp(x)`  
- `sqrt` – Square root: `sqrt(x)`  
- `pow` – Power: `x^y`  
- `frac` – Returns the fractional part of a real number: `frac(x)`  
- `intg` – Returns the integer part: `intg(x)`  
- `chs` – Change sign: `-x`  
- `inv` – Multiplicative inverse: `1/x`

### Complex Numbers

- `re` – Real part of complex number  
- `im` – Imaginary part  
- `abs` – Absolute value (modulus)  
- `arg` – Argument (phase angle)  
- `conj` – Complex conjugate  
- `re2c` – Combine two real numbers into a complex  
- `split_c` – Split complex number into real and imaginary parts  
- `j2r` – Join two reals into one complex.

### Trigonometry

- `sin`, `cos`, `tan` – Standard trigonometric functions  
- `asin`, `acos`, `atan` – Inverse trig functions

### Hyperbolic Functions

- `sinh`, `cosh`, `tanh` – Hyperbolic sine, cosine, tangent  
- `asinh`, `acosh`, `atanh` – Inverse hyperbolic functions

---

## Matrices and Linear Algebra

- `minv` – Matrix inverse  
- `pinv` – Pseudo-inverse  
- `det` – Determinant  
- `eig` – Eigenvalues (and eigenvectors?)  
- `tran` – Transpose  
- `reshape` – Change matrix shape  
- `get_aij` – Get element at (i,j)  
- `set_aij` – Set element at (i,j)  
- `split_mat` – Split matrix into elements like a pinata
- `kron` – Kronecker product  
- `diag` – Extract diagonal  
- `to_diag` – Convert vector to diagonal matrix  
- `chol` – Cholesky decomposition  
- `svd` – Singular Value Decomposition  
- `dim` – Dimensions of matrix  
- `eye` – Identity matrix  
- `join_v`, `join_h` – Vertical/horizontal concatenation  
- `cumsum_r`, `cumsum_c` – Cumulative sum (row/col)  
- `ones`, `zeroes` – Matrices of ones/zeros  
- `rand`, `randn` – Uniform/Gaussian random matrix  
- `rrange` – Range vector: like `[start:step:end]`  
- `cmean`, `rmean` – Column/row mean  
- `csum`, `rsum` – Column/row sum  
- `cvar`, `rvar` – Column/row variance  
- `cmin`, `cmax` – Column min/max  
- `rmin`, `rmax` – Row min/max

---

### Polynomials

- `roots` – Find roots of a polynomial  
- `pval` – Evaluate polynomial at x

### Special Functions

- `npdf` – Normal probability density function  
- `ncdf` – Normal cumulative distribution function  
- `nquant` – Quantile function for normal  
- `gamma`, `ln_gamma` – Gamma function and its log  
- `beta`, `ln_beta` – Beta function and its log

---

### Constants

- `gravity` – 9.81 
- `pi` – π 
- `e` – Euler’s constant 
- `inf`, `nan` – IEEE infinity and NaN

---

## Stack Operations

- `drop` – Remove top item 
- `clst` – Clear stack 
- `swap` – Swap top two items 
- `dup` – Duplicate top item 
- `nip` – Remove second item 
- `tuck` – Copy top under second 
- `roll` – Roll 3rd item to top 
- `over` – Copy second to top

---

## String Functions

- `scon` – Concatenate top two strings  
- `s2l`, `s2u` – Lower/upper case  
- `slen` – Length of string  
- `srev` – Reverse string  
- `int2str` – Convert int to string

---

## Register Operations

- `rcl` – Recall from register  
- `sto` – Store to register  
- `pr` – Print registers  
- `saveregs`, `loadregs`, `clregs` – Save/load/clear registers  
- `ffr` – First Free Register

---

## Printing and Format Control

- `print`, `pm`, `ps` – Print stack/item/matrix  
- `setprec` – Set print precision  
- `sfs` – Swap fixed/scientific format

---

## Tests and Logic

- `eq`, `leq`, `lt`, `gt`, `geq`, `neq` – Comparisons  
- `and`, `or`, `not` – Boolean logic

---

# Date Functions

- `ddays` – Date difference in days  
- `today` – Push today’s date  
- `dateplus` – Add days to date  
- `dow` – Day of week  
- `edmy` – Extract day/month/year?

---

## User Defined Words / Macros

- `listwords`, `loadwords`, `savewords`, `delword`, `selword`, `clrwords` – Manage user-defined words  
- `listmacros` – List available macros

---

## Programming

- `top_eq0?`, `top_ge0?`, etc. – Tests for top of stack
- `ctr_eq0?`, `ctr_ge0?`, etc. – Tests on counters
- [`set_ctr`, `clr_ctr`, `ctr_inc`, `ctr_dec` – Counter manipulation]
- `goto`, `xeq`, `rtn`, `end`, `lbl` – Control flow (jump, call, return, etc.) 
- `eval`, `batch`, `run` – Evaluate strings, scripts, or programs

---

## Miscellaneous

- `fuck` – Pretty self descriptive
- `help` – Print help screen
- `listfcns` – List all available functions
- `undo` – Undo the effects of the last line of input
- `clrhist` – Clear history

---

