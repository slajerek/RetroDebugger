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
| M2 Source document & view | P1 | RD-003, RD-004, RD-005 | review (view, lexers, gutter, navigation; tests green on macOS) |
| M3 Execution events & stepping engine | P2/P3 | RD-011, RD-037, RD-012, RD-013, RD-014, RD-015 | in-progress |
| M4 Machine mapping providers | P2/P3 | RD-028, RD-040, RD-029, RD-025, RD-026, RD-030, RD-034 | in-progress |
| M5 Compiler coverage & remote | P3/P4 | RD-009, RD-010, RD-036, RD-016, RD-017, RD-035, **RD-043 (64tass symbols)**, **RD-044 (1541)** | todo |
| M6 IDE workspace | P5 | RD-018, RD-042 (consumption), RD-019, RD-020, RD-021, **RD-045 (C64U observed mode)** | review (all rows in review, macOS) |
| M7 Advanced & research | P6 | RD-022, RD-023 | todo |

## Handover (2026-10-18, second session)

Where the work stands, for whoever picks it up next. Branches: app
`c-source-debugging`, **five commits ahead of `origin` and not pushed** (92aae8a,
8db60f4, 2d8e64f, 9be7c71 and this handover: the maintainer pushes); engine `c-source-debugging` ==
engine `devel` at 5d61f20, unchanged this session.

- **Done, all with tests (macOS suite 83/83, 8 skipped, on 9be7c71):** RD-040/
  RD-029 mapping providers (f75509a), RD-011/RD-037 boundary events + stack-based
  step-over/out (e7f498e), RD-012/RD-013 frame tracker + source stepping
  (7e143d0), RD-014 source breakpoints (ee67677), RD-015 trace runner. Tests to
  run first: `InstructionStepping`, `SourceStepping`, `SourceStepTrace`,
  `MachineMemoryMapping`.
- **RD-015 state:** `CTestSourceStepTrace` replays the 21 traces of
  `tests/source-debug/expectations/index.json` (format in `IMPLEMENTATION-PLAN.md`
  RD-015): Oscar64 `basic-flow`, `same-line`, `nested-calls`, `branches-loops`,
  `interrupts` x c64/atari/nes, cc65 `basic-flow`, `same-line` x c64/atari/nes.
  - Launch: C64 (PRG poke, $C060 preamble; first waits for the KERNAL's READY
    loop, a C64 taken mid-reset kept zeroed vectors and cc65's crt0 jumped
    through $0326 to $0000), Atari (XEX segments poked, jump to RUNAD), NES
    (cartridge inserted by VSync task on the emulation thread, jump to `entry`).
  - `irq` on all three: C64 CIA timer A through $0314 installed before entry;
    Atari POKEY timer 1 and NES DMC installed at the start line from a RAM stub
    that waits for the first interrupt, so the next lands early in the trace.
    The counter counts the trace's IRQs only (Oscar64 c64/atari/nes
    `interrupts`: 1/5/1, cc65 c64 `basic-flow`: 520). Atari800 runs POKEY
    timers per 114-cycle scanline; the NES DMC period is 432 cycles.
  - Findings as trace `note`s: Oscar64 compiles `return f(x);` as a tail `JMP`
    even at -O0 (Step Out skips a level); the shared store after an if/else is
    mapped to the else line; one stop per line per loop iteration; `-tm=nes`
    writes no vectors for a plain `main.c` (NES traces enter `startup` at
    $FF80); cc65 maps a function's prologue to its header line and its
    epilogue to the closing brace, so Step Into stops on `add(...) {` before
    the body and there is a stop on `}` (no prologue skipping -- see open
    questions); cc65 has no tail calls.
  - Fixture sources in `tests/source-debug/fixtures/`, Tier 1 builds in
    `tests/data/source-debug/<toolchain>/<machine>/`. `MATRIX.md` is now
    written from the committed manifests as well (`build_fixtures.py
    --matrix-only` without toolchains): the three M3 fixtures are Oscar64-only,
    cc65/KickC `not built`.
