# Source-level debugging — work tracker

Live status of the backlog in `source-level-debugging-ide-spec.md` #14. Update this
file in the same commit as the work it describes. Sequencing and milestone
definitions are in `IMPLEMENTATION-PLAN.md`.

**Status values:** `todo` · `in-progress` · `blocked(<on what>)` · `review` · `done(<commit>)` · `dropped(<why>)`
**Tracks:** core · ui · c64 · nes · atari · tests · ci · remote · compiler · research

**Legend for "Gate":** what must be true (test name, CI leg, doc) before the row can be `done`.

## Milestone overview

| Milestone | Phase | Scope | Status |
|---|---|---|---|
| M0 Discovery & safety net | P0 | RD-001, RD-038, RD-002, RD-024, RD-027, **RD-034**, **RD-042 (engine)** | RD-001 done; all other M0 items in review (remaining gates: CI on 3 platforms, maintainer review of RD-024) |
| M1 Canonical model & loaders | P2 (core) | RD-006, RD-031, RD-032, RD-007, RD-008, RD-033 | RD-006, RD-031 in review; RD-032 in progress |
| M2 Source document & view | P1 | RD-003, RD-004, RD-005 | todo |
| M3 Execution events & stepping engine | P2/P3 | RD-011, RD-037, RD-012, RD-013, RD-014, RD-015 | todo |
| M4 Machine mapping providers | P2/P3 | RD-028, RD-040, RD-029, RD-025, RD-026, RD-030, RD-034 | in-progress |
| M5 Compiler coverage & remote | P3/P4 | RD-009, RD-010, RD-036, RD-016, RD-017, RD-035, **RD-043 (64tass symbols)**, **RD-044 (1541)** | todo |
| M6 IDE workspace | P5 | RD-018, RD-042 (consumption), RD-019, RD-020, RD-021, **RD-045 (C64U observed mode)** | todo |
| M7 Advanced & research | P6 | RD-022, RD-023 | todo |

## Backlog

