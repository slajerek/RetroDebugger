# RetroDebugger MCP Skill

## When to Use

Use this skill when working with the RetroDebugger MCP server to debug 8-bit programs on emulated C64, Atari XL/XE, or NES hardware.

## Available Tools

### Platform Discovery
- `retro_list_platforms` — list active emulators and their state

### Inspection (read-only; see memory-read limits below)
- `retro_cpu_status` — CPU registers (PC, A, X, Y, SP, flags)
- `retro_memory_read` -- read CPU-visible memory (default), or C64 physical RAM with `memorySpace:"ram"` (base64 encoded)
- `retro_memory_last_access` — last recorded reader/writer PC and instruction cycle for one CPU address; no memory read, GUI navigation or rewind
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
- `retro_drive1541_pause(platform:"c64", device:8, timeoutMs:1000)` -- request whole-emulator pause without stepping and report observed mode/parked state.  Check `outcome`; timeout, busy, and unavailable are not successful suspension.  No exclusive lifecycle control.  See [drive8 control](drive1541.md).
- `retro_drive1541_run_until(platform:"c64", address:49152, device:8, timeoutMs:1000)` -- from an observed pause, resume toward an owned drive8 target without using user breakpoints.  Check `outcome`, `ownedTargetHit`, and `parked` separately; timeout disarms the target and requests pause, not forced suspension.
- `retro_drive1541_state(platform:"c64", device:8)` -- read actual drive registers/PC, complete flags, cycles, model, RAM adapter length, head position, and observed pause state.  No execution changes or global stop attribution; check `available`, and treat null values as unavailable.  Running samples are best effort, not coherent snapshots.
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
- `retro_disk_directory` -- inspect a mounted native C64 D64/G64 standard DOS directory (`platform:"c64"`, optional integer `device`: 8--11).  Check `complete` and `status`; busy, unavailable, timeout, and partial results are not empty directories.  No GUI, flush, host-file fallback, or run/pause change.  See [usage, result fields, and paused-site limits](../mcp-server/disk-directory.md).
- `retro_disk_attach` — attach a **known-valid** disk to C64 device 8 (`platform`, host-visible `path`, optional `device`: 8) **without resetting or loading/running the first PRG**, regardless of GUI autorun settings. Use after `retro_disk_detach` at a disk-change prompt; it is not a substitute for a program load. Bundled VICE has a known failed-replacement bug: an existing invalid image can leave the old mounted disk in an inconsistent, non-ejectable state despite an attach error. Validate media before live swaps
- `retro_drive_dos_status` -- inspect qualified native C64 buffered DOS status with `{"platform":"c64","device":8}` (devices 8-11).  Check `available` and `outcome`; never treat this as a fresh IEC reply or media-health proof.  Busy/timeout preserves pause and can retain one pending task until later drain.  See [supported firmware, provenance, and examples](dos-status.md).
- `retro_segment_read` — get current debug symbol segment name
- `retro_segment_write` — set active debug symbol segment by name
- `retro_load` — load a program file (PRG, XEX, NES ROM, D64, CRT). The path is opened by the
  Retro Debugger process, so it must be valid on the machine running the debugger — a Windows
  build needs `C:\data\game.d64`, not `/data/game.d64`. A path the debugger cannot see comes
  back as an MCP error (`retro_load failed: File not found: ...`), not as a silent no-op

### Interactive GUI controls
- `retro_gui(action:"capabilities")` -- check visible GUI/test-driver availability.  Only the C64 Memory Breakpoints panel and add editor are supported.  Actions are queued; poll the returned request ID to a terminal outcome before continuing.  GUI input is separate from emulator keys.  See [safe call sequences, capture output, and deadline semantics](gui-control.md).

### Existing C64 Timeline
- `retro_timeline(platform:"c64", action:"status")` -- read-only recording/history/frame-range status.
- `retro_timeline(platform:"c64", action:"frame", frame:120)` -- queue existing frame navigation from an already paused emulator.  Check `accepted`; `status:"queued"` is not completed or precise restoration.  Replay can temporarily resume execution.  No automatic recording or instruction/cycle seek.  See [the acceptance-only contract](timeline-navigation.md).

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
2. **Verify platform is not paused** before issuing commands. Use `retro_machine_state` — check `isPaused` and `isRunning`. If paused, call `retro_continue` first. PC=0 while paused is normal, not a crash.  Exception: `retro_timeline` frame navigation requires an already paused emulator -- do not resume before that request.
3. **Pause before memory inspection** for consistent state. Memory can change between reads if emulation is running.
4. **Use platform parameter** on every tool call. Don't assume which emulator is active.
5. **Prefer read-only tools** when exploring. Use mutation tools only when you have a clear goal.
6. **Save snapshot before patching.** Use `retro_snapshot_save` before writing memory, so you can restore with `retro_snapshot_load` if something goes wrong.
7. **Trust the error flag.** Command tools surface endpoint failures as MCP
   errors (`<tool> failed: <reason>`), so a call that returns without an error
   really did reach the emulator. In live/bridge mode the debugger tools
   disappear from the roster entirely while no desktop app is attached, and a
   call that loses the connection mid-flight fails with `desktop_unavailable`.
   For `retro_timeline`, also check `accepted`: a well-formed request can return
   `status:"refused"` without an MCP transport error, and `queued` is not completion.
