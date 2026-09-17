#!/usr/bin/env bash
set -euo pipefail

# macOS release: stage -> build -> copy .app -> sign -> ditto-zip -> releases/.
#
# Unlike the Linux/Windows scripts there is NO --no-zip mode: a macOS .app bundle
# must be packed with `ditto` to preserve internal symlinks and the executable
# bit. Uploading the bundle as a plain folder (GitHub artifact folder-upload)
# corrupts it. So this script always produces a ditto .zip and build-macos.yml
# uploads that file (GitHub then wraps it in its own artifact zip).
#
# Signing — "design for both":
#   * no signing env            -> ad-hoc sign (codesign -s -)
#   * MACOS_SIGN_IDENTITY set    -> Developer ID sign
#   * + MACOS_NOTARY_* set       -> notarize + staple
# The Certum/Developer-ID rollout is tracked as Phase 4 in the release plan.

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
# shellcheck source=_release-common.sh
source "$SCRIPT_DIR/_release-common.sh"

ROOT="$(rd_repo_root)"
cd "$ROOT"

VERSION="$(rd_version "$ROOT")"
[ -n "$VERSION" ] || { echo "ERROR: could not parse RETRODEBUGGER_VERSION_STRING" >&2; exit 1; }

RELEASE_NAME="RetroDebugger-v$VERSION"
ARTIFACT_NAME="RetroDebugger-v$VERSION-macos"   # universal: no arch suffix
ZIP_NAME="$ARTIFACT_NAME.zip"

echo "==> [1/5] Staging $RELEASE_NAME (macos)"
STAGE="$(rd_make_stage "$VERSION")"
trap 'rm -rf "$STAGE"' EXIT
RELEASE_DIR="$STAGE/$RELEASE_NAME"
rd_copy_common "$ROOT" "$RELEASE_DIR"   # no icons folder on macOS

echo "==> [2/5] Building RetroDebugger (clean)"
chmod +x "$ROOT/build-macos.sh"
# --clean: the stub+driver default is INCREMENTAL now (Phase 4); a release
# keeps the from-scratch guarantee explicitly.
# --clean cleans and EXITS (app-build-macos.sh); the build is the second call.
# MT_LOGS on|off reaches the driver as --logs; CI sets it per matrix leg.
( cd "$ROOT" && ./build-macos.sh --clean && ./build-macos.sh ${MT_LOGS:+--logs "$MT_LOGS"} )

echo "==> [3/5] Copying app bundle"
APP="$ROOT/build-macos/Build/Products/Release/Retro Debugger.app"
[ -d "$APP" ] || { echo "ERROR: app bundle not found at $APP" >&2; exit 1; }
ditto "$APP" "$RELEASE_DIR/Retro Debugger.app"
STAGED_APP="$RELEASE_DIR/Retro Debugger.app"

# --- signing ---
if [ -n "${MACOS_SIGN_IDENTITY:-}" ]; then
  echo "==> Signing with Developer ID: $MACOS_SIGN_IDENTITY"
  codesign --force --deep --options runtime --sign "$MACOS_SIGN_IDENTITY" "$STAGED_APP"
  if [ -n "${MACOS_NOTARY_APPLE_ID:-}" ] && [ -n "${MACOS_NOTARY_TEAM_ID:-}" ] \
     && [ -n "${MACOS_NOTARY_PASSWORD:-}" ]; then
    echo "==> Notarizing"
    NOTARY_ZIP="$STAGE/notarize.zip"
    ditto -c -k --sequesterRsrc --keepParent "$STAGED_APP" "$NOTARY_ZIP"
    xcrun notarytool submit "$NOTARY_ZIP" \
      --apple-id "$MACOS_NOTARY_APPLE_ID" \
      --team-id "$MACOS_NOTARY_TEAM_ID" \
      --password "$MACOS_NOTARY_PASSWORD" \
      --wait
    xcrun stapler staple "$STAGED_APP"
    rm -f "$NOTARY_ZIP"
  else
    echo "==> Notary credentials not set; skipping notarization"
  fi
else
  echo "==> No Developer ID identity; ad-hoc signing"
  codesign --force --deep --sign - "$STAGED_APP"
fi

echo "==> [4/5] Packaging $ZIP_NAME (ditto, preserves the .app bundle)"
mkdir -p "$ROOT/releases"
rm -f "$ROOT/releases/$ZIP_NAME"
( cd "$STAGE" && ditto -c -k --sequesterRsrc --keepParent "$RELEASE_NAME" "$ROOT/releases/$ZIP_NAME" )

echo "==> [5/5] Release ready: $ROOT/releases/$ZIP_NAME"
if [ -n "${GITHUB_OUTPUT:-}" ]; then
  {
    echo "artifact_name=$ARTIFACT_NAME"
    echo "zip_path=releases/$ZIP_NAME"
  } >> "$GITHUB_OUTPUT"
fi
