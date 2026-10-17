# Review of "Source-Level Debugging & IDE Evolution" spec v1.3

**Date:** 2026-10-17 · **Base:** `devel` 778ef3a · **Branch:** `c-source-debugging`
**Outcome:** spec revised to v1.4 (`source-level-debugging-ide-spec.md`, section #20), backlog extended (RD-031..RD-042), `TRACKER.md` and `IMPLEMENTATION-PLAN.md` added.

This review reads v1.3 against the actual repository. v1.3 was written without the
source tree (its own #16 says so) and is architecturally sound; most corrections are
about what already exists, what the emulator cores really expose, and a few internal
inconsistencies. Verdicts: **Keep** (no change), **Corrected** (text changed in 1.4),
**Added** (new content/backlog item), **Open** (needs the maintainer's decision).

## 1. Findings that change the plan

### F1. "Existing TASM integration" does not exist — Corrected
v1.3 refers to TASM in the header, #1.3, #2 (P0), #3, #5.3, #7.4, #9.1, #9.4, #10.3,
#13, RD-001 and open question 8. There is no TASM anywhere in `src/`, `docs/` or
`tools/`. What exists instead:

- KickAssembler `.dbg` XML loader: `CDebugSymbols::ParseSourceDebugInfo` →
  `CDebugAsmSource::ParseXML` (`src/DebugInterface/Symbols/CDebugAsmSource.cpp:67-694`),
  format documented in `docs/README-RetroDebugger.txt` ("KickAss debug symbols").
- Embedded 64tass (`src/Tools/64tass`) used only for in-memory assembly
  (`CDebuggerApi::Assemble64Tass*`, MCP `retro_assemble`); no debug output.
- KickAssembler invoked through Java by the CrtMaker plugin
  (`src/Plugins/CrtMaker/CViewC64CrtMaker.cpp:395-430`).

All references now name these three. Backward compatibility (open question 8) is
redefined around them.

### F2. An address→source map already exists — Corrected
`CDebugSymbolsSegment::codeSourceLineByMemoryAddress[]` is a flat per-segment array of
`CDebugAsmSourceLine*` with line **and column** ranges, filled from KickAss `.dbg`
`<Block>` rows (`START,END,FILE_IDX,LINE1,COL1,LINE2,COL2`). v1.3 planned the canonical
database as new work; 1.4 makes it a **migration** of this structure (#6.1 migration
rule, decision #20.6.1). The `.dbg` loader becomes the first provider and the regression
anchor (RD-038, fixture `assets/tests/c64/dankwars.prg` + `.dbg`, currently unused).

### F3. KickAss segments are already a "physical identity candidate" — Corrected
One `CDebugSymbolsSegment` per `<Segment>`, each with its own labels, watches,
breakpoints and source map; exactly one is active (`currentSegment`) and every emulator
hook evaluates only that one. Selection is manual (menu, `;`/`'`, `-segment`, remote
`segment/write`). v1.3's bank-aware model maps naturally onto this: RD-025 now
includes "mapping-driven segment activation" instead of inventing a parallel concept.

### F4. Step-over lives in a view and is unsafe — Corrected / Added RD-037
`CDebugInterface::StepOverSubroutine` delegates to `CViewDisassembly::StepOverJsr`
(`src/Views/CViewDisassembly.cpp:3224-3261`): temporary breakpoint at `pc+3`, no depth
check, not recursion-safe, not bank-aware. There is no step-out and no run-to-cursor.
The temporary breakpoint is a bare `int` per `CDebugBreakpointsAddr`, checked **before**
user breakpoints in VICE (a user breakpoint at the same address is skipped), and
produces no event/broadcast. RD-037 moves stepping into `CDebugInterface`, adds
instruction-level Step Out and fixes the priority before RD-013 builds on it.

### F5. The source-step controller must run in the emulation thread — Corrected (#8.1)
VICE's `c64d_set_debug_mode` has a watchdog that forces PAUSED after >300 single-step
re-arms less than 5 ms apart (`c64cpusc.c:4473`). A UI loop issuing
`RUN_ONE_INSTRUCTION` repeatedly (the obvious naive implementation) would be killed by
it. The state machine of #8.3 is evaluated inside the per-instruction hook (RD-011
adds a generic callback at the existing hook points), which is also what the spec's
pseudocode already implied. Also recorded: VICE parks a single step **after** the
instruction, Atari800/Nestopia **before** the next one (same observable PC contract);
VICE allows mid-instruction pauses from the per-cycle hook, which the step engine must
ignore; `BRK` auto-pauses in VICE; the 1541 CPU shares the C64 debug-mode word.

### F6. No emulator exposes bank identity to the debugger — Corrected / Added RD-040
v1.3 assumed providers would query "existing emulator interfaces". Reality:

| Machine | State exists in the core | Reachable from debugger code |
|---|---|---|
| C64 | `mem_get_current_bank_config()`, `roml_bank`/`romh_bank`/`export_ram`, per-cart peek | No (only `$01`, EXROM/GAME, Ultimax phi) |
| NES | `Memory::GetBank<>()` behind protected `Board::prg`, private `Cartridge::board`; iNES/NES2 header | No (CPU peek of the current window only) |
| Atari | `MEMORY_xe_bank`, `PIA_PORTB`, `CARTRIDGE_main{type,state}`, `under_*` arrays | No (`MEMORY_mem` is the CPU image; "RAM" view is the CPU view) |

RD-027/028/029 now explicitly include the accessor patches in the embedded cores
(`src/Emulators/...`, allowed; MTEngineSDL and the `[C64D-REFERENCE-ONLY]` copies are
not). v1.3 had C64 (RD-028) and Atari (RD-029) providers but **no NES provider item**;
RD-040 fills that hole. Three core bugs found on the way are attached to those items:
`nesd_peek_safe_io` reads past the 2 KB RAM array for `$0000`/`$2000-$4FFF`;
`c64d_peek_c64` can return an uninitialized byte in Ultimax open areas and bypasses
per-cartridge peek; the VICE cartridge adapter is hard-coded to 512 KB ROML.

### F7. CI runs the suite on macOS only — Added RD-034
`build-linux.yml` installs xvfb but runs no app tests; `build-windows.yml` runs none.
The "parallel three-machine matrix" acceptance gate is unmeasurable in CI until the
headless suite runs on Linux at least. `AGENTS.md` requires all three platforms green
on both log legs; RD-034 is scheduled before P3 acceptance.

### F8. Format collision: `.dbg` — Added RD-031
cc65/ld65 and KickAssembler both emit `.dbg`. Today loading is dispatched by extension
in three places (`CMainMenuBar`, `CMainMenuHelper::LoadLabelsAndWatches`,
`C64CommandLine`). RD-031 introduces a content-sniffing loader registry.

### F9. Process launching — Open (decision 11), Added RD-042
#9.3 requires argv arrays, cwd, env and timeout. The only launcher used is the engine's
`SYS_ExecSystemCommand(shellString, ...)`. Either propose an argv launcher to
MTEngineSDL or add app-side platform code. Needed for P5 only.

## 2. Internal consistency fixes in v1.3

- **Backlog "Priority" column mixed priority and phase.** RD-007 was "P1" but belongs
  to phase P2; RD-016 "P2" belongs to P4; RD-021 "P3" belongs to P5. The 1.4 table has
  separate `Phase` (delivery phase of #2) and `Priority` (track) columns. — Corrected
- **RD-005 depended on RD-007** (bank-aware indexes), which would block P1's
  "current-line highlighting" exit criterion. It now depends on RD-006 and ships first
  against the migrated `.dbg` map. — Corrected
- **Backlog ordering:** RD-023..030 were appended after RD-022 in the table; the table
  is now ordered by phase. — Corrected
- **`§` section markers** violate the repo writing convention (CLAUDE.md); all
  cross-references are now `#N`. — Corrected
- **"Follow PC"** (#4.2): the current view follows the disassembly *cursor*; 1.4 keeps a
  cursor-follow mode for parity and defines a true PC-follow mode. Views are polled per
  UI frame, there is no pause callback to subscribe to. — Corrected
- **Toolchain yaml list** had `tasm.yaml`; replaced by `kickass.yaml` and `64tass.yaml`.

## 3. Items kept as written (verified consistent with the code)

- Core rule "machine-level execution is ground truth", many-to-many mappings, no PC-only
  identity (#1, #3A): consistent; `CDebugMemoryCell.h:8` even states the limitation.
- #3B.2 C64 PLA table: matches the 32-mode table in `CViewC64MemoryBank`
  (`(exrom<<4)|(game<<3)|(port&7)`); 1.4 asks RD-028 to unify that table (it is
  duplicated in `CVicEditorLayerMemoryAccess.cpp`) into the provider.
- #3B.3 Atari: consistent with `atariMachinesDefs` and `MEMORY_HandlePORTB`. Added
  open decision 13: the debugger's default is 800XL with the 1088 KB RAM option, so
  fixtures must set machine/RAM explicitly.
- #8.2 "stack pointer is not a call stack": the existing `CStackAnnotationData`
  (per-byte JSR/BRK/IRQ/NMI tags with origin PC, pops not cleared) is exactly a
  seed for the physical tracker, not a call stack; RD-012 names it.
- #9.4 test layers, #9.5 trace format, #9.6 nightly matrix: kept; 1.4 adds the
  three fixture tiers (#9.1) because the repo already synthesizes fixtures in-process
  (`CTestViceInstructionStepping`, `CTestStackAnnotation`).
- #7.1 Oscar64 anchors: not verifiable here (no Oscar64 snapshot in the repo); kept as
  "verify against the pinned commit". Note for RD-002: Oscar64, cc65 and KickC all
  *document* C64, Atari and NES targets, so a full 3x3 compiler×machine matrix is
  plausible, but each cell is still proven only by a pinned invocation.
- #17 research spike and #18/#19 decisions: kept.

## 4. Open decisions added in 1.4

11. argv-based process launcher: engine proposal vs app-side code (before P5).
12. C64 Ultimate (`CDebugInterfaceC64U`) and the 1541 CPU: proposed out of scope
    through P4; the engine must only stay correct with `debugOnDrive1541` enabled.
13. Default Atari configuration for fixtures (explicit machine type + RAM size).

## 5. Pre-existing defects found during the review (not caused by the spec)

Tracked under RD-032 / RD-033 / RD-028 / RD-040 so they are fixed where the code is
touched anyway:

- `CDebugSymbols`: segments never deleted ("THE BUG" comments), `.dbg` re-import leaves
  dangling `codeSourceLineByMemoryAddress`, segment destructor double-deletes lines
  owned by `CDebugAsmSourceFile`, `SetSegment` bypasses `ActivateSegment`.
- `.vs` loader ignores `D:` (1541) labels; newer KickAss `<Watchpoints>` rows misparsed.
- CLI: `nes` matched twice (second branch unreachable), drag-in `-ines` has no
  handler, `-debuginfo` never targets NES; NES has no `source_code` layout slot.
- C64 PC/memory breakpoint checks ignore `breakOnPC`/`breakOnMemory`.
- `nesd_peek_safe_io` out-of-bounds read; `c64d_peek_c64` Ultimax uninitialized read;
  VICE cartridge adapter hard-coded 512 KB / ROML only; Atari "direct RAM" is the CPU view.
