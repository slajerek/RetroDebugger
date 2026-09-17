#!/bin/bash
# Register RetroDebugger MCP servers in Claude Code.
# Two modes: headless (embedded) and live (bridge to desktop app).
#
# Usage:  ./docs/mcp-server/claude-code-register-mcp-server.sh
#
# To remove: claude mcp remove retrodebugger-headless && claude mcp remove retrodebugger-live
# To verify: claude mcp list

set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"

# Find the built binary — pick the most recently modified if multiple exist
MATCHES=()
while IFS= read -r f; do
    MATCHES+=("$f")
done < <(ls -t ~/Library/Developer/Xcode/DerivedData/c64d-*/Build/Products/Release/Retro\ Debugger.app/Contents/MacOS/Retro\ Debugger 2>/dev/null)

if [ ${#MATCHES[@]} -eq 0 ]; then
    echo "ERROR: RetroDebugger binary not found."
    echo "Build it first:"
    echo "  xcodebuild -project ./platform/MacOS/c64d.xcodeproj -scheme \"Retro Debugger\" -quiet"
    exit 1
fi

if [ ${#MATCHES[@]} -gt 1 ]; then
    echo "WARNING: Found ${#MATCHES[@]} c64d DerivedData folders. Using the most recent:"
    for m in "${MATCHES[@]}"; do
        echo "  $m"
    done
    echo ""
fi

BINARY="${MATCHES[0]}"
echo "Found binary: $BINARY"

# Create a wrapper script that suppresses stderr.
# RetroDebugger emits ~900 lines of harmless warnings on stderr during startup
# (missing optional files, VICE parsing custom args, etc.). Claude Code's MCP
# health check may interpret these as errors and refuse to connect.
WRAPPER="$SCRIPT_DIR/retrodebugger-mcp-wrapper.sh"
cat > "$WRAPPER" << WEOF
#!/bin/bash
exec "$BINARY" "\$@" 2>/dev/null
WEOF
chmod +x "$WRAPPER"

echo "Created wrapper: $WRAPPER"
echo ""

# --- Register headless mode ---
echo "Registering retrodebugger-headless (embedded MCP, starts emulators)..."
claude mcp remove retrodebugger-headless 2>/dev/null || true
claude mcp add-json retrodebugger-headless "{\"type\":\"stdio\",\"command\":\"$WRAPPER\",\"args\":[\"--mcp-headless\",\"--log-dir\",\"/tmp\"]}"

# --- Register live mode ---
echo "Registering retrodebugger-live (bridge to running desktop app)..."
claude mcp remove retrodebugger-live 2>/dev/null || true
claude mcp add-json retrodebugger-live "{\"type\":\"stdio\",\"command\":\"$WRAPPER\",\"args\":[\"--mcp-live\",\"--log-dir\",\"/tmp\"]}"

# Remove old single-name registration if present
claude mcp remove retrodebugger 2>/dev/null || true

echo ""
echo "Done. Verify with: claude mcp list"
echo ""
echo "  retrodebugger-headless  Claude starts RetroDebugger, runs emulators in-process"
echo "  retrodebugger-live      Bridge to a running desktop RetroDebugger instance"
echo ""
echo "For live mode, start RetroDebugger desktop first, then use Claude."
echo "The bridge auto-reconnects if the desktop app starts later."
