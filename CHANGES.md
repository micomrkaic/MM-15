# MM-15 changes

## 2.0

Six commits on top of 1.1, each buildable and tested on its own.

### Behaviour you will notice

- **Atomic errors.** Every top-level token snapshots the stack; on error
  the stack is restored, the message goes to stderr, and the rest of the
  line is skipped. A user word is one token. Nothing silently pushes `0`,
  `inf` or a half-result any more; `fzero` no longer "pushes a back".
- **One promotion rule.** `sqrt`, `ln`, `log`, `asin`, `acos`, `acosh`,
  `atanh` and `^` promote a real argument outside the real domain to a
  complex result, and a real matrix with one such entry to a complex
  matrix. Previously `-1 sqrt` promoted but `-8 3 inv ^` gave `nan`.
  Division by zero is an error.
- **`sto` takes `value n sto`** (HP order; 1.x took `n value sto`). The
  `sto_add`/`sto_sub`/`sto_mul`/`sto_div` macros are updated.
- **`substr` takes `s start end substr`** (the string deepest, 0-based,
  end exclusive), matching its own help; 1.x wanted the string on top.
- **`roll` takes its depth from the stack** (`1 roll` = swap, `2 roll` =
  rot); 1.x ignored the argument and always rolled depth 2. `pick` and
  `depth` are new.
- **Numerical methods take `@name`** execution tokens instead of a word
  selected by index with `selword`: `0 pi @sin integrate`,
  `a b @f fzero`. `set_intg_tol`/`set_f0_tol` are replaced by one `tol`.
- **Batch mode.** `-e EXPR`, `-f FILE`, or stdin that is not a terminal:
  no splash, no per-line echo, final stack printed, exit status 1 on any
  error. `-n` skips the saved macros/words/config. `--commands` prints the
  command reference; `--version`.
- **Startup** no longer runs `curl` (1.1), reads config from
  `$XDG_CONFIG_HOME`/`$HOME` at run time (1.1), is silent on a missing
  config/macro file, and autoloads `user_words.txt`.
- **`roots` takes coefficients highest degree first**, like `pval` and
  Matlab. 1.x passed them to GSL in ascending order while `pval` used
  descending, so `[1 3 $ 1 -3 2] roots` gave 0.5 and 1 instead of 1 and 2.
  `pval` accepts a column vector and a complex x; both check their arguments.
- **Wrong argument types are errors, not crashes or leaks.** `join_v`,
  `join_h` and `pinv` segfaulted on a non-matrix; `det`, `eig`, `pval`,
  `roots`, `split_mat`, `cumsum_*`, `join_*`, `dow`, `dateplus`, `ddays`,
  `edmy` and the comparisons leaked their arguments on every call, and some
  thirty commands leaked the argument on a type error. A sweep of all
  built-ins over every combination of argument types, and a random fuzz, now
  run clean under ASan with leak detection.

### Words

- Compiled at definition time: unknown names are rejected immediately,
  control flow becomes branches. `if/else/then`, `begin/until`,
  `begin/while/repeat`, `do/loop/+loop` with `i`/`j` work inside words.
- Redefinition replaces the word; callers see the new body.
- Runaway recursion stops at 256 levels with an error (1.x crashed).
- Names are validated (no built-ins, no control words, no leading digit);
  tables grow dynamically; bodies are unbounded.
- Macros and words share one implementation; both files load in two passes
  (forward references work) and report the line of anything that fails.
- `\` and `#` start comments.

### Numerics (ported from MM-14 2.0, cross-checked against SciPy)

`integrate` (tanh-sinh with adaptive bisection; endpoint singularities at 0
exact), `romberg`, `fzero` (Brent), `solve` (one guess), `minimize`
(Brent), `deriv` (Ridders), `fsum`, `fprod`, `exec`, `tol`, `nevals`,
`interr`. 62 cases against SciPy: worst relative error 3.5e-11.

### Structure

- `src/ops.c` is the one dispatch table: name, minimum depth, function,
  category, stack picture, help, example. Dispatch, the depth check,
  `usage`, `listfcns`, tab completion and `COMMANDS.md` come from it. The
  250-line `strcmp` chain, `function_list.c` and `help.c`'s table are gone.
- `errors.c`: `mm_error()` replaces ~470 `fprintf(stderr)`/`perror` sites
  and counts errors; that count drives rollback and the exit status.
- `numerics.c` replaces `integration_and_zeros.c`.
- New built-ins: `conj` (was listed but not implemented), `pow` alias,
  `%`, `%t`, `%chg`, `mod`, `fact`, `ncr`, `npr`, `pick`, `depth`.
  Removed: the never-dispatched lowercase `goto/xeq/lbl/ctr_*/top_*?`
  names (programs use `LBL/GOTO/GOSUB/RTN/END` in files), `selword`,
  `set_intg_tol`, `set_f0_tol`.
- Tests: `tests/run_golden.sh` with `tests/golden/*.rpn` (full stdout +
  stderr + exit status) next to the existing `.t` cases; both run under
  ASan in `make test`. `make golden` regenerates the `.out` files.
- Repo: `README.html`, `README_MM-15.html`, `camel_2_snake.py`,
  `data/bbb`, LaTeX aux files, `.rpn_history` and editor files removed;
  `MANUAL.md` points at the generated `COMMANDS.md`; man page `mm_15.1`;
  `make install` installs the man page and docs, `make uninstall` removes
  them.

## 1.1

- `5 sto` read past the stack (ASan abort); fixed.
- A self-calling word overflowed the C stack; nesting now stops at 256.
- Config paths resolved at run time instead of `-DHOME_DIR` at compile time.
- Startup no longer runs `curl` against ipify/ip-api/wttr.in.
