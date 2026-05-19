# MM-15 test harness

Run it:

```
make test
```

That builds the ASan binary (`make MODE=asan all`) and runs every case in
`tests/cases/*.t` against it. Exit status is nonzero if anything fails, so
it works in CI. Because it tests the ASan build, **every functional test
also gets memory-leak and undefined-behavior checking for free** — a wrong
answer *or* a leak fails the relevant case.

To run against a different build:

```
make MODE=ubsan all && BIN=bin/ubsan/mm_15 bash tests/run_tests.sh
```

## Why it works the way it does

The calculator is a REPL that prints a splash screen and re-prints the
entire stack after every command. Diffing raw transcripts would be
hopelessly brittle — any precision tweak or extra stack line would break
every test at once. So instead each case ends with an automatically
appended `print`, which emits one recognizable line. The harness extracts
the final value from that line and compares it to the expected value with
a floating-point tolerance. This isolates "is the math right" from
cosmetic output noise.

Tests run with `HOME` redirected to a throwaway sandbox, so they never
read or write your real `~/.config/mm_15` and never load your personal
macros/words (which would make results machine-dependent).

## Writing a test case

Create `tests/cases/<name>.t`:

```
# any line starting with # is a comment
#EXPECT: 42            <- required: the expected final value
#TOL: 1e-6             <- optional: abs tolerance for floats (default 1e-9)
6 7 *                  <- calculator input, one or more lines
```

Do **not** put `print` or `q` in the case — the harness appends them.
`#EXPECT:` is compared numerically when both sides look like numbers,
otherwise as an exact string (for string/complex results).

### Known-leak cases

If a case exercises a code path with a *tracked, known* leak that hasn't
been fixed yet, mark it:

```
#ALLOW_LEAK: short reason / tracking note
```

The case still verifies the numeric result and still prints a loud `WARN`
line every run (so the bug stays visible), but the leak alone won't fail
the suite. **Hard memory errors (use-after-free, buffer overflow, UB) are
never allowed and always fail, even with `#ALLOW_LEAK`.** When the
underlying bug is fixed, delete the `#ALLOW_LEAK` line and the case
becomes a regression guard for the fix.

## Currently known issues surfaced by the harness

- `det` (and likely the whole matrix→scalar reduction family) leaks the
  input matrix — ASan reports ~96 bytes / 3 allocations per call. The
  `matrix_det.t` and `matrix_add_det.t` cases carry `#ALLOW_LEAK` and
  assert the numeric result. Remove those markers when the reduction
  path is fixed.