8. **Never call `retro_shutdown` on your own initiative.** It ends the session —
   the app quits and every later tool call fails. Call it only when the user
   explicitly asked to close RetroDebugger. It is not a way to recover from a
   stuck emulator: use `retro_reset`, `retro_continue` or `retro_snapshot_load`
   for that.

### Finding the last reader or writer

Call `retro_memory_last_access(platform:"c64", address:1024)` (also `atari800` or
`nes`; integer address 0–65535). The native endpoint is
`<platform>/cpu/memory/lastAccess`. The result contains `platform`, `address`,
`addressSpace:"cpu"`, and separate `read`/`write` records with `available`, `pc`,
`cycle`, `frame`, `rasterLine`, and `rasterCycle`.

- `available:false` means no PC-and-cycle-bearing record is retained; all record
  fields are `null`. PC zero and cycle zero are valid, not absence sentinels.
- `cycle` uses the backend's **execution-counter units**, not a guarantee of the
  exact bus-access cycle. C64/Atari report instruction-start CPU cycles; NES
  currently counts instructions, not hardware cycles. Do not compare these
  values across platforms. Frame/raster fields are `null` when not recorded.
- Records are copied coherently without pausing or reading emulated memory. A
  running emulator can produce a newer record immediately after the query; pause
  it separately when you need stable follow-up disassembly.
- This is the latest tracked access, not all writers/readers, a value's full
  provenance, or necessarily a physical RAM bank. Bank switches, mirrors, ROM,
  hardware/DMA and debugger-originated accesses follow each backend's existing
  marker hooks; the result does not resolve their physical identity.
- These are debugger records, not snapshot state or an execution-epoch guarantee.
  Resets/rewinds can leave retained records from an earlier timeline. Existing
  visual-only marker clearing preserves them; read/write-marker clearing erases
  them. An access hook without PC/cycle metadata invalidates that side's record
  instead of reporting an older or fabricated instruction.
- C64U hardware and 1541-drive address spaces are not supported by this tool.
  Missing/unavailable endpoints are errors, not successful empty results.

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

## Memory Read Selection and Limits

`retro_memory_read` accepts optional `memorySpace:"cpu"|"ram"`.  Omission or explicit `"cpu"` retains the CPU-visible adapter and existing result envelope on all supported platforms.  Explicit `"ram"` is C64-only and routes to the existing `c64/ram/readBlock` endpoint; other platforms, unknown selectors, and non-string selectors are refused before dispatch, without fallback.  No memory-space field is added to the response.

`length` takes precedence over the legacy `size` alias; if both are absent, the handler reads one byte.  The schema continues to advertise `platform`, `address`, and `length` as required, while the handler retains the legacy `platform:"c64"` default.  Address and selected length must be JSON integers, not booleans or floats.  Valid byte addresses are 0–65535.  Address 65536 is accepted only when length is zero, for compatibility with existing empty-read behavior.  Length must satisfy `0 <= length <= 65536 - address`.  Last-byte and full-64-KiB reads remain accepted.  Native CPU and RAM block handlers independently validate `{address,size}` against their adapter length before narrowing, allocation, or access.  Invalid native ranges/types return status 413 with no binary bytes.

Physical RAM reads backing bytes beneath ROM, I/O, and cartridge mappings, including raw RAM bytes at `$0000/$0001` rather than CPU-port register values.  RAM bypasses device-read callbacks, but the block is **not an atomic snapshot** against concurrent emulation or restoration.  The tool does not pause, switch banks, or restore snapshots.  CPU-visible reads are debugger peeks, not guaranteed exact CPU bus reads or universally side-effect-free inspection; an existing Ultimax undefined-return defect remains outside this change.  ROM, VIC, cart selectors, and arbitrary banking are not supported.

Example: `retro_memory_read(platform:"c64", address:40960, length:16, memorySpace:"ram")`.

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
