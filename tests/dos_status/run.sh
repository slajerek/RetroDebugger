#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/../.."
engine="${MTENGINE_ROOT:-../MTEngineSDL}"
out="${DOS_STATUS_TEST_OUT:-tests/dos_status/build}"
mkdir -p "$out"
"${CXX:-c++}" -std=c++17 -Wall -Wextra -Werror -pthread ${DOS_STATUS_TEST_FLAGS:-} \
  -Itests/dos_status/support -Isrc/Emulators/vice/ViceInterface \
  -I"$engine/src/Engine/Libs" tests/dos_status/status_test.cpp \
  "$engine/src/Engine/Libs/sgSHA256/sgSHA256.cpp" -o "$out/status_test"
"$out/status_test" assets.roms/dos1541 1541
"$out/status_test" assets.roms/dos1541II 1542
python3 tests/dos_status/extract_enqueue.py src/DebugInterface/CDebugInterface.cpp "$out/enqueue_method.inc"
"${CXX:-c++}" -std=c++17 -Wall -Wextra -Werror -pthread ${DOS_STATUS_TEST_FLAGS:-} \
  -DDOS_STATUS_TEST_REAL_ENQUEUE -I"$out" \
  -Itests/dos_status/support -Isrc/Emulators/vice/ViceInterface \
  -I"$engine/src/Engine/Libs" tests/dos_status/review_test.cpp \
  "$engine/src/Engine/Libs/sgSHA256/sgSHA256.cpp" -o "$out/review_test"
for control in lower-current lower-execution lower-both lower-idle-trap enqueue \
  unknown-mirrored-rom upper-idle-disabled upper-idle-enabled; do
  "$out/review_test" assets.roms/dos1541 "$control"
done
