#!/usr/bin/env bash
# Portable models only; no running emulator, engine build, or installation.
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
cd "$ROOT"
OUT="${1:?Pass a new output directory}"
JSON_INCLUDE="${JSON_INCLUDE:?Set to an installed directory containing json.hpp}"
mkdir -p "$OUT"
OUT="$(cd "$OUT" && pwd)"
# Preserve upstream expectations and run them unchanged before new tests.
bash tests/disk_contents/run.sh "$OUT/upstream"
V="$ROOT/src/Emulators/vice"
SUPPORT="$ROOT/tests/disk_contents/support"
FLAGS=(-std=c++17 -g -O1 -ffunction-sections -fdata-sections)
if [[ "${DISK_CONTENTS_SANITIZE:-0}" == 1 ]]; then
 FLAGS+=(-fsanitize=address,undefined -fno-omit-frame-pointer)
fi
INCLUDES=(-I"$SUPPORT" -I"$V/root" -I"$V/diskimage" -I"$V/vdrive" -I"$V/drive" -I"$V/fileio" -I"$V/arch" -I"$V/lib/p64" -I"$V/ViceInterface/Adapters" -I"$ROOT/src/DebugInterface/C64")
"${CXX:-c++}" "${FLAGS[@]}" "${INCLUDES[@]}" -c tests/disk_directory/model_test.cpp -o "$OUT/model.o"
"${CXX:-c++}" "${FLAGS[@]}" -Wl,--gc-sections "$OUT/model.o" "$OUT/upstream/"{adapter,parser,gcr,diskimage,fsimage,fsimage-gcr,fsimage-dxx,fsimage-check}.o -o "$OUT/model_test"
timeout 30 "$OUT/model_test" "$OUT" > "$OUT/model.log" 2>&1
cat "$OUT/model.log"
"${CXX:-c++}" "${FLAGS[@]}" -pthread -Itests/disk_directory/support -I"$SUPPORT" -I"$JSON_INCLUDE" -Isrc/DebugInterface -I"$V/ViceInterface" -Isrc/Remote tests/disk_directory/request_test.cpp src/DebugInterface/CDebugInterfaceTask.cpp -o "$OUT/request_test"
timeout 10 "$OUT/request_test" > "$OUT/request.log" 2>&1
cat "$OUT/request.log"
python3 tests/disk_directory/registration_test.py
bash tests/disk_directory/run_controls.sh "$OUT/controls"
