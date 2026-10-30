#!/usr/bin/env bash
# Focused portable checks only.  No application or dependency build.
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
OUT="${1:?Pass a new evidence directory}"
[[ ! -e "$OUT" ]] || { echo "Refusing to overwrite evidence: $OUT" >&2; exit 2; }
mkdir -p "$OUT"
OUT="$(cd "$OUT" && pwd)"
JSON_INCLUDE="${JSON_INCLUDE:?Set the directory containing json.hpp}"
"${CXX:-c++}" -std=c++17 -Wall -Wextra -Werror -I"$ROOT/tests/disk_directory/support" -I"$ROOT/src/Remote" -I"$JSON_INCLUDE" "$ROOT/tests/gui_control/protocol_test.cpp" -o "$OUT/protocol_test"
"$OUT/protocol_test" > "$OUT/schema-cases.json"
python3 - "$OUT/schema-cases.json" <<'PY'
import json, sys
from jsonschema import Draft7Validator
data = json.load(open(sys.argv[1]))
Draft7Validator.check_schema(data["schema"])
validator = Draft7Validator(data["schema"])
for case in data["cases"]:
    assert validator.is_valid(case["args"]) == case["valid"], case
print(f'PASS: production parser and JSON Schema agree on {len(data["cases"])} positive/negative cases')
PY
git -C "$ROOT" diff --check
