#!/bin/bash
# Fails if a [SKIPPED-TEST: name] marker has no row in tests/SKIPPED_TESTS.md,
# or a row has no marker. Same shape as MTEngineSDL's test-imgui-patches.sh --
# a skipped test is invisible, so the registry is the only thing that remembers.
set -u
cd "$(dirname "$0")/.."
REG=tests/SKIPPED_TESTS.md
[ -f "$REG" ] || { echo "FAIL: $REG missing"; exit 1; }

markers=$(grep -rho "\[SKIPPED-TEST: [A-Za-z0-9_]*\]" src/ 2>/dev/null \
          | sed 's/.*: //; s/\]//' | sort -u)
rows=$(grep -oE '^\| `[A-Za-z0-9_]+`' "$REG" | tr -d '|` ' | sort -u)

rc=0
for m in $markers; do
  echo "$rows" | grep -qx "$m" || { echo "FAIL: marker '$m' has no row in $REG"; rc=1; }
done
for r in $rows; do
  echo "$markers" | grep -qx "$r" || { echo "FAIL: row '$r' has no marker in src/"; rc=1; }
done

n=$(echo "$markers" | grep -c . || true)
[ $rc -eq 0 ] && echo "PASS: $n skipped test(s), markers and registry agree"
exit $rc