- **CI:** on f31426e (remote head before this session's commits): macOS green
  both legs (39088966574), Windows green both legs (39088968493), Linux logs-on
  green, logs-off segfault in `SourceStepTrace` at the NES cartridge insert
  (39088965038) -- fixed by 9be7c71 (insert/unload on the emulation thread), not
  yet verified on CI. 0caf9b4 macOS both legs green (39087179010), confirming the
  `MemoryAccessTiming` fix. **First thing next session:** once the five commits
  are pushed, dispatch all three workflows on the head (workflow_dispatch only).
  Containers on the maintainer's Mac have no network, so neither a local Linux
  build nor the pinned toolchains (Oscar64 1.32.273, cc65 2.19) were available
  this session.
- **Open questions for the maintainer (all three decided on 2026-10-18 in the autonomous session: (1) yes, gdb-style prologue skipping; (2) AltirraOS enabled; (3) the stop reason shows in the events history and the source view header):** (1) prologue skipping on Step Into
  (cc65 stops on the function header line; gdb-style skipping would need a
  per-producer prologue end); (2) the built-in AltirraOS: `EMUOS_ALTIRRA` was
  never defined, so a run without user ROMs (every test, CI, a fresh install)
  has an Atari with no OS -- the runner banks the OS out and takes POKEY out of
  init itself; (3) where the step stop reason should surface: there is no status
  bar, and a notification per step would be noise.
- **Next items, in suggested order:**
  1. RD-015 leftovers: cc65 builds of `nested-calls`/`branches-loops`/`interrupts`
     and their traces (needs the cc65 2.19 pin; cc65 functions take arguments on
     the C stack, so its `irq` must not call one with parameters); KickC traces
     wait on RD-010 (its `.dbg` maps `main.asm` lines only); the `recursion`/
     `indirect-calls`/`optimized`/`inline` fixtures of spec #9.1.
  2. RD-013 leftovers: stop reason in the UI (open question 3); an event-history
     entry when a step stops.
  3. RD-014 leftovers: persist source breakpoints in the workspace; stale-binary
     warning.
  4. M2 source view (RD-003/004/005), with a gutter for source breakpoints and
     the PC line; then RD-007/RD-025 bank awareness, RD-032, RD-028 cartridge
     fixtures, RD-011 reset/mapping events and 1541, RD-012 seeding from the
     stack annotation, RD-010, RD-043/044/045.

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
| RD-032 | P2 | core | Symbol lifetime fixes (segments, re-import, dtor, `D:` labels, Watchpoints layout) | review(done: line ownership, re-import, `SetSegment`, `D:` labels, Watchpoints; replaced segments are retired: events forget them, a VSync task marks when the emulation thread (which reads `currentSegment` per instruction without a lock) has passed a frame, the UI frame loop or the next load deletes them; `CDebugInterface::DoFrame` now drains VSync tasks on every machine (only the NES did); `SourceStepping` reloads 3x while running; ASan run not done) | RD-006 | ASan/valgrind-clean re-import test |
| RD-007 | P2 | core | Bank-aware address indexes, source remapping, binary/cartridge identity checks | in-progress(image offsets: mappings and functions carry the program file offset when the producer records it (cc65 `ooffs`); `CDebugDatabase::ResolveImage`/`FindFunctionImage`/`IsBankedAddress`; `CDebugInterface::ResolveSourceAt`/`FindFunctionAt` consult the machine's resolver only where banks overlap (stepper, source view, remote, runner); NES resolver reports the iNES file offset of PRG-ROM; the VICE resolver reports the .crt file offset of ROML bytes (VICE's CRT loader records each CHIP packet's data offset, `c64d_cartridge_roml_file_offset`; generic and Magic Desk cartridges); the Atari resolver reports the file offset of standard 8/16 KB and XEGS/SWXEGS 32 KB cartridge bytes (Atari800 records the .car header size); `BankedSource` + banked traces; the source breakpoint binder resolves a line's range in its own bank (two banks whose lines start at the same byte kept only one); open: KickAss segment remap table, binary hash identity (stale check by file time is in RD-014)) | RD-006, RD-024 | Lookup tests: multi-span, missing, ambiguous, same-PC two identities |
| RD-008 | P2 | compiler | Oscar64 `.dbj` adapter + contract tests | review(`DebugInfoLoadOscar64`; DebugInfoFormats steps 20-26 green on Linux) | RD-002, RD-006, RD-031 | Tier 1 Oscar64 fixture resolves file:line for known addresses |
| RD-033 | P3 | core | Generalize loading entry points (NES `-debuginfo`/autoload, CLI `nes`/`-ines`, NES source slot) | review(autoload, `-ines`, CLI target machine, NES layout; DefaultWorkspaceSpecs green) | RD-031 | NES loads `.dbg` from CLI and menu; workspace test updated |
| RD-003 | P1 | ui | Document registry + read-only `SourceDocumentView` replacing `CViewSourceCode` internals | review(`CSourceDocument`/`CSourceDocumentRegistry` in `src/Views/SourceDocument/`; `CViewSourceCode` rewritten on ImGui: file combo, missing-file panel with candidates and source roots, gutter; `SourceDocument` CTest, ImGui `source_view_opens_and_scrolls`) | RD-001, RD-038 | ImGui test `source_view_opens_and_scrolls`; C64/Atari/NES instances |
| RD-004 | P1 | ui | C lexer + 6502 dialect profiles (KickAss, 64tass, ca65, Oscar64 asm) | review(`CSyntaxLexerC`, `CSyntaxLexer6502` KickAss/64tass/ca65; golden tokens in `tests/data/source-debug/lexer/`, `MT_UPDATE_GOLDEN=1` rewrites) | RD-003 | Lexer snapshot tests (golden tokens) |
| RD-005 | P1 | ui | PC/breakpoint/selection decorations + bidirectional navigation | review(PC arrow + line, ambiguous badge, bound/unresolved breakpoint marks, code dots; follow PC/cursor/off; double-click to disassembly with a popup for several ranges; disassembly "Show source line"; last step reason + stale warnings in the header) | RD-003, RD-006 | ImGui test: click line → disassembly address; PC line follows pause |
| RD-011 | P2 | core | Generic execution events at existing hook points (+ RTS/RTI/reset/interrupt-entry) | in-progress(boundary events on C64/Atari/NES CPUs, classifier + C64 event counts green, pacing gate green; reset events from each core's CPU reset (`cpu_reset`, `CPU_Reset`, `Cpu::Reset`) -> `CDebugInterface::NotifyCpuReset`: armed steps Cancelled, history and stack annotation start over, `OnCpuReset` for observers; decided: mapping changes are the resolvers' epoch (`GetMappingEpoch`), polled, no push event; open: 1541 (RD-044)) | RD-001, RD-024 | Event-order test per machine; no perf regression vs `VicePlatformAbstraction` pacing gate |
| RD-037 | P2 | core | Step-over/step-out in `CDebugInterface`; proper temporary breakpoint with priorities | review(stack-based step-over/out on 3 machines, user bp wins; `CTestInstructionStepping` 18 steps; open: event-history entry for step stops, 1541 still PC+3) | RD-011 | `CTestStepOverRecursion`, `CTestStepOut`, user bp beats temp bp |
| RD-028 | P2 | c64 | C64 mapping provider (6510 port/PLA/cart/Ultimax), unify `CViewC64MemoryBank` table | in-progress(provider from VICE tables, no-cart tests green; Magic Desk ROML resolved (bank * 8 KB in roml_banks, .crt file offset); open: other banked cart types (Ocean, EasyFlash), view unification) | RD-024, RD-027 | #3B.2 "Required C64 tests" pass |
| RD-040 | P2 | nes | NES mapping provider (mapper PRG identity, header→ROM offset, PRG-RAM), fix OOB peek | in-progress(provider from the board page tables, UxROM + NROM tests green; open: iNES file offset, MMC1/MMC3 fixtures in RD-026) | RD-024, RD-027 | NROM + UxROM identity tests pass |
| RD-029 | P2 | atari | Atari provider (model, PORTB, OS/BASIC, 130XE banks, ANTIC view, cartridge) | in-progress(provider from Atari800 attributes + banking state; OS/BASIC/130XE CPU-vs-ANTIC tests green; open: XEGS and other cart types, direct-RAM adapter) | RD-024, RD-027 | #3B.3 "Required Atari tests" pass or explicit skips recorded |
| RD-012 | P3 | core | Physical/logical execution context tracker (seeded from stack annotations) | in-progress(physical frames from boundary events, unknown base, tracker tests green; `DebugSeedFramesFromStack` rebuilds the frames on the stack from the annotation (JSR pairs, IRQ/NMI/BRK triples, incomplete at an unannotated byte), exposed as `<platform>/cpu/callstack` and MCP `retro_callstack` with caller function/line; the source stepper keeps its unseeded tracker; open: logical/inline frames (RD-017)) | RD-011, RD-006 | Tracker tests: JSR/RTS, IRQ/RTI nesting, tail JMP, attach mid-run → unknown |
| RD-013 | P3 | core | Source-step state machine in the emulation-thread hook, stop reasons, budget | review(`CSourceStepController` Into/Over/Out, interrupt skip, budget, FrameExited; Oscar64 basic-flow trace green with a 150-cycle IRQ calling mapped code; Code menu, remote `source/step`, MCP `retro_source_step`; history entry per step stop (`CDebugEventStep`, Over/Out and source steps); a source step that cannot start warns; decided: Step Into runs a callee's prologue to its first statement (gdb-style) when the definition line is known (Oscar64 records it, cc65's is found in the source text: `CDebugFunction::lineIsDefinition`); decided: no status line exists, the stop reason shows in the events history and in the M2 source view header; open: statement ids/columns) | RD-007, RD-012, RD-037 | Into/Over/Out on Tier 1 Oscar64 fixture; watchdog never trips |
| RD-025 | P3 | core | Bank-aware CPU/source/physical breakpoints + labels; mapping-driven segment activation | in-progress(PC breakpoints can require image offsets (`CDebugBreakpointAddr::imageOffsets`); source breakpoints bind with their bank where banks share the address; `BankedSource` on the NES UxROM and MMC1 builds and on a C64 Magic Desk cartridge, and an Atari XEGS cartridge (both banks' lines at $8003), each proven to fail without the condition (C64: with the .crt offsets off, the bank 1 breakpoint never stops); open: banked labels, mapping-driven segment activation) | RD-007, RD-011, RD-024 | Same-PC/different-identity breakpoint tests on 3 machines |
| RD-014 | P3 | core | Source breakpoints + re-resolution after rebuild | in-progress(file+line intent on `CDebugSymbols`, slide within the function, N address bps per line, re-bound on every database swap, user bps shared; tests green; saved as file + line in the breakpoints HJSON (`SourceBreakpoints`), its own address bps left out; stale debug info by file time (`DebugInfoCheckStale`: a source newer than the debug file, program and debug file more than 120 s apart), warned on autoload and menu load, kept on `CDebugSymbols::debugInfoWarnings`; open: bank identity via RD-025, gutter UI in M2, workspace persistence with RD-018) | RD-007, RD-013, RD-025 | Breakpoint survives reload; stale-binary warning test |
| RD-015 | P3 | tests | Headless step-trace runner + golden expectations (#9.5 format) | review (43 traces: basic-flow, same-line, nested-calls, branches-loops, interrupts, recursion, indirect-calls x {Oscar64, cc65} x c64/atari/nes + cc65 UxROM and MMC1 banked; `irq` on all three machines; the C64 IRQ counter moved off zero page ($02 was cc65's C stack pointer); the C64 timer IRQ starts at the start line like Atari/NES (cc65's BSS overlays its ONCE startup code, and tick()'s INC _ticks patched initlib before main); cells built on macOS with Homebrew cc65 2.19 and Oscar64 1.32.273 from its tag (`RD_OSCAR64`), code byte-identical to the container builds; open: KickC after RD-010, optimized/inline with RD-017/RD-022) | RD-002, RD-013 | `tests/source-debug/expectations/**` replayed in suite |
| RD-026 | P3 | tests | NES NROM/UxROM/MMC1-or-MMC3 banked fixtures with physical-fetch traces | review(cc65 `banked` fixture built for UxROM and MMC1 (own crt0 + linker config each, `nes-uxrom`/`nes-mmc1` builds in cc65.yaml; MMC1 in PRG mode 3, the bank loaded serially by five writes to $E000, variables in console RAM), traces `cc65/banked-nes-{uxrom,mmc1}.json`, `BankedSource` on both; NROM covered by every NES trace; MMC3 not covered) | RD-002, RD-024, RD-025, RD-040 | #3A.6 assertion list green |
| RD-030 | P3 | tests | C64 + Atari banked suites; shared 3-machine matrix | review(C64: cc65 `banked` Magic Desk build (`c64-magicdesk`: ld65 writes the .crt with its header and CHIP packets, so cc65's output offsets are .crt offsets; main() and the runtime copied to RAM), trace `cc65/banked-c64-magicdesk.json` (the runner attaches a `.crt` binary and detaches it before the next PRG trace), `BankedSource` steps 16-21; Atari: cc65 `banked` XEGS 32 KB build (`atari-xegs`, `car_checksum.py` writes the .car checksum after the link), trace `cc65/banked-atari-xegs.json` (the runner inserts a `.car` on the emulation thread and removes it before the next XEX trace, waiting for the OS's MEMTOP), `BankedSource` steps 23-28; the matrix has c64-magicdesk/atari-xegs/nes-uxrom/nes-mmc1 columns; open: 130XE extended-RAM banks) | RD-002, RD-025, RD-028, RD-029 | #3B.4 trace schema produced for every fixture |
| RD-034 | P0 | ci | **Must fix.** Run headless suite on Linux CI (both log legs) | review(ea99097; local Linux 74/74, 7 skipped) | engine `CPianoKeyboard` fix (merged to engine `devel`, 40c95f8) | `build-linux.yml` runs `tests/run_test.sh --skip-build` under xvfb; AGENTS.md updated |
| RD-036 | P3 | remote | Remote/MCP: `step/source/*`, `step/out`, `source/location`, `symbols/load`; skill doc | review(endpoints `source/step`, `source/step/status`, `step/out` (RD-013/037), `source/location`, `source/resolve`, `source/breakpoint/{add,remove,list}`, `symbols/load`; MCP `retro_source_step`, `retro_step_out`, `retro_source_location`, `retro_source_breakpoint`, `retro_symbols_load`; `MCPProtocol` test 38 routes, `SourceStepping` checks the API on Oscar64 data; skill doc workflow) | RD-013, RD-031 | `CTestMCPProtocol` extended; `docs/mcp/retrodebugger-mcp-skill.md` updated |
| RD-009 | P2 | compiler | cc65 `.dbg` adapter + contract tests | review(`DebugInfoLoadCc65`; DebugInfoFormats steps 27-35 green on Linux) | RD-002, RD-006, RD-031 | Tier 1 cc65 fixture (C64, Atari, NES where target builds) |
| RD-010 | P2 | compiler | KickC artifact audit + supported subset | in-progress(KickC -Sl `// file:line` comments in main.asm become C mappings (Approximate) and `name: {` scopes functions, per region; NES builds add `toolchains/kickc-nes-vectors.c`; KickC rejects recursion and static function-pointer tables (matrix `fail`); 15 KickC traces pass; segment activation prefers the segment with functions) | RD-002, RD-006, RD-031 | Capability flags documented; `Approximate` where only line mapping |
| RD-016 | P4 | compiler | Oscar64 upstream: range coalescing/column export | todo(no longer blocks RD-017: Oscar64 1.32.273 already writes inline provenance in "origins"; columns still absent) | RD-008 | Upstream PR link; before/after `.dbj` fixture |
| RD-017 | P4 | compiler | Structured inline provenance + logical inline frames | review(the Oscar64 loader reads each function's "origins" -- a range's lines innermost first, the calls it was inlined from after its own -- into inline chains on the mappings (`CDebugInlineSite`: caller file:line and the inlined function, named from the source text since Oscar64 lists no fully inlined function); the source stepper counts inline calls as calls: Over skips an inlined body, Into enters it, Out leaves it to the caller's next line, the result's depth is logical; the runner names the logical function; `inline` fixture built by a new Oscar64 `-O2` cell (`c64-O2`, `main-O2.c`) and traces `oscar64/inline.json` (Into the inlined work(), into a real call from it, Out twice) and `inline-over.json`; open: the source view and call stack do not show inline frames yet) | RD-016, RD-012 | Inline fixture Step Out of inlined frame |
| RD-035 | P4 | ci | Nightly toolchain provisioning (cc65, Oscar64 pinned, KickC+Java, KickAss jar licensing) | todo | RD-002 | Nightly workflow green with version report artifact |
| RD-018 | P5 | ui | Workspace explorer + tabs | review(`CWorkspace` (src/Tools): a folder, no project file needed; optional `retrodebugger-workspace.hjson` (machine, sourceRoots, buildTasks); lazy listing without build/VCS/hidden entries, file search; "Project" window (`CViewWorkspace`, Code menu: Project, Open Project Folder..., Build Project): explorer, one tab per file, highlighted view with line numbers, the PC line and the source breakpoint gutter of the project's machine; `CSourceRendering` shared with the Source code view; `Workspace` CTest, ImGui `project_opens_folder_and_switches_tabs`) | RD-003 | ImGui test opens folder, switches tabs |
| RD-042 | P0/P5 | core | **Must fix, decided: engine.** Extend the engine's existing `CProcessSpawner` with `Run` (cwd, captured output, PATH, timeout, cancel); consumed by RD-019 in P5 | review(engine `devel` 40c95f8; app suite registers `ProcessSpawner`, 16/16 green on Linux) | — | Merged into engine `devel`; `CTestProcessSpawner` green in the app suite on 3 platforms |
| RD-043 | P4 | compiler | Embedded 64tass emits symbols + address→line map on every in-process assembly (spec #20.9) | review(64tass.c reports each emitted byte's line at its logical address per pass and the root labels after the last (`c64debugger_assemble_*` callbacks); `CDebuggerApi` turns them into a database fragment (`DebugInfoFromAssembly`: the text is the file's own, `64tass-<start>.asm` or the stored file's name) merged on top of the loaded debug info (`DebugDatabaseMerge`, `CDebugSymbols::AddAssembledSource`: code assembled over a fragment's bytes replaces it, a load drops all) and code labels into the current segment; the source view shows a source without a file from that text; remote `cpu/assemble` / MCP `retro_assemble` take `assembler:"64tass"` and return labels and line ranges; the memory callback dispatches through `CDebuggerApi::AssembleResultToMemory` (it cast every API to the VICE one); `AssembledSource` 9 steps) | RD-006, RD-031 | `retro_assemble` result carries labels; source view shows the assembled text with PC line; plugin `A(...)` code gets labels |
| RD-044 | P4 | c64 | 1541 drive CPU source debugging (spec #20.8) | review(the drive CPU reports instruction boundaries from its pre-instruction hook only (the post-instruction check would report each PC twice) while a drive step is armed; `CDebugBoundaryHistory` is the per-CPU boundary stream, the main CPU uses it too; a second `CSourceStepController` without an owner (plain lookups: the drive's address space is flat) on `symbolsDrive1541`; `CDebugInterfaceVice::SourceStepDrive` cancels a main CPU step, pausing stays machine-wide; a C64 reset cancels it; drive steps go to the drive's events history (a newer step entry replaces an older one at the same C64 cycle: the drive runs in catch-up slices); "1541 Source code" view (follows the drive PC and stepper); remote/MCP source tools take `cpu:"drive1541"`; cc65 `drive1541` fixture (ca65 code for drive RAM); `DriveSourceStepping` 15 steps; `D:` labels were RD-032) | RD-013, RD-025, RD-032 | Source step on drive code with C64 running; `D:` labels land in `symbolsDrive1541`; "1541 Source code" view |
| RD-045 | P5 | c64 | C64 Ultimate observed-mode source support (spec #20.8) | review(execution observers take replayed boundaries (`CDebugInterface::ReplayInstructionBoundary`, `CDebugInstructionBoundary::replayed`: guard 3) -- the C64U replays each opcode fetch its trace decoder finds while observed; `CDebugSourceTraceRecorder` keeps the executed source lines from live or replayed boundaries (runs of one line, unmapped stretches); `CDebugInterface::GetExecutionCapabilities` (guard 2: instructionHook, stepInstruction, stepOut, sourceStep, fetchMapping, executionTrace), the C64U's has only executionTrace and fetchMapping; source step menu items grey out and `source/step` refuses without sourceStep; "C64U Source code" view; the source view lists executed lines while recording (context menu "Record executed lines"); remote `source/trace`, MCP `retro_source_trace` (56 tools); `SourceTrace` test: live C64 loop, replayed synthetic C64U bus cycles; not covered: a real C64U) | RD-011, RD-028, RD-003 | Source view + labels + mapping on C64U; trace replay shows last executed source lines; stepping UI greyed by capabilities |
| RD-019 | P5 | ui | Toolchain registry, build tasks, diagnostics console | review(`CBuildTaskRun`: steps are argv vectors run through the engine's `CProcessSpawner::Run` on a worker thread (no shell, time limit, cancel); `${workspace}`, `${env:NAME}`, `${tool:NAME}` (user tool paths in the settings folder, `CWorkspaceTools`, the bare name on PATH otherwise); output kept line by line, diagnostics parsed from Oscar64, cc65/ca65, ld65, gcc-style and KickAssembler output, clickable in the console (opens the file at the line); consent (spec #10.4): a project's commands run only after the user allowed them, remembered per folder and command set in the settings folder (`CWorkspaceTrust`, edited commands ask again); the embedded 64tass is RD-043's in-process path, not a build task) | RD-018, RD-002, RD-042 | Build an Oscar64 sample from the UI; clickable diagnostic |
| RD-020 | P5 | core | Build/run/debug orchestration + artifact consistency | review(`CWorkspaceSession`: a successful build loads its program (by extension: PRG, XEX, NES, CRT, CAR) and its debug info together on the task's machine, with the stale checks against this build's program; `afterBuild` none/load/run; a build that exits 0 without its outputs failed; a failed or cancelled build keeps the loaded session and marks it in the symbols' warnings ("Build failed ...: the loaded program and debug info are from an earlier build", shown by the Source code view header and remote symbols/load) until a build loads) | RD-019, RD-007 | Failed build keeps old session marked stale |
| RD-021 | P5 | ui | Editing, save, undo/redo, search, file watchers | review(`CSourceBuffer`: whole-text undo/redo per edit burst, save to a temporary file renamed over the target, dirty state; the tab's Edit mode edits the text (ImGui multiline, its own undo off), Save/Undo/Redo/Revert/Find (wrapping, case option); a build saves the project's dirty tabs first; closing a dirty tab asks; the watcher polls open files' modification times once a second: a clean tab reloads, a dirty one keeps its edits and is flagged until saved or reverted; no syntax colours while editing) | RD-003, RD-018 | Dirty-buffer + save + watcher tests |
| RD-022 | P6 | core | Variables/types model, optimized stepping | todo | RD-017 | Feature-gated; optimized fixture |
| RD-023 | P6 | research | C symbols / Watch / Memory Viewer research spike (proposal only) | review(`RESEARCH-c-symbols.md`: capability matrix from the committed Oscar64/cc65/KickC/KickAss/64tass artifacts, the reusable Watch/Memory Viewer/label APIs, a variable resolution model, UX sketch, follow-up rows RD-046..RD-056; for maintainer review) | RD-008, RD-009, RD-010, RD-020 | Capability matrix + follow-up spec reviewed |

## Open decisions (spec #15)

| # | Question | Owner | Status |
|---|---|---|---|
| 4 | Exact Oscar64 / cc65 / KickC baseline versions and flags | RD-002 | **decided 2026-10-17 (RD-002):** Oscar64 1.32.273 `-n -g -O0`; cc65 2.19 `-g`, unoptimized; KickC 0.8.6 + KickAssembler 5.24-65ce02.e `-debugdump`; 64tass 1.59.3120 (`tests/source-debug/toolchains/`) |
| 6 | Step into function without metadata: skip (default) vs enter disassembly | maintainer | proposed: skip tracked runtime calls, option to enter disassembly |
| 7 | Optimization levels with precise stepping | RD-002 | proposed: debug/unoptimized exact, optimized best-effort with warning |
| 9 | NES mapper families for headless tests | RD-026 | **done 2026-10-18:** NROM (every NES trace), UxROM and MMC1 (`banked`) |
| 11 | argv process launcher: engine proposal vs app-side | maintainer | **decided 2026-10-17: engine proposal (RD-042)** |
| 12 | C64U and 1541 CPU scope | maintainer | **decided 2026-10-17: must-have, later phases (RD-044 P4, RD-045 P5); guards in spec #20.8** |
| 13 | Default Atari fixture configuration | RD-002 | proposed: explicit 800XL/64K and 130XE/128K per fixture |
| 14 | Vendored 64tass 1.46: callbacks first, or upgrade the assembler first? | maintainer | proposed: callbacks first (RD-043), upgrade as a separate item if needed |

Questions 1, 2, 3, 5, 8 and 10 are answered in spec #15/#20.

## Known defects to close on the way (not separate rows)

| Defect | Closed by |
|---|---|
| Segments never deleted; `.dbg` re-import dangling pointers; segment dtor double delete | RD-032 (fixed: retired, deleted after the emulation thread's next frame) |
| `.vs` `D:` labels ignored; newer `<Watchpoints>` layout misparsed | RD-032 (fixed) |
| Menu "Import labels" with a `.dbg` skips `DeleteAllSymbols`: stale same-name segments keep freed source lines and `ActivateSelectedSegment` picks the stale one (use-after-free) | RD-032 (fixed: segments hold no line pointers, a `.dbg` load replaces the segment set) |
| `.dbg` `<Sources>` paths: split on every comma; a Windows path is not split into a file name on macOS/Linux (`GetFileNameComponentFromPath` uses the host separator); relative paths resolve against the process cwd, not the `.dbg` folder | RD-006 (file names from both separators; a source missing at its path is looked up next to the `.dbg`; comma split remains) |
| CLI `nes` unreachable branch; `-ines` missing; `-debuginfo` never NES; NES source layout slot | RD-033 (fixed) |
| C64 PC/memory checks ignore `breakOnPC`/`breakOnMemory`; temp bp hides user bp, no event | RD-037 (main CPU: user bp now wins and ends the step; `breakOnPC` and the event entry still open) |
| `nesd_peek_safe_io` OOB read | RD-027 (fixed) |
| Engine: `CPianoKeyboard` dereferenced a NULL `midiIn` without an ALSA sequencer (startup segfault on Linux CI/containers) | RD-034 (fixed on engine branch, 623b76e) |
| `GT2Patterns` step 16 assumed macOS key input (`virtualkeycode`); failed on every other platform | RD-034 (test fixed, 7fed440) |
| `c64d_peek_c64` Ultimax uninitialized read; per-cart peek bypass (also in `_for_cycle` and `_whole_map`) | RD-027 (fixed) |
| Cartridge data adapter ignored its type (always ROML), instance mislabelled RamL; view stays 512 KB (cell cost) | RD-027 (fixed type/label; size kept, identity via accessors) |
| NES: loading a cartridge after Detach Everything never reset the new board (PRG unmapped, CPU on open bus) | RD-027 (fixed in `nesd_insert_cartridge`) |
| `tests/test-reference-files.sh` fails on three unregistered USBSID files under `vice/arch/hwsiddrv/` (since 895f4fc) | fixed on `devel` (4a362e5), merged into this branch |
| Atari "direct RAM" adapter returns the CPU view | RD-029 |
| A 1541 PC set while the drive is parked in its pause check (after that instruction's breakpoint check) is applied at the opcode fetch, so the new PC's first instruction runs without a breakpoint check (`drivecpu.c` `C64D_FETCH_OPCODE_LOAD`); `DriveSourceStepping` breaks on a loop line instead | open (found 2026-10-18, RD-044) |
| A 1541 stop (breakpoint or source step) that lands while a timeline snapshot catches the drive up to the C64's clock does not park the drive: `c64d_debug_pause_check(0)` returns during snapshot operations (it would deadlock them), so the drive runs on to the C64's next pause point (`DriveSourceStepping` failed 1 run in 4 at drive PC $050C past its $0502 breakpoint; the test turns timeline recording off) | open (found 2026-10-18, RD-044) |
| `EMUOS_ALTIRRA` never defined: without user ROM files the Atari has no OS (zeroed vectors, POKEY left in init with SKCTL 0, no MEMTOP: cc65's C stack landed on zero page) | fixed: `EMUOS_ALTIRRA` in `atari-config.h`, AltirraOS/ATBASIC when no ROM file is configured (decided 2026-10-18, autonomous session) |
| NES cartridge insert/unload deleted the board while the memory map view's thread read it (`CViewDataMap::UpdateWholeMap` -> `GetMemory` -> `Board::Peek_Prg_C`): intermittent Linux CI segfaults in `SourceStepTrace`, `NesInputReplay`, `StackAnnotation` (core backtrace, run 39115276371) | fixed: a cartridge lifetime lock in `NesWrapper.cpp` (insert/unload exclusive; `GetByte`/`GetMemory`/`nesd_get_prg_mapping`/`nesd_get_cartridge_info` shared); tests swap cartridges through VSync tasks (`CTestNesCartridge.h`) |

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
| 2026-10-17 | RD-011 + RD-037: instruction-boundary events (`CDebugExecutionEvents`) from the three CPU hooks, classified from the previous opcode and SP; stack-based step-over/step-out (`CDebugInstructionStepper`), Step Out in the Code menu (Ctrl+F11), remote `step/out`, MCP `retro_step_out`; `CTestInstructionStepping` (18 steps: recursion, user bp priority, IRQ through the return address, all three machines). Linux suite 82/82 (7 skipped); ImGui 10/15 unchanged |
| 2026-10-17 | RD-012 + RD-013: `CDebugExecutionContextTracker` (physical frames from boundary events) and `CSourceStepController` (source Into/Over/Out in the emulation-thread hook, interrupt handlers skipped, instruction budget, FrameExited); `CTestSourceStepping` (16 steps: tracker cases, Oscar64 basic-flow scripted trace under a fast IRQ). Linux suite 83/83 (7 skipped); ImGui 10/15 unchanged |
| 2026-10-17 | RD-014 first part: `CDebugSourceBreakpoint` + `CDebugSymbols::SetDebugDatabase` (all loaders swap through it); `CTestSourceStepping` +6 steps. Linux suite 83/83 |
| 2026-10-17 | First CI runs of this branch (workflow_dispatch on 7e143d0): Linux running; macOS red on the engine's `CProcessSpawner.cpp` (`__GLIBC_PREREQ` in a shared `#if`, from this work, also on engine `devel`); Windows red on the engine's `CImGuiMarkdownEditorTests.cpp` (`<windows.h>` `Yield()` macro, pre-existing from f7ee69a). Fixes committed locally on engine `c-source-debugging` (818b530, 5d61f20); pushed with the maintainer's approval on 2026-10-18 to engine `c-source-debugging` and fast-forwarded to engine `devel` (5d61f20); the app pins `origin/devel`, so the macOS and Windows runs were re-dispatched |
| 2026-10-18 | Handover section added: state, CI, RD-015 design, open bits |
| 2026-10-18 | RD-015 first part: `CTestSourceStepTrace` + `expectations/index.json`, Oscar64 `basic-flow` and `same-line` traces; `CSourceStepResult::depth` (also in the remote status); `CTestSourceStepping` restores the CIA timer latch through I/O (it wrote RAM) |
| 2026-10-18 | RD-015: fixtures `nested-calls`, `branches-loops`, `interrupts` (Oscar64 c64/atari/nes) and their C64 traces; runner checks `pc` when given and that the IRQ ran (16-bit counter). `SourceStepTrace` 5/5 |
| 2026-10-18 | CI on ee67677 after the engine fixes: Linux green; macOS logs-on green; macOS logs-off 82/83 (`MemoryAccessTiming`: no marked writes to $0400) -- it runs right after `SourceStepping`, which on that commit left the 150-cycle CIA IRQ running (latch restored to RAM, fixed in the RD-015 commit); macOS re-dispatched on the current head to confirm; Windows pending |
| 2026-10-18 | RD-015: runner drives the Atari (XEX) and the NES (cartridge + entry); 15 traces (5 Oscar64 fixtures x 3 machines) pass. Linux suite 84/84. Handover rewritten for the next agent |
| 2026-10-18 | CI on f31426e: macOS and Windows green both legs; Linux logs-off segfault in `SourceStepTrace` at the NES insert. 0caf9b4 macOS both legs green |
| 2026-10-18 | RD-015: cc65 `basic-flow`/`same-line` traces on c64/atari/nes; the runner waits for the C64 KERNAL boot and restores the 6510 port (cc65 banks BASIC out; `ViceMemoryAccess`/`ViceEmbeddedRoms` failed after it) |
| 2026-10-18 | RD-015: `irq` on the Atari (POKEY timer 1, OS banked out, SKCTL out of init) and the NES (DMC, patched IRQ vector); counts only the trace's IRQs |
| 2026-10-18 | RD-002: `MATRIX.md` from the committed manifests too, `--matrix-only`; the three M3 fixtures listed |
| 2026-10-18 | `SourceStepTrace` inserts/unloads NES cartridges through VSync tasks (the Linux CI segfault). macOS suite 83/83 (8 skipped) |
| 2026-10-18 | RD-015: recursion and indirect-calls fixtures (Oscar64 + cc65, 3 machines), 12 traces written from the semantics first and passing as written; 43 traces |
| 2026-10-18 | RD-007/RD-025/RD-026 (NES): image offsets, bank-aware resolution and breakpoints, cc65 UxROM `banked` fixture, `BankedSource`, 31 traces. macOS suite 85/85 |
| 2026-10-18 | RD-015: cc65 builds of nested-calls/branches-loops/interrupts and their 9 traces (30 in all); Step Into skips prologues; AltirraOS enabled; C64 IRQ counter off zero page. macOS suite 84/84 |
| 2026-10-18 | RD-032: replaced segments deleted safely; VSync tasks drained on C64 and Atari too. macOS suite 84/84, ImGui 12/16 (pre-existing 4) |
| 2026-10-18 | RD-011: CPU reset events on all three machines; a reset cancels an armed step. macOS suite 84/84 |
| 2026-10-18 | RD-012: frames seeded from the stack annotation, call stack endpoint + `retro_callstack` (55 tools) |
| 2026-10-18 | RD-036: source location/resolve/breakpoint/symbols-load endpoints and MCP tools (54 tools), skill doc. macOS suite green |
| 2026-10-18 | Linux CI NES segfault root-caused from the core dump (edfc875 prints it): the memory map thread read a board an insert was deleting; cartridge lifetime lock + `CTestNesCartridge` helper for the five tests. macOS suite 84/84 |
| 2026-10-18 | M2: source document registry, C/6502 lexers, ImGui source view with gutter and navigation (`SourceDocument`, `source_view_opens_and_scrolls`). macOS suite 84/84; ImGui 12/16, the 4 failures pre-existing (`tests/SKIPPED_TESTS.md`) |
| 2026-10-18 | RD-013/RD-037: step stops in the events history; RD-014: source breakpoints persisted in the breakpoints file, stale debug info warning. `SourceStepping` 25 steps |
| 2026-10-18 | RD-015: `cc65/c64/interrupts` (failed alone 8/8, ~1 in 4 in the full run): the preamble's timer IRQ ran tick() during cc65's startup, and `INC _ticks` ($093F, BSS over ONCE) patched initlib. The C64 IRQ is installed at the start line. Alone 7/7, all 58 traces 5/5, macOS suite 85/85 |
| 2026-10-18 | RD-026: MMC1 build of the `banked` fixture (`nes-mmc1`), its trace and `BankedSource` steps 9-15; trace names carry the build cell (`cc65/nes-mmc1/banked`). 59 traces |
| 2026-10-18 | RD-007/RD-025/RD-030 C64: .crt file offsets of ROML bytes from VICE's CRT loader; cc65 Magic Desk build of `banked` and its trace (60 traces); `BankedSource` C64 steps; `build_fixtures.py` keeps failed cells of toolchains a filtered run did not build. macOS suite 85/85 |
| 2026-10-18 | RD-025/RD-030 Atari: XEGS .car image offsets, cc65 XEGS build of `banked`, its trace (61 traces), `BankedSource` Atari steps; binder fix for banks whose lines start at the same byte. Runner: a NES launch already stopped at the start line is not resumed (the pause after the insert lands at the end of its frame, which can reach the start breakpoint: macOS CI logs-off failed `kickc/nes/same-line` on 2edc788). macOS suite 85/85 |
| 2026-10-18 | RD-043: embedded 64tass line map and labels into the symbols, the assembled text as a source, remote `assembler:"64tass"`; `AssembledSource` test. macOS suite 86/86 |
| 2026-10-18 | RD-044: 1541 drive CPU source stepping, source breakpoints, 1541 Source code view, remote `cpu:"drive1541"`; `DriveSourceStepping` test; runner fix for a cartridge removed after the next trace's breakpoints (macOS CI, ff8bd0d) |
| 2026-10-18 | RD-045: replayed execution boundaries, executed source lines recorder (live and C64U bus trace), execution capabilities gating stepping, C64U Source code view, `source/trace` + `retro_source_trace`; `SourceTrace` test |
| 2026-10-18 | RD-018..021: the Project window (a folder as a workspace: explorer, tabs, editor with undo/redo/save/find, file watcher), build tasks through `CProcessSpawner::Run` with diagnostics and consent, build outputs loaded together or the session marked stale; `Workspace` CTest (9 steps), ImGui `project_opens_folder_and_switches_tabs`. macOS suite 89/89, ImGui 13/17 (the four known failures) |
| 2026-10-18 | RD-017: inline frames from Oscar64's origins, inline-aware Over/Into/Out and logical depth, `inline` fixture (Oscar64 -O2) and two traces (63 in all); RD-023 research note. macOS suite 89/89 |