| ID | Phase | Track | Title | Status | Depends on | Gate |
|---|---|---|---|---|---|---|
| RD-001 | P0 | core | Inventory source view, hooks, breakpoints, loaders, memory translation APIs | done(53d397d+1.4) | — | Spec #20 written |
| RD-038 | P0 | tests | Regression test for KickAss `.dbg` + VICE `.vs` loaders (`dankwars.dbg`, `labelstest1.vs`) | review(d1784e3; 25/25 steps green on Linux locally) | RD-001 | `CTestSymbolLoaders` (`SymbolLoaders`) passes on all 3 CI platforms |
| RD-002 | P0 | tests | Portable C/ASM fixtures, pinned toolchains, verified compiler×machine matrix | review(first increment: 2 fixtures, 5 toolchains, 20 cells; matrix in plan #8) | — | `tests/source-debug/toolchains/*.yaml` with versions; matrix table in plan; Tier 1 artifacts committed |
| RD-024 | P0 | core | Logical vs physical code identity + `IMemoryMappingResolver` contract | review(`CDebugMemoryMapping.h`; awaiting maintainer review) | RD-001 | Header + unit test of `Unknown`/alias semantics; reviewed by maintainer |
| RD-027 | P0 | core | Audit + accessor patches in VICE/Nestopia/Atari800 cores (fetch/read/write, bus master) | review(accessors + fixes; `CoreMappingAccessors` 15 steps green on Linux) | RD-001, RD-024 | Accessors compiled on 3 platforms; `tests/test-reference-files.sh` still green |
| RD-006 | P2 | core | Canonical debug database + capability model; migrate `CDebugAsmSource` | review(`CDebugDatabase`; `CDebugAsmSource` removed; SymbolLoaders + DebugDatabase green on Linux) | RD-001, RD-038 | RD-038 tests unchanged and green; interval index unit tests |
| RD-031 | P2 | core | **Must fix.** Loader registry with content sniffing (`.dbg` XML vs text, `.dbj`) | review(`DebugInfoProbe`/`DebugInfoLoadFile`, all dispatch sites; DebugInfoFormats green on Linux) | RD-006 | Menu/autoload/CLI go through registry; sniff unit test |
| RD-032 | P2 | core | Symbol lifetime fixes (segments, re-import, dtor, `D:` labels, Watchpoints layout) | in-progress(done: line ownership, re-import, `SetSegment`, `D:` labels, Watchpoints; open: deleting replaced segments, blocked on purging `CDebugEventsHistory` pointers) | RD-006 | ASan/valgrind-clean re-import test |
| RD-007 | P2 | core | Bank-aware address indexes, source remapping, binary/cartridge identity checks | todo | RD-006, RD-024 | Lookup tests: multi-span, missing, ambiguous, same-PC two identities |
| RD-008 | P2 | compiler | Oscar64 `.dbj` adapter + contract tests | review(`DebugInfoLoadOscar64`; DebugInfoFormats steps 20-26 green on Linux) | RD-002, RD-006, RD-031 | Tier 1 Oscar64 fixture resolves file:line for known addresses |
| RD-033 | P3 | core | Generalize loading entry points (NES `-debuginfo`/autoload, CLI `nes`/`-ines`, NES source slot) | review(autoload, `-ines`, CLI target machine, NES layout; DefaultWorkspaceSpecs green) | RD-031 | NES loads `.dbg` from CLI and menu; workspace test updated |
| RD-003 | P1 | ui | Document registry + read-only `SourceDocumentView` replacing `CViewSourceCode` internals | todo | RD-001, RD-038 | ImGui test `source_view_opens_and_scrolls`; C64/Atari/NES instances |
| RD-004 | P1 | ui | C lexer + 6502 dialect profiles (KickAss, 64tass, ca65, Oscar64 asm) | todo | RD-003 | Lexer snapshot tests (golden tokens) |
| RD-005 | P1 | ui | PC/breakpoint/selection decorations + bidirectional navigation | todo | RD-003, RD-006 | ImGui test: click line → disassembly address; PC line follows pause |
| RD-011 | P2 | core | Generic execution events at existing hook points (+ RTS/RTI/reset/interrupt-entry) | todo | RD-001, RD-024 | Event-order test per machine; no perf regression vs `VicePlatformAbstraction` pacing gate |
| RD-037 | P2 | core | Step-over/step-out in `CDebugInterface`; proper temporary breakpoint with priorities | todo | RD-011 | `CTestStepOverRecursion`, `CTestStepOut`, user bp beats temp bp |
| RD-028 | P2 | c64 | C64 mapping provider (6510 port/PLA/cart/Ultimax), unify `CViewC64MemoryBank` table | in-progress(provider from VICE tables, no-cart tests green; open: cart fixtures/profiles, view unification) | RD-024, RD-027 | #3B.2 "Required C64 tests" pass |
| RD-040 | P2 | nes | NES mapping provider (mapper PRG identity, header→ROM offset, PRG-RAM), fix OOB peek | in-progress(provider from the board page tables, UxROM + NROM tests green; open: iNES file offset, MMC1/MMC3 fixtures in RD-026) | RD-024, RD-027 | NROM + UxROM identity tests pass |
| RD-029 | P2 | atari | Atari provider (model, PORTB, OS/BASIC, 130XE banks, ANTIC view, cartridge) | in-progress(provider from Atari800 attributes + banking state; OS/BASIC/130XE CPU-vs-ANTIC tests green; open: XEGS and other cart types, direct-RAM adapter) | RD-024, RD-027 | #3B.3 "Required Atari tests" pass or explicit skips recorded |
| RD-012 | P3 | core | Physical/logical execution context tracker (seeded from stack annotations) | todo | RD-011, RD-006 | Tracker tests: JSR/RTS, IRQ/RTI nesting, tail JMP, attach mid-run → unknown |
| RD-013 | P3 | core | Source-step state machine in the emulation-thread hook, stop reasons, budget | todo | RD-007, RD-012, RD-037 | Into/Over/Out on Tier 1 Oscar64 fixture; watchdog never trips |
| RD-025 | P3 | core | Bank-aware CPU/source/physical breakpoints + labels; mapping-driven segment activation | todo | RD-007, RD-011, RD-024 | Same-PC/different-identity breakpoint tests on 3 machines |
| RD-014 | P3 | core | Source breakpoints + re-resolution after rebuild | todo | RD-007, RD-013, RD-025 | Breakpoint survives reload; stale-binary warning test |
| RD-015 | P3 | tests | Headless step-trace runner + golden expectations (#9.5 format) | todo | RD-002, RD-013 | `tests/source-debug/expectations/**` replayed in suite |
| RD-026 | P3 | tests | NES NROM/UxROM/MMC1-or-MMC3 banked fixtures with physical-fetch traces | todo | RD-002, RD-024, RD-025, RD-040 | #3A.6 assertion list green |
| RD-030 | P3 | tests | C64 + Atari banked suites; shared 3-machine matrix | todo | RD-002, RD-025, RD-028, RD-029 | #3B.4 trace schema produced for every fixture |
| RD-034 | P0 | ci | **Must fix.** Run headless suite on Linux CI (both log legs) | review(ea99097; local Linux 74/74, 7 skipped) | engine `CPianoKeyboard` fix (merged to engine `devel`, 40c95f8) | `build-linux.yml` runs `tests/run_test.sh --skip-build` under xvfb; AGENTS.md updated |
| RD-036 | P3 | remote | Remote/MCP: `step/source/*`, `step/out`, `source/location`, `symbols/load`; skill doc | todo | RD-013, RD-031 | `CTestMCPProtocol` extended; `docs/mcp/retrodebugger-mcp-skill.md` updated |
| RD-009 | P2 | compiler | cc65 `.dbg` adapter + contract tests | review(`DebugInfoLoadCc65`; DebugInfoFormats steps 27-35 green on Linux) | RD-002, RD-006, RD-031 | Tier 1 cc65 fixture (C64, Atari, NES where target builds) |
| RD-010 | P2 | compiler | KickC artifact audit + supported subset | todo | RD-002, RD-006, RD-031 | Capability flags documented; `Approximate` where only line mapping |
| RD-016 | P4 | compiler | Oscar64 upstream: range coalescing/column export | todo | RD-008 | Upstream PR link; before/after `.dbj` fixture |
| RD-017 | P4 | compiler | Structured inline provenance + logical inline frames | todo | RD-016, RD-012 | Inline fixture Step Out of inlined frame |
| RD-035 | P4 | ci | Nightly toolchain provisioning (cc65, Oscar64 pinned, KickC+Java, KickAss jar licensing) | todo | RD-002 | Nightly workflow green with version report artifact |
| RD-018 | P5 | ui | Workspace explorer + tabs | todo | RD-003 | ImGui test opens folder, switches tabs |
| RD-042 | P0/P5 | core | **Must fix, decided: engine.** Extend the engine's existing `CProcessSpawner` with `Run` (cwd, captured output, PATH, timeout, cancel); consumed by RD-019 in P5 | review(engine `devel` 40c95f8; app suite registers `ProcessSpawner`, 16/16 green on Linux) | — | Merged into engine `devel`; `CTestProcessSpawner` green in the app suite on 3 platforms |
| RD-043 | P4 | compiler | Embedded 64tass emits symbols + address→line map on every in-process assembly (spec #20.9) | todo | RD-006, RD-031 | `retro_assemble` result carries labels; source view shows the assembled text with PC line; plugin `A(...)` code gets labels |
| RD-044 | P4 | c64 | 1541 drive CPU source debugging (spec #20.8) | todo | RD-013, RD-025, RD-032 | Source step on drive code with C64 running; `D:` labels land in `symbolsDrive1541`; "1541 Source code" view |
| RD-045 | P5 | c64 | C64 Ultimate observed-mode source support (spec #20.8) | todo | RD-011, RD-028, RD-003 | Source view + labels + mapping on C64U; trace replay shows last executed source lines; stepping UI greyed by capabilities |
| RD-019 | P5 | ui | Toolchain registry, build tasks, diagnostics console | todo | RD-018, RD-002, RD-042 | Build an Oscar64 sample from the UI; clickable diagnostic |
| RD-020 | P5 | core | Build/run/debug orchestration + artifact consistency | todo | RD-019, RD-007 | Failed build keeps old session marked stale |
| RD-021 | P5 | ui | Editing, save, undo/redo, search, file watchers | todo | RD-003, RD-018 | Dirty-buffer + save + watcher tests |
| RD-022 | P6 | core | Variables/types model, optimized stepping | todo | RD-017 | Feature-gated; optimized fixture |
| RD-023 | P6 | research | C symbols / Watch / Memory Viewer research spike (proposal only) | todo | RD-008, RD-009, RD-010, RD-020 | Capability matrix + follow-up spec reviewed |

## Open decisions (spec #15)

| # | Question | Owner | Status |
|---|---|---|---|
| 4 | Exact Oscar64 / cc65 / KickC baseline versions and flags | RD-002 | **decided 2026-10-17 (RD-002):** Oscar64 1.32.273 `-n -g -O0`; cc65 2.19 `-g`, unoptimized; KickC 0.8.6 + KickAssembler 5.24-65ce02.e `-debugdump`; 64tass 1.59.3120 (`tests/source-debug/toolchains/`) |
| 6 | Step into function without metadata: skip (default) vs enter disassembly | maintainer | proposed: skip tracked runtime calls, option to enter disassembly |
| 7 | Optimization levels with precise stepping | RD-002 | proposed: debug/unoptimized exact, optimized best-effort with warning |
| 9 | NES mapper families for headless tests | RD-026 | proposed: NROM, UxROM, MMC1 |
| 11 | argv process launcher: engine proposal vs app-side | maintainer | **decided 2026-10-17: engine proposal (RD-042)** |
| 12 | C64U and 1541 CPU scope | maintainer | **decided 2026-10-17: must-have, later phases (RD-044 P4, RD-045 P5); guards in spec #20.8** |
| 13 | Default Atari fixture configuration | RD-002 | proposed: explicit 800XL/64K and 130XE/128K per fixture |
| 14 | Vendored 64tass 1.46: callbacks first, or upgrade the assembler first? | maintainer | proposed: callbacks first (RD-043), upgrade as a separate item if needed |

Questions 1, 2, 3, 5, 8 and 10 are answered in spec #15/#20.

## Known defects to close on the way (not separate rows)

| Defect | Closed by |
|---|---|
| Segments never deleted; `.dbg` re-import dangling pointers; segment dtor double delete | RD-032 |
| `.vs` `D:` labels ignored; newer `<Watchpoints>` layout misparsed | RD-032 (fixed) |
| Menu "Import labels" with a `.dbg` skips `DeleteAllSymbols`: stale same-name segments keep freed source lines and `ActivateSelectedSegment` picks the stale one (use-after-free) | RD-032 (fixed: segments hold no line pointers, a `.dbg` load replaces the segment set) |
| `.dbg` `<Sources>` paths: split on every comma; a Windows path is not split into a file name on macOS/Linux (`GetFileNameComponentFromPath` uses the host separator); relative paths resolve against the process cwd, not the `.dbg` folder | RD-006 (file names from both separators; a source missing at its path is looked up next to the `.dbg`; comma split remains) |
| CLI `nes` unreachable branch; `-ines` missing; `-debuginfo` never NES; NES source layout slot | RD-033 (fixed) |
| C64 PC/memory checks ignore `breakOnPC`/`breakOnMemory`; temp bp hides user bp, no event | RD-037 |
| `nesd_peek_safe_io` OOB read | RD-027 (fixed) |
| Engine: `CPianoKeyboard` dereferenced a NULL `midiIn` without an ALSA sequencer (startup segfault on Linux CI/containers) | RD-034 (fixed on engine branch, 623b76e) |
| `GT2Patterns` step 16 assumed macOS key input (`virtualkeycode`); failed on every other platform | RD-034 (test fixed, 7fed440) |
| `c64d_peek_c64` Ultimax uninitialized read; per-cart peek bypass (also in `_for_cycle` and `_whole_map`) | RD-027 (fixed) |
| Cartridge data adapter ignored its type (always ROML), instance mislabelled RamL; view stays 512 KB (cell cost) | RD-027 (fixed type/label; size kept, identity via accessors) |
| NES: loading a cartridge after Detach Everything never reset the new board (PRG unmapped, CPU on open bus) | RD-027 (fixed in `nesd_insert_cartridge`) |
| `tests/test-reference-files.sh` fails on three unregistered USBSID files under `vice/arch/hwsiddrv/` (since 895f4fc) | fixed on `devel` (4a362e5), merged into this branch |
| Atari "direct RAM" adapter returns the CPU view | RD-029 |

## Change log

| Date | Change |
|---|---|
| 2026-10-17 | Tracker created from spec v1.4; RD-001 marked done; RD-031..RD-042 added |
| 2026-10-17 | Maintainer feedback: RD-034/031/042 must-fix (RD-034 and the RD-042 proposal moved to M0, decision 11 = engine); RD-043 (64tass symbols), RD-044 (1541), RD-045 (C64U) added; decision 12 = must-have later phases; decision 14 added |
| 2026-10-17 | RD-042: engine already had `CProcessSpawner`; `Run` added on engine branch `c-source-debugging` (ab37e96). RD-038 test written as `CTestSymbolLoaders`. Two loader defects added (menu `.dbg` re-import use-after-free; `<Sources>` path handling) |
| 2026-10-17 | Linux build + suite run locally under Xvfb: startup segfault fixed in the engine (`CPianoKeyboard`), `GT2Patterns` made platform-correct, suite 74/74 (7 skipped). RD-034 workflow step added; RD-038 green (25 steps) |
| 2026-10-17 | Engine `c-source-debugging` fast-forwarded into engine `devel` (40c95f8, changelog under 3.21.22); app suite registers `CTestProcessSpawner`; Linux suite 75/75 (7 skipped) |
| 2026-10-17 | RD-024: `CDebugMemoryMapping.{h,cpp}`, `CDebugInterface::GetMemoryMappingResolver()`, `CTestMemoryMappingContract` (14 steps green on Linux) |
| 2026-10-17 | RD-027: VICE/Atari/NES banking accessors, three core fixes plus the NES load-after-detach bug found by the new test; `CTestCoreMappingAccessors` (15 steps, NROM alias at $8000/$C000); Linux suite 77/77 (7 skipped) |
| 2026-10-17 | RD-002 first increment: toolchains pinned and built in the container (Oscar64 from tag, cc65/64tass from apt, KickC from tag + its KickAssembler jar), `basic-flow` + `same-line` fixtures, reproducible `build_fixtures.py`, 20 Tier 1 cells; decision 4 closed. Found: KickC `.dbg` is asm-level only |
| 2026-10-17 | RD-006 + RD-031: `CDebugDatabase` replaces `CDebugAsmSource` and the per-segment address arrays; content-sniffing `DebugInfoLoadFile` at all dispatch sites; RD-032 re-import fixed. Linux suite 79/79 (7 skipped) |
| 2026-10-17 | RD-032: `SetSegment` activates fully, `D:` labels reach the 1541, both `<Watchpoints>` layouts; segment deletion traced to `CDebugEventsHistory` raw pointers (still open). Linux suite 79/79 |
| 2026-10-17 | RD-009 (early): cc65 `.dbg` loader; relative source paths resolve against the debug file. Linux suite 79/79 |
| 2026-10-17 | RD-033: NES autoload of labels/debug info, `-ines`, CLI symbols to the loaded machine, NES source-code layout. Linux suite 79/79; ImGui suite 10/15 before and after (five pre-existing Linux failures, AGENTS.md) |
| 2026-10-17 | RD-028 part 1: `CViceMemoryMappingResolver` classifies accesses from VICE's own memory tables; `CTestC64MemoryMapping` (all `$01` patterns). Spec #3B.2 `$34` row corrected (RAM, not I/O). Linux suite 80/80 |
| 2026-10-17 | RD-040 + RD-029 first parts: `CNesMemoryMappingResolver` (board page tables) and `CAtariMemoryMappingResolver` (Atari800 attributes + banking state); `CTestMachineMemoryMapping` (16 steps: synthesized UxROM bank switch, mapper writes, RAM mirrors; Atari OS/BASIC via PORTB, 130XE CPU vs ANTIC bank). Linux suite 81/81 (7 skipped) |
