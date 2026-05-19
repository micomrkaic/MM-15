#!/usr/bin/env bash
#
# MM-15 test harness
# ------------------
# Runs every *.t case in tests/cases/ against the calculator binary and
# checks the final printed scalar against an expected value.
#
# Why this design (read before changing it):
#   The binary is a REPL that prints a splash screen and re-prints the WHOLE
#   stack after every command. Diffing raw transcripts would be hopelessly
#   brittle: any precision tweak, splash edit, or extra stack line breaks
#   every test at once. Instead each case ends with `print`, which emits ONE
#   recognizable line ("[N] <type> : <value>"). We extract the LAST such line
#   and compare just the value, with float tolerance. That isolates "did the
#   computation produce the right answer" from cosmetic output noise.
#
# A .t case file format (see tests/cases/*.t for examples):
#   # comment lines start with #
#   #EXPECT: <expected value>      <- exactly one, anywhere in the file
#   #TOL: <abs tolerance>          <- optional, default 1e-9
#   <calculator input lines...>    <- fed to the binary on stdin
# The harness automatically appends `print` then `q`, so cases should NOT
# include them.
#
# Exit status: 0 if all pass, 1 if any fail (suitable for CI / make).

set -u

HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO="$(cd "$HERE/.." && pwd)"
CASES_DIR="$HERE/cases"

# Which binary to test. Default to the ASan build so memory bugs surface
# as part of the normal test run (this is the whole point — every bug we
# fixed would have been caught here). Override with BIN=... make test.
MODE="${MODE:-asan}"
BIN="${BIN:-$REPO/bin/$MODE/mm_15}"

if [[ ! -x "$BIN" ]]; then
  echo "ERROR: binary not found or not executable: $BIN" >&2
  echo "Build it first, e.g.:  make MODE=$MODE all" >&2
  exit 2
fi

# Isolate config/history so tests never touch the user's real ~/.config/mm_15
# and never load the user's macros/words (which would make tests
# machine-dependent). HOME is redirected to a throwaway dir.
SANDBOX="$(mktemp -d)"
trap 'rm -rf "$SANDBOX"' EXIT
mkdir -p "$SANDBOX/.config/mm_15"
# Empty macro/word files so the binary's startup load finds nothing.
: > "$SANDBOX/.config/mm_15/predefined_macros.txt"
: > "$SANDBOX/.config/mm_15/user_words.txt"

# ASan: don't abort the whole run on first leak; we want every test to run.
# detect_leaks=1 keeps leak detection on so a regression still fails the case.
export ASAN_OPTIONS="detect_leaks=1:exitcode=1:abort_on_error=0"
export UBSAN_OPTIONS="halt_on_error=1:print_stacktrace=1"

pass=0
fail=0
failed_names=()

# Extract the numeric value from the last "[N] <type> : <value>" line.
# Handles real ([..] ℝ : 42), the value may be complex/string — for those
# the expected value is compared as a raw string instead.
extract_last_value() {
  # Last line containing the " : " result separator produced by `print`.
  local line
  line="$(grep -E '^\[[0-9]+\] .+ : ' "$1" | tail -n 1)"
  # Strip everything up to and including the first " : ".
  printf '%s' "${line#*: }"
}

# Numeric compare with tolerance. Returns 0 if |a-b| <= tol.
num_close() {
  awk -v a="$1" -v b="$2" -v t="$3" 'BEGIN{
    d = a - b; if (d < 0) d = -d;
    exit (d <= t) ? 0 : 1
  }'
}

shopt -s nullglob
cases=( "$CASES_DIR"/*.t )
if (( ${#cases[@]} == 0 )); then
  echo "No test cases found in $CASES_DIR" >&2
  exit 2
fi

echo "MM-15 test harness"
echo "binary : $BIN"
echo "cases  : ${#cases[@]}"
echo "------------------------------------------------------------"

for case_file in "${cases[@]}"; do
  name="$(basename "$case_file" .t)"

  expect="$(grep -E '^#EXPECT:' "$case_file" | head -n1 | sed 's/^#EXPECT:[[:space:]]*//')"
  tol="$(grep -E '^#TOL:' "$case_file" | head -n1 | sed 's/^#TOL:[[:space:]]*//')"
  tol="${tol:-1e-9}"
  allow_leak_reason="$(grep -E '^#ALLOW_LEAK:' "$case_file" | head -n1 | sed 's/^#ALLOW_LEAK:[[:space:]]*//')"

  if [[ -z "$expect" ]]; then
    echo "SKIP $name  (no #EXPECT: line)"
    continue
  fi

  # Build the stdin: the case body (minus directive lines), then print, then q.
  input="$(grep -vE '^#(EXPECT|TOL):' "$case_file" | grep -vE '^#')"
  out_file="$SANDBOX/$name.out"

  printf '%s\nprint\nq\n' "$input" \
    | HOME="$SANDBOX" "$BIN" > "$out_file" 2>&1
  run_status=$?

  # Sanitizer triage:
  #  - Hard memory errors (UAF, overflow, UB) ALWAYS fail, no exceptions.
  #  - A pure leak fails UNLESS the case is marked #ALLOW_LEAK (a tracked,
  #    known issue); then it's a loud warning but not a suite failure, so
  #    the bug stays visible every run without blocking unrelated work.
  hard_err=0
  leak_only=0
  if grep -qE 'ERROR: AddressSanitizer|runtime error:' "$out_file"; then
    hard_err=1
  fi
  if grep -qE 'LeakSanitizer: detected' "$out_file"; then
    leak_only=1
  fi

  if (( hard_err )); then
    fail=$((fail+1)); failed_names+=("$name [sanitizer]")
    echo "FAIL $name  (sanitizer: memory error)"
    grep -E 'ERROR: AddressSanitizer|runtime error:|SUMMARY:' "$out_file" | head -3 | sed 's/^/      /'
    continue
  fi

  if (( leak_only )) && [[ -z "$allow_leak_reason" ]]; then
    fail=$((fail+1)); failed_names+=("$name [leak]")
    echo "FAIL $name  (sanitizer: memory leak)"
    grep -E 'LeakSanitizer: detected|SUMMARY:' "$out_file" | head -2 | sed 's/^/      /'
    continue
  fi

  if (( leak_only )) && [[ -n "$allow_leak_reason" ]]; then
    echo "WARN $name  (known leak allowed: $allow_leak_reason)"
  fi

  actual="$(extract_last_value "$out_file")"

  # Decide numeric vs string comparison based on whether both look numeric.
  if [[ "$expect" =~ ^-?[0-9.eE+-]+$ && "$actual" =~ ^-?[0-9.eE+-]+$ ]]; then
    if num_close "$actual" "$expect" "$tol"; then
      pass=$((pass+1)); echo "PASS $name  ($actual)"
    else
      fail=$((fail+1)); failed_names+=("$name")
      echo "FAIL $name  expected $expect, got '$actual' (tol $tol)"
    fi
  else
    # Non-numeric: exact string match (strings, complex literals, etc.)
    if [[ "$actual" == "$expect" ]]; then
      pass=$((pass+1)); echo "PASS $name  ($actual)"
    else
      fail=$((fail+1)); failed_names+=("$name")
      echo "FAIL $name  expected '$expect', got '$actual'"
    fi
  fi
done

echo "------------------------------------------------------------"
echo "passed: $pass   failed: $fail"
if (( fail > 0 )); then
  printf 'failing: %s\n' "${failed_names[*]}"
  exit 1
fi
exit 0
