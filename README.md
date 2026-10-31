# MM-15 — Mico’s RPN Calculator

**MM-15** is a programmable, stack-based Reverse Polish Notation (RPN) calculator written in C.

It supports **real and complex scalars**, **strings**, and **real/complex matrices**, with a programming model inspired by **HP-41C**, **FORTH**, and numerical systems like **MATLAB / Octave** — but with instant startup and interactive control.

MM-15 is built for users who:
- think in stacks rather than syntax trees
- want interactive numerical power without a full scripting language
- value composability, transparency, and speed

---

## Key Characteristics

- Stack-based RPN evaluation; every token is atomic (an error rolls the stack back)
- Real **and** complex arithmetic, with one promotion rule: `-1 sqrt` is `(0,1)`,
  `2 asin`, `-8 3 inv ^` and `-100 log` promote the same way
- Matrix and linear-algebra support (via GNU GSL)
- User-defined words compiled at definition time, with `if`/`do`/`begin` control flow
- Numerical integration, root finding, minimisation, derivatives and sums over
  any word: `0 pi @sin integrate`
- HP-41 style programs (labels, jumps, subroutines) loaded from files
- A batch mode (`-e`, `-f`, stdin) with a meaningful exit status
- Instant startup, REPL-centric workflow; one dispatch table drives help,
  completion, `usage` and the generated [COMMANDS.md](COMMANDS.md)
- Written in portable C17 with minimal POSIX extensions

---

## 30-Second Demo

```text
$ mm_15
MM-15 2.0

> 2 3 +
5

> (1,2) (3,-1) *
(5,5)

> : f dup * 2 - ;
> 0 3 @f fzero
1.414214

$ mm_15 -e "[2 2 $ 4 7 2 6] det"
[0] ℝ : 10.000000
```

---

## Input Rules (Quick Start)

- Case-sensitive
- Scalars: `3`, `-1.2e-4`
- Complex numbers: `(re,im)`
- Inline matrices: `[rows cols $ elements...]`
- Matrix from file: `[rows cols "filename"]`
- Command history via ↑ / ↓
- TAB completion supported

---

## Scalars & Complex Numbers

```text
> 3 4 /
0.75

> (1,3) abs
3.16228

> 1 2 re2c
(1,2)

> (1,2) conj
(1,-2)
```

---

## Matrices & Linear Algebra

```text
> [2 2 $ 1 2 3 4]
> [2 2 $ 1 2 3 4] [2 2 $ 5 6 7 8] *
> [2 2 $ 4 7 2 6] dup det
10
> inv
```

---

## Statistics & Probability

```text
> 0 1 npdf
0.398942

> 0 1 ncdf
0.5

> 0 1 nquant 0.975
1.95996
```

---

## Dates, Time & Sunrise

Dates are strings in `d.m.yyyy` form. `today` and `now` read the clock.

```text
> today
"25.09.2026"

> now
"14:07"

> today 100 dateplus
"03.01.2027"

> "1.1.2025" "1.1.2026" ddays
365
```

Sunrise, sunset, and civil dawn/dusk take a date, latitude, longitude, and
a time zone. The zone can be a UTC offset in hours **or an IANA zone name**;
with a zone name the offset is resolved *for the date on the stack*, so DST
is never your problem:

```text
> "21.6.2026" 46.24 14.36 "Europe/Ljubljana" sunset
"21:37"

> "21.12.2026" 46.24 14.36 "Europe/Ljubljana" sunset
"16:54"

> "1.12.2026" "America/New_York" tz_offset
-5
```

The shipped `predefined_macros.txt` includes location macros that carry
their zone, so planning a drive is one line:

```text
> today 100 dateplus durham dawn
"06:58"
```

Zone names are validated against the system tzdata (`/usr/share/zoneinfo`).

---

## Words and control flow

```text
> : fac dup 1 gt if dup 1 - fac * then ;
> 10 fac
3628800

> : sum 0 swap 1 + 1 do i + loop ;
> 100 sum
5050
```

Words are compiled when defined, so a typo is reported immediately; a word
may call itself (nesting stops at 256); redefining a word replaces it.
Control words inside definitions: `if else then`, `begin until`,
`begin while repeat`, `do loop +loop` with `i` and `j`. `"fac" usage` shows a
definition; `savewords` keeps them between sessions.

## Numerical methods

```text
> 0 1 @exp integrate            \ 1.718281828459
> : rsqrt sqrt inv ;
> 0 1 @rsqrt integrate          \ 2 — endpoint singularity, exact
> 3 @sin solve                  \ 3.141592653590
> : par 3 - dup * 1 + ;
> 0 5 @par minimize             \ 3
> 2 @exp deriv                  \ 7.389056098931
> 1 1000 @rsqrt fsum
```

`@name` pushes an execution token for any word or command. `integrate` is
tanh-sinh with adaptive bisection, `fzero`/`solve`/`minimize` are Brent's
methods, `deriv` is Ridders' extrapolation. `1e-6 tol` sets the tolerance;
`nevals` and `interr` tell you what the last call did.

## HP-41 style programs

A program is a file with one instruction per line:

```text
LBL loop
dup 1 leq?
GOTO end
dup 1 - *
GOTO loop
LBL end
END
```

```text
> "fact.prg" run
```

`LBL`, `GOTO`, `GOSUB`, `RTN`, `END`; a test ending in `?` skips the next
line when false. `"file" batch` runs a file of ordinary lines.

## Batch mode

```text
$ mm_15 -e "2 10 ^"               # one line
$ mm_15 -f script.rpn             # a file ('\' and '#' start comments)
$ echo "1 3 /" | mm_15 -n         # stdin; -n skips your saved words/config
```

The final stack is printed; the exit status is 1 if any error occurred.
This is what the golden-file tests use.

## Errors

Every top-level token is atomic. `[2 2 $ 1 2 2 4] minv` on a singular
matrix reports the error and leaves the matrix on the stack; the rest of
the line is skipped. A word that fails half-way leaves no trace. `undo`
restores the stack as it was before the previous line.

---

## Scripts & Batch Execution

```text
# example.mm
2 3 +
[2 2 $ 1 2 3 4] det
```

```text
> batch "example.mm"
```

---

## Build, test, install

```bash
make                 # release build in bin/release/
make test            # .t cases + golden files, under AddressSanitizer
make install         # ~/.local: bin/mm_15, man page, docs, seed config
make uninstall
make docs            # regenerate COMMANDS.md from the dispatch table
make MODE=asan       # or ubsan, harden
```

`PREFIX=/usr/local make install` installs system-wide. After installing,
`man mm_15`.

---

## Requirements

- C compiler (gcc or clang, C17)
- GNU make
- GNU Readline
- GNU Scientific Library (GSL)

---

## Philosophy

MM-15 favors:
- transparency over abstraction
- composability over syntax
- interactivity over ceremony
