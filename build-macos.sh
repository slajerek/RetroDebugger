#!/usr/bin/env bash
# Build RetroDebugger for macOS.
# Produces "Retro Debugger.app" under ./build-macos/Build/Products/Release/.
#
# Code signing is disabled here (CODE_SIGNING_ALLOWED=NO); the release script
# (tools/make-release/make-release-macos.sh) signs the bundle afterwards
# (ad-hoc by default, Developer ID + notarization when signing secrets exist).
set -euo pipefail

CURRENT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

# --- sibling dependencies (mirror build-linux.sh) ---
cd "$CURRENT_DIR/.."
if [ ! -d "MTEngineSDL" ]; then
	echo "Cloning MTEngineSDL library repository"
	git clone --recursive https://github.com/slajerek/MTEngineSDL.git
else
	( cd MTEngineSDL && git submodule update --init --recursive )
fi
if [ ! -d "uSockets" ]; then
	echo "Cloning uSockets library repository"
	git clone https://github.com/uNetworking/uSockets.git
fi

# uSockets: the macOS MTEngineSDL Xcode project links a prebuilt
# platform/MacOS/libs/uSockets.a (mirrors build-linux.sh, which builds it into
# platform/Linux/libs/). Build it and stage the archive.
echo "Building uSockets and staging uSockets.a for MTEngineSDL (macOS)"
( cd "$CURRENT_DIR/../uSockets" && make -j"$(sysctl -n hw.ncpu)" )
mkdir -p "$CURRENT_DIR/../MTEngineSDL/platform/MacOS/libs/"
cp -f "$CURRENT_DIR/../uSockets/uSockets.a" "$CURRENT_DIR/../MTEngineSDL/platform/MacOS/libs/"

# --- build RetroDebugger via Xcode ---
cd "$CURRENT_DIR"
DERIVED="$CURRENT_DIR/build-macos"
rm -rf "$DERIVED"

# The MTEngineSDL macOS project links libSDL2.a via LIBRARY_SEARCH_PATHS that
# points at /usr/local/lib (Intel Homebrew). On Apple Silicon Homebrew lives at
# /opt/homebrew, and GitHub's macos-latest runners are Apple Silicon too. Add the
# active Homebrew lib dir so libtool can locate SDL2 regardless of host arch.
XCODE_EXTRA=()
if command -v brew >/dev/null 2>&1; then
	XCODE_EXTRA+=("OTHER_LIBTOOLFLAGS=-L$(brew --prefix)/lib")
fi

echo "Building RetroDebugger (Release) via xcodebuild..."
xcodebuild \
	-project platform/MacOS/c64d.xcodeproj \
	-scheme "Retro Debugger" \
	-configuration Release \
	-derivedDataPath "$DERIVED" \
	CODE_SIGN_IDENTITY="-" \
	CODE_SIGNING_REQUIRED=NO \
	CODE_SIGNING_ALLOWED=NO \
	"${XCODE_EXTRA[@]}" \
	build

APP="$DERIVED/Build/Products/Release/Retro Debugger.app"
if [ ! -d "$APP" ]; then
	echo "ERROR: build did not produce $APP" >&2
	exit 1
fi
echo "RetroDebugger built: $APP"
