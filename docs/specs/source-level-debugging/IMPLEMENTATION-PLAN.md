# Source-level debugging — implementation plan

**Spec:** `source-level-debugging-ide-spec.md` v1.4 · **Status board:** `TRACKER.md`
**Branch:** `c-source-debugging` (from `devel` 778ef3a) · **Date:** 2026-10-17

This plan turns the spec's backlog into an ordered sequence of milestones with concrete
code touch-points, the tests that gate each one, and what can run in parallel. It is
written for the person (or agent session) picking up the next item: each milestone
says where to start, what to read first, and what "done" looks like in this repo.

## 0. Ground rules for every item

**Compatibility guards for later must-haves (1541 CPU, C64 Ultimate).** Spec #20.8
lists six guards; the short form, applied to every M0-M3 change:
- own everything per `CDebugSymbols` (per CPU), never per interface or global;
- publish execution-control capabilities (`CDebugBackendCapabilities`, generalized from
  `CC64BackendCapabilities`) and gate UI/remote on them;
- feed the execution observer from live hooks *or* replayed traces through one interface;
- route symbols by device (`D:` → `symbolsDrive1541`; project table for `.dbg` segments);
- source view and decorations take a `CDebugSymbols*`;
- test the stepping engine against a fake CPU as well as against VICE.

- **Three build systems.** Every new source file goes through
  `tools/add_source_file.py` (CMake + Xcode + vcxproj/filters). A file added to one
  list only fails on another platform.
- **CI is three platforms, two log legs each** (`AGENTS.md`). A change is done when
  `build-linux`, `build-macos` and `build-windows` are green on both legs. Until
  RD-034 lands, only macOS actually runs the app suite; run `tests/run_test.sh`
  locally on Linux as well.
- **No engine edits.** MTEngineSDL is pinned by `MTENGINE_REF`; anything the engine
  lacks (argv process launcher, multiline text widget) is either app-side code or an
  engine proposal, never a local patch.
- **Embedded emulator cores may be patched** (`src/Emulators/vice`, `atari800`,
  `nestopiaue`) for accessors and hooks, but never the `[C64D-REFERENCE-ONLY]` copies
  (`tests/test-reference-files.sh` enforces it).
- **Tests before refactors.** The existing KickAss `.dbg` path has no test; RD-038
  gives it one before `CDebugAsmSource` is touched.
- **Logging:** `LOGD`/`LOGM` for traces (compiled out unless `MT_DEBUG_LOGS`),
  `LOGError` for real faults. Stop reasons of the step engine are logged at `LOGD`.
- **Commits:** imperative, repo-scoped subjects (`feat(debug): ...`, `fix(vice): ...`,
  `test: ...`, `docs(specs): ...`), no attribution trailers. Update `TRACKER.md` in the
  same commit as the work.
- **Threading:** anything that mutates emulator state while running goes through
  `CDebugInterfaceTask` queues (see `AGENTS.md`, detach/attach saga). The step engine's
  arming state is written from the UI thread but only *read and acted on* in the
  emulation-thread hook.

## 1. Milestones and sequencing

```text
M0 Discovery & safety net ─┬─► M1 Canonical model & loaders ─┬─► M3 Events & stepping engine ─┐
                           │                                  │                               ├─► M5 Compilers & remote ─► M6 IDE ─► M7
                           └─► M4 Machine mapping providers ──┘        M2 Source view ─────────┘
```

- M0 is short and unblocks everything.
- M1, M2 and M4 can run in parallel by different people/sessions once M0's contracts
  (RD-024 header, RD-038 tests) exist. M2 only needs the *migrated* `.dbg` map (M1 first
  half), not bank awareness.
- M3 needs M1 (resolution) and the hook work of RD-011; the mapping providers of M4 are
  wired into M3 at RD-025.
- The three machine tracks inside M4 (RD-028 c64, RD-040 nes, RD-029 atari) are
  independent of each other and are the natural place to parallelize.

Rough sizing (one experienced developer, uninterrupted; use for ordering, not for
promises): M0 1-2 weeks · M1 3-4 weeks · M2 2-3 weeks · M3 4-6 weeks · M4 4-6 weeks
(across three tracks) · M5 3-5 weeks · M6 6-10 weeks · M7 research.

## 2. M0 — Discovery & safety net (P0)

**Goal:** contracts and regression tests in place; nothing user-visible changes.

### RD-001 Inventory — done
Results are spec #20. Re-read #20.2-#20.5 before touching any of the files named there.

