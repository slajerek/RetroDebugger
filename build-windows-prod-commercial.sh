#!/bin/bash
# FINAL build, COMMERCIAL tier -- the one that may actually be sold or shipped.
# Run it on Windows from Git Bash / MSYS2 / WSL.
#
# Thin wrapper around build-windows-prod-commercial.ps1 -- pass the same flags,
# PowerShell-style. The reasoning lives in that script's header and is not
# repeated here; the two facts that surprise people are that the resolver
# SILENTLY forces MT_CAP_OPENCV_NONFREE to 0 (asking for SURF here returns exit
# code 0 and gives you a build without it), and that MT_CAP_TEST_ENGINE goes to
# 0 as well, so run_test.sh --package reports ImGuiTests: SKIPPED against this
# tier and runs the CTestSuite half only.
#
# Usage: ./build-windows-prod-commercial.sh [options]
#   -Platform <x64|ARM64>            default: this machine (auto-detected)
#   -Configuration <Release|Debug>   default Release
#   -Compiler <Clang|MSVC>           default Clang
#   -Logs <on|off>                   off under -Prod unless you ask
#   -Jobs <N>                        cap build parallelism (MT_BUILD_JOBS)
#   --help, -h

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

if [[ "${1:-}" == "--help" || "${1:-}" == "-h" ]]; then
    sed -n '2,/^$/{ s/^# \?//; p; }' "${BASH_SOURCE[0]}"
    exit 0
fi

# The same probe order as build-windows.sh; keep the three in step.
if command -v pwsh.exe &>/dev/null; then PS=pwsh.exe
elif command -v powershell.exe &>/dev/null; then PS=powershell.exe
elif command -v pwsh &>/dev/null; then PS=pwsh
elif command -v powershell &>/dev/null; then PS=powershell
else echo "Error: PowerShell not found. Install PowerShell or use build-windows-prod-commercial.ps1 directly." >&2; exit 1
fi

PS1_SCRIPT="$SCRIPT_DIR/build-windows-prod-commercial.ps1"
command -v cygpath &>/dev/null && PS1_SCRIPT="$(cygpath -w "$PS1_SCRIPT")"

exec "$PS" -ExecutionPolicy Bypass -File "$PS1_SCRIPT" "$@"
