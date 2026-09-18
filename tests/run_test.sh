#!/bin/bash
#
# CLI Test Runner for RetroDebugger
#
# Usage:
#   tests/run_test.sh [OPTIONS] [TestName] [-- APP_OPTIONS...]
#
# Options:
#   --skip-build    Skip the build step
#   --package       Run the newest release package under platform/*/prod/ from
#                   its own directory (a FINAL build, ./build-macos.sh --prod).
#                   An error when there is none -- never a fallback.
#   --clean-build   Force a clean rebuild before running tests
#   --visible       Run without --headless
#   --imgui         Run all ImGui UI tests (alias: --imgui-tests)
#   --imgui-test FILTER  Run ImGui UI tests matching FILTER
#   --timeout N     Set timeout in seconds (default: 60)
#   --log-dir DIR   Set log output directory (default: /tmp)
#   --layouts-fixture FILE  Copy layout fixture to /tmp and pass via --layouts-file
#
# App options:
#   Pass extra Retro Debugger arguments after --
#   Example: -- --layouts-file tests/data/layouts-test.dat
#
# Examples:
#   tests/run_test.sh                            # Run all suite tests
#   tests/run_test.sh EmulatorStartup            # Run single test
#   tests/run_test.sh --skip-build EmulatorStartup  # Skip build, run single test
#   tests/run_test.sh --imgui-tests              # Run all ImGui UI tests
#   tests/run_test.sh --imgui-test open_all_views  # Run filtered ImGui UI tests
#   tests/run_test.sh --visible LayoutSmoke -- --layouts-file tests/data/layouts-test.dat
#   tests/run_test.sh --layouts-fixture tests/data/layouts-test.dat AutoLayoutPreservation

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"

SKIP_BUILD=false
RUN_PACKAGE=false
CLEAN_BUILD=false
VISIBLE=false
RUN_IMGUI_ALL=false
RUN_IMGUI_FILTER=""
# 300, not 60: the full --run-suite is ~90 s of emulation on the maintainer's
# Mac and much slower on a CI VM, so 60 s TIMEOUT-killed a healthy suite and
# reported whatever had finished (e.g. "72/75") as if it were the result.
# Single-test runs finish in seconds either way, so a generous ceiling costs
# nothing and a tight one silently truncates.
TIMEOUT=900
TEST_NAME=""
LOG_DIR="/tmp"
LAYOUTS_FIXTURE=""
EXTRA_APP_ARGS=()
RESERVED_APP_FLAGS=(--run-test --run-suite --run-tests --run-imgui-test --exit-after-tests --headless --visible)
COPIED_LAYOUTS_FILE=""
COPIED_LAYOUTS_DIR=""
FORWARDED_LAYOUTS_FILE=""
FORWARDED_LAYOUTS_FILE_COUNT=0
KNOWN_LAYOUTS_FIXTURE_PATH="$PROJECT_DIR/tests/data/layouts-test.dat"
COPIED_LAYOUTS_MATCHES_KNOWN_FIXTURE=false

cleanup_temp_layouts_file() {
    if [ -n "$COPIED_LAYOUTS_FILE" ] && [ -f "$COPIED_LAYOUTS_FILE" ]; then
        rm -f "$COPIED_LAYOUTS_FILE"
    fi
    if [ -n "$COPIED_LAYOUTS_DIR" ] && [ -d "$COPIED_LAYOUTS_DIR" ]; then
        rmdir "$COPIED_LAYOUTS_DIR" 2>/dev/null || true
    fi
}

trap cleanup_temp_layouts_file EXIT

copy_layouts_file_to_tmp() {
    local source_path="$1"

    COPIED_LAYOUTS_DIR="$(mktemp -d "/tmp/retrodebugger-layouts.XXXXXX")"
    COPIED_LAYOUTS_FILE="$COPIED_LAYOUTS_DIR/layouts.dat"
    cp "$source_path" "$COPIED_LAYOUTS_FILE"
}