### RD-038 Regression tests for the existing loaders
Implemented as one test, `src/Tests/CTestSymbolLoaders.{h,cpp}` (`SymbolLoaders`), with
four parts, each parsing into a scratch `CDebugSymbolsC64` swapped in for
`debugInterfaceC64->symbols` (`CDebugAsmSource` reads that pointer directly), so the
live symbols are untouched:
- VICE `.vs` (`assets/tests/labelstest1.vs`): one `Default` segment, 89 labels for 91
  lines, leading `.` stripped, duplicate address keeps the last label.
- 64tass `NAME = $addr` / decimal, spaces and tabs around `=`.
- KickAss `.dbg` (`assets/tests/c64/dankwars.dbg`) loaded the way PRG autoload does it
  (`DeleteAllSymbols` then `ParseSourceDebugInfo`): segment order, first segment
  activated, per-segment address → file:line:column, `<Block>` names, labels per
  segment, duplicate label text resolving to the last address, `<Breakpoints>` as PC
  breakpoints, a missing source file being silent.
- Source text: an in-memory `.dbg` pointing at `assets/tests/symbols/simple.asm`
  (absolute path via `ResolveProjectPath`): 1-based line index, each tab becomes four
  spaces, address → line → text end to end.
Re-import is deliberately NOT exercised: the menu import path (`CMainMenuBar`
"Import labels" with a `.dbg`) does not call `DeleteAllSymbols`, so the old segments
stay in `segments` with source-line pointers that `ParseSourceDebugInfo` just freed, and
`ActivateSelectedSegment` finds the OLD segment of the same name first. That is a
use-after-free, recorded as a defect for RD-032; its fix comes with its own test.

