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

# SDL2: Homebrew's "sdl2" formula is nowadays an alias for sdl2-compat (a dynamic
# shim over SDL3) and no longer ships a static libSDL2.a, so `brew install sdl2`
# stopped providing the archive MTEngineSDL's libtool step merges in. Build static
# SDL2 from source and stage it next to uSockets.a — with the file present, Xcode
# hands libtool a full path (same as uSockets.a) instead of relying on -L/-lSDL2.
SDL2_VER=2.32.10
SDL2_A="$CURRENT_DIR/../MTEngineSDL/platform/MacOS/libs/libSDL2.a"
if [ ! -f "$SDL2_A" ]; then
	echo "Building static SDL2 $SDL2_VER and staging libSDL2.a for MTEngineSDL (macOS)"
	curl -fL -o "$CURRENT_DIR/../SDL2-src.tar.gz" "https://github.com/libsdl-org/SDL/releases/download/release-$SDL2_VER/SDL2-$SDL2_VER.tar.gz"
	tar xzf "$CURRENT_DIR/../SDL2-src.tar.gz" -C "$CURRENT_DIR/.."
	cmake -S "$CURRENT_DIR/../SDL2-$SDL2_VER" -B "$CURRENT_DIR/../sdl2-build" \
		-DSDL_STATIC=ON -DSDL_SHARED=OFF -DSDL_TEST=OFF \
		-DCMAKE_BUILD_TYPE=Release -DCMAKE_OSX_DEPLOYMENT_TARGET=10.15
	cmake --build "$CURRENT_DIR/../sdl2-build" -j"$(sysctl -n hw.ncpu)"
	cp -f "$CURRENT_DIR/../sdl2-build/libSDL2.a" "$SDL2_A"
fi

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
