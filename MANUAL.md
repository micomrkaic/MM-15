# MM-15 quick reference

The full manual is [doc/mm_15-manual.pdf](doc/mm_15-manual.pdf) (also installed
under `share/doc/mm_15`). This page is the one-screen version; the complete
command list with stack pictures is [COMMANDS.md](COMMANDS.md).

## Input

| | |
|---|---|
| numbers | `3` `-1.25` `.5` `1e3` |
| complex | `(1,2)` `(-1.2e-4, 0.7e2)` |
| strings | `"text"` with `\"` `\\` `\n` `\t` |
| matrices | `[rows cols $ a11 a12 ...]` row-major; `[2, 2, "file.txt"]` from a file |
| comments | `\` or `#` to end of line |
| execution token | `@name` — a reference to a command or word, for the numerical methods |
| shell | `!command` pushes the command's output as a string |

All input is case sensitive. The stack is printed after every line:
`[0]` is the bottom, the highest index is the top. `undo` restores the stack
before the previous line; `q` quits.

## Stack

`dup drop swap over nip tuck` · `n pick` (copy depth n) · `n roll` (move depth
n to top; `1 roll` = swap, `2 roll` = rot) · `depth` · `clst` · `print ps pm` ·
`setprec` · `sfs` (fixed ↔ scientific) · `savestack loadstack`

## Arithmetic and functions

`+ - * / ^` on reals, complex and matrices (`*` is the matrix product, `A B /`
is `A B⁻¹`, `A n ^` an integer matrix power); `.* ./ .^` elementwise.
`chs inv abs sqrt intg frac mod fact ncr npr` · `% %t %chg` (HP, keep y) ·
`exp ln log` · `sin cos tan asin acos atan sinh cosh tanh asinh acosh atanh` ·
`re im arg conj re2c split_c j2r` · `npdf ncdf nquant gamma ln_gamma beta ln_beta` ·
`pi e gravity inf nan` · `eq neq lt leq gt geq and or not` (flags 0/1).

Promotion: `-1 sqrt` → `(0,1)`, `2 asin`, `0.5 acosh`, `2 atanh`, `-1 ln`,
`-8 3 inv ^` all return complex. Division by zero is an error. Unary
functions apply elementwise to matrices.

## Matrices

`eye ones zeroes rand randn` · `start step end rrange` · `tran '` · `minv pinv
det eig chol svd kron` · `dim` · `A i j get_aij` · `A i j x set_aij` (0-based) ·
`reshape split_mat diag to_diag join_v join_h` · `csum rsum cmean rmean cvar
rvar cmin rmin cmax rmax` (c = per column → row vector; r = per row → column
vector) · `cumsum_r cumsum_c` · `roots pval` (coefficients highest degree first).

## Strings, registers, dates

`scon` · `s start end substr` · `s2u s2l slen srev int2str` · `"1 2 +" eval`.
`x n sto` · `n rcl` · `pr ffr saveregs loadregs clregs` (64 registers, any type).
Dates are `"d.m.yyyy"`: `today now dateplus ddays dow edmy num2date days2eoy
tz_offset` · `date lat lon zone sunrise|sunset|dawn|dusk` (zone: IANA name or
UTC offset).

## Words

```
: sq dup * ;                      one line; nothing after the ;
: fac dup 1 gt if dup 1 - fac * then ;
: sum 0 swap 1 + 1 do i + loop ;
```
Control words inside definitions: `flag if ... else ... then` ·
`begin ... flag until` · `begin ... flag while ... repeat` ·
`limit start do ... loop` / `n +loop` with `i`, `j`.
`listwords` · `"name" usage` · `"name" delword` · `clrwords` · `savewords` /
`loadwords` (`user_words.txt`, loaded at startup) · `listmacros`.

## Numerical methods

`a b @f integrate` · `a b @f romberg` · `a b @f fzero` · `g @f solve` ·
`a b @f minimize` · `x @f deriv` · `lo hi @f fsum` / `fprod` · `@f exec` ·
`x tol` · `nevals` · `interr`. The function takes x and must leave one real.

## Programs and batch

`"file.prg" run` — one instruction per line: `LBL name`, `GOTO name`,
`GOSUB name`, `RTN`, `END`, and tests (`top_gt?`, `top_eq0?`, ...) that skip
the next line when false. `"file" batch` runs ordinary lines.

`mm_15 -e EXPR` · `-f FILE` · stdin · `-n` (no saved state) · `--commands` ·
`--version`. Exit status 1 if any error occurred.

## Files

`$XDG_CONFIG_HOME/mm_15/` (default `~/.config/mm_15/`): `config.txt`,
`predefined_macros.txt`, `user_words.txt`, `registers.txt`,
`stack_backup.dat`, `.rpn_history`.