### RD-002 Fixtures and pinned toolchains
Done (first increment): `tests/source-debug/` with `fixtures/{basic-flow,same-line}`
(plus `kickass-c64.asm` and `64tass-c64.asm` for basic-flow), `toolchains/` (oscar64,
cc65, kickc, kickass, 64tass), `scripts/build_fixtures.py` (reproducible: fixed build
directory, pinned mtimes, `--check` byte-compares), `MATRIX.md`, and 20 Tier 1 cells
under `tests/data/source-debug/` with manifests beside the artifacts (not under
`expectations/`, which stays for step expectations, #9.5). Matrix in #8. Remaining
fixtures (nested-calls ... banked) are added with the milestones that use them. The
artifacts are not yet executed in the emulator: that is RD-015's trace runner.

Original task list:
- Create `tests/source-debug/` as in spec #9.1 with `fixtures/`, `toolchains/`,
  `expectations/`, `scripts/` and a gitignored `artifacts/`.
- `scripts/build_fixtures.py` (run with `python3 -I`): reads `toolchains/*.yaml`,
  builds every fixture × compiler × machine cell that the toolchain claims to support,
  records `tool`, `version`, `commit`, `argv`, `sha256` of inputs/outputs into
  `expectations/<compiler>/<fixture>.manifest.json`, and copies the resulting binary +
  debug artifact into `tests/data/source-debug/<compiler>/<machine>/<fixture>/`
  (Tier 1 golden artifacts, committed).
- Produce the verified compiler × machine matrix table (which cells build, which are
  `skip(reason)`) and put it in this file's #8. Do not fill it from documentation.
- Start with `basic-flow` and `same-line` only; the rest follow with M3/M5.
- Proposed pins to try first (confirm, then write them into the yaml): Oscar64 latest
  tagged release at the time of RD-002, cc65 distro package (Ubuntu `cc65`), KickC
  latest release zip with a pinned KickAssembler jar (licensing review before
  committing any jar).

### RD-024 Identity and mapping contract
- Done: `src/DebugInterface/CDebugMemoryMapping.{h,cpp}` (kept in an existing include
  directory so no project needs a new search path) with `DebugMemoryKind` (incl.
  `Unknown`, `Unmapped`, `CpuPort`, `ExtendedRam`), `DebugMemoryAccess {Fetch, Read,
  Write}`, `DebugMemoryBusMaster {Cpu, Vic, Antic, Ppu}`, `CDebugPhysicalAddress`
  (device, kind, region, offset; Unknown never compares equal), `CDebugMappedAddress`
  (CPU address, access, bus master, epoch, physical, optional image offset) and the
  abstract `CDebugMemoryMappingResolver` (`Resolve`, `ResolveFetch/Read/Write`,
  `ResolveCpuWindows` with a generic 64 KB scan). No `std::optional`: some Xcode
  targets still compile as gnu++0x.
- `CDebugInterface::GetMemoryMappingResolver()` returns NULL until RD-028/RD-029/RD-040
  provide one; callers treat NULL as Unknown.
- Epoch decision: per-resolver counter. Each provider calls
  `ObserveMappingState(signature)` with a cheap signature of the core state its mapping
  depends on whenever it reads that state; the epoch advances when the signature
  changes (the cores send no change notifications).
- `CTestMemoryMappingContract` (14 steps, fake banked resolver): fetch vs write,
  same PC two identities, aliases, Unmapped writes, Unknown, epoch movement.

### RD-027 Core audit + accessors
Done (review). What landed, per core:
- **VICE:** `c64d_get_banking_state(memConfig, cartType, exrom, game, romlBank, romhBank,
  exportRam)` in `c64memsc.c` (memConfig = VICE `mem_config`, the `mem_read_tab` index);
  all cartridge-window peeks in `c64d_peek_c64`, `_for_cycle`, `_whole_map` go through
  `cartridge_peek_mem` (fixes the uninitialized Ultimax read and the per-cart bypass);
  `c64d_peek/poke_cart_buffer(kind, offset)` for ROML/ROMH/RAM; the cartridge view honours
  its type. Not added: a separate `c64d_get_cart_banks` (folded into the state call) and
  per-cart bank registers beyond the generic ones (EasyFlash etc. keep their own; RD-028
  decides per cart type).
- **Atari800:** `atrd_get_banking_state(...)`, `atrd_get_memory_attrib(addr)`,
  `atrd_peek_backing(region, offset)` in `memory.c` (reads its static buffers).
- **Nestopia:** `Memory::GetSourceOffset`, `Board::GetPrgMapping`, `Cartridge::GetBoard`;
  wrapper `nesd_get_prg_mapping(addr, isWrk, source, offset, writable)` and
  `nesd_get_cartridge_info(mapper, subMapper, prgRomSize)`. Not added: iNES header/trainer
  file offset (the profile does not keep the header; RD-040 records it at load time).
  Also fixed: `nesd_peek_safe_io` OOB, and loading a cartridge after Detach Everything.
- Test: `CTestCoreMappingAccessors` (15 steps), including NROM-128 `$8000`/`$C000` aliasing
  to PRG offset 0 with a real ROM (`tests/data/test_input_replay.nes`).

Original task list:
- **VICE:** export `mem_get_current_bank_config()` through `ViceWrapper`; add
  `c64d_get_cart_banks(int *roml, int *romh, int *exportRam)` and a
  `c64d_get_cart_type()`; route cartridge-window peeks through
  `cartridge_peek_mem_slotmain` instead of `generic_peek_mem`; initialize the Ultimax
  open-bus result in `c64d_peek_c64`.
- **Nestopia:** add a `Board` accessor that returns, for a CPU address, the PRG source
  (ROM/RAM), bank index via `Memory::GetBank<SIZE>(addr)` and the byte offset; expose the
  `Api::Cartridge::Profile` header (`prgRom` size, `trainer`, mapper/submapper) to
  `CDebugInterfaceNes`; fix `nesd_peek_safe_io` so `$0000` and `$2000-$4FFF` never
  index `ram[]` out of bounds.
- **Atari800:** expose `MEMORY_xe_bank`, the decoded PORTB state (OS/BASIC/self-test
  enable, CPU/ANTIC bank selection), `CARTRIDGE_main.type/state`, and read access to
  `under_atarixl_os`/`under_cart*`/`atarixe_memory` for the "physical backing" view.
- Each accessor gets a smoke test that only asserts it compiles and returns sane values
  on the default machine; the behavioural tests come in M4.

### RD-034 Headless suite on Linux CI (must fix)
- Local baseline first: a Linux container builds with `./build-linux.sh` after the same
  `apt-get install` list the workflow uses, and runs the suite under `xvfb-run`; the
  per-test result of that run is the expected first CI result.
- `.github/workflows/build-linux.yml`: after the build step add
  `xvfb-run -a --server-args="-screen 0 1280x800x24" ./tests/run_test.sh --skip-build`
  with `SDL_AUDIODRIVER=dummy` (the runner sets it) on both log legs; upload
  `tests/results/last_run.txt` and the log dir as artifacts; fix the stale "See the
  Linux job" comment in `build-macos.yml`.
- Expect first-run failures that are Linux-only (the macOS-only baseline hides them);
  record them in `tests/SKIPPED_TESTS.md` with the marker convention or fix them,
  never mark green by skipping silently. Update `AGENTS.md` "Known state".

### RD-042 argv process launcher — engine (must fix, decided: engine)
- Correction to the inventory: the engine already has `CProcessSpawner`
  (`src/Engine/Tools/CProcessSpawner.{h,cpp}`: argv `Spawn`, `SpawnWithEnv`, `Kill`,
  `TryWait`, `BuildWindowsCommandLine`), used by NetGame. What build tasks lacked was a
  working directory, captured stdout/stderr, PATH lookup, timeout and cancellation.
- Done on the engine branch `c-source-debugging` (commit ab37e96): blocking
  `CProcessSpawner::Run(args, CProcessSpawnOptions{workingDirectory, envVars,
  searchPath}, CProcessRunResult&, timeoutMs, const atomic<bool> *cancel, onOutput)`,
  draining both pipes together, no shell. POSIX: `posix_spawn` with file actions and
  `addchdir_np` (glibc 2.29+, macOS 10.15+), own process group, SIGTERM then SIGKILL to
  the group. Windows: `CreateProcessW` (UTF-8 to UTF-16), explicit inherited-handle
  list, kill-on-close job object. Engine test `CTestProcessSpawner` (16 POSIX steps;
  a smaller Windows set) for a host suite.
- Merged into engine `devel` (40c95f8) with the `CPianoKeyboard` fix; the app suite
  registers `CTestProcessSpawner`. RD-019 wraps `Run` in a `CProcessRunner`; nothing
  in this repo calls the shell-string API for new code.

**M0 exit:** RD-038 tests green on all three platforms; RD-024 header reviewed by the
maintainer; RD-027 accessors merged; first Tier 1 fixtures committed with manifests;
Linux CI runs the suite on both legs; RD-042 `CProcessSpawner::Run` is in engine
`devel` and tested by the app suite.

## 3. M1 — Canonical model & loaders (P2 core)

**Goal:** one debug database per `CDebugSymbols` that every consumer (source view,
disassembly labels, breakpoints, remote `segment` API) reads, fed by pluggable loaders.

### RD-006 Canonical database (migration)
- New `src/DebugInterface/Symbols/CDebugDatabase.{h,cpp}` holding immutable snapshots:
  files (normalized path + original path + remap rules), compilation units, functions,
  source mappings with `AddressRange` (half-open, exclusive end) and
  `MappingQuality`, capability flags, build identity (hash of the loaded binary when
  known).
- Interval index: sorted vector of ranges + binary search (per identity candidate);
  reverse index file:line → ranges. No linear scans in the hook path.
- `CDebugAsmSource::ParseXML` becomes `CDebugInfoLoaderKickAss` producing a
  `CDebugDatabase`; `CDebugSymbolsSegment::codeSourceLineByMemoryAddress` is replaced by
  a lookup into the database for that segment's identity candidate. Keep
  `CDebugAsmSourceLine`-shaped results available to `CViewSourceCode` until M2 replaces
  the view.
- RD-038 tests must pass unchanged (that is the whole point of writing them first).

### RD-031 Loader registry
- `CDebugInfoLoaderRegistry` with `Probe(CByteBuffer*) → loader | none` based on
  content: `<C64debugger` root → KickAss; `version\tmajor=` → cc65; leading `{` with
  `"functions"`/`"memory"` keys → Oscar64 `.dbj`; `al C:` → VICE labels.
- Replace extension dispatch in `CMainMenuBar.cpp:5694-5718`,
  `CMainMenuHelper::LoadLabelsAndWatches`, `C64CommandLine.cpp` (both the startup and
  `-pass` paths).

### RD-032 Lifetime fixes
Fix in this order, each with a test: ownership of `CDebugAsmSourceLine` (single owner:
the database snapshot), segment deletion on database replacement (atomic swap under
`symbols->LockMutex()`), `SetSegment` → `ActivateSegment`, `.vs` `D:` labels routed to
`symbolsDrive1541`, `<Watchpoints>` newer layout.

### RD-007 Bank-aware indexes and identity checks
- Mapping ranges carry a `PhysicalIdentity` (from RD-024) or `Unknown`. KickAss segments
  map to identity candidates by name (`BANK1` style) through a per-project remap table;
  without a table they are `Ambiguous`, never `Exact`.
- Binary identity: hash the loaded PRG/XEX/ROM at load time (`CMainMenuHelper` load
  paths, `InsertCartridge`) and compare against the artifact's recorded hash when the
  loader supplies one (cc65/Oscar64 do not; the manifest from RD-002 does). Mismatch →
  warning in the view and `Approximate` quality.

### RD-008 Oscar64 `.dbj` adapter
- Parse `functions[].lines[]` (start/end/source/line), `memory[]`, `variables[]`,
  `types[]` where present; line-only mapping → `isStatementBoundary=false`, quality
  `Exact` for debug builds. Record every field actually emitted by the pinned build in
  `expectations/oscar64/schema-notes.md` (the spec's field list is a hypothesis).
- Contract test: for the `basic-flow` Tier 1 fixture, resolve three known addresses from
  the manifest to file:line and resolve `main.c:12` back to at least one range.

### RD-033 Entry points
NES: `-debuginfo` and autoload (`LoadLabelsAndWatches` is called from the C64 and Atari
load paths only), fix the duplicate `nes` CLI match, add the `-ines` handler the drag-in
path emits, add `nes.source_code` to `CDefaultWorkspaceLayouts` and its spec test.

**M1 exit:** `CViewSourceCode` and disassembly labels unchanged for the user; KickAss,
Oscar64 and VICE labels load through the registry; RD-038 + new contract tests green.

## 4. M2 — Source document & view (P1)

**Goal:** the Source Code window becomes a real read-only document view.

### RD-003 Document registry + view
- `src/Views/SourceDocument/CSourceDocument.{h,cpp}` (UTF-8 load, BOM, CRLF/LF, lines as
  `std::vector<std::string>` plus a UTF-16 cache only if the font blitter needs it),
  `CSourceDocumentRegistry` (URI → document, path remapping roots), and
  `CSourceDocumentView` rendering with ImGui draw lists (`ImDrawList::AddText` with the
  disassembly font atlas) so scrolling, clipping and hit-testing come from ImGui instead
  of `VID_SetClipping`. Keep `CViewSourceCode` as the thin `CGuiView` host so layouts,
  menu entries and the workspace slot do not change.
- Gutter: breakpoint marker, current-PC arrow, selected line; multiple indicators per
  line.
- Missing file: show the recorded path, a "locate" button and the remap roots; never
  show stale text.
- ImGui tests in `CImGuiTests.cpp`: open view, scroll, click gutter toggles a PC
  breakpoint in `currentSegment->breakpointsPC`.

### RD-004 Lexers
- `CSyntaxLexerC`, `CSyntaxLexer6502` with dialect tables (KickAss, 64tass, ca65,
  Oscar64 asm). Token categories only; colours come from a theme table.
- Line-state continuation for block comments/strings; cap per-frame lexing to visible
  lines + a small lookback.
- Golden token snapshot tests under `tests/data/source-debug/lexer/`.

### RD-005 Decorations and navigation
- PC line: poll like the disassembly does (`CViewC64::Render` → `SetCurrentPC`), resolve
  through the database, pick the best candidate, show an "ambiguous" badge if several.
- Follow modes: `cursor` (today's behaviour), `pc`, `off`.
- Double-click/Enter on a line → `viewDisassembly->ScrollToAddress` for a single range,
  popup for several; non-executable lines show a hollow gutter state.
- Reverse: disassembly context menu "Show source" when the PC/cursor resolves.

**M2 exit:** spec #13 P1 acceptance; ImGui tests green; NES instance gets a layout.

## 5. M3 — Execution events & stepping engine (P2/P3)

**Goal:** Source Into/Over/Out that cannot hang, cannot be killed by the VICE watchdog
and never fights user breakpoints.

### RD-011 Execution events
- `CDebugInterface` gets an `IExecutionObserver` list with `OnBeforeInstruction(pc,
  mapped)`, `OnAfterInstruction(lastPc, opcode, nextPc)`, `OnInterruptEntry(kind,
  vectorPc)`, `OnReturn(kind)`, `OnReset()`, `OnMappingChanged(epoch)`.
- Wire them at the existing hook points: VICE `c64cpusc.c` pre-execute block
  (`:3352-3376`) and post-execute (`:4417-4447`), `DO_INTERRUPT` for interrupt entry,
  the RTS/RTI opcode cases for returns, `cpu_reset` for reset; Atari `cpu.c:762-808`;
  Nestopia `NstCpu.cpp:2083-2136` and `DoISR`. Only the CPU-side (not the 1541) in this
  milestone.
- Zero-cost when no observer is armed (one atomic flag check); measure with the
  existing `VicePlatformAbstraction` pacing test.

### RD-037 Instruction-level step-over/step-out in the interface
- Move `StepOverJsr` logic into `CDebugInterface::StepOverSubroutine` using the
  observer: arm, count JSR/RTS depth from events (with interrupt nesting tracked
  separately), stop when depth returns to zero at an instruction boundary. Keep the
  `pc+3` temporary breakpoint only as a fast path when the mapping is stable and no
  recursion is observed.
- Add `StepOut()` (run until the current frame's RTS completes) and expose it in the
  Code menu, remote `step/out`, MCP `retro_step_out`.
- Replace `temporaryBreakpointPC` int with a `CDebugBreakpointAddr` flagged
  `isTemporary`; evaluate user breakpoints first; emit event history + broadcast.
- Tests: `CTestStepOverRecursion` (recursive JSR), `CTestStepOverIrq` (IRQ fires during
  the subroutine), `CTestStepOut`, `CTestTempBreakpointPriority`.

### RD-012 Execution context tracker
- Physical frames from observer events (JSR/RTS/IRQ/NMI/BRK/RTI) with confidence;
  seeded from `CStackAnnotationData` when attaching while paused; `Unknown` when the
  stack is not annotated.
- Logical frames from the database (function ranges, later inline contexts).

### RD-013 Source-step state machine
- `CSourceStepController` owned by `CDebugInterface`; UI arms it
  (`SourceStepInto/Over/Out`) by storing the start context (PC, identity, mapping,
  statement/line, logical frame, physical depth, interrupt nesting, policy, budget) and
  setting RUNNING once. The observer's `OnAfterInstruction` evaluates the policy and
  calls `SetDebugMode(PAUSED)` with a `StepStopReason`.
- Stop reasons from spec #8.3; reason surfaced in the status line and `LOGD`.
- Interrupt policy default: skip handler code (frames entered via interrupt are
  ignored until `RTI`).
- `CancelRestore()` on arm, inert while `IsPerformingSnapshotRestore()`.

### RD-014 Source breakpoints
- `CDebugBreakpointSource {file, line, column?, statementId?}` kept in the database
  snapshot's companion list; resolved to N `CDebugBreakpointAddr` with identity; re-resolved
  on every database swap; unresolved shown hollow in the gutter.

### RD-015 Trace runner
- `CTestSourceStepTrace` reads `tests/source-debug/expectations/<compiler>/<fixture>.json`
  (spec #9.5), loads the Tier 1 binary + debug artifact, runs the steps, compares
  `(file, line, function, depth, reason)`; failures dump the machine step trace to the
  log dir.

**M3 exit:** spec #13 P3 (non-banking) acceptance on Oscar64 `basic-flow`,
`same-line`, `nested-calls`, `branches-loops`, `interrupts` fixtures.

## 6. M4 — Machine mapping providers (P2/P3, three parallel tracks)

Each track: provider class in the emulator's interface folder, wired as
`debugInterface->memoryMapping`, used by `CDataAdapter*` for a new `physical backing`
read mode and by the database resolver; a test class with the spec's "Required tests"
list; explicit `skip(reason)` for unsupported emulator configurations.

- **RD-028 C64** (`src/Emulators/vice/ViceInterface/CMemoryMappingVice.{h,cpp}`):
  decode from `pport` + EXROM/GAME + cart accessors; fetch/read → ROM/IO/RAM per the
  32-mode table (move it out of `CViewC64MemoryBank` and
  `CVicEditorLayerMemoryAccess.cpp` into the provider and make both views consumers);
  write → RAM under ROM, I/O when visible, cartridge rules by type; Ultimax open bus →
  `OpenBus`. Identity for cart windows = `{cartImage, ROML/ROMH, bank, offset}`.
- **RD-040 NES** (`.../NestopiaInterface/CMemoryMappingNes.{h,cpp}`): fetch/read/write via
  the new board accessor; identity `{romImage, PRG_ROM|PRG_RAM, bankIndex, offset}` with
  ROM file offset = 16 + trainer + bankIndex*bankSize + offset for iNES/NES 2.0 (from
  the profile, not computed by hand elsewhere); `$0000-$1FFF` mirrors to internal RAM.
- **RD-029 Atari** (`.../AtariInterface/CMemoryMappingAtari.{h,cpp}`): model from
  `Atari800_machine_type` + `MEMORY_ram_size`; OS/BASIC/self-test from decoded PORTB;
  XE bank window from `MEMORY_xe_bank` (CPU vs ANTIC flag); cartridge from
  `CARTRIDGE_main` (XEGS, standard 8K/16K first; others `Unknown` with the type
  reported); writes to ROM → `Dropped`.
- **RD-025** then switches breakpoint evaluation and label lookup to identity-aware
  keys (`{identity, cpuAddress}` with a CPU-only intent flag) and makes
  `currentSegment` follow the resolved identity when the loaded database says which
  segment is which identity.
- **RD-026 / RD-030** fixtures: Tier 0 (synthesized in test code: a two-bank UxROM ROM,
  a two-bank Magic Desk-style CRT built like the CrtMaker does, an XEGS cart image, a
  130XE bank-switch program) plus the shared trace schema of spec #3B.4.
- **RD-034** runs the suite on Linux CI so these tests are measured on two platforms
  (`build-linux.yml`: `xvfb-run -a ./tests/run_test.sh --skip-build`; update `AGENTS.md`
  "Known state" once green).

**M4 exit:** spec #13 P2/P3 banking gates on all three machines (or recorded skips).

## 7. M5 — Compiler coverage & remote (P3/P4)

- **RD-009 cc65:** parse `.dbg` records (`file`, `line` with `span` refs, `span` with
  `seg`, `seg` with `start`/`addrsize`, `scope`, `sym`, `csym`); relocate spans by segment
  start; C lines vs asm lines by `type`; functions from `scope` entries of type `scope`
  with a `sym`; test on C64, Atari and NES targets where the pinned toolchain builds.
- **RD-010 KickC:** audit the generated `.asm` + KickAssembler `-debugdump` `.dbg` path
  (KickC can emit the same XML the existing loader reads, with C source referenced in
  `<Sources>` only if KickC propagates it); document the capability flags; `Approximate`
  where only asm lines map.
- **RD-036 Remote/MCP:** `step/source/into|over|out`, `step/out`, `source/location`
  (PC → candidates), `source/resolve` (file:line → ranges), `symbols/load` (path,
  through the registry); MCP tools `retro_step_source_*`, `retro_step_out`,
  `retro_source_location`, `retro_symbols_load`; extend `CTestMCPProtocol`; update
  `docs/mcp/retrodebugger-mcp-skill.md`.
- **RD-043 Embedded 64tass emits symbols (spec #20.9):** add
  `c64debugger_assemble_label(userData, name, addr, size)` and
  `c64debugger_assemble_source_range(userData, start, end, line, col)` callbacks in
  `src/Tools/64tass/64tass.c` next to `c64debugger_set_assemble_result_to_memory`, fed
  from the label table (`labelprint()` path) and from the listing pass run in-memory
  (`listing=1` with a null `flist` and the callback instead of `fputs`); `assemble_64tass`
  returns them to `CDebuggerApi`, which builds a `CDebugDatabase` fragment (identity =
  the address space written to) and merges it into `symbols` under the mutex. Register
  the assembled text as a synthetic document (`asm://<n>`) so the source view can show
  it. Consumers: MCP `retro_assemble` (result gains `labels`), plugins' `A(...)` macro,
  later build tasks. Decision 14 (callbacks on 1.46 first, assembler upgrade separately)
  is the default. Tests: assemble a snippet with two labels and a loop, assert labels
  and the line map resolve, assert the source view shows the PC on the right line after
  one step.
- **RD-044 1541 drive CPU source debugging (spec #20.8):** instantiate database,
  resolver (flat RAM/VIA/ROM provider), observer, tracker and step controller on
  `symbolsDrive1541`; wire observer events at `drivecpu.c:2592-2598` and `:3697-3711`;
  seed the tracker from `driveStack`; `D:` labels from `.vs` and a project table for
  `.dbg` segments; add a "1541 Source code" view instance and layout slot. Test: source
  step through a drive routine while the C64 CPU keeps running; a C64 breakpoint during
  a drive source step wins and reports `UserBreakpoint`.
- **RD-016 / RD-017 Oscar64 upstream:** regression fixtures first, then the column /
  provenance patches as separate upstream PRs; the adapter consumes `column` when
  present and keeps working without it.
- **RD-035 nightly:** `.github/workflows/source-debug-nightly.yml` installs the pinned
  toolchains (Linux only), runs `build_fixtures.py`, diffs against Tier 1 manifests, runs
  the trace suite; uploads manifests and traces as artifacts.

## 8. Compiler × machine matrix (RD-002, verified by building)

Built from the pinned toolchains by `tests/source-debug/scripts/build_fixtures.py`
(`basic-flow` and `same-line`); the generated, authoritative table is
`tests/source-debug/MATRIX.md`.

| | C64 | Atari 8-bit | NES |
|---|---|---|---|
| Oscar64 1.32.273 (`-n -g -O0`) | ok(`-tm=c64`, `.prg` + `.dbj`) | ok(`-tm=atari`, `.xex` + `.dbj`) | ok(`-tm=nes`, `.nes` + `.dbj`) |
| cc65 2.19 (`-g`, unoptimized) | ok(`-t c64`, text `.dbg`) | ok(`-t atari`, text `.dbg`) | ok(`-t nes`, text `.dbg`) |
| KickC 0.8.6 + KickAssembler 5.24 | ok(`-t=c64`, XML `.dbg` of the generated asm) | ok(`-t=atarixl`, idem) | ok(`-t=nes`, idem) |
| KickAssembler 5.24 (asm) | ok(`basic-flow`) | n/a (no fixture yet) | n/a (no fixture yet) |
| 64tass 1.59 (asm) | ok(`basic-flow`, labels + `--line-numbers` listing) | n/a (no fixture yet) | n/a (no fixture yet) |

What the builds showed (inputs for M1/M5):
- **KickC's `.dbg` maps addresses to lines of its generated `main.asm`, not of `main.c`**;
  the C source survives only as comments in that asm. C-level stepping for KickC needs
  those comments parsed into a line map (RD-010), or is `Approximate`.
- **Oscar64** writes `.dbj` (JSON) with absolute source paths, including its own
  `include/` runtime; it also has `-tm=nes_mmc1` / `nes_mmc3` / `nes_nrom_h|v`, which
  makes it a candidate producer of the banked NES fixtures (RD-026).
- **cc65** `.dbg` is a text format with relative file names and file mtimes, sharing the
  `.dbg` extension with KickAssembler's XML: the RD-031 sniffing case, now with fixtures.
- **64tass** on the command line is 1.59; the embedded copy is 1.46 (RD-043). Its address
  to source-line map is the `--line-numbers` listing.
- KickAssembler's own site is unreachable from the build container; the pinned jar is
  the one KickC's source repo carries. KickC's binary release is only linked through a
  URL shortener, so KickC is built from its tag (Maven).

## 9. M6 — IDE workspace (P5) and M7 — advanced (P6)

Not detailed here beyond the backlog rows; two prerequisites are already identified:
the RD-042 engine launcher must have landed (proposal filed in M0) before RD-019, and a
file-watcher strategy (none exists; platform APIs or polling with mtimes as the CrtMaker
plugin does) before RD-021.

**RD-045 C64 Ultimate observed mode (M6):** publish `CDebugBackendCapabilities` for
C64U from the existing `CC64BackendCapabilities` (stepping/source breakpoints `NONE`,
mapping `DIRECT` from polled `$01`/EXROM/GAME, `executionTrace` `OBSERVED`); add a
trace-replay event source that turns `C64UTraceBuffer` records into observer events
with a no-op stop channel; the source view shows "last executed" decorations from the
replayed trace; stepping controls are greyed with the spec #11 warning. Test with the
existing `CTestC64U*` fixtures (`CTestC64UMemoryCache`, `CTestC64UModeSwitch`).

M7 starts with the RD-023 research spike and ends in a separate spec.

## 10. First three pull requests (suggested)

1. `test: cover KickAss .dbg and VICE label loaders` (RD-038) + `docs(specs)` tracker
   update. Small, no behaviour change, unblocks everything.
   In the same week, separately: `ci(linux): run the headless suite under xvfb` (RD-034)
   and the RD-042 proposal in the MTEngineSDL repo.
2. `feat(debug): memory mapping contract and emulator-core accessors` (RD-024 + RD-027)
   with the three core bug fixes (NES OOB peek, Ultimax uninitialized read, cart adapter
   size).
3. `refactor(symbols): canonical debug database behind the KickAss loader` (RD-006 +
   RD-031 + RD-032), proven by the PR-1 tests staying green.

After these, M2 (view), M3 (stepping) and the M4 machine tracks can proceed in parallel.
