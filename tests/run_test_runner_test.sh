#!/bin/bash

set -euo pipefail

PROJECT_DIR="$(cd "$(dirname "$0")/.." && pwd)"

# This test drives run_test.sh with FABRICATED binaries that write
# tests/results/last_run.txt relative to their own working directory. Since
# 2026-09-02 the runner otherwise launches from the release package under
# platform/*/prod, because that is where a real app finds its assets -- which
# on a machine that has built c64d to prod would send these fakes' output
# somewhere this test does not look. Pin the behaviour these assertions were
# written against.
export MT_TEST_RUN_DIR="$PROJECT_DIR"
TMP_DIR="$(mktemp -d)"
DERIVED_DATA_DIR="$PROJECT_DIR/platform/MacOS/DerivedData"
LOCAL_DECOY_DIR="$DERIVED_DATA_DIR/run-test-a-decoy/Build/Products/Debug/Retro Debugger.app/Contents/MacOS"
LOCAL_APP_DIR="$DERIVED_DATA_DIR/run-test-z-app/Build/Products/Debug/Retro Debugger.app/Contents/MacOS"
LOCAL_DECOY_BINARY="$LOCAL_DECOY_DIR/Retro Debugger"
LOCAL_APP_BINARY="$LOCAL_APP_DIR/Retro Debugger"
EXACT_LOCAL_RELEASE_BINARY="$DERIVED_DATA_DIR/Build/Products/Release/Retro Debugger.app/Contents/MacOS/Retro Debugger"
EXACT_LOCAL_DEBUG_BINARY="$DERIVED_DATA_DIR/Build/Products/Debug/Retro Debugger.app/Contents/MacOS/Retro Debugger"
TEST_HOME="$TMP_DIR/home"
HOME_DERIVED_DATA_DIR="$TEST_HOME/Library/Developer/Xcode/DerivedData"
HOME_DECOY_DIR="$HOME_DERIVED_DATA_DIR/run-test-a-decoy/Build/Products/Release/Retro Debugger.app/Contents/MacOS"
HOME_APP_DIR="$HOME_DERIVED_DATA_DIR/run-test-z-app/Build/Products/Debug/Retro Debugger.app/Contents/MacOS"
HOME_DECOY_BINARY="$HOME_DECOY_DIR/Retro Debugger"
HOME_APP_BINARY="$HOME_APP_DIR/Retro Debugger"
ARGS_LOG="$TMP_DIR/app-args.log"
ENV_LOG="$TMP_DIR/app-env.log"
DECOY_LOG="$TMP_DIR/decoy.log"
BUILD_LOG="$TMP_DIR/xcodebuild.log"
BUILD_APP_LOG="$TMP_DIR/build-app.log"
ERROR_LOG="$TMP_DIR/error.log"
FAKE_BIN_DIR="$TMP_DIR/bin"
CUSTOM_LAYOUTS_FILE="$TMP_DIR/custom-layouts.dat"

cleanup() {
    rm -rf "$TMP_DIR"
    rm -rf "$DERIVED_DATA_DIR/run-test-a-decoy" "$DERIVED_DATA_DIR/run-test-z-app" "$DERIVED_DATA_DIR/run-test-build"
    rm -f "$EXACT_LOCAL_RELEASE_BINARY" "$EXACT_LOCAL_DEBUG_BINARY"
}
trap cleanup EXIT

rm -rf "$DERIVED_DATA_DIR/run-test-a-decoy" "$DERIVED_DATA_DIR/run-test-z-app" "$DERIVED_DATA_DIR/run-test-build"
rm -f "$EXACT_LOCAL_RELEASE_BINARY" "$EXACT_LOCAL_DEBUG_BINARY"

write_decoy_binary() {
    local path="$1"
    mkdir -p "$(dirname "$path")"
    cat > "$path" <<'EOF'
#!/bin/bash
set -euo pipefail
printf 'wrong-binary\n' > "$RUNNER_DECOY_LOG"
exit 9
EOF
    chmod +x "$path"
}

