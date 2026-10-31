# MM-15 — Mico's RPN calculator

MM-15 is a programmable, stack-based RPN calculator for the terminal, written in
C17 on top of the GNU Scientific Library and GNU Readline. It handles **real and
complex scalars, strings, and real and complex matrices**, and it is programmable
in the spirit of the HP-41C and FORTH: user-defined words with conditionals and
loops, numerical methods that take any word as the function, and HP-41 style
programs with labels and jumps.

```text
$ mm_15
MM_RPN>> 2 3 + 4 *
[0] ℝ : 20.000000
MM_RPN>> (1,2) (3,-1) *
[0] ℝ : 20.000000
[1] ℂ : (5.000000, 5.000000i)
MM_RPN>> [2 2 $ 4 7 2 6] det
[2] ℝ : 10.000000
MM_RPN>> : f dup * 2 - ;
MM_RPN>> 0 3 @f fzero
[3] ℝ : 1.414214
```

The **[user manual](doc/mm_15-manual.pdf)** (PDF, 27 pages) covers everything
below in detail, with a complete command reference. `man mm_15` and
[COMMANDS.md](COMMANDS.md) are the short forms; inside the calculator,
`help`, `listfcns` and `"name" usage`.

## What it does

- **Atomic errors.** Every token either succeeds or leaves the stack exactly as
  it was; the error is printed and the rest of the line is skipped. A user word
  that fails half-way leaves no trace. `undo` restores the previous line's stack.
- **One promotion rule.** `-1 sqrt` is `(0,1)`, `2 asin` is `(1.5708,1.3170)`,
  `-8 3 inv ^` is `(1,1.7321)`; a real matrix with one such entry becomes a
  complex matrix. Division by zero is an error, not `inf`.
- **Matrices** via GSL: products, elementwise operators (`.* ./ .^`), inverse,
  pseudoinverse, determinant, eigen-, Cholesky and singular value
  decompositions, Kronecker product, reshaping and joining, column/row
  statistics, polynomial roots and evaluation. Real and complex matrices mix.
- **Words**, compiled at definition time, with `if/else/then`, `begin/until`,
  `begin/while/repeat`, `do/loop/+loop` and `i`/`j`; recursion allowed.
- **Numerical methods** over any word or command: `integrate` (tanh-sinh with
  adaptive bisection), `romberg`, `fzero` and `solve` (Brent), `minimize`,
  `deriv` (Ridders), `fsum`, `fprod`. Cross-checked against SciPy.
- **Strings, registers, dates and the sun**: 64 registers of any type; date
  arithmetic; sunrise, sunset and civil dawn/dusk for any location, with IANA
  time zones resolved for the date in question.
- **HP-41 style programs** from files (`LBL`, `GOTO`, `GOSUB`, `RTN`, tests that
  skip the next line).
- **Batch mode**: `-e EXPR`, `-f FILE`, or piped stdin; exit status 1 on any
  error, so it works in shell scripts and in the project's own golden tests.
- Instant startup; one dispatch table drives dispatch, help, completion, the man
  page's command list and the manual's reference appendix.

## A taste

```text
: fac dup 1 gt if dup 1 - fac * then ;
10 fac                                   \ 3628800
: sum 0 swap 1 + 1 do i + loop ;
100 sum                                  \ 5050

0 pi @sin integrate                      \ 2
: rsqrt sqrt inv ;
0 1 @rsqrt integrate                     \ 2   (endpoint singularity, exact)
3 @sin solve                             \ 3.141592653590
: par 3 - dup * 1 + ;
0 5 @par minimize                        \ 3

[2 2 $ 4 7 2 6] minv pm                  \ | 0.6 -0.7 | / | -0.2 0.4 |
[1 3 $ 1 -3 2] roots pm                  \ | (1,0) (2,0) |
[2 2 $ 1 2 3 4] [2 2 $ 1 (0,1) 0 1] +    \ complex matrix

"1.1.2025" "31.12.2025" ddays            \ 364
"21.6.2026" 46.24 14.36 "Europe/Ljubljana" sunset     \ "20:58"
today 100 dateplus durham dawn           \ location macros carry their zone
```

```sh
mm_15 -e "2 10 ^"                        # batch: prints the final stack, exits 1 on error
echo "[2 2 $ 1 2 3 4] det" | mm_15 -n    # -n: ignore saved words and config
```

## Input in one paragraph

Tokens are separated by whitespace and all input is case sensitive. Numbers:
`3`, `-1.2e-4`, `.5`. Complex: `(1,2)`. Strings: `"text"` with `\" \\ \n \t`.
Matrices: `[rows cols $ elements...]`, or `[rows, cols, "file"]` to read one
from a file. `\` and `#` start comments. `@name` pushes an execution token for
the numerical methods. A line starting with `!` runs a shell command and pushes
its output.

## Build, test, install

Requires a C17 compiler, GNU make, GNU Readline and GSL
(`apt install build-essential libreadline-dev libgsl-dev`, or
`brew install gsl readline`).

```sh
make                 # release build in bin/release/
make test            # .t cases + golden files, under AddressSanitizer
make install         # ~/.local: bin/mm_15, man page, manual and docs; seeds ~/.config/mm_15
make uninstall
make MODE=asan       # or ubsan, harden
make docs            # regenerate COMMANDS.md and doc/commands.tex from the dispatch table
make manual          # rebuild doc/mm_15-manual.pdf (needs LuaLaTeX; LATEX=xelatex also works)
```

`PREFIX=/usr/local make install` installs system-wide. Configuration, saved words,
macros and registers live in `$XDG_CONFIG_HOME/mm_15` (default `~/.config/mm_15`).

## Files

| | |
|---|---|
| `src/`, `include/` | the calculator; `src/ops.c` is the dispatch table |
| `data/` | default `config.txt`, `predefined_macros.txt`, sample programs |
| `doc/manual.tex` → `doc/mm_15-manual.pdf` | the user manual |
| `mm_15.1`, `MANUAL.md`, `COMMANDS.md`, `CHANGES.md` | man page, quick reference, generated command list, changelog |
| `tests/` | unit cases and golden files |

MM-15 is free software under the GNU GPL, version 3 or later.
