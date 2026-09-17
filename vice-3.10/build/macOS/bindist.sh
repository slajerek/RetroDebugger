#!/bin/bash
set -o errexit

function err {
    >&2 echo -e $@
    >&2 echo
    exit 1
}

if [ $# != 1 ]; then
    err "Usage:\n\t$(basename $0) <path to VICE source>"
fi

SOURCE="$1"

if [ ! -d "$SOURCE" ]; then
    err "$SOURCE is not a directory"
fi
if [ -z "$DEPS_PREFIX" ]; then
    err "Missing env DEPS_PREFIX. Set to something like /opt/homebrew, /usr/local, or /opt/local"
fi
if [ -z "$CODE_SIGN_ID" ]; then
    err "Missing env CODE_SIGN_ID. Set to something like 'Developer ID Application: <NAME> (<ID>)', try $ security find-identity -v -p codesigning"
fi
if [ -z "$APPLE_ID_EMAIL" ]; then
    err "Missing env APPLE_ID_EMAIL. Set to your apple ID email address"
fi
if [ -z "$APPLE_TEAM_ID" ]; then
    err "Missing env APPLE_TEAM_ID. Get it from https://developer.apple.com/account#MembershipDetailsCard"
fi
if [ -z "$APPLE_NOTARISATION_PASSWORD" ]; then
    err "Missing env APPLE_NOTARISATION_PASSWORD. NOT YOUR APPLE ID PASSWORD. Refer to https://support.apple.com/en-us/102654"
fi

set -o nounset

cd "$SOURCE"
SOURCE="$(pwd)"

if [ ! -f configure ]; then
    ./autogen.sh
fi

BUILD_DIR=$(mktemp -d)
cd $BUILD_DIR
echo "Build Dir: $BUILD_DIR"

#
# Notarisation utility
#

function notarise {
    OUTPUT="$(mktemp)"
    xcrun notarytool submit \
        --apple-id "$APPLE_ID_EMAIL" \
        --password "$APPLE_NOTARISATION_PASSWORD" \
        --team-id "$APPLE_TEAM_ID" \
        --wait \
        "$1" \
        2>&1 | tee "$OUTPUT"

    xcrun stapler staple "$1"
}

#
# Build flags
#

BUILD_FLAGS="\
    --enable-option-checking \
    --disable-arch \
    --disable-html-docs \
    --enable-cpuhistory \
    --enable-ethernet \
    --enable-midi \
    --enable-parsid \
    --enable-pdf-docs \
    --with-fastsid \
    --with-flac \
    --with-gif \
    --with-lame \
    --with-libcurl \
    --with-png \
    --with-resid \
    --with-vorbis \
    "

if [ "$(uname -m)" == "x86_64" ]; then
    BUILD_FLAGS="$BUILD_FLAGS --enable-macos-minimum-version=12.0"
else
    BUILD_FLAGS="$BUILD_FLAGS --enable-macos-minimum-version=12.0"
fi

THREADS=$(sysctl -n hw.ncpu)

#
# GTK3 build
#

mkdir gtk3
cd gtk3

"$SOURCE/configure" --enable-gtk3ui $BUILD_FLAGS

make -j $THREADS
make bindistzip
notarise *.dmg
mv *.dmg "$SOURCE"
cd ..

#
# SDL2 build
#

mkdir sdl2
cd sdl2

"$SOURCE/configure" --enable-sdl2ui $BUILD_FLAGS

make -j $THREADS
make bindistzip
notarise *.dmg
mv *.dmg "$SOURCE"
cd ..

echo
echo "Looks like it worked."

cd "$SOURCE"
rm -rf "$BUILD_DIR"
open .

