#!/usr/bin/env bash
# Portable review controls.  Run against current product or explicit preserved
# bodies; all generated code, disk fixtures, and logs stay in a fresh OUT.
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
cd "$ROOT"
OUT="${1:?Pass a new output directory}"
JSON_INCLUDE="${JSON_INCLUDE:?Set to an installed directory containing json.hpp}"
[[ ! -e "$OUT" ]] || { echo "Refusing to replace evidence: $OUT" >&2; exit 2; }
mkdir -p "$OUT"
OUT="$(cd "$OUT" && pwd)"
PREPARE=("$OUT")
[[ -z "${QUEUE_SOURCE:-}" ]] || PREPARE+=(--queue-source "$QUEUE_SOURCE")
[[ -z "${READER_SOURCE:-}" ]] || PREPARE+=(--reader-source "$READER_SOURCE")
python3 tests/disk_directory/prepare_controls.py "${PREPARE[@]}"
V="$ROOT/src/Emulators/vice"
ADAPTER="${ADAPTER_SOURCE:-$V/ViceInterface/Adapters/CDataAdapterViceDrive1541DiskContents.cpp}"
FLAGS=(-g -O1 -ffunction-sections -fdata-sections)
if [[ "${DISK_CONTENTS_SANITIZE:-0}" == 1 ]]; then
 FLAGS+=(-fsanitize=address,undefined -fno-omit-frame-pointer)
fi
INCLUDES=(-I"$OUT/backend-support" -Itests/disk_directory/support -I"$JSON_INCLUDE" -I"$OUT"
 -I"$V/root" -I"$V/diskimage" -I"$V/vdrive" -I"$V/drive" -I"$V/drive/iec" -I"$V/core" -I"$V/rtc"
 -I"$V/arch" -I"$V/lib/p64" -I"$V/ViceInterface/Adapters" -I"$V/ViceInterface"
 -Isrc/DebugInterface/C64 -Isrc/DebugInterface -Isrc/Remote)
for unit in root/gcr diskimage/diskimage diskimage/fsimage diskimage/fsimage-gcr diskimage/fsimage-dxx diskimage/fsimage-check drive/driveimage; do
 "${CC:-cc}" -std=c99 "${FLAGS[@]}" "${INCLUDES[@]}" -c "$V/$unit.c" -o "$OUT/$(basename "$unit").o"
done
"${CC:-cc}" -std=c99 "${FLAGS[@]}" "${INCLUDES[@]}" -c tests/disk_directory/support/drive_reload.c -o "$OUT/drive_reload.o"
"${CXX:-c++}" -std=c++17 "${FLAGS[@]}" "${INCLUDES[@]}" -D_CDataAddressEditBoxDiskContents_h_ -c "$ADAPTER" -o "$OUT/adapter.o"
"${CXX:-c++}" -std=c++17 "${FLAGS[@]}" "${INCLUDES[@]}" -D_CDebugInterfaceC64_H_ -c src/DebugInterface/C64/CDiskImageD64.cpp -o "$OUT/parser.o"
"${CXX:-c++}" -std=c++17 "${FLAGS[@]}" "${INCLUDES[@]}" -pthread -c tests/disk_directory/backend_control_test.cpp -o "$OUT/backend.o"
"${CXX:-c++}" "${FLAGS[@]}" -pthread -Wl,--gc-sections "$OUT/"{backend,adapter,parser,gcr,diskimage,fsimage,fsimage-gcr,fsimage-dxx,fsimage-check,driveimage,drive_reload}.o -o "$OUT/backend_test"
"${CXX:-c++}" -std=c++17 "${FLAGS[@]}" -pthread -Itests/disk_directory/support/queue -Itests/disk_directory/support -I"$OUT" -I"$JSON_INCLUDE" -Isrc/DebugInterface -I"$V/ViceInterface" tests/disk_directory/queue_failure_test.cpp src/DebugInterface/CDebugInterfaceTask.cpp -o "$OUT/queue_test"
FAILED=0
for mode in insert format perform-format routing; do
 if timeout 30 "$OUT/backend_test" "$mode" "$OUT" > "$OUT/$mode.log" 2>&1; then
  tail -1 "$OUT/$mode.log"
 else
  cat "$OUT/$mode.log"
  FAILED=1
 fi
done
if timeout 10 "$OUT/queue_test" > "$OUT/queue.log" 2>&1; then
 cat "$OUT/queue.log"
else
 cat "$OUT/queue.log"
 FAILED=1
fi
exit "$FAILED"
