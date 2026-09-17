#!/usr/bin/env bash
#
# Checker for the vendored-emulator reference registry.
#
# Modelled on ~/develop/MTEngineSDL/tests/test-imgui-patches.sh. Three checks:
#
#   1. a file carrying a [C64D-REFERENCE-*] marker has a row in
#      src/Emulators/REFERENCE_FILES.md
#   2. a file with a row carries the marker
#   3. THE ONE WITH TEETH: a file marked REFERENCE-ONLY has not grown a LIVE
#      SDL library call
#
# (3) is the whole point. The other two keep the registry honest; (3) catches a
# future atari800/VICE upgrade quietly reintroducing SDL usage into a file
# everybody believes is inert -- which would then be missed by the next SDL
# port, because nobody reads 140 files.
#
# "Live SDL call" is deliberately narrow, for the reason recorded in
# REFERENCE_FILES.md: a raw `grep -c SDL_` over this tree returns 2026 and is
# worthless, because it counts VICE's menu macros and atari800's own
# SDL_-prefixed module globals. We strip comments first, then match only
# SDL_CamelCase( -- SDL's own naming convention -- and exclude the emulators'
# SDL_UPPERCASE_ identifiers.

set -uo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"
cd "$PROJECT_DIR"

REGISTRY="src/Emulators/REFERENCE_FILES.md"
DIRS=(src/Emulators/atari800/sdl src/Emulators/vice/arch)
MARKER_REF="[C64D-REFERENCE-ONLY]"
MARKER_LIVE="[C64D-REFERENCE-LIVE-EXCEPTION]"

fail=0
note() { printf '%s\n' "$*"; }
bad()  { printf 'FAIL: %s\n' "$*"; fail=1; }

if [ ! -f "$REGISTRY" ]; then
    bad "registry not found: $REGISTRY"
    exit 1
fi

marked=0
registered=0

for d in "${DIRS[@]}"; do
    while IFS= read -r path; do
        has_marker=0
        is_live=0
        if head -12 "$path" | grep -qF "$MARKER_REF"; then has_marker=1; fi
        if head -12 "$path" | grep -qF "$MARKER_LIVE"; then has_marker=1; is_live=1; fi

        in_registry=0
        if grep -qF "\`$path\`" "$REGISTRY"; then in_registry=1; fi

        [ "$has_marker" = 1 ] && marked=$((marked + 1))
        [ "$in_registry" = 1 ] && registered=$((registered + 1))

        # 1 + 2: marker and registry must agree
        if [ "$has_marker" = 1 ] && [ "$in_registry" = 0 ]; then
            bad "$path carries a marker but has no row in $REGISTRY"
        fi
        if [ "$has_marker" = 0 ] && [ "$in_registry" = 1 ]; then
            bad "$path has a registry row but no marker"
        fi
        if [ "$has_marker" = 0 ] && [ "$in_registry" = 0 ]; then
            bad "$path is neither marked nor registered -- new vendored file? decide live vs reference and record it"
        fi

        # 3: a REFERENCE-ONLY file must contain no live SDL library call.
        if [ "$has_marker" = 1 ] && [ "$is_live" = 0 ]; then
            # LC_ALL=C: two of these vendored files contain non-UTF-8 bytes,
            # and without it sed aborts with "RE error: illegal byte sequence"
            # -- printing nothing, which this check would then read as "no live
            # SDL calls". A checker that silently passes on the files it cannot
            # read is worse than no checker.
            hits=$(LC_ALL=C sed 's://.*::' "$path" \
                   | LC_ALL=C perl -0777 -pe 's{/\*.*?\*/}{}gs' \
                   | grep -oE '\bSDL_[A-Z][a-zA-Z0-9]*\(' \
                   | sort -u | tr '\n' ' ')
            if [ -n "$hits" ]; then
                bad "$path is marked REFERENCE-ONLY but contains LIVE SDL calls: $hits"
            fi
        fi
    done < <(find "$d" -type f \( -name '*.c' -o -name '*.h' \) | sort)
done

note "checked: $marked marked file(s), $registered registry row(s)"

if [ "$fail" = 0 ]; then
    note "PASS: reference-file markers and registry agree, and no marked file has a live SDL call"
else
    note "FAILED"
fi
exit $fail
