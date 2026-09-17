#!/bin/bash
#
# Compile c64d's MSL shaders to .metallib and embed them in committed headers.
#
# c64d MUST be a single executable -- it embeds its fonts, icons and ROMs into
# src/Embedded/*.h precisely for that -- so a .metallib sitting beside the
# binary is exactly what this app exists to avoid. Precompiling also moves
# shader syntax errors from a user's first launch to our build.
#
# Reuses MTEngineSDL's tools/bin2header.py rather than carrying a second copy:
# the header format is shared, and two generators would drift.
#
# Usage:
#   ./tools/embed-metal-shaders.sh          regenerate
#   ./tools/embed-metal-shaders.sh --check  fail if stale (CI/build)

set -euo pipefail

REPO="$(cd "$(dirname "$0")/.." && pwd)"
ENGINE="$REPO/../MTEngineSDL"
SHADER_DIR="$REPO/platform/MacOS/shaders"
OUT_DIR="$REPO/src/Tools/Shaders/Generated"
CHECK_ONLY=false

[ "${1:-}" = "--check" ] && CHECK_ONLY=true

if [ ! -f "$ENGINE/tools/bin2header.py" ]; then
    echo "embed-metal-shaders: MTEngineSDL not found at $ENGINE" >&2
    exit 1
fi

mkdir -p "$OUT_DIR"
TMP_DIR="$(mktemp -d)"
trap 'rm -rf "$TMP_DIR"' EXIT

status=0
for src in "$SHADER_DIR"/*.metal; do
    [ -e "$src" ] || continue
    name="$(basename "$src" .metal)"
    out="$OUT_DIR/${name}Metallib.h"

    if [ "$CHECK_ONLY" = true ]; then
        if [ ! -f "$out" ]; then
            echo "STALE: $out does not exist" >&2
            status=1
            continue
        fi
        want="$(shasum -a 256 "$src" | cut -d' ' -f1)"
        have="$(grep -o 'k[A-Za-z0-9_]*SourceSha256 = "[0-9a-f]*"' "$out" | grep -o '"[0-9a-f]*"' | tr -d '"')"
        if [ "$want" != "$have" ]; then
            echo "STALE: $out was generated from a different $name.metal" >&2
            echo "       run ./tools/embed-metal-shaders.sh and commit the result" >&2
            status=1
        fi
        continue
    fi

    # -ffast-math is NOT passed: the CRT port is judged against its GLSL
    # original, and relaxed floating point shows up as a small colour shift.
    xcrun -sdk macosx metal -std=macos-metal2.0 -c "$src" -o "$TMP_DIR/$name.air"
    xcrun -sdk macosx metallib "$TMP_DIR/$name.air" -o "$TMP_DIR/$name.metallib"
    python3 "$ENGINE/tools/bin2header.py" "$name" "$src" "$TMP_DIR/$name.metallib" "$out"
done

if [ "$CHECK_ONLY" = true ] && [ $status -eq 0 ]; then
    echo "embed-metal-shaders: all generated headers are up to date"
fi
exit $status
