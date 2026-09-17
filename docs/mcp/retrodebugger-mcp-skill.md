# RetroDebugger MCP Skill

## When to Use

Use this skill when working with the RetroDebugger MCP server to debug 8-bit programs on emulated C64, Atari XL/XE, or NES hardware.

## Available Tools

### Platform Discovery
- `retro_list_platforms` — list active emulators and their state

### Inspection (safe, read-only)
- `retro_cpu_status` — CPU registers (PC, A, X, Y, SP, flags)
- `retro_memory_read` — read memory block (base64 encoded)
- `retro_memory_search` — **search all RAM for a specific byte value** (returns list of matching addresses — use this to find lives/score/level counters)
- `retro_search_pattern` — search executed code for opcode patterns (`DEC ??`, `STA $0340`, etc.)
- `retro_screenshot` — **capture current screen as PNG** — use this to observe game state, verify patches, read on-screen text, or confirm the game has started/died. Two modes:
  - `retro_screenshot(platform)` — returns base64-encoded PNG inline
  - `retro_screenshot(platform, savePath:"/tmp/screen.png")` — saves PNG to file, returns metadata only (use this when you need to view the image with the Read tool)
- `retro_breakpoint_list` — list all CPU breakpoints
- `retro_watch_list` — list all data watches
- `retro_machine_state` — platform state (running/paused, PC) — **use this for pre-flight check, not retro_list_platforms**
- `retro_snapshot_save` — save emulator state (file path)

### Control (changes emulator state)
- `retro_pause` — pause emulation
- `retro_continue` — resume emulation
- `retro_reset` — hard or soft reset
- `retro_step_instruction` — step one CPU instruction
- `retro_step_cycle` — step one CPU clock cycle
- `retro_step_subroutine` — step over a JSR (runs until the subroutine returns)
- `retro_cpu_jump` — force PC to a specific address. Returns `{"status":"jumped","applied":true}`
  once the new PC has been committed to the CPU, so a following `retro_step_instruction`
  executes at the target. `{"status":"queued","applied":false}` means the change did not
  reach the CPU (emulation thread not running, or an unsupported backend) — do not chain a
  step onto a queued jump.
- `retro_cpu_counters` — read cycle, instruction, and frame counters
- `retro_warp` — enable/disable warp speed (run as fast as possible)
- `retro_media_detach` — detach all cartridges, disks, and tapes. **Power-cycles the C64** (RAM and CPU state are lost) — do not use it just to remove a disk
- `retro_disk_detach` — detach the disk from one drive **without resetting** (`platform`, optional `device`: C64 8-11 default 8, Atari 1-8 default 1). Use this when swapping or ejecting a disk mid-session
- `retro_segment_read` — get current debug symbol segment name
- `retro_segment_write` — set active debug symbol segment by name
- `retro_load` — load a program file (PRG, XEX, NES ROM, D64, CRT). The path is opened by the
  Retro Debugger process, so it must be valid on the machine running the debugger — a Windows
  build needs `C:\data\game.d64`, not `/data/game.d64`. A path the debugger cannot see comes
  back as an MCP error (`retro_load failed: File not found: ...`), not as a silent no-op

### Memory & Breakpoints (changes emulator state)
- `retro_memory_write` — write base64-encoded data to memory
- `retro_breakpoint_add` — add CPU execution breakpoint (fires when PC reaches address)
- `retro_breakpoint_remove` — remove CPU execution breakpoint
- `retro_breakpoint_list` — list all CPU execution breakpoints
- `retro_memory_breakpoint_add` — add memory write/read breakpoint; use comparison `>=`, value `0` to break on any write
- `retro_memory_breakpoint_remove` — remove memory write/read breakpoint
- `retro_memory_breakpoint_list` — list all memory write/read breakpoints
- `retro_watch_add` — add data watch at address
- `retro_watch_remove` — remove data watch
- `retro_snapshot_load` — restore emulator state from base64 snapshot

### Session control (ends the session)
- `retro_shutdown` — quit RetroDebugger through its normal File > Quit path.
  Settings, layouts, symbols and plugin state are saved.