write_app_binary() {
    local path="$1"
    mkdir -p "$(dirname "$path")"
cat > "$path" <<'EOF'
#!/bin/bash
set -euo pipefail
printf '%s\n' "$@" > "$RUNNER_ARGS_LOG"
printf '%s\n' "${C64D_TEST_EXPECTED_LAYOUTS_FILE-}" > "$RUNNER_ENV_LOG"
printf '%s\n' "${C64D_TEST_EXPECTED_LAYOUTS_FIXTURE-}" >> "$RUNNER_ENV_LOG"
mkdir -p tests/results
cat > tests/results/last_run.txt <<'RESULTS'
RESULT: 1/1 passed
RESULTS
exit "${RUNNER_APP_EXIT_CODE:-0}"
EOF
    chmod +x "$path"
}

mkdir -p "$FAKE_BIN_DIR"

write_decoy_binary "$LOCAL_DECOY_BINARY"
write_app_binary "$LOCAL_APP_BINARY"
touch -t 209912310101 "$LOCAL_DECOY_BINARY"
touch -t 209912310102 "$LOCAL_APP_BINARY"

write_decoy_binary "$HOME_DECOY_BINARY"
write_app_binary "$HOME_APP_BINARY"
touch -t 209912310201 "$HOME_DECOY_BINARY"
touch -t 209912310202 "$HOME_APP_BINARY"

cp "$PROJECT_DIR/tests/data/layouts-test.dat" "$CUSTOM_LAYOUTS_FILE"

cat > "$FAKE_BIN_DIR/xcodebuild" <<'EOF'
#!/bin/bash
set -euo pipefail
printf '%s\n' "$@" > "$RUNNER_BUILD_LOG"
derived_data_path=""
while [[ $# -gt 0 ]]; do
    case "$1" in
        -derivedDataPath)
            derived_data_path="$2"
            shift 2
            ;;
        *)
            shift
            ;;
    esac
done

if [ -n "$derived_data_path" ]; then
    app_binary="$derived_data_path/Build/Products/Debug/Retro Debugger.app/Contents/MacOS/Retro Debugger"
    mkdir -p "$(dirname "$app_binary")"
cat > "$app_binary" <<'APP'
#!/bin/bash
set -euo pipefail
printf 'built-binary\n' > "${RUNNER_BUILD_APP_LOG:-/dev/null}"
printf '%s\n' "$@" > "$RUNNER_ARGS_LOG"
printf '%s\n' "${C64D_TEST_EXPECTED_LAYOUTS_FILE-}" > "$RUNNER_ENV_LOG"
printf '%s\n' "${C64D_TEST_EXPECTED_LAYOUTS_FIXTURE-}" >> "$RUNNER_ENV_LOG"
mkdir -p tests/results
    cat > tests/results/last_run.txt <<'RESULTS'
RESULT: 1/1 passed
RESULTS
    exit "${RUNNER_APP_EXIT_CODE:-0}"
APP
    chmod +x "$app_binary"
    touch -t 209912310303 "$app_binary"
fi

exit 0
EOF
chmod +x "$FAKE_BIN_DIR/xcodebuild"

assert_file_missing() {
    local file="$1"
    if [ -e "$file" ]; then
        echo "Did not expect $file to exist"
        exit 1
    fi
}

assert_contains() {
    local needle="$1"
    local file="$2"
    if ! grep -Fx -- "$needle" "$file" >/dev/null; then
        echo "Expected '$needle' in $file"
        cat "$file"
        exit 1
    fi
}

assert_count() {
    local needle="$1"
    local expected_count="$2"
    local file="$3"
    local actual_count
    actual_count=$(grep -Fxc -- "$needle" "$file" || true)
    if [ "$actual_count" != "$expected_count" ]; then
        echo "Expected '$needle' count $expected_count in $file, got $actual_count"
        cat "$file"
        exit 1
    fi
}

assert_not_contains() {
    local needle="$1"
    local file="$2"
    if grep -Fx -- "$needle" "$file" >/dev/null; then
        echo "Did not expect '$needle' in $file"
        cat "$file"
        exit 1
    fi
}

assert_order() {
    local first="$1"
    local second="$2"
    local file="$3"
    local first_line second_line
    first_line=$(grep -n -Fx -- "$first" "$file" | tail -1 | cut -d: -f1)
    second_line=$(grep -n -Fx -- "$second" "$file" | tail -1 | cut -d: -f1)
    if [ -z "$first_line" ] || [ -z "$second_line" ] || [ "$first_line" -ge "$second_line" ]; then
        echo "Expected '$first' before '$second' in $file"
        cat "$file"
        exit 1
    fi
}

