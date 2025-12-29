# MM-15 — Mico’s RPN Calculator

**MM-15** is a programmable, stack-based Reverse Polish Notation (RPN) calculator written in C.

It supports **real and complex scalars**, **strings**, and **real/complex matrices**, with a programming model inspired by **HP-41C**, **FORTH**, and numerical systems like **MATLAB / Octave** — but with instant startup and interactive control.

MM-15 is built for users who:
- think in stacks rather than syntax trees
- want interactive numerical power without a full scripting language
- value composability, transparency, and speed

---

## Key Characteristics

- Stack-based RPN evaluation
- Real **and** complex arithmetic
- Full matrix and linear-algebra support (via GNU GSL)
- Programmable control flow (labels, jumps, counters)
- User-defined words and macros
- Instant startup, REPL-centric workflow
- Written in portable C17 with minimal POSIX extensions

---

## 30-Second Demo

```text
$ ./mm15
MM-15 v1.0

> 2 3 +
5

> (1,2) (3,-1) *
(5,5)

> 5 dup * 1 -
24
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

## Programming Model

```text
> lbl fact
> dup 1 leq?
> goto end
> dup 1 - fact *
> end
```

```text
> 5 fact
120
```

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

## Build Instructions

```bash
git clone https://github.com/micomrkaic/MM-s-Toy-Calculator
cd MM-s-Toy-Calculator
make
```

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
