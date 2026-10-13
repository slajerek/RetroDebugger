#!/usr/bin/env bash
# Compile production disk code without the GUI SDK or a running emulator.
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
cd "$ROOT"
OUT="${1:-$(mktemp -d)}"
mkdir -p "$OUT"
OUT="$(cd "$OUT" && pwd)"
V="$ROOT/src/Emulators/vice"
SUPPORT="$ROOT/tests/disk_contents/support"
ADAPTER="$V/ViceInterface/Adapters/CDataAdapterViceDrive1541DiskContents.cpp"
PARSER="$ROOT/src/DebugInterface/C64/CDiskImageD64.cpp"
BASE_FLAGS=()
if [[ -n "${DISK_CONTENTS_BASE_SOURCE:-}" ]]; then
 ADAPTER="$DISK_CONTENTS_BASE_SOURCE/CDataAdapterViceDrive1541DiskContents.cpp"
 PARSER="$DISK_CONTENTS_BASE_SOURCE/CDiskImageD64.cpp"
 BASE_FLAGS=(-DDISK_CONTENTS_BASELINE)
fi
FLAGS=(-g -O1 -ffunction-sections -fdata-sections)
if [[ "${DISK_CONTENTS_SANITIZE:-0}" == 1 ]]; then
 FLAGS+=(-fsanitize=address,undefined -fno-omit-frame-pointer)
fi
INCLUDES=(-I"$SUPPORT" -I"$V/root" -I"$V/diskimage" -I"$V/vdrive" -I"$V/drive" -I"$V/fileio" -I"$V/arch" -I"$V/lib/p64" -I"$(dirname "$ADAPTER")" -I"$(dirname "$PARSER")")
for unit in root/gcr diskimage/diskimage diskimage/fsimage diskimage/fsimage-gcr diskimage/fsimage-dxx diskimage/fsimage-check; do
 "${CC:-cc}" -std=c99 "${FLAGS[@]}" "${INCLUDES[@]}" -c "$V/$unit.c" -o "$OUT/$(basename "$unit").o"
done
"${CXX:-c++}" -std=c++17 "${FLAGS[@]}" "${INCLUDES[@]}" -D_CDataAddressEditBoxDiskContents_h_ -c "$ADAPTER" -o "$OUT/adapter.o"
"${CXX:-c++}" -std=c++17 "${FLAGS[@]}" "${INCLUDES[@]}" -D_CDebugInterfaceC64_H_ -c "$PARSER" -o "$OUT/parser.o"
"${CXX:-c++}" -std=c++17 "${FLAGS[@]}" "${INCLUDES[@]}" "${BASE_FLAGS[@]}" -c tests/disk_contents/CDataAdapterViceDrive1541DiskContents_test.cpp -o "$OUT/test.o"
"${CXX:-c++}" "${FLAGS[@]}" -Wl,--gc-sections "$OUT"/{test,adapter,parser,gcr,diskimage,fsimage,fsimage-gcr,fsimage-dxx,fsimage-check}.o -o "$OUT/sector_views_test"
FAILED=0
MODES=(ordinary errors writes)
if [[ -n "${DISK_CONTENTS_BASE_SOURCE:-}" ]]; then MODES=(ordinary errors); fi
for mode in "${MODES[@]}"; do
 if timeout 30 "$OUT/sector_views_test" "$mode" "$OUT" > "$OUT/$mode.log" 2>&1; then
  tail -1 "$OUT/$mode.log"
 else
  cat "$OUT/$mode.log"
  FAILED=1
 fi
done
exit "$FAILED"