RUNNER_ARGS_LOG="$ARGS_LOG" RUNNER_ENV_LOG="$ENV_LOG" RUNNER_BUILD_LOG="$BUILD_LOG" RUNNER_DECOY_LOG="$DECOY_LOG" HOME="$TEST_HOME" PATH="$FAKE_BIN_DIR:$PATH" \
    bash "$PROJECT_DIR/tests/run_test.sh" --skip-build
assert_file_missing "$DECOY_LOG"
assert_contains "--headless" "$ARGS_LOG"
assert_contains "--run-suite" "$ARGS_LOG"

RUNNER_ARGS_LOG="$ARGS_LOG" RUNNER_ENV_LOG="$ENV_LOG" RUNNER_BUILD_LOG="$BUILD_LOG" RUNNER_DECOY_LOG="$DECOY_LOG" HOME="$TEST_HOME" PATH="$FAKE_BIN_DIR:$PATH" \
    bash "$PROJECT_DIR/tests/run_test.sh" --skip-build --imgui-tests
assert_file_missing "$DECOY_LOG"
assert_not_contains "--headless" "$ARGS_LOG"
assert_contains "--run-tests" "$ARGS_LOG"
assert_not_contains "--run-suite" "$ARGS_LOG"
assert_not_contains "--run-test" "$ARGS_LOG"

RUNNER_ARGS_LOG="$ARGS_LOG" RUNNER_ENV_LOG="$ENV_LOG" RUNNER_BUILD_LOG="$BUILD_LOG" RUNNER_DECOY_LOG="$DECOY_LOG" HOME="$TEST_HOME" PATH="$FAKE_BIN_DIR:$PATH" \
    bash "$PROJECT_DIR/tests/run_test.sh" --skip-build --imgui-test open_all_views
assert_file_missing "$DECOY_LOG"
assert_not_contains "--headless" "$ARGS_LOG"
assert_contains "--run-imgui-test" "$ARGS_LOG"
assert_contains "open_all_views" "$ARGS_LOG"
assert_not_contains "--run-tests" "$ARGS_LOG"
assert_not_contains "--run-suite" "$ARGS_LOG"
assert_not_contains "--run-test" "$ARGS_LOG"

RUNNER_ARGS_LOG="$ARGS_LOG" RUNNER_ENV_LOG="$ENV_LOG" RUNNER_BUILD_LOG="$BUILD_LOG" RUNNER_DECOY_LOG="$DECOY_LOG" HOME="$TEST_HOME" PATH="$FAKE_BIN_DIR:$PATH" \
    bash "$PROJECT_DIR/tests/run_test.sh" --skip-build --visible LayoutSmoke -- --layouts-file tests/data/layouts-test.dat
assert_file_missing "$DECOY_LOG"
assert_not_contains "--headless" "$ARGS_LOG"
assert_contains "--run-test" "$ARGS_LOG"
assert_not_contains "--run-suite" "$ARGS_LOG"
assert_contains "LayoutSmoke" "$ARGS_LOG"
assert_contains "--layouts-file" "$ARGS_LOG"
assert_count "--layouts-file" 1 "$ARGS_LOG"
if ! grep -E '^/tmp/' "$ARGS_LOG" >/dev/null; then
    echo "Expected forwarded layouts path under /tmp in $ARGS_LOG"
    cat "$ARGS_LOG"
    exit 1
fi
if grep 'XXXXXX' "$ARGS_LOG" >/dev/null; then
    echo "Expected a real unique layouts path in $ARGS_LOG"
    cat "$ARGS_LOG"
    exit 1
fi
assert_not_contains "tests/data/layouts-test.dat" "$ARGS_LOG"
if ! grep -E '^/tmp/' "$ENV_LOG" >/dev/null; then
    echo "Expected exported layouts path under /tmp in $ENV_LOG"
    cat "$ENV_LOG"
    exit 1
fi
assert_contains "1" "$ENV_LOG"
if grep 'XXXXXX' "$ENV_LOG" >/dev/null; then
    echo "Expected a real unique layouts path in $ENV_LOG"
    cat "$ENV_LOG"
    exit 1
fi

