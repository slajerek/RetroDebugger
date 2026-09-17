#!/usr/bin/env bash
set -euo pipefail

usage() {
  cat <<'EOF'
Usage: make-release-linux.sh [--no-zip]

  (default)  Assemble the release and produce a .zip in ./releases/
             (RetroDebugger-v<version>-linux-<arch>.zip).
  --no-zip   Assemble the release FOLDER in ./releases/ but do not zip it.
             Intended for CI: GitHub Actions zips the uploaded folder once,
             avoiding a pointless pack -> unpack -> repack round-trip.
             Prints the folder path; under GitHub Actions it also sets the
             step outputs 'release_dir' and 'artifact_name'.
  -h|--help  Show this help.
EOF
}

NO_ZIP=0
while [ $# -gt 0 ]; do
  case "$1" in
    --no-zip)  NO_ZIP=1; shift ;;
    -h|--help) usage; exit 0 ;;
    *)         echo "ERROR: unknown option: $1" >&2; usage >&2; exit 1 ;;
  esac
done

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
# shellcheck source=_release-common.sh
source "$SCRIPT_DIR/_release-common.sh"

ROOT="$(rd_repo_root)"
cd "$ROOT"

VERSION="$(rd_version "$ROOT")"
[ -n "$VERSION" ] || { echo "ERROR: could not parse RETRODEBUGGER_VERSION_STRING" >&2; exit 1; }

case "$(uname -m)" in
  x86_64)        ARCH="x64" ;;
  aarch64|arm64) ARCH="arm64" ;;
  *)             ARCH="$(uname -m)" ;;
esac

RELEASE_NAME="RetroDebugger-v$VERSION"
ARTIFACT_NAME="RetroDebugger-v$VERSION-linux-$ARCH"
ZIP_NAME="$ARTIFACT_NAME.zip"

echo "==> [1/5] Staging $RELEASE_NAME (linux-$ARCH)"
STAGE="$(rd_make_stage "$VERSION")"
trap 'rm -rf "$STAGE"' EXIT
RELEASE_DIR="$STAGE/$RELEASE_NAME"
rd_copy_common "$ROOT" "$RELEASE_DIR"

# Linux-only: icons folder with the contents of assets/icons.
mkdir -p "$RELEASE_DIR/icons"
cp -R "$ROOT/assets/icons/." "$RELEASE_DIR/icons/"

echo "==> [2/5] Building RetroDebugger (clean)"
rm -rf "$ROOT/build"
echo "==> Running build-linux.sh with verbose logging"
set -x
chmod +x "$ROOT/build-linux.sh"
( cd "$ROOT" && ./build-linux.sh ${MT_LOGS:+--logs "$MT_LOGS"} )
set +x

echo "==> [3/5] Copying binary"
BIN="$ROOT/build/retrodebugger"
[ -f "$BIN" ] || { echo "ERROR: built binary not found at $BIN" >&2; exit 1; }
cp "$BIN" "$RELEASE_DIR/"

mkdir -p "$ROOT/releases"

if [ "$NO_ZIP" -eq 1 ]; then
  echo "==> [4/5] Collecting release folder (no zip)"
  DEST="$ROOT/releases/$RELEASE_NAME"
  rm -rf "$DEST"
  mv "$RELEASE_DIR" "$DEST"

  echo "==> [5/5] Release folder ready: $DEST"
  echo "==> Artifact name: $ARTIFACT_NAME"
  if [ -n "${GITHUB_OUTPUT:-}" ]; then
    {
      echo "release_dir=releases/$RELEASE_NAME"
      echo "artifact_name=$ARTIFACT_NAME"
    } >> "$GITHUB_OUTPUT"
  fi
else
  echo "==> [4/5] Packaging $ZIP_NAME"
  FINAL="$(rd_package_and_collect "$ROOT" "$STAGE" "$RELEASE_NAME" "$ZIP_NAME")"

  echo "==> [5/5] Cleaning staging dir"
  echo "==> Release ready: $FINAL"
fi
