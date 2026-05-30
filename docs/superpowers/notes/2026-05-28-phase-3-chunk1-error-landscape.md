# Phase 3 Chunk 1 — Task 6: build attempt + error landscape

**Date:** 2026-05-28
**Branch:** `vice-3.10-phase-3-vendor-swap`
**Build:** macOS Release, `xcodebuild ... -configuration Release -arch arm64` (unsigned). Log: `/tmp/chunk1-build.log`.

## Result: BUILD FAILED (exit 65), failed FAST on missing headers

The build stopped after 5 errors — all **missing-header** errors — before reaching a
single `CompileC` of a vice translation unit's *body*. So we have NOT yet seen the
type/subsystem error wave; the tree is blocked at the `#include` resolution layer.

The 3 distinct first errors:
- `root/uiapi.h:127` → `arch/shared/uiactions.h` not found
- `root/maincpu.h:31` → `mainlock.h` not found
- `c64/c64rom.c:42` → `sha1.h` not found

## HEADLINE FINDING: the vendor drop is incomplete — 3.10's NEW files were never added

Task 2 only **replaced same-path files** (858 SAME + the MOVED/hooked merges). It never
**added** the files 3.10 introduced. The vendor-dropped 3.10 files now `#include` those
new headers, which don't exist in the tree → immediate build failure.

Static scan (`#include "x.h"` in the vice tree whose basename has no file in the tree,
cross-referenced against `~/vice-ref/3.10/src/`): **62 genuine new-3.10 headers referenced
but absent** (the other 86 "missing" are RetroDebugger GUI / system headers that resolve via
other search paths — false positives). The 62, grouped:

| Group | Examples | ~count |
|-------|----------|--------|
| **New cartridge types** | bisplus, blackbox3/4/8/9, bmpdataturbo, drean, freezeframe2, gmod3, hyperbasic, ieeeflash64, ltkernal, magicdesk16, maxbasic, megabyter, multimax, partner64, profidos, ramlink, rexramfloppy, sdbox, spaceballs, tapecart, turtlegraphics, uc1, uc2, zippcode48 | ~25 |
| **New userport devices** | userport_{diag_pin,funmp3,hks_joystick,hummer_joystick,io_sim,petscii_snespad,ps2mouse,spt_joystick,superpad64,synergy_joystick,wic64,woj_joystick} | ~12 |
| **New drive types** | cmdhd (drive/iec/cmdhd.h) | 1 |
| **New / reorganized subsystems** | `mainlock.h` (mainlock — needed by 3.10 mainc64cpu), `sha1.h` (ROM checksums, used by c64rom), `arch/shared/uiactions.h` + `uihotkeys.h` (UI actions/hotkeys), `profiler.h` (new profiler), `monitor_binary.h` (binary monitor), `render-common.h` (video render), `fsdevice-filename.h` (fsdevice split), `archdep.h`/`archdep_dir.h`/`archdep_exit.h` (archdep reorg), `rawnetarch.h`, `opencbm.h`, `plus4parallel.h`, `iec-ieee488-shared.h`, `usbsid.h` | ~15 |

Each new header generally has a `.c` counterpart that also must be added to the tree AND the
build lists. So the real first task is a **second vendor-drop pass that ADDS 3.10's new files**
(largely mechanical: copy from `~/vice-ref/3.10/src/` + `types.h`→`vicetypes.h` rename + add to
CMake/Xcode). Most (carts, userport devices) are self-contained; a few (mainlock, archdep,
uiactions) need integration with RetroDebugger's own arch layer.

## Second finding: vicetypes.h must be reconciled to 3.10's type model

`src/Emulators/vice/arch/vicetypes.h` (slajerek's shim, included everywhere via the rename)
diverges from vanilla 3.10 `types.h`:
- **`CLOCK` is 32-bit** (`typedef DWORD CLOCK`, DWORD=`unsigned int`) vs 3.10's **`typedef uint64_t CLOCK`**. This won't error (silent truncation) but is a real correctness bug — and 3.10's clock-overflow handling (which replaced `clkguard`) assumes 64-bit CLOCK. Must change to `uint64_t` + `CLOCK_MAX`.
- 3.10 `types.h` includes `<stdint.h>`/`<inttypes.h>` + **`<stdbool.h>`** unconditionally; vicetypes.h only pulls stdint under `#ifdef LINUX` (relies on SDL on macOS) and never includes stdbool — but 3.10 now uses `bool` (e.g. `via_context_s.enabled`). vicetypes.h must unconditionally provide the stdint types + `bool`.
- Keep the `BYTE`/`WORD`/`DWORD`/`SWORD` shims (slajerek's ungated c64d_ code still uses them).

## Still-deferred (will surface once the header layer clears) — Chunk 2 subsystem merges

The 22 conflict files left at 3.1 content (CIA core, drive/diskunit cluster, SID 2→8 + ReSID-FP,
c64memsc mem model, keyboard signature ripple, sound.c, vsync+vicii, snapshot, mouse) plus the
CPU cores (c64cpusc/drivecpu), monitor.c, and Tier C resid/soundsdl. These need build feedback,
which only becomes available after the header layer + vicetypes are fixed.

## GONE files to remove from the build (Task 5)

`root/clkguard.c`, `c64/patchrom.c`, `platform/*.c` (11) — deleted/removed upstream. Still in the
build list (and still present at 3.1 content). Remove from CMake + Xcode Sources phase.

## Recommended Chunk 2 plan (in order)

1. **Complete the vendor drop:** add 3.10's new files (62+ headers + their .c) to the tree
   (mechanical copy + rename) and to the build lists; remove the GONE files. Reconcile
   `vicetypes.h` (CLOCK=uint64_t, unconditional stdint/stdbool). → clears the header layer.
2. **Heavy subsystem merges** (the 22 deferred conflict files) + CPU-core re-inline
   (c64cpusc/drivecpu) + monitor + Tier C resid/soundsdl, reconciled WITH the cross-cutting
   infra (clkguard removal in includers, diskunit dual-drive rename, mainlock stub). Iterate
   against the compiler.
3. **Link + run:** resolve undefined symbols (new-file/build-list gaps), get the app launching,
   then re-enable the WebSocket regression suite as the acceptance gate (target 28/1/1).

## Note on this build attempt

Built as-is (no build-list surgery) for the fastest, safest signal. Because it failed at the
header layer, the GONE-file errors and the type/subsystem error wave were never reached — they'll
appear after step 1 above. The key takeaway (incomplete vendor drop + vicetypes mismatch) is the
actionable Chunk 2 starting point.