RUNNER_ARGS_LOG="$ARGS_LOG" RUNNER_ENV_LOG="$ENV_LOG" RUNNER_BUILD_LOG="$BUILD_LOG" RUNNER_DECOY_LOG="$DECOY_LOG" HOME="$TEST_HOME" PATH="$FAKE_BIN_DIR:$PATH" \
    bash "$PROJECT_DIR/tests/run_test.sh" --skip-build LayoutSmoke -- --layouts-file "$CUSTOM_LAYOUTS_FILE"
assert_file_missing "$DECOY_LOG"
assert_contains "--layouts-file" "$ARGS_LOG"
assert_count "--layouts-file" 1 "$ARGS_LOG"
if ! grep -E '^/tmp/' "$ARGS_LOG" >/dev/null; then
    echo "Expected forwarded layouts path under /tmp in $ARGS_LOG"
    cat "$ARGS_LOG"
    exit 1
fi
assert_not_contains "$CUSTOM_LAYOUTS_FILE" "$ARGS_LOG"
if ! grep -E '^/tmp/' "$ENV_LOG" >/dev/null; then
    echo "Expected exported layouts path under /tmp in $ENV_LOG"
    cat "$ENV_LOG"
    exit 1
fi
assert_not_contains "1" "$ENV_LOG"

RUNNER_ARGS_LOG="$ARGS_LOG" RUNNER_ENV_LOG="$ENV_LOG" RUNNER_BUILD_LOG="$BUILD_LOG" RUNNER_DECOY_LOG="$DECOY_LOG" HOME="$TEST_HOME" PATH="$FAKE_BIN_DIR:$PATH" \
    bash "$PROJECT_DIR/tests/run_test.sh" --skip-build LayoutSmoke --layouts-fixture tests/data/layouts-test.dat
assert_file_missing "$DECOY_LOG"
assert_contains "--headless" "$ARGS_LOG"
assert_contains "--run-test" "$ARGS_LOG"
assert_contains "LayoutSmoke" "$ARGS_LOG"
assert_contains "--layouts-file" "$ARGS_LOG"
assert_count "--layouts-file" 1 "$ARGS_LOG"
if ! grep -E '^/tmp/' "$ARGS_LOG" >/dev/null; then
    echo "Expected forwarded layouts path under /tmp in $ARGS_LOG"
    cat "$ARGS_LOG"
    exit 1
fi
if grep 'XXXXXX' "$ARGS_LOG" >/dev/null; then
    echo "Expected a real unique layouts path in $ARGS_LOG"
    cat "$ARGS_LOG"
    exit 1
fi
assert_not_contains "tests/data/layouts-test.dat" "$ARGS_LOG"
if ! grep -E '^/tmp/' "$ENV_LOG" >/dev/null; then
    echo "Expected exported layouts path under /tmp in $ENV_LOG"
    cat "$ENV_LOG"
    exit 1
fi
assert_contains "1" "$ENV_LOG"
if grep 'XXXXXX' "$ENV_LOG" >/dev/null; then
    echo "Expected a real unique layouts path in $ENV_LOG"
    cat "$ENV_LOG"
    exit 1
fi

RUNNER_ARGS_LOG="$ARGS_LOG" RUNNER_ENV_LOG="$ENV_LOG" RUNNER_BUILD_LOG="$BUILD_LOG" RUNNER_DECOY_LOG="$DECOY_LOG" HOME="$TEST_HOME" PATH="$FAKE_BIN_DIR:$PATH" \
    bash "$PROJECT_DIR/tests/run_test.sh" --skip-build LayoutSmoke -- --layouts-file tests/data/layouts-test.dat --some-extra-flag
assert_file_missing "$DECOY_LOG"
assert_contains "--headless" "$ARGS_LOG"
assert_contains "--run-test" "$ARGS_LOG"
assert_contains "LayoutSmoke" "$ARGS_LOG"
assert_contains "--layouts-file" "$ARGS_LOG"
assert_count "--layouts-file" 1 "$ARGS_LOG"
if ! grep -E '^/tmp/' "$ARGS_LOG" >/dev/null; then
    echo "Expected forwarded layouts path under /tmp in $ARGS_LOG"
    cat "$ARGS_LOG"
    exit 1
fi
assert_not_contains "tests/data/layouts-test.dat" "$ARGS_LOG"
assert_contains "--some-extra-flag" "$ARGS_LOG"

rm -rf "$DERIVED_DATA_DIR/run-test-a-decoy" "$DERIVED_DATA_DIR/run-test-z-app"

