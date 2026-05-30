#!/bin/sh
# Run the full WebSocket regression suite.
# Assumes RetroDebugger is running and the WebSocket server is enabled.
set -eu
cd "$(dirname "$0")"
. .venv/bin/activate
exec pytest websocket/ "$@"