### Platform-Specific Endpoints (via WebSocket API)

These are available as WebSocket endpoints, not yet wrapped as MCP tools:

**C64 (VICE):**
- `c64/vic/read`, `c64/vic/write` — VIC-II registers
- `c64/sid/read`, `c64/sid/write` — SID registers
- `c64/cia/read`, `c64/cia/write` — CIA 1/2 registers
- `c64/drive1541/cpu/status`, `c64/drive1541/cpu/memory/readBlock` — 1541 drive
- `c64/drive1541/via/read`, `c64/drive1541/via/write` — drive VIA registers
- `c64/savePrg` — save memory range as PRG file

**Atari XL/XE:**
- `atari800/antic/read`, `atari800/antic/write` — ANTIC display processor
- `atari800/gtia/read`, `atari800/gtia/write` — GTIA graphics
- `atari800/pokey/read`, `atari800/pokey/write` — POKEY sound/IO
- `atari800/pia/read`, `atari800/pia/write` — PIA joystick/banking
- `atari800/media/disk/attach` — mount ATR/XFD disk
- `atari800/media/tape/attach` — attach cassette
- `atari800/media/load` — load XEX executable

**NES:**
- `nes/ppu/read` — PPU registers ($2000-$2007)
- `nes/ppu/clocks` — scanline position (hClock, vClock, cycle)
- `nes/ppu/nametable/readBlock`, `nes/ppu/nametable/writeBlock` — nametable VRAM
- `nes/apu/read` — APU registers ($4000-$4017)
- `nes/apu/mute` — mute individual channels (square1/2, triangle, noise, dmc, ext)
- `nes/media/cart/insert` — load NES ROM
- `nes/fds/insert`, `nes/fds/eject`, `nes/fds/changeSide`, `nes/fds/status`, `nes/fds/setBIOS` — FDS control

## Safe Debugging Defaults

1. **Always inspect before mutating.** Read CPU status and memory before writing or stepping.
2. **Verify platform is not paused** before issuing commands. Use `retro_machine_state` — check `isPaused` and `isRunning`. If paused, call `retro_continue` first. PC=0 while paused is normal, not a crash.
3. **Pause before memory inspection** for consistent state. Memory can change between reads if emulation is running.
4. **Use platform parameter** on every tool call. Don't assume which emulator is active.
5. **Prefer read-only tools** when exploring. Use mutation tools only when you have a clear goal.
6. **Save snapshot before patching.** Use `retro_snapshot_save` before writing memory, so you can restore with `retro_snapshot_load` if something goes wrong.
7. **Trust the error flag.** Command tools surface endpoint failures as MCP
   errors (`<tool> failed: <reason>`), so a call that returns without an error
   really did reach the emulator. In live/bridge mode the debugger tools
   disappear from the roster entirely while no desktop app is attached, and a
   call that loses the connection mid-flight fails with `desktop_unavailable`.
8. **Never call `retro_shutdown` on your own initiative.** It ends the session —
   the app quits and every later tool call fails. Call it only when the user
   explicitly asked to close RetroDebugger. It is not a way to recover from a
   stuck emulator: use `retro_reset`, `retro_continue` or `retro_snapshot_load`
   for that.

### Using `retro_shutdown`

- `force: false` (default) gives the graceful path an 8 second watchdog;
  `force: true` shortens it to 1.5 seconds. **Force does not skip saving** —
  layouts, settings and plugin state are written either way. Use it only when a
  normal shutdown has already proven slow.
- `timeoutMs` overrides the watchdog deadline directly.
- `exitBridge` (default `true`, live/bridge mode only) also exits the local
  bridge process once the desktop app is gone, so nothing is left running.
  Set it to `false` to keep the bridge alive and reconnect to a later instance.
- The reply arrives *before* the app quits, so a normal result means the
  shutdown was accepted, not that it has already finished.
- If the desktop app is already gone, the tool still reports
  `desktop_unavailable_bridge_exiting` and cleans up the orphaned bridge.