if [ ! -f "$EXACT_LOCAL_RELEASE_BINARY" ] && [ ! -f "$EXACT_LOCAL_DEBUG_BINARY" ]; then
    RUNNER_ARGS_LOG="$ARGS_LOG" RUNNER_ENV_LOG="$ENV_LOG" RUNNER_BUILD_LOG="$BUILD_LOG" RUNNER_DECOY_LOG="$DECOY_LOG" HOME="$TEST_HOME" PATH="$FAKE_BIN_DIR:$PATH" \
        bash "$PROJECT_DIR/tests/run_test.sh" --skip-build HomeFallback
    assert_file_missing "$DECOY_LOG"
    assert_contains "--run-test" "$ARGS_LOG"
    assert_contains "HomeFallback" "$ARGS_LOG"
fi

write_decoy_binary "$LOCAL_DECOY_BINARY"
touch -t 209912310101 "$LOCAL_DECOY_BINARY"

RUNNER_ARGS_LOG="$ARGS_LOG" RUNNER_ENV_LOG="$ENV_LOG" RUNNER_BUILD_LOG="$BUILD_LOG" RUNNER_BUILD_APP_LOG="$BUILD_APP_LOG" RUNNER_DECOY_LOG="$DECOY_LOG" HOME="$TEST_HOME" PATH="$FAKE_BIN_DIR:$PATH" \
    bash "$PROJECT_DIR/tests/run_test.sh" --timeout 5 BuildFresh
assert_file_missing "$DECOY_LOG"
assert_contains "built-binary" "$BUILD_APP_LOG"
assert_contains "-derivedDataPath" "$BUILD_LOG"
assert_contains "$DERIVED_DATA_DIR" "$BUILD_LOG"
assert_contains "BuildFresh" "$ARGS_LOG"

if RUNNER_ARGS_LOG="$ARGS_LOG" RUNNER_ENV_LOG="$ENV_LOG" RUNNER_BUILD_LOG="$BUILD_LOG" RUNNER_DECOY_LOG="$DECOY_LOG" HOME="$TEST_HOME" PATH="$FAKE_BIN_DIR:$PATH" \
    bash "$PROJECT_DIR/tests/run_test.sh" --skip-build LayoutSmoke -- --run-suite >"$ERROR_LOG" 2>&1; then
    echo "Expected reserved forwarded flag to fail"
    exit 1
fi
assert_contains "ERROR: App options after -- cannot include reserved runner flags: --run-suite" "$ERROR_LOG"

if RUNNER_ARGS_LOG="$ARGS_LOG" RUNNER_ENV_LOG="$ENV_LOG" RUNNER_BUILD_LOG="$BUILD_LOG" RUNNER_DECOY_LOG="$DECOY_LOG" HOME="$TEST_HOME" PATH="$FAKE_BIN_DIR:$PATH" \
    bash "$PROJECT_DIR/tests/run_test.sh" --skip-build --imgui-tests LayoutSmoke >"$ERROR_LOG" 2>&1; then
    echo "Expected mixed suite/imgui runner mode to fail"
    exit 1
fi
assert_contains "ERROR: Positional suite test names cannot be combined with ImGui test options" "$ERROR_LOG"

if RUNNER_ARGS_LOG="$ARGS_LOG" RUNNER_ENV_LOG="$ENV_LOG" RUNNER_BUILD_LOG="$BUILD_LOG" RUNNER_DECOY_LOG="$DECOY_LOG" HOME="$TEST_HOME" PATH="$FAKE_BIN_DIR:$PATH" \
    bash "$PROJECT_DIR/tests/run_test.sh" --imgui-test >"$ERROR_LOG" 2>&1; then
    echo "Expected missing --imgui-test value to fail"
    exit 1
fi
assert_contains "ERROR: --imgui-test requires a filter" "$ERROR_LOG"

if RUNNER_ARGS_LOG="$ARGS_LOG" RUNNER_ENV_LOG="$ENV_LOG" RUNNER_BUILD_LOG="$BUILD_LOG" RUNNER_DECOY_LOG="$DECOY_LOG" HOME="$TEST_HOME" PATH="$FAKE_BIN_DIR:$PATH" \
    bash "$PROJECT_DIR/tests/run_test.sh" --timeout abc >"$ERROR_LOG" 2>&1; then
    echo "Expected invalid --timeout to fail"
    exit 1
