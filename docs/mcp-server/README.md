# RetroDebugger MCP Server for Claude Code

RetroDebugger can act as an MCP server, giving Claude Code direct access to
8-bit emulators — CPU registers, memory, breakpoints, snapshots, and execution
control.

Two modes are available:

| Mode | Flag | What it does |
|------|------|-------------|
| **Headless** | `--mcp-headless` | Claude launches RetroDebugger as a subprocess with emulators running in-process. No GUI window. |
| **Live** | `--mcp-live` | Claude launches a bridge that connects to your running RetroDebugger desktop app over WebSocket. |

## Quick Setup (macOS)

### 1. Build RetroDebugger

```bash
cd /path/to/c64d
xcodebuild -project ./platform/MacOS/c64d.xcodeproj -scheme "Retro Debugger" -quiet
```

### 2. Register both MCP servers

```bash
./docs/mcp-server/claude-code-register-mcp-server.sh
```

This registers:
- `retrodebugger-headless` — embedded mode
- `retrodebugger-live` — bridge to desktop app

### 3. Verify

```bash
claude mcp list
```

Both servers should appear. Start a new Claude Code session to use them.

## Manual Registration

If you prefer to register manually or the script doesn't work:

```bash
# Find your binary
BINARY="$(ls -t ~/Library/Developer/Xcode/DerivedData/c64d-*/Build/Products/Release/Retro\ Debugger.app/Contents/MacOS/Retro\ Debugger | head -1)"

# Create wrapper (suppresses harmless stderr warnings)
cat > /tmp/retrodebugger-wrapper.sh << EOF
#!/bin/bash
exec "$BINARY" "\$@" 2>/dev/null
EOF
chmod +x /tmp/retrodebugger-wrapper.sh

# Register headless
claude mcp add-json retrodebugger-headless \
  "{\"type\":\"stdio\",\"command\":\"/tmp/retrodebugger-wrapper.sh\",\"args\":[\"--mcp-headless\",\"--log-dir\",\"/tmp\"]}"

# Register live
claude mcp add-json retrodebugger-live \
  "{\"type\":\"stdio\",\"command\":\"/tmp/retrodebugger-wrapper.sh\",\"args\":[\"--mcp-live\",\"--log-dir\",\"/tmp\"]}"
```

## Which Mode to Use

### Headless (`retrodebugger-headless`)

- Claude starts and stops the emulators automatically
- No GUI window — pure automation
- Best for: writing 6502 code, automated testing, bulk memory inspection, CI

### Live (`retrodebugger-live`)

- You run the desktop app yourself, Claude connects to it
- See the screen, VIC state, memory maps while Claude operates
- Bridge auto-reconnects if you restart the desktop app
- Best for: interactive debugging, visual inspection alongside AI analysis

**Live mode setup:**
1. Start RetroDebugger desktop app
2. The WebSocket debugger server starts automatically (default port: 3563)
3. Start a Claude Code session — the bridge connects within seconds
4. If the desktop app isn't running yet, the bridge waits and connects when it appears

**Live mode with custom settings:**
```bash
claude mcp add-json retrodebugger-live \
  "{\"type\":\"stdio\",\"command\":\"/tmp/retrodebugger-wrapper.sh\",\"args\":[\"--mcp-live\",\"--host\",\"192.168.1.10\",\"--port\",\"4000\",\"--log-dir\",\"/tmp\"]}"
```

## Installing the MCP Skill

The skill file teaches agent best practices for using the retro_ tools —
safe debugging defaults, tool call order, example workflows. Without it,
the agent can still call the tools but won't know the recommended patterns.

The skill doc ships with this repository at
[docs/mcp/retrodebugger-mcp-skill.md](../mcp/retrodebugger-mcp-skill.md).

### Option A: Add to the project's agent guidance (recommended)

Add this to the bottom of your project's agent guidance file:

```markdown
## RetroDebugger MCP Skill
When the retrodebugger MCP server is connected, read and follow
docs/mcp/retrodebugger-mcp-skill.md for tool usage, safe debugging
defaults, and workflows.
```

This makes the skill available automatically in every session inside
the project directory.

### Option B: Tell the agent manually each session

At the start of a session, paste:

```
Read docs/mcp/retrodebugger-mcp-skill.md and follow those guidelines
when using the retro_ MCP tools.
```

### Option C: Per-user global (all projects)

Append the same snippet from Option A to your agent's per-user global
guidance file, so every project picks it up.

## Available Tools

When connected, Claude gets 18 debugger tools + 2 bridge-only tools:

| Tool | Description |
|------|-------------|
| `retro_list_platforms` | List active emulators (C64, Atari, NES) |
| `retro_cpu_status` | Read CPU registers (PC, A, X, Y, SP, flags) |
| `retro_memory_read` | Read memory block (base64) |
| `retro_memory_write` | Write memory block (base64) |
| `retro_pause` | Pause emulation |
| `retro_continue` | Resume emulation |
| `retro_reset` | Hard or soft reset |
| `retro_step_instruction` | Step one CPU instruction |
| `retro_breakpoint_add` | Set a CPU breakpoint |
| `retro_breakpoint_remove` | Remove a breakpoint |
| `retro_breakpoint_list` | List all breakpoints |
| `retro_machine_state` | Get run/pause state |
| `retro_load` | Load a PRG/XEX/NES ROM/D64 from a path the RetroDebugger process can open |
| `retro_watch_add` | Add a memory watch |
| `retro_watch_remove` | Remove a watch |
| `retro_watch_list` | List all watches |
| `retro_snapshot_save` | Save emulator state (base64) |
| `retro_snapshot_load` | Restore emulator state (base64) |
| `retro_transport_diagnostics` | Bridge connection status (live mode only) |
| `retro_reconnect` | Force bridge reconnect (live mode only) |

## Troubleshooting

### `retro_load` says the file was not found

The path is opened by the RetroDebugger process, not by the MCP client, so it
has to be valid on the machine running the debugger. A Windows build needs a
Windows path, drive letter included (`C:\docker-share\c64TestData\test.d64`);
a container-style path such as `/docker-share/c64TestData/test.d64` is rejected
with `retro_load failed: File not found: ...`.

### Claude says "MCP server failed to start"
- Build RetroDebugger first: `xcodebuild -project ./platform/MacOS/c64d.xcodeproj -scheme "Retro Debugger" -quiet`
- Check the binary exists: `ls ~/Library/Developer/Xcode/DerivedData/c64d-*/Build/Products/Release/Retro\ Debugger.app`
- Check logs: `ls -lt /tmp/RetroDebugger-*.txt | head -3`

### Live mode: tools show "desktop_unavailable"
- Start the RetroDebugger desktop app
- Check port 3563 is not blocked: `lsof -i :3563`
- Ask Claude: `Use retro_transport_diagnostics` to see bridge state

### Live mode: bridge doesn't connect
- The default target is `127.0.0.1:3563/stream`
- If the desktop app uses a different port, re-register with `--port`
- The bridge retries automatically with exponential backoff (500ms to 10s)

### Removing servers
```bash
claude mcp remove retrodebugger-headless
claude mcp remove retrodebugger-live
```
