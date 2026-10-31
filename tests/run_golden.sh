#!/bin/sh
# Golden-file tests: each tests/golden/NAME.rpn is run in batch mode with a
# throw-away config directory; stdout+stderr and the exit status are compared
# with tests/golden/NAME.out.
#   tests/run_golden.sh BIN            run
#   UPDATE=1 tests/run_golden.sh BIN   regenerate the .out files (review the diff!)
BIN=${1:-./bin/release/mm_15}
DIR=$(cd "$(dirname "$0")" && pwd)/golden
TMP=$(mktemp -d)
trap 'rm -rf "$TMP"' EXIT INT TERM
pass=0; fail=0
export LC_ALL=C.UTF-8
for t in "$DIR"/*.rpn; do
  name=$(basename "$t" .rpn)
  rm -rf "$TMP/cfg"
  XDG_CONFIG_HOME="$TMP/cfg" HOME="$TMP" "$BIN" -n -f "$t" > "$TMP/$name.actual" 2>&1
  echo "[exit $?]" >> "$TMP/$name.actual"
  if [ -n "$UPDATE" ]; then
    cp "$TMP/$name.actual" "$DIR/$name.out"
  elif diff -u "$DIR/$name.out" "$TMP/$name.actual" > "$TMP/diff"; then
    pass=$((pass + 1))
  else
    fail=$((fail + 1)); echo "FAIL: $name"; cat "$TMP/diff"
  fi
done
[ -n "$UPDATE" ] && echo "golden files updated" && exit 0
echo "golden: $pass passed, $fail failed"
[ "$fail" -eq 0 ]