fi
assert_contains "ERROR: --timeout requires an integer number of seconds" "$ERROR_LOG"

if RUNNER_ARGS_LOG="$ARGS_LOG" RUNNER_ENV_LOG="$ENV_LOG" RUNNER_BUILD_LOG="$BUILD_LOG" RUNNER_DECOY_LOG="$DECOY_LOG" HOME="$TEST_HOME" PATH="$FAKE_BIN_DIR:$PATH" \
    bash "$PROJECT_DIR/tests/run_test.sh" --log-dir >"$ERROR_LOG" 2>&1; then
    echo "Expected missing --log-dir value to fail"
    exit 1
fi
assert_contains "ERROR: --log-dir requires a directory" "$ERROR_LOG"

if RUNNER_ARGS_LOG="$ARGS_LOG" RUNNER_ENV_LOG="$ENV_LOG" RUNNER_BUILD_LOG="$BUILD_LOG" RUNNER_DECOY_LOG="$DECOY_LOG" HOME="$TEST_HOME" PATH="$FAKE_BIN_DIR:$PATH" \
    bash "$PROJECT_DIR/tests/run_test.sh" --layouts-fixture >"$ERROR_LOG" 2>&1; then
    echo "Expected missing --layouts-fixture value to fail"
    exit 1
fi
assert_contains "ERROR: --layouts-fixture requires a file" "$ERROR_LOG"

if RUNNER_ARGS_LOG="$ARGS_LOG" RUNNER_ENV_LOG="$ENV_LOG" RUNNER_BUILD_LOG="$BUILD_LOG" RUNNER_DECOY_LOG="$DECOY_LOG" HOME="$TEST_HOME" PATH="$FAKE_BIN_DIR:$PATH" \
    bash "$PROJECT_DIR/tests/run_test.sh" --skip-build --imgui-tests --imgui-test main_menu_bar_items >"$ERROR_LOG" 2>&1; then
    echo "Expected duplicate ImGui mode flags to fail"
    exit 1
fi
assert_contains "ERROR: Use either --imgui-tests or --imgui-test, not both" "$ERROR_LOG"

if RUNNER_ARGS_LOG="$ARGS_LOG" RUNNER_ENV_LOG="$ENV_LOG" RUNNER_BUILD_LOG="$BUILD_LOG" RUNNER_DECOY_LOG="$DECOY_LOG" HOME="$TEST_HOME" PATH="$FAKE_BIN_DIR:$PATH" \
    bash "$PROJECT_DIR/tests/run_test.sh" --skip-build LayoutSmoke -- --layouts-file tests/data/layouts-test.dat --layouts-file tests/data/layouts-test.dat >"$ERROR_LOG" 2>&1; then
    echo "Expected duplicate forwarded --layouts-file to fail"
    exit 1
fi
assert_contains "ERROR: Forwarded app options can include --layouts-file at most once" "$ERROR_LOG"

if RUNNER_ARGS_LOG="$ARGS_LOG" RUNNER_ENV_LOG="$ENV_LOG" RUNNER_BUILD_LOG="$BUILD_LOG" RUNNER_DECOY_LOG="$DECOY_LOG" RUNNER_APP_EXIT_CODE=9 HOME="$TEST_HOME" PATH="$FAKE_BIN_DIR:$PATH" \
    bash "$PROJECT_DIR/tests/run_test.sh" --skip-build >"$ERROR_LOG" 2>&1; then
    echo "Expected non-zero app exit to fail"
    exit 1
fi
assert_contains "APPLICATION FAILED (exit 9) despite passing-looking results file" "$ERROR_LOG"

RUNNER_ARGS_LOG="$ARGS_LOG" RUNNER_ENV_LOG="$ENV_LOG" RUNNER_BUILD_LOG="$BUILD_LOG" RUNNER_BUILD_APP_LOG="$BUILD_APP_LOG" RUNNER_DECOY_LOG="$DECOY_LOG" HOME="$TEST_HOME" PATH="$FAKE_BIN_DIR:$PATH" \
    bash "$PROJECT_DIR/tests/run_test.sh" --clean-build --timeout 5
assert_file_missing "$DECOY_LOG"
assert_contains "clean" "$BUILD_LOG"
assert_contains "build" "$BUILD_LOG"

echo "run_test.sh checks passed"
