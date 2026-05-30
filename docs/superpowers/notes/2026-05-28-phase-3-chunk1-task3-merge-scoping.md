# Phase 3 Chunk 1 — Task 2 done + Task 3 scoping & findings

**Date:** 2026-05-28
**Branch:** `vice-3.10-phase-3-vendor-swap`

## Landed this session

| Commit | Content |
|--------|---------|
| `36683b3` | **Task 2** — vendor-dropped 695 unhooked files (3.10 content + `types.h`→`vicetypes.h`). Verified: each file == vanilla-3.10(col2)+rename, 0 failures; 0 hook tokens in the changed set. |
| `4e41816` | **Task 3a** — 14 hooked files whose upstream delta did NOT overlap a hook → diff3 auto-merged with 0 conflicts. Verified: hook-token counts unchanged pre/post, no leaked markers, rename preserved. |
| `f76ee03` | **Task 3b** — 4 hooked headers (via1d1541.h, viad.h, via.h, interrupt.h): 3.10 content + gated c64d_ decls/fields adapted to 3.10 types. |
| `bc40261` | **Task 3c** — viacore.c (alarm_context+zero/uflow alarms; kept VICE_HOOK_VIA_IRQ_FLAG_CLEAR + c64d_viacore_peek) + c64cia1.c/c64cia2.c (uint sigs; kept VICE_HOOK_CIA* macros, externs, peek wrappers). |
| `f432ce7` | **Task 3d** — vicii-draw-cycle.c (VICE_HOOK_VIC_DRAW_SPRITES guard around 3.10's s→as switch; c64d_set_color_register). |

## REFINED Chunk 1 / Chunk 2 boundary (decided 2026-05-28 mid-Task-3)

Triaging the 30 conflict files revealed a clean split. **Chunk 1 = vendor drop +
tractable hook merges** (additive/localized hooks onto modestly-changed 3.10
code, verifiable via the additions-only OFF check). **Chunk 2 = heavy subsystem
rewrites** where 3.10 restructured the model AND slajerek's ungated c64d_
accessors read that model directly → need build feedback to re-apply correctly
(same character as the CPU cores). Forcing the heavy ones now, with no test net,
risks subtle timing/model bugs.

**Chunk 1 DONE (8 conflict files merged + 14 auto-merged + headers):** all of the
above commits.

**Chunk 1 TRACTABLE-MERGE PHASE COMPLETE (2026-05-28).** 24 hooked files merged
total (commits 4e41816, f76ee03, bc40261, f432ce7, 0eee15c). The
"remaining-tractable" candidates were re-triaged by conflict CONTENT (not count):
only `root/machine.c` and `c64/cart/reu.c` (`0eee15c`) were genuinely tractable;
the rest turned out heavy/coupled and moved to Chunk 2:
- `joyport/mouse.c` — 3.10 rewrote joyport + calls the REMOVED `clk_guard_add_callback`
- `c64/c64-snapshot.c` — 3.10 changed snapshot_create/drive_snapshot sigs; couples to reu.c
- `iecbus/iecbus.c` — 3.10 multi-drive iecbus restructure
- `root/vsync.c` + `viciisc/vicii.c` — 3.10 rewrote the vsync timing/metrics model; vicii.c calls it
- `sid/fastsid.c`, `sid/sid-resources.h` — entangled with SID 2→8 + ReSID-FP + float-sound

**Net: every remaining conflict file is now a Chunk 2 item** (see the deferral list
above + these). Chunk 2 = the heavy subsystem rewrites + CPU cores + monitor +
Tier C, reconciled WITH the infra (clkguard/diskunit/mainlock) using build feedback,
to a building + suite-green state.

**Remaining Chunk 1 tasks:** Task 5 (build-list reconciliation: drop GONE files —
clkguard.c, patchrom.c, platform/* — add NEW 3.10 files), Task 6 (build attempt →
categorized error-landscape doc = the Chunk 2 planning input). Task 4 infra
(clkguard include removal, diskunit rename, mainlock stub) is largely entangled
with the deferred subsystem files, so it folds into Chunk 2 rather than Chunk 1.

**Chunk 2 — heavy subsystem rewrites (defer, need build feedback):**
- CPU cores: c64/c64cpusc.c, drive/drivecpu.c (already deferred)
- monitor: monitor/monitor.c (already deferred)
- Tier C: resid/*, resid-fp/*, sounddrv/soundsdl.c (already deferred)
- **CIA core:** core/ciacore.c (SDR delay-line rewrite; c64d_ciacore_peek/get_cia_context)
- **drive/diskunit model** (3.10 dual-drive `diskunit_context[u]->drives[d]`):
  drive/drive.c, drive/drivemem.c, drive/iec/via1d1541.c, drive/iecieee/via2d.c,
  drive/iec/memiec.c — their c64d_ accessors read drive-VIA/drive internals
- **SID 2→8 + ReSID-FP:** sid/sid.c, sid/sid-snapshot.c, sid/sid-resources.c
- **mem model:** c64/c64memsc.c (mem_ram pointer; 33 hook-hunk-lines)
- **signature ripple:** root/keyboard.c + root/keyboard.h (3.10 added `mod` param,
  slajerek changed return void→int; ripples to ViceInterface callers)
- root/sound.c (SID-run/pause/mutex gating; 23 conflict hunks, entangled w/ sound subsystem)

Verification method (reusable, bash-3.2 safe — macOS has no `declare -A`):
- Per-file gate for the vendor drop: `sed 's/#include "vicetypes.h"/#include "types.h"/' live | diff -q - vanilla31` → empty ⇒ file diverged from 3.1 by ONLY the rename ⇒ safe to overwrite with `sed 's/types.h/vicetypes.h/' vanilla310 > live`.
- 3-way merge: `git merge-file -p --diff3 ours base theirs` where `base`/`theirs` are vanilla 3.1/3.10 **pre-renamed to vicetypes.h** (so the rename isn't a spurious conflict) and `ours` = the gated live file.

## The 64-file hook surface (token scan: `c64d_|RETRODEBUGGER|VICE_HOOK_`) — partition

- **44 mergeable hooked files** (all SAME-fate, vanilla path = filemap col2, both 3.1/3.10 exist):
  - **14 DONE** (Task 3a, clean auto-merge).
  - **30 CONFLICTED** — need hand resolution (135 conflict hunks total). See table below.
- **8 RetroDebugger arch glue** — `arch/{mousedrv,ui,ui.h,uimon,uimsgbox,uistatusbar,video,vsyncarch}.c` map (namesake) to vanilla `arch/gtk3|sdl/*`. These are RD's OWN archdep layer, NOT vendored VICE. **Leave as-is**; they will surface as archdep-interface errors in Task 6 → later integration chunk.
- **12 Chunk 2 deferrals:** `c64/c64cpusc.c`, `drive/drivecpu.c` (CPU-core inline); `monitor/monitor.c` (monitor subsystem); `resid/resid-{filter,sid}.{cpp,h}`, `resid-fp/residfp-{sid.cpp,sid.h,voice.h}`, `sounddrv/soundsdl.c` (Tier C).
- **1 infra (Task 4):** `c64/patchrom.c` — GONE in 3.10 (deleted). Fast-boot was `#ifdef RETRODEBUGGER`-gated → OFF-safe to drop.

## The 30 conflict files (Task 3 hand-merge queue)

`hookhunk` = conflict-hunk lines carrying hook tokens; `rename` = theirs-side lines with `diskunit_context`/`uint8_t`/`uint16_t`.

| File | hunks | hookhunk | rename | Notes |
|---|---|---|---|---|
| c64/c64-snapshot.c | 3 | 3 | 1 | snapshot signature ext + `_in_memory` fn |
| c64/c64cia1.c | 2 | 4 | 1 | |
| c64/c64cia2.c | 2 | 4 | 1 | |
| c64/c64memsc.c | 3 | 33 | 3 | `mem_ram` pointer-vs-array; heavy hooks |
| c64/cart/reu.c | 7 | 2 | 7 | REU — relevant to riscv-c64 work; rename-heavy |
| core/ciacore.c | 5 | 2 | 1 | CIA core |
| core/viacore.c | 1 | 1 | 0 | 3.10 added t1/t2 zero/underflow alarms; VICE_HOOK_VIA_IRQ_FLAG_CLEAR placement |
| drive/drive.c | 3 | 0 | 2 | **take-theirs UNSAFE**: ours has untokened `*_for_snapshot` GCR/P64 fields + NULL guards; 3.10 restructured to `diskunit_context[unit]->drives[d]` |
| drive/drivemem.c | 3 | 2 | 1 | |
| drive/iec/memiec.c | 8 | 6 | 7 | |
| drive/iec/via1d1541.c | 5 | 12 | 2 | |
| drive/iec/via1d1541.h | 1 | 1 | 4 | take theirs (diskunit+uint) + re-add gated `c64d_via1d1541_peek` |
| drive/iecieee/via2d.c | 4 | 10 | 3 | |
| drive/viad.h | 1 | 1 | 5 | take theirs + re-add gated `c64d_via2d_peek` |
| iecbus/iecbus.c | 2 | 25 | 1 | big hook regions |
| joyport/mouse.c | 3 | 6 | 2 | static-removal linkage |
| root/interrupt.h | 1 | 0 | 0 | **TRAP**: slajerek `DEBUG`→`VICE_DEBUG` rename half-merged by diff3 (some blocks kept VICE_DEBUG, conflict region would flip to DEBUG). Resolve ALL guards consistently to VICE_DEBUG, fold in theirs' `LOG_DEFAULT` logging. |
| root/keyboard.c | 23 | 14 | 0 | `key_pressed/released` void→int; many #else vanilla arms collide w/ upstream |
| root/keyboard.h | 1 | 0 | 0 | needs FULL 3.10-file context (3.10 added `key_custom_func_t`; also `key_pressed/released` signature) |
| root/machine.c | 3 | 2 | 0 | |
| root/sound.c | 23 | 33 | 1 | SID-run gating, pause path, mutex; Step-2 #else arms collide w/ upstream |
| root/via.h | 2 | 2 | 13 | take theirs struct (bool enabled, alarm_context, sr_underflow, uint types) + re-add gated `c64d_irq_flagged` field + `c64d_viacore_peek` decl |
| root/vsync.c | 8 | 10 | 0 | **COUPLED**: 3.10 changed `vsync_do_vsync(canvas)` (dropped skip_frame + raster_skip_frame wrap); slajerek added isPaused. Drives the vicii.c/raster callers. |
| sid/fastsid.c | 6 | 2 | 4 | |
| sid/sid-resources.c | 2 | 0 | 0 | **take-theirs UNSAFE**: ours has untokened `parsid_port` + `HAVE_RESID_FP` wiring (coupled to deferred ReSID-FP); 3.10 added USBSID |
| sid/sid-resources.h | 1 | 0 | 0 | 3.10 dropped stereo/triple addr, added sid2-8; ours added set_sid_engine/model externs |
| sid/sid-snapshot.c | 1 | 2 | 0 | |
| sid/sid.c | 7 | 12 | 8 | 2→8 SID support; `sid_read` restructure; `sid_sound_machine_open(+chipno)` |
| viciisc/vicii-draw-cycle.c | 1 | 1 | 0 | `VICE_HOOK_VIC_DRAW_SPRITES` guard region |
| viciisc/vicii.c | 3 | 0 | 0 | **take-theirs MOSTLY**: vsync_do_vsync caller (coupled to vsync.c) + 3.10 deleted `__MSDOS__` blocks |

## Critical findings (shape how the hand-merge must be done)

1. **"Take theirs" is NOT mechanically safe**, even for hookhunk=0 files. `drive.c`, `sid-resources.c`, `vicii.c` carry **functional, untokened** slajerek changes (snapshot GCR fields; ReSID-FP wiring; the isPaused param) that taking theirs would silently drop. Every conflict needs per-hunk judgment.
2. **diff3 can silently mis-merge** (no marker) by line-combining ours+theirs into plausible-but-wrong code — e.g. `interrupt.h` ended with `#ifdef VICE_DEBUG` (ours) wrapping `log_debug(LOG_DEFAULT,…)` (theirs). Must eyeball auto-merged regions of conflict files too, not just the markers.
3. **Coupling:** `vsync.c`'s new signature drives `vicii.c` (and other raster callers); `sid-resources.c`↔ deferred ReSID-FP; `drive.c`↔ the diskunit restructure. Resolve coupled files together.
4. **Some files need full 3.10-file context** (e.g. `keyboard.h`) — the diff3 hunk alignment alone is misleading.
5. **diskunit rename** lands naturally through these merges (theirs side) for hooked drive files; unhooked drive files already got it via Task 2. Task 4's diskunit step then mostly reconciles `ViceInterface/` consumers + verifies consistency.

## Suggested hand-merge order (low-risk → coupled)

1. **Headers:** via1d1541.h, viad.h, via.h (rename + re-add gated decl/field), interrupt.h (VICE_DEBUG consistency), keyboard.h (full-context).
2. **CIA/VIA cores:** core/viacore.c, core/ciacore.c, c64cia1/2.c — chip cores, careful.
3. **Drive cluster (with diskunit):** drive.c, drivemem.c, via1d1541.c, via2d.c, memiec.c, iecbus.c.
4. **SID cluster:** sid.c, sid-resources.c/.h, fastsid.c, sid-snapshot.c (note ReSID-FP coupling → may partially defer to Chunk 2).
5. **VIC-II + vsync coupling:** vsync.c + vicii.c + vicii-draw-cycle.c together.
6. **Remaining:** c64memsc.c (mem_ram), c64-snapshot.c, machine.c, mouse.c, reu.c, keyboard.c, sound.c (the two 23-hunk files last — most #else-arm collisions).

After all 30: Task 4 (clkguard/diskunit/patchrom/mainlock/soundsdl/resid infra), Task 5 (build lists), Task 6 (build attempt + error landscape). CPU cores + Tier C + monitor = Chunk 2.