# Parse arguments
while [[ $# -gt 0 ]]; do
    case "$1" in
        --skip-build)
            SKIP_BUILD=true
            shift
            ;;
        --package)
            RUN_PACKAGE=true
            SKIP_BUILD=true
            shift
            ;;
        --clean-build)
            CLEAN_BUILD=true
            shift
            ;;
        --visible)
            VISIBLE=true
            shift
            ;;
        --imgui|--imgui-tests)
            RUN_IMGUI_ALL=true
            shift
            ;;
        --imgui-test)
            if [ $# -lt 2 ]; then
                echo "ERROR: --imgui-test requires a filter"
                exit 2
            fi
            RUN_IMGUI_FILTER="$2"
            shift 2
            ;;
        --timeout)
            if [ $# -lt 2 ]; then
                echo "ERROR: --timeout requires a value"
                exit 2
            fi
            if ! [[ "$2" =~ ^[0-9]+$ ]]; then
                echo "ERROR: --timeout requires an integer number of seconds"
                exit 2
            fi
            TIMEOUT="$2"
            shift 2
            ;;
        --log-dir)
            if [ $# -lt 2 ]; then
                echo "ERROR: --log-dir requires a directory"
                exit 2
            fi
            LOG_DIR="$2"
            shift 2
            ;;
        --layouts-fixture)
            if [ $# -lt 2 ]; then
                echo "ERROR: --layouts-fixture requires a file"
                exit 2
            fi
            LAYOUTS_FIXTURE="$2"
            shift 2
            ;;
        --)
            shift
            EXTRA_APP_ARGS=("$@")
            break
            ;;
        *)
            TEST_NAME="$1"
            shift
            ;;
    esac
done

if [ "$CLEAN_BUILD" = true ]; then
    SKIP_BUILD=false
fi

if [ "$RUN_IMGUI_ALL" = true ] && [ -n "$RUN_IMGUI_FILTER" ]; then
    echo "ERROR: Use either --imgui-tests or --imgui-test, not both"
    exit 2
fi

RUN_IMGUI_MODE=false
if [ "$RUN_IMGUI_ALL" = true ] || [ -n "$RUN_IMGUI_FILTER" ]; then
    RUN_IMGUI_MODE=true
fi

if [ "$RUN_IMGUI_MODE" = true ] && [ -n "$TEST_NAME" ]; then
    echo "ERROR: Positional suite test names cannot be combined with ImGui test options"
    exit 2
fi

RESULTS_DIR="$PROJECT_DIR/tests/results"
RESULTS_FILE="$RESULTS_DIR/last_run.txt"
# Not where this script builds -- that is build-macos/, via build-macos.sh. This
# is Xcode.app's per-project DerivedData, kept in the search below because a
# maintainer who just hit Cmd-B in the IDE expects --skip-build to find it.
BUILD_DIR="$PROJECT_DIR/platform/MacOS/DerivedData"
APP_BINARY=""
APP_BINARY_MTIME=0

select_newest_binary() {
    local candidate=""
    local mtime=0

    for candidate in "$@"; do
        if [ -f "$candidate" ]; then
            # macOS/BSD stat takes -f "%m" for a file's mtime. GNU stat
            # (Linux) has a DIFFERENT -f that means "show filesystem info
            # instead of file info", so `stat -f "%m" file` does not fail on
            # Linux -- it silently succeeds and prints multi-line filesystem
            # info that is not a number.
            #
            # GNU FORM FIRST, then BSD, rather than branching on "is it Linux".
            # That branch sent Git Bash -- where uname -s is MINGW64_NT-... --
            # down the BSD path, onto a GNU stat, and back into exactly the
            # garbage it was written to avoid. Trying -c first is safe in both
            # directions: BSD stat rejects -c, GNU stat accepts it.
            mtime=$(stat -c "%Y" "$candidate" 2>/dev/null \
                    || stat -f "%m" "$candidate" 2>/dev/null \
                    || printf '0')
            if [ "$mtime" -gt "$APP_BINARY_MTIME" ]; then
                APP_BINARY="$candidate"
                APP_BINARY_MTIME="$mtime"
            fi
        fi
    done
}

# Ensure results directory exists
mkdir -p "$RESULTS_DIR"

