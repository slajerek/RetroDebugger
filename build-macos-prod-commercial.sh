#!/usr/bin/env bash
#
# FINAL build, COMMERCIAL tier -- the one that may actually be sold or shipped.
#
# Wrapper over build-macos.sh. --tier commercial is the whole of it, but that
# one switch moves several things at once and this script exists to record them
# in the place someone will look. The Windows and Linux twins are
# build-windows-prod-commercial.ps1 and build-linux-prod-commercial.sh; keep the
# three in step.
#
# WHAT --tier commercial SETS. MT_COMMERCIAL_BUILD=1 and MT_PRIVATE_BUILD=0,
# together -- the two are mutually exclusive halves of one question, and an
# artifact is either sold or never distributed. Setting both is a configure-time
# error naming both keys.
#
# "FULL" MEANS "EVERYTHING A COMMERCIAL LICENCE PERMITS", WHICH IS LESS THAN
# EVERYTHING. MT_COMMERCIAL_BUILD is not a whole-capability veto; per capability
# it either does nothing, turns an effect off, or is refused outright. Two
# consequences worth knowing before you read the summary and wonder what
# happened:
#
#   * NONFREE IS NOT A CHOICE HERE. MT_CAP_OPENCV_NONFREE is `commercial:
#     capability-off` in the vocabulary, so a commercial resolve forces it to 0
#     -- but this app does not build OpenCV at all (MT_CAP_OPENCV=0 in
#     mtengine.caps), so there is nothing for it to force off and no
#     -prod-nonfree wrapper beside this one. The template
#     (MTEngineSDLDummyApp) has both, because it is the app that demonstrates
#     the vision capabilities.
#   * MT_CAP_TEST_ENGINE GOES OFF TOO, for the same class of reason: a shipped
#     build carries no imgui test engine. So `tests/run_test.sh --package`
#     against this tier runs the CTestSuite half only and reports
#     "ImGuiTests: SKIPPED (this build resolved MT_CAP_TEST_ENGINE=0)" -- the
#     runner detects the missing symbols and says so rather than passing
#     silently. That is correct, not a regression; exercise the UI suite on the
#     development build instead.
#
# Anything the deny-list still catches is a hard error naming the key, not a
# warning: a commercial resolve that would enable a dependency marked
# commercial_safe:false stops the build. Distribution-restricted dependencies
# are dropped, because they ride on MT_PRIVATE_BUILD=1 and this tier sets it to
# 0; FFmpeg is additionally rebuilt in `commercial` mode, and the licence gate
# REFUSES to link an app of this tier against an install marked otherwise -- so
# the first run here rebuilds FFmpeg and takes a while.
#
# SYMBOLS ARE OFF AND CANNOT BE TURNED ON. --prod defaults MT_RELEASE_SYMBOLS to
# 0, and the driver REFUSES --symbols on together with --tier commercial: a
# store build never ships symbols. The dSYM is still extracted, outside the
# bundle, under $MT_OUT/symbols -- which is what you keep for symbolicating
# crash reports from a shipped build. Do not discard it.
#
# SIGNING AND NOTARIZATION ARE NOT DONE HERE. The driver ad-hoc signs what it
# builds; a bundle you intend to distribute still needs your Developer ID
# signature, a hardened runtime and a notarization round trip. This script
# produces the artifact, not a shippable one.
#
# Usage: ./build-macos-prod-commercial.sh [extra args passed through]
set -euo pipefail
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

# --help is answered BEFORE the banner. Passing it through unannounced printed
# the "COMMERCIAL build:" / "PRIVATE build:" banner, then the driver's usage,
# then a "Bundle: <path>" line for a bundle no build had produced -- measured,
# and the path named did not exist. Anything that does not build must not
# advertise an artifact.
if [[ "${1:-}" == "--help" || "${1:-}" == "-h" ]]; then
    # Stops at the first NON-COMMENT line, not at the first blank one. These
    # headers contain no blank line, so a '2,/^$/' range ran on past the
    # comment block and printed `set -euo pipefail` and the SCRIPT_DIR
    # assignment as if they were help. And s/^#//; s/^ // rather than
    # s/^# \?//: BSD sed, which is macOS's, has no \? quantifier in a basic
    # regex, so that substitution silently fails and every '#' survives. GNU
    # sed accepts both, which is why the Git Bash twins never showed either.
    sed -n '2,${ /^#/!q; s/^#//; s/^ //; p; }' "${BASH_SOURCE[0]}"
    echo "Everything else is passed through to the driver; try:"
    echo "  ./build-macos.sh --help"
    exit 0
fi

echo
echo "  COMMERCIAL build: MT_COMMERCIAL_BUILD=1, MT_PRIVATE_BUILD=0."
echo "  This app builds no OpenCV, so there is no NONFREE half to force off."
echo "  The tier still drops distribution-restricted dependencies, and this"
echo "  build does NOT run the symbol gate. To check a capability by hand:"
echo "    MTEngineSDL/tools/appbuild/scan-capability-symbols.sh <binary> \$MT_CAPS_OUT"
echo "  See the resolve summary above for what the tier withheld."
echo

"$SCRIPT_DIR/build-macos.sh" --release --prod --tier commercial "$@"

# No -nc suffix on a commercial deploy: the driver names it after MT_MACOS_SCHEME.

# THE TRAILER ONLY IF THE ARTIFACT IS REALLY THERE. The driver exits 0 for
# --help and for any other non-building request, and an unconditional echo then
# names a path that does not exist.
if [ -d "$SCRIPT_DIR/platform/MacOS/prod/$(uname -m)/Retro Debugger.app" ]; then
    echo
    echo "Bundle: $SCRIPT_DIR/platform/MacOS/prod/$(uname -m)/Retro Debugger.app"
    echo "  Check the resolve summary above for what the tier withheld."
    echo "  Still unsigned for distribution: Developer ID + notarization are yours."
    echo "  Verify it from the package, not the git root:  tests/run_test.sh --package"
fi
