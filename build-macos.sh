#!/usr/bin/env bash
# THE APP STUB -- canonical template: MTEngineSDL/tools/appbuild/stubs/.
#
# An app owns two files: mtengine.caps (what to enable) and mtengine-app.conf
# (what to build). The BUILD FLOW -- and even the MTENGINE_REF verification --
# lives in the engine's app-build driver; this stub keeps ONLY the
# chicken-and-egg job the driver cannot do for itself: clone the engine when
# absent. The driver warns when this file drifts from the template.
set -euo pipefail
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
MTENGINE_DIR="$SCRIPT_DIR/../MTEngineSDL"

if [[ ! -d "$MTENGINE_DIR" ]]; then
    REF="$(grep -v '^[[:space:]]*#' "$SCRIPT_DIR/MTENGINE_REF" | grep -v '^[[:space:]]*$' | head -n 1 | tr -d '[:space:]')"
    echo "Cloning MTEngineSDL at ${REF:-origin/master}"
    git clone https://github.com/slajerek/MTEngineSDL.git "$MTENGINE_DIR"
    # A branch ref stays ON the branch; only a SHA/tag detaches. The driver
    # verifies the ref on every build after this.
    git -C "$MTENGINE_DIR" checkout "${REF#origin/}" 2>/dev/null \
        || git -C "$MTENGINE_DIR" checkout --detach "$REF"
fi

DRIVER="$MTENGINE_DIR/tools/appbuild/app-build-macos.sh"
if [[ ! -f "$DRIVER" ]]; then
    echo "ERROR: $DRIVER not found -- the engine checkout predates the app-build driver." >&2
    echo "       Update it: git -C \"$MTENGINE_DIR\" pull" >&2
    exit 1
fi
exec bash "$DRIVER" --app-dir "$SCRIPT_DIR" "$@"