if [ ${#EXTRA_APP_ARGS[@]} -gt 0 ]; then
    for extra_arg in "${EXTRA_APP_ARGS[@]}"; do
        for reserved_flag in "${RESERVED_APP_FLAGS[@]}"; do
            if [ "$extra_arg" = "$reserved_flag" ]; then
                echo "ERROR: App options after -- cannot include reserved runner flags: $reserved_flag"
                exit 2
            fi
        done
    done

    for ((i = 0; i < ${#EXTRA_APP_ARGS[@]}; i++)); do
        if [ "${EXTRA_APP_ARGS[$i]}" = "--layouts-file" ]; then
            if [ $((i + 1)) -ge ${#EXTRA_APP_ARGS[@]} ]; then
                echo "ERROR: --layouts-file requires a path"
                exit 2
            fi

            FORWARDED_LAYOUTS_FILE_COUNT=$((FORWARDED_LAYOUTS_FILE_COUNT + 1))
            FORWARDED_LAYOUTS_FILE="${EXTRA_APP_ARGS[$((i + 1))]}"
        fi
    done
fi

if [ "$FORWARDED_LAYOUTS_FILE_COUNT" -gt 1 ]; then
    echo "ERROR: Forwarded app options can include --layouts-file at most once"
    exit 2
fi

if [ -n "$LAYOUTS_FIXTURE" ]; then
    if [ -n "$FORWARDED_LAYOUTS_FILE" ]; then
        echo "ERROR: Use either --layouts-fixture or forwarded --layouts-file, not both"
        exit 2
    fi

    if [[ "$LAYOUTS_FIXTURE" = /* ]]; then
        LAYOUTS_FIXTURE_PATH="$LAYOUTS_FIXTURE"
    else
        LAYOUTS_FIXTURE_PATH="$PROJECT_DIR/$LAYOUTS_FIXTURE"
    fi

    if [ ! -f "$LAYOUTS_FIXTURE_PATH" ]; then
        echo "ERROR: Layout fixture not found: $LAYOUTS_FIXTURE"
        exit 2
    fi

    copy_layouts_file_to_tmp "$LAYOUTS_FIXTURE_PATH"
    if [ "$LAYOUTS_FIXTURE_PATH" = "$KNOWN_LAYOUTS_FIXTURE_PATH" ]; then
        COPIED_LAYOUTS_MATCHES_KNOWN_FIXTURE=true
    fi
elif [ -n "$FORWARDED_LAYOUTS_FILE" ]; then
    if [[ "$FORWARDED_LAYOUTS_FILE" = /* ]]; then
        FORWARDED_LAYOUTS_FILE_PATH="$FORWARDED_LAYOUTS_FILE"
    else
        FORWARDED_LAYOUTS_FILE_PATH="$PROJECT_DIR/$FORWARDED_LAYOUTS_FILE"
    fi

    if [ ! -f "$FORWARDED_LAYOUTS_FILE_PATH" ]; then
        echo "ERROR: Layouts file not found: $FORWARDED_LAYOUTS_FILE"
        exit 2
    fi

    copy_layouts_file_to_tmp "$FORWARDED_LAYOUTS_FILE_PATH"
    if [ "$FORWARDED_LAYOUTS_FILE_PATH" = "$KNOWN_LAYOUTS_FIXTURE_PATH" ]; then
        COPIED_LAYOUTS_MATCHES_KNOWN_FIXTURE=true
    fi

    REWRITTEN_APP_ARGS=()
    replaced_layouts_file=false
    for ((i = 0; i < ${#EXTRA_APP_ARGS[@]}; i++)); do
        arg="${EXTRA_APP_ARGS[$i]}"
        if [ "$arg" = "--layouts-file" ] && [ "$replaced_layouts_file" = false ]; then
            REWRITTEN_APP_ARGS+=("--layouts-file" "$COPIED_LAYOUTS_FILE")
            i=$((i + 1))
            replaced_layouts_file=true
            continue
        fi
        REWRITTEN_APP_ARGS+=("$arg")
    done
    EXTRA_APP_ARGS=("${REWRITTEN_APP_ARGS[@]}")
fi

# Step 1: Build
#
# Through the PLATFORM WRAPPER, never `xcodebuild -project ...` directly. Under
# the MTEngineSDL capability programme the app cannot be built from the Xcode
# project alone: the wrapper first stages the vendored uSockets into the keyed
# dependency directory for the current engine revision, then resolves
# mtengine.caps into the MT_ENABLE_* settings that BOTH the engine target and the
# app target have to be compiled with. A bare xcodebuild does neither, so it
# builds the engine with the engine's own defaults (MT_ENABLE_MBEDTLS=1 on macOS)
# while linking against the capability-keyed libs directory this app's manifest
# produced (MT_CAP_HTTPS=0 -> an empty mbedTLS archive), and the link dies on
# missing _mbedtls_* symbols with nothing to suggest the cause. That is what this
# runner did until 2026-08-27, which made it unusable on macOS.
if [ "$SKIP_BUILD" = false ]; then
    echo "=== Building Retro Debugger ==="

    if [ "$(uname -s)" = "Linux" ]; then
        # build-linux.sh is cmake + make: already incremental, no clean switch.
        BUILD_CMD=("$PROJECT_DIR/build-linux.sh")
    else
        # Default to --incremental: this runs on every test invocation, and the
        # release default of wiping build-macos/ would make each run a full
        # rebuild. --clean-build asks for that wipe explicitly.
        BUILD_CMD=("$PROJECT_DIR/build-macos.sh" --incremental)
        if [ "$CLEAN_BUILD" = true ]; then
            BUILD_CMD=("$PROJECT_DIR/build-macos.sh")
        fi
    fi

    if ! bash "${BUILD_CMD[@]}"; then
        echo "BUILD FAILED"
        exit 2
    fi
    echo "=== Build succeeded ==="
fi

# Step 2: Remove old results
rm -f "$RESULTS_FILE"

# Step 3: Find the built binary
select_newest_binary \
    "$BUILD_DIR"/Build/Products/Release/"Retro Debugger.app"/Contents/MacOS/"Retro Debugger" \
    "$BUILD_DIR"/Build/Products/Debug/"Retro Debugger.app"/Contents/MacOS/"Retro Debugger" \
    "$BUILD_DIR"/*/Build/Products/Release/"Retro Debugger.app"/Contents/MacOS/"Retro Debugger" \
    "$BUILD_DIR"/*/Build/Products/Debug/"Retro Debugger.app"/Contents/MacOS/"Retro Debugger"

# ALSO consider build-macos/ and Xcode's shared DerivedData -- unconditionally,
# not "only if nothing was found above".
#
# This used to be an `if [ -z "$APP_BINARY" ]` fallback, and that was a real
# bug, found 2026-08-18 while capturing the S-2 SDL3 baseline: build-macos.sh
# writes to $PROJECT_DIR/build-macos, which was not searched AT ALL, while
# platform/MacOS/DerivedData still held a binary from 9 June. The runner
# happily selected the ten-week-old one and reported 72/75 for a tree that had
# just been rebuilt. select_newest_binary already picks by mtime across
# everything it is handed -- it just was not being handed the right paths.
select_newest_binary \
    "$PROJECT_DIR"/build-macos/Build/Products/Release/"Retro Debugger.app"/Contents/MacOS/"Retro Debugger" \
    "$PROJECT_DIR"/build-macos/Build/Products/Debug/"Retro Debugger.app"/Contents/MacOS/"Retro Debugger" \
    "$HOME"/Library/Developer/Xcode/DerivedData/*/Build/Products/Release/"Retro Debugger.app"/Contents/MacOS/"Retro Debugger" \
    "$HOME"/Library/Developer/Xcode/DerivedData/*/Build/Products/Debug/"Retro Debugger.app"/Contents/MacOS/"Retro Debugger"

# Linux: build-linux.sh / CMake put the binary straight at build/retrodebugger
# -- no app-bundle wrapper. Never searched here before this line was added
# (2026-08-18, first real Linux pass), so a Linux checkout always hit the
# "could not find binary" error below regardless of --skip-build.
select_newest_binary \
    "$PROJECT_DIR"/build/retrodebugger

# Windows: MSBuild leaves c64d.exe at platform/Windows/bin/<Platform>/<Config>/
# -- the convention every app in this programme shares, through the engine's
# Directory.Build.props and the vcxproj OutDir. bin/ holds x64 and ARM64,
# Debug and Release side by side, and select_newest_binary picks by mtime as
# it does everywhere else. Never searched here before, so Git Bash reported
# "could not find binary" straight after a successful build.
select_newest_binary \
    "$PROJECT_DIR"/platform/Windows/bin/*/*/c64d.exe

# --package: the binary AND the working directory come from the newest
# release package (a FINAL build). Without it this runner never looks at
# platform/*/prod/ -- a development build runs from the git root, and a
# package can be older than the build that just happened. See
# MTEngineSDL/docs/testing.md.
PACKAGE_DIR=""
if [ "$RUN_PACKAGE" = true ]; then
    _mt_lib="${MTENGINE_DIR:-$PROJECT_DIR/../MTEngineSDL}/tools/appbuild/appbuild-lib.sh"
    [ -f "$_mt_lib" ] || { echo "ERROR: --package needs the engine (appbuild-lib.sh) at ${MTENGINE_DIR:-$PROJECT_DIR/../MTEngineSDL}"; exit 2; }
    . "$_mt_lib"
    _mt_hit="$(mt_appbuild_prod_binary "$PROJECT_DIR" "Retro Debugger" || true)"
    [ -z "$_mt_hit" ] && _mt_hit="$(mt_appbuild_prod_binary "$PROJECT_DIR" "c64d" || true)"
    if [ -z "$_mt_hit" ]; then
        echo "ERROR: --package given but no release package under platform/*/prod/."
        echo "       Build one first: ./build-macos.sh --prod"
        exit 2
    fi
    APP_BINARY="${_mt_hit##*|}"
    PACKAGE_DIR="${_mt_hit%%|*}"
fi

if [ -z "$APP_BINARY" ]; then
    echo "ERROR: Could not find Retro Debugger binary. Build first, or check the"
    echo "       DerivedData / build tree path, or build a release package"
    echo "       (platform/*/prod/<arch>/) which this runner also accepts."
    exit 2
fi

echo "=== Using binary: $APP_BINARY ==="

# Step 4: Run the binary with test flags.
# Extra app arguments can be forwarded after --, for example layout files.
# Change to project directory so results file path is correct
# ---------------------------------------------------------------------------
# WHERE THE BINARY RUNS FROM -- the git root, unless --package.
#
# An MTEngineSDL app finds its assets through the CURRENT WORKING DIRECTORY and
# nothing else. A DEVELOPMENT build runs from the git root, which holds assets/
# as tracked. A FINAL build (./build-macos.sh --prod) is verified from its
# package with --package. Nothing is copied into a package to make a test work:
# fixtures are reached through CTest::ResolveProjectPath(), and
# MT_TEST_PROJECT_DIR short-cuts its walk. Procedure: MTEngineSDL/docs/testing.md.
# ---------------------------------------------------------------------------
RUN_DIR="$PROJECT_DIR"
MTENGINE_DIR="${MTENGINE_DIR:-$PROJECT_DIR/../MTEngineSDL}"
# MT_TEST_RUN_DIR pins the working directory outright; for tests OF this runner.
if [ -n "${MT_TEST_RUN_DIR:-}" ]; then
    RUN_DIR="$MT_TEST_RUN_DIR"
    echo "=== Run directory pinned by MT_TEST_RUN_DIR: $RUN_DIR ==="
elif [ -n "$PACKAGE_DIR" ]; then
    RUN_DIR="$PACKAGE_DIR"
    echo "=== Running from release package: $RUN_DIR ==="
else
    echo "=== Running from the git root: $RUN_DIR ==="
fi
# The binary must be ABSOLUTE before the cd, or a relative --binary resolves
# against the wrong directory the moment we move.
case "$APP_BINARY" in
    /* | [A-Za-z]:[\/]*) ;;
    *) APP_BINARY="$(cd "$(dirname "$APP_BINARY")" && pwd)/$(basename "$APP_BINARY")" ;;
esac

# Where the repository is, for CTest::ResolveProjectPath().
# A test run must never write into the user's real settings folder. The app
# rewrites layouts.dat, imgui.ini and settings.dat on every shutdown, so a run
# pointed at it silently replaces the workspace the user built. MT_SETTINGS_DIR
# redirects it (honoured by MTEngineSDL's SYS_InitFileSystem on all three
# platforms). Respect an explicit one from the caller; otherwise use a
# per-run temp folder.
if [ -z "${MT_SETTINGS_DIR:-}" ]; then
    MT_SETTINGS_DIR="$(mktemp -d "${TMPDIR:-/tmp}/retrodebugger-settings.XXXXXX")"

    # Seed it from the real folder, so the run sees the same configuration the
    # user has and only the COPY is written. Two reasons not to start empty:
    # tests would silently exercise a different configuration than the one
    # being debugged, and an empty settings folder is a first-run path that
    # currently aborts partway through the suite (see src/TODO.txt) -- a real
    # bug, but not one every test run should trip over.
    REAL_SETTINGS_DIR=""
    case "$(uname -s)" in
        Darwin) REAL_SETTINGS_DIR="$HOME/Library/RetroDebugger" ;;
        Linux)  REAL_SETTINGS_DIR="$HOME/.RetroDebugger" ;;
    esac
    if [ -n "$REAL_SETTINGS_DIR" ] && [ -d "$REAL_SETTINGS_DIR" ]; then
        cp -R "$REAL_SETTINGS_DIR/." "$MT_SETTINGS_DIR/" 2>/dev/null || true
        echo "=== Settings folder for this run: $MT_SETTINGS_DIR (copied from $REAL_SETTINGS_DIR) ==="
    else
        echo "=== Settings folder for this run: $MT_SETTINGS_DIR (empty) ==="
    fi
fi
export MT_SETTINGS_DIR

# A test run must be silent -- it must not play out of the speakers of whatever
# machine it happens to run on. The app forces SDL_AUDIODRIVER=dummy for
# automated runs itself, with overwrite=0 semantics so an explicit value still
# wins; set it here too so the rule holds for any binary and is visible in the
# run log.
export SDL_AUDIODRIVER="${SDL_AUDIODRIVER:-dummy}"

export MT_TEST_PROJECT_DIR="$PROJECT_DIR"

export MT_TEST_RESULTS="$RESULTS_FILE"
mkdir -p "$RESULTS_DIR"
cd "$RUN_DIR"

APP_ARGS=(--log-dir "$LOG_DIR")
if [ "$VISIBLE" = false ]; then
    if [ "$RUN_IMGUI_MODE" = true ]; then
        :
    else
    APP_ARGS=(--headless "${APP_ARGS[@]}")
    fi
fi

if [ -n "$COPIED_LAYOUTS_FILE" ]; then
    export C64D_TEST_EXPECTED_LAYOUTS_FILE="$COPIED_LAYOUTS_FILE"
    if [ "$COPIED_LAYOUTS_MATCHES_KNOWN_FIXTURE" = true ]; then
        export C64D_TEST_EXPECTED_LAYOUTS_FIXTURE=1
    else
        unset C64D_TEST_EXPECTED_LAYOUTS_FIXTURE
    fi
    if [ -z "$FORWARDED_LAYOUTS_FILE" ]; then
        APP_ARGS=(--layouts-file "$COPIED_LAYOUTS_FILE" "${APP_ARGS[@]}")
    fi
else
    unset C64D_TEST_EXPECTED_LAYOUTS_FILE
    unset C64D_TEST_EXPECTED_LAYOUTS_FIXTURE
fi

if [ ${#EXTRA_APP_ARGS[@]} -gt 0 ]; then
    APP_ARGS=("${EXTRA_APP_ARGS[@]}" "${APP_ARGS[@]}")
fi

if [ "$RUN_IMGUI_ALL" = true ]; then
    echo "=== Running all ImGui UI tests ==="
    APP_ARGS+=(--run-tests --exit-after-tests)
elif [ -n "$RUN_IMGUI_FILTER" ]; then
    echo "=== Running ImGui UI tests matching: $RUN_IMGUI_FILTER ==="
    APP_ARGS+=(--run-imgui-test "$RUN_IMGUI_FILTER" --exit-after-tests)
elif [ -n "$TEST_NAME" ]; then
    echo "=== Running test: $TEST_NAME ==="
    APP_ARGS+=(--run-test "$TEST_NAME" --exit-after-tests)
else
    echo "=== Running all suite tests ==="
    APP_ARGS+=(--run-suite --exit-after-tests)
    # Tests that know they're flaky in full-suite context can check
    # this and skip. Single-test mode does not set it.
    export C64D_IN_SUITE=1
fi

"$APP_BINARY" "${APP_ARGS[@]}" &

APP_PID=$!

# Step 5: Wait with timeout
ELAPSED=0
TIMED_OUT=false
while kill -0 "$APP_PID" 2>/dev/null; do
    sleep 1
    ELAPSED=$((ELAPSED + 1))
    if [ "$ELAPSED" -ge "$TIMEOUT" ]; then
        echo "TIMEOUT: Test did not complete within ${TIMEOUT}s"
        kill "$APP_PID" 2>/dev/null || true
        wait "$APP_PID" 2>/dev/null || true
        TIMED_OUT=true
        break
    fi
done

APP_STATUS=0
if [ "$TIMED_OUT" = false ]; then
    wait "$APP_PID" 2>/dev/null || APP_STATUS=$?
fi

# On an abnormal exit, the newest macOS crash report is the fastest route to
# the faulting frame: the CI runners have no human at the console, so without
# this a sigsegv only ever shows up as an exit code. Print the faulting
# frames from the JSON body (first line is the report header) and, for
# frames that belong to the app's own image, symbolicate them against the
# binary that just ran (the runner keeps symbols in the release build).
if [ "$APP_STATUS" != "0" ] && [ "$TIMED_OUT" = false ] && uname -s | grep -q Darwin; then
    CRASH_FILE=$( { ls -t "$HOME"/Library/Logs/DiagnosticReports/Retro\ Debugger-*.ips 2>/dev/null || true; } | head -1 )
    if [ -n "$CRASH_FILE" ]; then
        echo "=== Crash report found: $CRASH_FILE (app exit status $APP_STATUS) ==="
        python3 - "$CRASH_FILE" "$APP_BINARY" <<'PY_REPORT'
import json, subprocess, sys

path, binary = sys.argv[1], sys.argv[2]
with open(path, "rb") as f:
    raw = f.read()
nl = raw.find(b"\n")
meta = json.loads(raw[nl+1:])
used = meta.get("usedImages", [])
threads = meta.get("threads", [])
fi = meta.get("faultingThread", 0)
th = threads[fi] if isinstance(fi, int) and fi < len(threads) else threads[0]

print("    exceptionType:", json.dumps(meta.get("exception", {})))
term = meta.get("termination") or {}
print("    terminationReason:", json.dumps(term.get("details", term)))

app_img = None
for img in used:
    if img.get("name") == "Retro Debugger" or img.get("path", "").endswith("/Retro Debugger"):
        app_img = img
        break

addrs = []
for fr in th.get("frames", [])[:25]:
    i = fr.get("imageIndex", -1)
    img = used[i] if 0 <= i < len(used) else {}
    name = img.get("name", "?")
    base = img.get("base") or 0
    off = fr.get("imageOffset", 0)
    addr = base + off
    print("      %-26s 0x%x" % (name, addr))
    if app_img is not None and img is app_img:
        addrs.append("0x%x" % addr)

if app_img is not None and addrs and binary:
    base = app_img.get("base") or 0
    try:
        r = subprocess.run(
            ["atos", "-o", binary, "-arch", "arm64", "-l", "0x%x" % base] + addrs,
            text=True, capture_output=True, timeout=30,
        )
        print("    symbolicated (app image, load 0x%x):" % base)
        for line in (r.stdout or "").splitlines():
            print("      ->", line)
        if r.stderr:
            print("    atos stderr:", r.stderr.strip())
    except Exception as e:
        print("    atos failed:", e)
else:
    print("    (no app-image frame or binary to symbolicate)")
PY_REPORT
    else
        echo "(no crash report found for the failed run; exit status $APP_STATUS)"
    fi
fi

# Step 6: Check results
if [ ! -f "$RESULTS_FILE" ]; then
    if [ "$TIMED_OUT" = true ]; then
        echo "ERROR: Timed out AND no results file found. The app likely crashed or never ran tests."
        exit 3
    fi
    echo "ERROR: Results file not found at $RESULTS_FILE"
    echo "The application may have crashed before writing results."
    exit 1
fi

echo ""
echo "=== Test Results ==="
cat "$RESULTS_FILE"
echo ""

# Check the RESULT line for pass/fail
RESULT_LINE=$(grep "^RESULT:" "$RESULTS_FILE" || true)
if [ -z "$RESULT_LINE" ]; then
    echo "ERROR: No RESULT line found in results file"
    exit 1
fi

# Extract passed/total
PASSED=$(echo "$RESULT_LINE" | sed 's/RESULT: \([0-9]*\)\/.*/\1/')
TOTAL=$(echo "$RESULT_LINE" | sed 's/RESULT: [0-9]*\/\([0-9]*\).*/\1/')

if [ "$PASSED" = "$TOTAL" ] && [ "$TOTAL" != "0" ]; then
	if [ "$TIMED_OUT" = true ]; then
		echo "APPLICATION TIMED OUT after passing-looking results file"
		exit 1
	fi
    if [ "$TIMED_OUT" = false ] && [ "$APP_STATUS" != "0" ]; then
        echo "APPLICATION FAILED (exit $APP_STATUS) despite passing-looking results file"
        exit 1
    fi
    echo "ALL TESTS PASSED ($PASSED/$TOTAL)"
    exit 0
else
    echo "TESTS FAILED ($PASSED/$TOTAL passed)"
    exit 1
fi