## Platform Selection

| Platform | Name String | CPU | Chip Endpoints |
|----------|------------|-----|----------------|
| Commodore 64 | `c64` | 6510 (6502 variant) | VIC-II, SID, CIA, 1541 drive w/ VIA |
| Atari XL/XE | `atari800` | 6502 | ANTIC, GTIA, POKEY, PIA |
| NES | `nes` | 2A03 (6502 variant) | PPU, APU, FDS |

## Workflow: Inspect Current State

```
1. retro_list_platforms → see what's running
2. retro_cpu_status(platform:"c64") → see PC, registers
3. retro_memory_read(platform:"c64", address:<PC>, size:32) → see code around PC
4. Decode base64 data, disassemble as 6502 instructions
5. retro_breakpoint_list(platform:"c64") → see active breakpoints
6. retro_watch_list(platform:"c64") → see watched addresses
```

## Workflow: Find a Bug

```
1. retro_pause(platform:"c64") → stop execution
2. retro_cpu_status → note current PC
3. retro_snapshot_save(platform:"c64") → save state as safety net
4. retro_breakpoint_add(platform:"c64", address:<suspect>) → set breakpoint
5. retro_continue → run until breakpoint hits
6. retro_cpu_status → check registers at breakpoint
7. retro_memory_read → inspect relevant memory
8. retro_step_instruction → step through suspected code
9. Analyze and report findings
```

## Workflow: Find Game State Variable (e.g. lives counter)

```
1. retro_pause(platform) → freeze state
2. retro_memory_search(platform, value:4) → candidate list A (all addresses = 4)
3. retro_continue → resume
4. [trigger event that changes the value, e.g. lose a life]
5. retro_pause → freeze again
6. retro_memory_search(platform, value:3) → candidate list B (all addresses = 3)
7. Intersect A ∩ B → 1–3 addresses remain
8. Repeat (die again → search for 2) until single address confirmed
9. retro_breakpoint_add(platform, address:<candidate>) → confirm with write-breakpoint
10. retro_cpu_status → read PC when breakpoint fires → that's the patch target
```
**Never manually read large memory blocks to scan for values — always use retro_memory_search.**

## Workflow: Patch and Verify

```
1. retro_pause(platform:"c64") → stop execution
2. retro_snapshot_save(platform:"c64", path:"/tmp/backup.snap") → save state for rollback; note returned path
3. retro_memory_write(platform:"c64", address:<target>, data:<base64>) → patch
4. retro_continue → run and observe
5. If broken: retro_snapshot_load(platform:"c64", path:"/tmp/backup.snap") → restore
```

## Resources

- `retrodebugger://platforms` — active platform list (dynamic)
- `retrodebugger://reference/c64/memory-map` — C64 memory map + SID registers
- `retrodebugger://reference/c64/drive-1541` — 1541 drive memory map, VIA registers, GCR format
- `retrodebugger://reference/atari800/memory-map` — Atari memory map + ANTIC/GTIA/POKEY/PIA registers
- `retrodebugger://reference/nes/memory-map` — NES CPU/PPU/APU memory maps + register reference

## Binary Data Encoding

Memory read/write uses **base64** encoding:
- `retro_memory_read` returns `{ "data": "<base64>", "encoding": "base64", "byteCount": N }`
- `retro_memory_write` accepts `{ "data": "<base64>" }`

Snapshots use **file paths**, NOT base64 blobs:
- `retro_snapshot_save(platform, path)` — saves to a file, returns `{ "path": "...", "size": N }`
- `retro_snapshot_load(platform, path)` — loads from a file path
- `retro_snapshot_quick_save(platform, slot)` — saves to numbered slot file, returns `{ "slot": N, "path": "...", "size": N }`
- `retro_snapshot_load(platform, path)` — loads from the path returned by save

> **WARNING:** Snapshot files are several MB. MCP has a message size limit that makes base64-encoded blobs impractical — always use the file-path API, never attempt to pass snapshot data as base64 through MCP.
