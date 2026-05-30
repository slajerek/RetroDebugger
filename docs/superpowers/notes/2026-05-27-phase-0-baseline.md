# Phase 0 — VICE 3.10 upgrade baseline findings

**Date:** 2026-05-27
**Branch:** vice-3.10-upgrade

## Reference sources acquired

- Vanilla VICE 3.1: `~/vice-ref/3.1/src/` (downloaded from SourceForge releases — vice-3.1.tar.gz)
- Vanilla VICE 3.10: `~/vice-ref/3.10/src/` (downloaded from SourceForge releases — vice-3.10.tar.gz)

## Baseline diff summary

Diff command: `diff -ruN ~/vice-ref/3.1/src/ src/Emulators/vice/`
Diff size: 2,715,007 lines (92 MB on disk)

The large size is expected: the `-N` flag in `diff -ruN` treats absent files as empty, so files that exist on one side only each contribute their entire content as additions or deletions. There are no "Only in ..." summary lines — every difference is rendered as a full unified diff.

Breakdown (using 1969-epoch timestamp as the marker for a file absent on that side):

- Files only in slajerek's tree (new additions — ViceInterface/, root/ extras, arch/ extensions): **408**
- Files only in vanilla 3.1 (absent from slajerek's tree — removed, restructured, or renamed): **7,174**
- Files present in both trees but different (the real modification set): **524**
- Files with `c64d_` / RetroDebugger hook markers (C64DEBUGGER, c64d, RetroDebugger, RETRO_DEBUGGER) in the canonical hook directories: **67**

Note: The expected 96 from the spec was not reached — the grep covered only 7 directories
(`c64/`, `drive/`, `monitor/`, `core/`, `root/`, `arch/`, `viciisc/`). The 67 count is correct
for those directories. Additional hook-carrying files may exist in other subdirectories not yet
surveyed (e.g., `sound/`, `resid/`).

The high "only in vanilla" count (7,174) reflects that slajerek's tree is a subset of vanilla
VICE: many platform/architecture/machine targets from the vanilla source (CBM-II, PET, VIC20,
Plus/4, C128, DTV, SCPU, etc.) were removed or not carried over. The in-tree `src/Emulators/vice/`
is scoped to the C64 + drive + monitor stack.

## Spot-check of modification style

Three representative files were diffed against their vanilla 3.1 counterparts:

### `c64/cart/reu.c` — 268 lines of diff

Additions (+79), Deletions (-24). The deletions are **not** functional removals — they are:
- `#include "types.h"` replaced by `#include "vicetypes.h"` (renamed header)
- `static` qualifier removed from several functions to expose them for external linkage (e.g.,
  `set_reu_enabled`, `set_reu_size`, `set_reu_filename`) — these become callable from ViceInterface
- One `static int reu_enabled` made `volatile int reu_enabled`

Additions include: `#include "ViceWrapper.h"`, `LOGD(...)` debug log calls, and
`c64d_maincpu_clk++` increments inserted at two clock-bump sites in REU cycle functions.

Pattern: mixed diff (both `+` and `-` lines), but the `-` lines are header renames and
`static`-to-public promotions, not logic deletions.

### `drive/drive.c` — 199 lines of diff

Additions (+138), Deletions (-1). The single deletion is `#include "types.h"` (replaced by
`#include "vicetypes.h"`). Everything else is pure addition:
- Two new fields initialized in `drive_init_structure`: `GCR_dirty_track_for_snapshot`,
  `GCR_dirty_track_needs_refresh`, `P64_dirty_for_snapshot`
- Commented-out `LOGD` stubs (left as `//`)
- NULL-guard additions in `drive_gcr_data_writeback_all` before dereferencing `drive_context[i]`
  and `drive` — these are safety patches, not control-flow changes
- 125-line block of new functions appended at end of file (beyond vanilla's last line)

Pattern: almost entirely additive; the one `-` line is a header rename.

### `monitor/monitor.c` — 186 lines of diff

Additions (+41), Deletions (-9). Deletions are header rename (`types.h` → `vicetypes.h`) plus
`static` qualifier removal from `make_prompt`, `monitor_open`, `monitor_process`, `monitor_close`
(same pattern as reu.c — promoting internal functions to external linkage). A closing brace and
one `monitor_close(1)` call are also restructured.

Additions include: `#include "DebuggerDefs.h"`, a new `execute_monitor_command_jump_trap`
function + trampoline inserted into `mon_jump`, and `c64d_set_debug_mode(DEBUGGER_MODE_RUNNING)`
calls in `mon_instructions_step`, `mon_instructions_next`, and `mon_instruction_return`.

Pattern: mixed diff; deletions are `static`-removal and header renames, not logic deletions.

**Overall modification style:** The dominant pattern across all three samples is:
1. Header rename: `types.h` → `vicetypes.h` (appears in every modified file)
2. `static` qualifier removal to expose functions
3. Pure additions: new `c64d_*` hook calls, `LOGD` debug traces, new functions appended at EOF
4. No original VICE logic is deleted or restructured — control flow is augmented, not replaced

## MTEngineSDL coupling

- MTEngineSDL location: `/Users/garion/develop/MTEngineSDL`
  (CMakeLists references it as `../MTEngineSDL` relative to the RetroDebugger repo root;
  the repo lives at `~/develop/MTEngineSDL`, not `~/Projects/MTEngineSDL`)
- Files referencing VICE internals: **7** (across 3 platform variants + 1 GUI file)
- Categories:
  - 6 files: logging infrastructure only — `DBG_Log.h`/`.mm` (macOS), `DBG_Log.h`/`.cpp` (Linux),
    `DBG_Log.h`/`.cpp` (Windows). These define three log-level bitmasks:
    `DBGLVL_VICE_DEBUG (1<<23)`, `DBGLVL_VICE_MAIN (1<<24)`, `DBGLVL_VICE_VERBOSE (1<<25)`,
    and the corresponding macros `LOGVD`, `LOGVM`, `LOGVV`.
  - 1 file: `src/Engine/GUI/Helpers/CGuiViewDebugLog.cpp` — renders toggle switches for the
    three VICE log levels in the engine's debug log UI.
- Concrete VICE symbols MTEngineSDL touches: **none**. MTEngineSDL references no VICE-internal
  symbols, types, or headers. The VICE coupling is entirely in named log-level constants
  (`DBGLVL_VICE_*`) that are defined within MTEngineSDL's own `DBG_Log.h`. These are
  string-labelled log channels, not structural dependencies on VICE internals.
- Risk assessment: **Low**. Upgrading VICE from 3.1 to 3.10 requires zero changes to MTEngineSDL.
  The log-level bitmasks are stable values with no tie to VICE version. No MTEngineSDL source file
  includes any VICE header or references any `c64d_*`, `vicii_*`, `maincpu_*`, or `drive_*`
  symbol. The only action needed is confirming MTEngineSDL is still at `../MTEngineSDL` relative
  to the repo at build time (it is, at `~/develop/MTEngineSDL`).

## Known surprises / open issues

1. **`types.h` → `vicetypes.h` rename is pervasive.** Every modified vanilla VICE file replaces
   `#include "types.h"` with `#include "vicetypes.h"`. This rename must be forward-ported
   mechanically when replaying patches onto 3.10 — the 3.10 source may have a different type
   header name or layout.

2. **`static` promotions create ABI surface.** The pattern of removing `static` from previously
   internal functions (to make them callable from ViceInterface) means those function signatures
   are part of the upgrade contract. Phase 1 must catalog every such promotion.

3. **7,174 "only in vanilla" entries.** The slajerek tree is a heavily stripped version of the
   vanilla source. The diff is not a delta of a complete VICE install — it is a curated subset.
   The 7,174 missing files are intentionally absent (other machine targets, autoconf infrastructure,
   build tooling). Phase 4 should not attempt to restore these.

4. **MTEngineSDL not at expected `~/Projects/MTEngineSDL` path.** CMakeLists says `../MTEngineSDL`
   (relative to the repo at `~/Projects/RetroDebugger/`), which resolves to
   `~/Projects/MTEngineSDL`. But the actual install is at `~/develop/MTEngineSDL`. This path
   mismatch will cause a build failure unless the CMakeLists path or a symlink is corrected
   before Phase 4 (the build phase). Track as a pre-build blocker.

5. **c64d_ file count discrepancy (67 vs spec's ~96).** The 67 count covers 7 subdirectories.
   The spec's ~96 likely includes additional directories not in the grep scope (e.g., `resid/`,
   `sound/`, platform-specific files under `arch/`). Phase 1 should run the hook scan with a
   wider glob.

## Followup decisions for Phase 1

- Phase 1 hook catalog should grep all of `src/Emulators/vice/` recursively (not directory-by-
  directory) to avoid missing hook files outside the 7 directories surveyed here.
- Phase 1 should specifically catalog every `static`-removal (function promotion) in addition
  to `c64d_*` hook sites — these are implicit API surface.
- Phase 1 should note the `types.h`→`vicetypes.h` rename in each file, since this is a
  mechanical change that will need automation when replaying patches onto 3.10.
- Before Phase 4 begins: resolve the MTEngineSDL path discrepancy (symlink or CMakeLists edit).

## WebSocket regression suite baseline (3.1)

Date captured: 2026-05-27
RetroDebugger: 3.1 (slajerek's current fork)

### Summary

- Total tests: 30
- Passed: 28
- Skipped: 2
- Failed: 0 (suite must be green to merge this plan)
- Suite wall time: 21.5 seconds

### Skipped tests (3.1 baseline findings — Phase 4 fix targets)

- **`test_load.py::test_load_nonexistent_file_does_not_crash`** — "Phase 4 bug: RD 3.1 closes the WebSocket with no close frame when load is given a nonexistent path — the bridge crashes instead of returning an error response. Baseline finding; fix needed before Phase 4."
  - Phase 4 expected behavior: 3.10 should return a 4xx error response and keep the WebSocket connection alive.

- **`test_input.py::test_key_down_accepted_by_api`** — dynamically skipped at runtime when KERNAL keyboard buffer is empty after key/down+up: "KERNAL keyboard buffer empty after key/down+up — known threading limitation: keyboard alarm_set() is called without LockMutex from the WebSocket handler thread (see module docstring). Flag for Phase 4: wrap key/down and key/up in LockMutex in CDebuggerServerApiVice.cpp."
  - Phase 4 expected behavior: wrapping key/down and key/up in LockMutex in CDebuggerServerApiVice.cpp should make the KERNAL keyboard buffer reliably populated, and the buffer-level assertion should pass.

### Behavioral notes captured in test code (preserved 3.1 contracts)

These are NOT bugs — they are behaviors the suite EXPECTS, so we know if 3.10 changes them.

- `makejmp` is queued, takes effect on next step_instruction
- `step/cycle` counter updates asynchronously, needs settle window
- `reset(hard=True)` from paused state keeps CPU paused
- `c64/cpu/memory/breakpoint/add` requires `comparison` + `value` params even for "any write" (e.g. `comparison=">="`, `value=0`)
- Async breakpoint-fired event frames pollute response stream (test_breakpoints uses local `_drain_events` helper)
- Chip endpoints: response wraps results in a `registers` key (not bare dict at `result`)
- CIA endpoint param is `num`, not `cia`
- Joystick endpoint uses `axis` (compass names like `"n"`, `"sw"`), not `direction`
- Keyboard endpoint uses `keyCode` (int, ASCII for letters)
- `cpu/counters/read` `cycle` counter resets to 0 asynchronously ~10-20ms after `cont()` is called — it is NOT monotonic across pause/cont boundaries; `reset(hard=True)` does NOT reset it
- VIC register map is serialized as a JSON array of `[reg_num, value]` pairs (not a plain dict) — due to nlohmann/json serialization of C++ unordered_map

### Suite is the gate

This is the behavior contract preserved through Phase 2 (refactor on 3.1) and
re-established during Phase 4 (3.10 integration). Any divergence is either:

1. **A fix** — the test was wrong, update it
2. **An expected change** — 3.10 behaves differently for a documented reason

The suite must run green before each merge gate during the upgrade.

## Phase 0 + 0.5 + 1 closeout

**Date completed:** 2026-05-27

### Artifacts produced

- Branch `vice-3.10-upgrade` on origin (all commits pushed)
- `~/vice-ref/3.1/src/` — vanilla VICE 3.1 reference (outside repo)
- `~/vice-ref/3.10/src/` — vanilla VICE 3.10 reference (outside repo)
- `tests/websocket/` — pytest regression suite, 30 tests (28 pass / 2 skip), runtime ~21.5 s
- `tests/fixtures/known_state.{asm,prg,sym}` + `build.sh` — fixture infrastructure (park label = $0837)
- `tests/retrodebugger.py` — vendored WebSocket client
- `tests/run-ws-tests.sh` — suite runner
- `docs/vice-hook-surface.md` — Phase 1 catalog (1234 lines): 64 modified files, 633 call sites, 282 distinct `c64d_*` symbols, 15 hook categories, ~14 files with genuine non-additive changes

### Catalog headline findings (drive the Phase 2-5 plan)

- **"Essentially all additive" upheld with nuance.** ~50 of 64 files are pure additive. The large
  apparent "deletion" counts in `c64cpusc.c` (166) and `drivecpu.c` (409) are **inline expansion**
  of `mainc64cpu.c` / `6510core.c` into the host file (slajerek inlined the CPU core to inject
  hooks), NOT removed logic. Verified directly: `c64cpusc.c` contains a "mainc64cpu.c starts here"
  block and is 4357 lines.
- **Genuine non-additive patches** (confirmed against corrected ground-truth diff): `resid-filter.cpp`
  (261 lines — 8580 SID filter section deleted, only 6581 kept), `soundsdl.c` (155 — audio callback
  rewrite), `resid-sid.cpp` (89 — TTL/read-path behavior + waveform-callback ctor param), plus
  smaller logic changes in `sid.c`, `joystick.c`, `keyboard.c`, `vsync.c`, `c64-snapshot.c`,
  `ciacore.c`, and several `viciisc/` files.

### Hardest 3.10 migration risks (from catalog)

1. **CRITICAL — CPU inline expansion.** `c64cpusc.c` / `drivecpu.c` must be re-inlined from 3.10's
   `mainc64cpu.c` / `6510core.c`, and 3.10 **removes `clkguard.c/.h`** which slajerek's tree uses
   (`root/clkguard.c`).
2. **HIGH — soundsdl.c.** Moved to `arch/shared/sounddrv/soundsdl.c` in 3.10; callback structure differs.
3. **HIGH — monitor.** 3.10's `monitor.c` grew ~1000 lines; injection points must be re-identified.
4. **HIGH — c64memsc.c `mem_ram`.** slajerek converted it to a pointer; 3.10 reverts to a static array.

### Phase 4 vanilla-path mapping cheat sheet

When replaying patches, map a repo file to its vanilla equivalent as follows (verified 2026-05-27):

| Repo path pattern | Vanilla 3.1 path |
|---|---|
| `src/Emulators/vice/<sub>/<f>` | `~/vice-ref/3.1/src/<sub>/<f>` (note the `src/` component) |
| `src/Emulators/vice/root/<f>` | `~/vice-ref/3.1/src/<f>` (slajerek's `root/` = vanilla top-level `src/`) |
| `src/Emulators/vice/resid/resid-<x>.cpp` | `~/vice-ref/3.1/src/resid/<x>.cc` (renamed: `resid-` prefix dropped, `.cpp`→`.cc`) |
| `src/Emulators/vice/resid-fp/residfp-<x>.cpp` | check `~/vice-ref/3.1/src/resid-fp/` for the analogous `.cc` |

A naive `diff` that drops the `src/` prefix or ignores the resid rename will silently skip files
(the `[ -f ]` guard treats a missing vanilla file as "all additive"). Use the mapping above.

### Open issues for Phase 2 planning

- **MTEngineSDL path mismatch** (pre-build blocker for Phase 4): CMakeLists expects `../MTEngineSDL`
  → `~/Projects/MTEngineSDL`, but it lives at `~/develop/MTEngineSDL`. Symlink or CMakeLists edit needed.
- **2 skipped tests** are 3.1 bugs (load-crash, keyboard mutex race) — Phase 4 fix targets, listed above.
- **Async event-frame pollution** in the WebSocket stream — the client's `call()` can return an event
  frame instead of the response. Currently worked around per-test (`_drain_events`); Phase 2/4 may
  want to fix this in the shared `retrodebugger.py` client.

### Phase 2 plan input

The followup plan (Phases 2-5) should:
1. Begin with `VICE_HOOK_*` macro-family design driven by the catalog's 15 categories
2. Refactor on 3.1 in `src/Emulators/vice/`, keeping the regression suite green between every commit
3. Treat the CPU inline-expansion files and the ~14 non-additive patches as the high-risk set
   requiring careful per-file handling, distinct from the mechanical additive-hook sweep
