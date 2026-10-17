# RetroDebugger — Source-Level Debugging & IDE Evolution

**Technical specification / implementation plan**  
**Status:** Proposed, reconciled with the codebase (see #20) · **Version:** 1.4 · **Date:** 2026-10-17  
**Parallel implementation targets from P0:** C64 (6510/PLA), NES (cartridge mappers), Atari 8-bit (400/800/XL/XE/XEGS) · **Shared core:** generic bank-aware 6502-family debugging · **Compiler targets:** Oscar64, cc65, KickC · **Assembler targets:** existing KickAssembler `.dbg` integration (must not regress), embedded 64tass, ca65 and other external tools

## 1. Executive summary

Extend RetroDebugger from a machine-code-centric debugger with a simple **Source Code** viewer into a source-aware debugger and, incrementally, a lightweight IDE. The first deliverable is **correct read-only source display and C-level stepping**, not a full text editor. The architecture must nevertheless accommodate editable documents, a workspace folder tree, tabs, build tasks, compiler diagnostics, and eventually build/run/debug workflows.

**Core architectural rule:** machine-level execution remains the ground truth. Compiler metadata describes *where* machine instructions originate and, when available, their logical C execution context. Source-level actions are asynchronous stepping policies layered over the existing emulator/debugger execution loop, not new CPU execution implementations.

### 1.1 Goals

1. Reuse/replace the current `C64 Source Code`-style viewer with a reusable document component that can later become an editor.
2. Provide syntax highlighting for C and 6502 assembly, selected per file/language (not merely by compiler).
3. Introduce compiler-neutral debug information, loaders, address resolution, call/interrupt tracking, and reliable source Step Into / Over / Out.
4. Build reproducible sample projects and an automated compilation + debugger test pipeline for Oscar64, cc65, and KickC.
5. Make CPU-to-physical memory mapping and bank identity first-class from the first debug-information milestone, including C64 PLA/6510 banking, NES cartridge mappers, and Atari 8-bit memory/cartridge banking.
6. Define an incremental path toward an IDE: open folder, explorer, tabs, editing, external build tools, diagnostics, and build/run/debug.

### 1.2 Non-goals for initial release

Full editing and save workflows, language server integration, autocomplete, refactoring, variable evaluation across all compilers, reverse debugging, and perfect stepping through optimized/inlined code. These are future capabilities; their data-model requirements are addressed now.

### 1.3 Principles

- Preserve current disassembly stepping and the existing symbol workflows: KickAssembler `.dbg` (`CDebugAsmSource`), VICE `.vs` / 64tass labels (`CDebugSymbols::ParseSymbols`), HJSON labels/watches/breakpoints and the `-symbols`/`-debuginfo`/`-segment` command line.
- Do not infer a C call from every `JSR`; compiler-generated runtime helpers and inline code must be distinguished where possible.
- Treat debug mappings as **many-to-many, non-contiguous and sometimes ambiguous**; never assume one C line equals one contiguous address range.
- Handle absent/invalid metadata explicitly; never silently guess a source line.
- Do not assume the hardware stack pointer is a C call stack.
- Use emulator hooks to observe actual control flow, including interrupts, bank changes, and abnormal transfers.
- Never use CPU PC alone as a unique identity for code, breakpoints, source maps, or symbols; distinguish logical CPU address from mapped physical storage and ROM-file offset.
- Debugger UI must remain responsive during stepping; support cancellation, instruction budgets and event processing.

---

## 2. Scope and staged delivery

| Phase | Deliverable | Exit criteria |
|---|---|---|
| **P0 — Discovery & fixtures** | Inventory current Source Code view, CPU stepping API, symbol loaders (KickAss `.dbg`, VICE `.vs`, 64tass), 64tass/KickAssembler integrations and the C64/NES/Atari memory-translation hooks; compile sample fixtures | Inventory recorded in #20 (done 2026-10-17); golden binaries and metadata retained; the existing `.dbg` path is covered by a regression test before any refactor (RD-038) |
| **P1 — Source document foundation** | Read-only source document, C/ASM highlighting, line numbers, tabs-ready document model, source↔disassembly navigation | Correct display and current-line highlighting; no editor dependency leaks |
| **P2 — Debug information core** | Canonical bank-aware source map, function/statement metadata, mapper-backed physical address resolution, loaders, diagnostics and address lookup | Correct lookup on known fixtures, multiple spans, missing mappings, and same CPU address mapped to distinct physical memories on C64, NES and Atari |
| **P3 — Source stepping MVP** | Source Into/Over/Out; call/interrupt-aware state machine; breakpoints; safe fallback | Integration tests pass for unoptimized C and assembly, including bank/overlay switching on C64, NES and Atari |
| **P4 — Compiler coverage** | Oscar64, cc65 and KickC adapters with capability flags and fixture tests | Explicitly documented feature support for each compiler |
| **P5 — IDE workspace** | Folder explorer, tabs, file watchers, editor mode, build tasks and diagnostics | Open/build/debug a sample folder without leaving RetroDebugger |
| **P6 — Advanced debugging** | Inline frames, local variable locations, optimized stepping and richer call stack | Feature-gated, validated on optimized and banked programs |

C64, NES and Atari integration tracks MUST proceed in parallel against the same generic contracts and shared tests; do not treat NES/Atari as post-C64 ports. Phases P1–P4 can proceed in parallel where interfaces are stable. P5 is designed now but is **not a prerequisite** for P2/P3.

---

## 3. Proposed architecture

```text
Workspace / Project (future)
  ├─ File Explorer ─ Document Registry ─ Source View / Editor
  │                                      ├─ C lexer / syntax theme
  │                                      ├─ 6502 ASM lexer / syntax theme
  │                                      └─ execution / breakpoint decorations
  ├─ Build Manager ─ Toolchain Adapters ─ Diagnostics ─ Artifacts
  │                    ├─ Oscar64
  │                    ├─ cc65
  │                    ├─ KickC + KickAssembler
  │                    └─ KickAssembler (Java) / 64tass (embedded) / custom commands
  └─ Debug Session
       ├─ Binary/metadata loader
       ├─ Compiler Debug Adapters
       │    └─ Canonical Debug Database
       ├─ Machine Memory Translation Providers (C64 PLA / NES mapper / Atari PIA + cart)
       ├─ Bank-aware Address Resolver + Source Navigator
       ├─ Execution/Call/Interrupt Tracker
       ├─ Source Step Controller (state machine)
       └─ Existing Emulator / CPU / Disassembly / Breakpoints
```

**Module boundaries:** UI never parses compiler-specific metadata; compiler adapters never control the CPU; source stepping never reads GUI state to decide execution semantics; build tasks never assume a particular emulator.

Suggested C++ interfaces (names illustrative; adapt to RetroDebugger's existing architecture):

```cpp
enum class SourceLanguage { Unknown, C, Assembly6502 };
enum class StepKind { InstructionInto, InstructionOver, SourceInto, SourceOver, SourceOut };
enum class MappingQuality { Exact, Approximate, Missing, Ambiguous };

struct SourcePosition {
    std::string fileId;
    uint32_t line = 0;      // Canonical API: 1-based
    uint32_t column = 0;    // 1-based; 0 means unavailable
};

struct AddressContext {
    uint16_t pc = 0;
    std::string machineId;
    std::string addressSpaceId;
    std::optional<uint32_t> bankId; // Physical bank ID, not a CPU-visible register value
    uint64_t mappingEpoch = 0; // Runtime mapping revision; not a persistent symbol identity
};

struct AddressRange {
    AddressContext start;
    uint32_t size = 0; // Half-open [start, start + size)
};

struct SourceMapping {
    AddressRange range;
    SourcePosition position;
    std::optional<uint64_t> statementId;
    std::optional<uint64_t> functionId;
    std::optional<uint64_t> inlineContextId;
    MappingQuality quality = MappingQuality::Exact;
    bool isStatementBoundary = false;
};

class IDebugInfoProvider {
public:
    virtual ~IDebugInfoProvider() = default;
    virtual DebugDatabase load(const DebugArtifacts& artifacts) = 0;
    virtual DebugCapabilities capabilities() const = 0;
};

class ISourceDocument {
public:
    virtual ~ISourceDocument() = default;
    virtual const std::string& uri() const = 0;
    virtual SourceLanguage language() const = 0;
    virtual std::string_view text() const = 0;
};
```

Avoid making `AddressContext` an implicit flat 16-bit address in persistent metadata. The bank/overlay may be unknown, but **physical memory identity must be resolvable via a machine-specific mapper adapter before claiming an exact source match**. A runtime mapping epoch identifies a snapshot, not a stable physical bank; do not persist it as a substitute for ROM/RAM object identity.

---

## 3A. Generic bank-aware address identity and machine mapping providers (mandatory foundation)

### 3A.1 Why this is core, not advanced functionality

NES CPUs execute within a 16-bit address space, but cartridge mappers can place different PRG-ROM or PRG-RAM banks into the same CPU window. For example, CPU `$8000` may execute instructions from one PRG-ROM bank and later from another after a mapper register write. A map keyed only by `$8000` will silently show the wrong source file, labels, breakpoints, or disassembly. The debugger must therefore separate **CPU logical address**, **current mapping**, **physical memory identity**, and **ROM file offset** from P0/P2 onward. This is equally mandatory for C64 PLA/6510 overlays and Atari 8-bit OS/BASIC/cartridge/extended-RAM mapping; all three systems are first-class parallel targets.

### 3A.2 Canonical identity and lookup

Define these concepts separately:

- **LogicalAddress:** machine, CPU/address space, 16-bit CPU address. This is what PC and most user-facing disassembly addresses display.
- **PhysicalCodeIdentity:** cartridge/image or loaded-artifact ID, memory kind (`PRG_ROM`, `PRG_RAM`, internal RAM, overlay), physical bank/chip/region, and offset within that object. Distinguish RAM that can be modified at runtime from immutable ROM.
- **RomFileOffset:** optional byte offset in the exact loaded ROM file; iNES/NES 2.0 headers, trainers, and PRG layout must be handled by the cartridge loader, not guessed by multiplying bank numbers.
- **MappingSnapshot / epoch:** runtime state that resolves CPU accesses to physical memory at an instant. Increment on effective mapping changes; capture atomically with instruction fetch events. Epoch is transient and is not the stable identity of code.
- **BankedSourceRange:** compiler/linker source mapping attached to a physical section/bank or a resolved ROM offset, optionally with a CPU load/run window. Do not assume the compiler's bank numbering equals the emulator's mapper bank numbering.

Suggested interface (illustrative):

```cpp
enum class MemoryKind { Rom, Ram, Io, CartridgeRom, CartridgeRam, InternalRam, Overlay, OpenBus, Unknown };

struct PhysicalAddress {
    std::string imageId;
    MemoryKind kind = MemoryKind::Unknown;
    uint32_t regionId = 0;
    uint64_t offset = 0; // Offset in the physical memory object
};

struct MappedAddress {
    uint16_t cpuAddress = 0;
    uint64_t mappingEpoch = 0;
    std::optional<PhysicalAddress> physical;
    std::optional<uint64_t> romFileOffset;
};

class IMemoryMappingResolver {
public:
    virtual ~IMemoryMappingResolver() = default;
    virtual MappedAddress resolveInstructionFetch(uint16_t pc) const = 0;
    virtual MappedAddress resolveRead(uint16_t address) const = 0;
    virtual MappedAddress resolveWrite(uint16_t address) const = 0;
    virtual std::vector<uint16_t> resolveCpuWindows(const PhysicalAddress& address) const = 0;
};
```

`resolveInstructionFetch` must use the **actual fetch mapping**; read, write, and instruction-fetch routing must remain distinct. On C64, a write under BASIC/KERNAL ROM normally targets hidden RAM even when reads/fetches see ROM; on Atari XL/XE, ROM overlays generally block write-through. NES mapper behavior also depends on access type. `resolveCpuWindows` may return zero, one, or several currently mapped aliases. Expose an explicit `Unknown`/`Unmapped` result; never silently fall back to an arbitrary bank. The exact API must be adapted to the existing emulator interfaces; the RD-001 findings per machine are in #20.4 (none of the three emulators exposes physical bank identity to debugger code today).

### 3A.3 NES-specific behavior

- Treat mapper implementation as the authority for translating CPU PRG accesses to physical ROM/RAM; do not duplicate mapper register logic inside the source debugger.
- Cover switchable and fixed PRG windows, PRG-RAM, mirrored aliases, bank-size differences, and mapper-specific switching rules. A mapper's logical register value is not necessarily a unique physical bank ID.
- Source mappings and labels for banked code must be attached to physical identity or artifact section identity; resolving by CPU address alone is allowed only when uniquely determined by current mapping.
- If the same physical ROM bank is mapped to two CPU windows, both windows may refer to the same code bytes but can have different CPU-visible addresses; preserve alias information and relocation/run-address semantics.
- If PRG-RAM code is copied or self-modified, a bank identity alone does not prove source fidelity. Mark the mapping stale/approximate when bytes diverge from the loaded debug artifact; optionally use code-generation/version tracking later.
- Keep PPU/CHR banking **separate** from CPU instruction stepping. A general address-space model should permit PPU/CHR inspection in future, but CHR banking is not a prerequisite for C source stepping.
- Mapper state changes during IRQ/NMI, subroutine calls, or bank-switch trampolines must not corrupt the call stack. Track actual execution and returns rather than assuming that the caller's bank stays mapped.

### 3A.4 Bank-aware breakpoints, labels, and stepping

Support distinct breakpoint intents:

1. **CPU address breakpoint:** triggers whenever the selected CPU address executes, regardless of which physical bank is mapped (explicit user choice).
2. **Physical code breakpoint:** triggers only when the chosen physical code identity executes, regardless of which CPU window currently exposes it.
3. **Source breakpoint:** resolves to one or more physical code identities/locations and follows bank changes without rebinding to an unrelated bank. Preserve unresolved/pending state if the source artifact lacks bank metadata.

Watchpoints must distinguish CPU-visible address from physical storage identity. Disassembly/source navigation should show CPU PC plus mapped bank/physical identity and indicate ambiguous or unknown mappings. A source Step Over/Out can cross bank switches and must compare logical frame/return context and physical code identity, not just CPU PC or source line. Re-resolve the fetch mapping on every executed instruction or consume authoritative mapper change notifications; never use stale cached CPU→bank mappings.

### 3A.5 Debug artifact contract

A compiler adapter should expose, where available, **section/segment ID, bank assignment, physical PRG offset, CPU run/load address, and source spans**. Add an artifact-specific translation layer that aligns linker sections with the cartridge image and emulator physical regions. Validate ROM hash/image identity, mapper ID/submapper and cartridge header metadata before trusting banked source maps. If artifacts only contain CPU addresses, mark banked mappings as ambiguous unless the build layout proves uniqueness.

### 3A.6 Required fixtures and tests

Use small reproducible NES test ROMs with a pinned assembler/linker and explicit bank layout. Start with NROM (no switchable PRG banks) as baseline, then UxROM/UNROM-style switchable PRG banking, and MMC1 or MMC3 as a more complex mapper. Verify actual mapper support in the RetroDebugger emulator before selecting fixtures.

Automated assertions must cover: same `$8000` PC mapping to two different PRG banks and two different source lines; a fixed bank while another window switches; source breakpoint in an inactive bank becoming effective when mapped; CPU-only breakpoint firing in both banks; stepping through a bank-switch trampoline and returning to the caller; IRQ while banks change; ROM file offset with iNES header/trainer; PRG-RAM and missing metadata; bank aliasing; and deterministic mapping change traces. A test must fail if a wrong-bank source line is shown as exact.

**Acceptance gate:** P2/P3 cannot be declared bank-ready until C64, NES, and Atari fixtures pass their respective same-PC/different-physical-identity and read/write visibility assertions. All three machine adapters are parallel delivery tracks, not future ports.

---

## 3B. C64 and Atari 8-bit memory banking — mandatory parallel machine profiles

### 3B.1 Cross-machine architectural decision

Implement a **single, machine-independent memory mapping and source-debugging core**, with three concurrent machine adapters: `C64MemoryMappingProvider`, `NESMemoryMappingProvider`, and `Atari8BitMemoryMappingProvider` (illustrative names). Do not hard-code `bankId` as a globally meaningful integer: physical identity requires a device/image/region/offset, and mapping may vary by **access type**, machine variant, and bus master. Capture instruction-fetch identity at execution time, and use the emulator's authoritative bus/mapper state; never independently simulate bank registers in the debugger. Expose a mapping snapshot with `cpuAddress`, `accessKind`, `physicalAddress`, `visibleDevice`, `mappingEpoch`, `machineVariant`, and optional `imageOffset`. A mapping may resolve to RAM, ROM, I/O, open bus, or no backing object.

The CPU-visible mapping and source identity are separate: **source breakpoints** bind to code/artifact identity; **CPU breakpoints** bind to logical PC; **physical breakpoints** bind to actual storage. Memory Viewer and Watch must be able to request `CPU read view`, `CPU write destination`, or `physical backing` without conflating them. For instruction stepping, use CPU instruction-fetch mapping; for memory writes, resolve the write destination rather than the ROM seen on reads. Do not make the final C-variable research spike (RD-023) a dependency for this foundational memory/breakpoint behavior.

### 3B.2 Commodore 64 — 6510 processor port, PLA, cartridge and hidden RAM

The C64 is **already a banked machine without a cartridge**. The 6510 processor port uses `$0000` as its data-direction register (DDR) and `$0001` as its port data latch; effective pin levels (DDR, outputs and input pull-ups), not merely the last byte written to `$0001`, determine `LORAM` (bit 0), `HIRAM` (bit 1), and `CHAREN` (bit 2). Cartridge `/GAME` and `/EXROM` lines feed the PLA too. A correct provider must query the existing C64 emulator memory/PLA mapping after these signals are applied.

For **no cartridge** (`/GAME=/EXROM=1`), the relevant CPU *read/fetch* windows are:

| Effective low bits of `$01` | `$A000–BFFF` | `$D000–DFFF` | `$E000–FFFF` |
|---|---|---|---|
| `111` (typically `$37`) | BASIC ROM | I/O | KERNAL ROM |
| `110` (`$36`) | RAM | I/O | KERNAL ROM |
| `101` (`$35`) | RAM | I/O | RAM |
| `100` (`$34`) | RAM | RAM | RAM |
| `011` (`$33`) | BASIC ROM | character ROM | KERNAL ROM |
| `010` (`$32`) | RAM | character ROM | KERNAL ROM |
| `001` (`$31`) | RAM | character ROM | RAM |
| `000` (`$30`) | RAM | RAM | RAM |

The table shows typical `$3x` values with the usual DDR setup; low-bit combinations, not literal register values alone, define the mapping. BASIC visibility requires both LORAM and HIRAM; with LORAM and HIRAM both 0 the whole map is RAM whatever CHAREN says (the v1.3 table listed I/O for `$34`; VICE and the hardware show RAM, verified by `CTestC64MemoryMapping`). `CHAREN=0` selects character ROM only when LORAM or HIRAM is active; otherwise `$D000–DFFF` can expose RAM. **Read and write routing are different:** in normal C64 ROM-overlay modes, writes to `$A000–BFFF` or `$E000–FFFF` go to underlying RAM while CPU reads/fetches see ROM. When I/O is visible at `$D000–DFFF`, writes target I/O registers, not underlying RAM. Cartridge configurations and Ultimax mode alter these rules; do not generalize ordinary ROM write-through to every cartridge/Ultimax window.

Cartridge handling MUST include `/GAME`/`/EXROM` modes (8 KB, 16 KB, Ultimax), `ROML`/`ROMH`, `IO1` `$DE00–DEFF`, `IO2` `$DF00–DFFF`, mapper/bank-switch registers, RAM-under-ROM, open/unmapped Ultimax windows, and cartridge-specific bank IDs. Treat EasyFlash/Action Replay/etc. as **device-specific follow-up profiles** selected based on actual emulator cartridge support, not as generic PLA behavior. `VIC-II` sees a different memory/bus view than the CPU (including character ROM visibility); represent bus master/address space explicitly, even though CPU stepping only requires CPU fetch resolution in the MVP. Also distinguish physical underlying RAM from the 6510 port at `$0000/$0001`.

**Required C64 tests:** execute at `$A000` with BASIC visible, switch to underlying RAM and execute at the same PC; write hidden RAM while ROM remains readable; switch `$D000` among I/O, character ROM, and RAM; test `$E000` KERNAL versus RAM; test a simple 8 KB and 16 KB cartridge plus Ultimax open-bus/ROMH windows; test a cartridge bank change at the same CPU address; verify a source breakpoint follows physical code while a CPU breakpoint fires in all applicable mappings. Validate fetch, read and write identities separately.

### 3B.3 Atari 8-bit — 400/800 versus XL/XE/XEGS, PIA PORTB, cartridges, expanded RAM

**Do not treat 'Atari 800' as a single universal banking profile.** Original Atari 400/800 models have different ROM/expansion behavior from XL/XE machines; `PORTB`-controlled OS/BASIC/extended-RAM overlays described below apply to appropriate XL/XE/XEGS configurations, **not universally to the original Atari 800**. Machine model, installed RAM, cartridge type, and expansion standard are mandatory provider inputs.

On common XL/XE machines, PIA `PORTB` (register `$D301`, subject to PIA direction/control configuration) controls internal ROM and RAM overlays. Typically `PORTB` bit 0 selects OS ROM versus RAM underneath (0 = OS enabled), and bit 1 selects BASIC ROM versus RAM (0 = BASIC enabled); exact decode must be delegated to the emulator and machine profile. The XL/XE OS occupies upper address ranges, including `$C000–CFFF` and `$D800–FFFF`, with I/O in `$D000–D7FF`; built-in BASIC commonly occupies `$A000–BFFF`. Cartridge ROM can override BASIC in that window. **Unlike ordinary C64 ROM overlays, Atari XL/XE generally does not allow writes through enabled ROM into hidden RAM**; verify physical write routing via the emulator rather than copying C64 behavior.

For Atari 130XE and compatible RAM expansions, a switchable **16 KB extended RAM window at `$4000–7FFF`** can replace main RAM. On standard 130XE-like mapping, PORTB bit 4 enables extended RAM for CPU accesses when clear, and bits 2–3 select one of the 16 KB extended banks; PORTB bit 5 may independently control ANTIC visibility. Different expansions (RAMBO/COMPY and larger configurations) use different bit layouts, and ANTIC and CPU can observe different backing memory at the same logical address. The provider must distinguish CPU versus ANTIC access; the source stepper uses CPU fetch mapping.

Cartridge windows commonly include `$8000–9FFF` (`S4`) and `$A000–BFFF` (`S5`), while control accesses around `$D500–D5FF` (`CCTL`) select/disable banks on many cartridge types. **The `$D5xx` register semantics are cartridge-specific**, not one standard universal mapper. As a concrete test profile, XEGS bank-switched cartridges map a selectable 8 KB bank at `$8000–9FFF` and a fixed final 8 KB bank at `$A000–BFFF`; OSS, Atarimax, SpartaDOS X and other types have different layouts and bank-control protocols. Use the cartridge type recognized by the emulator as the authority; avoid deriving physical bank from the last byte written to `$D500` for arbitrary carts.

**Required Atari tests:** original 800 baseline cartridge mapping; XL/XE BASIC/OS ROM enable-disable with correct read/write semantics; 130XE CPU bank switching at `$4000–7FFF` with same PC/different physical RAM and distinct ANTIC view; XEGS cartridge two selectable banks at `$8000` plus fixed bank at `$A000`; cartridge disable/re-enable where supported; bank switch during a subroutine/interrupt; source breakpoints and labels bound to physical identity. Record explicit skips for unsupported machine/expansion combinations, not fake success.

### 3B.4 Shared fixtures, CI matrix and compiler independence

Run a **parallel three-machine matrix** with a common step-trace schema: `machine`, `model`, `busMaster`, `accessKind`, `pc`, `mappingEpoch`, `physicalIdentity`, `sourceLocation`, `stopReason`. Required baseline fixtures: C64 ROM/RAM overlay and cartridge, NES NROM plus switchable mapper, Atari XL/XE ROM/RAM and XEGS cartridge, plus 130XE extended RAM when supported. For each fixture assert same logical PC can map to different physical code, distinct fetch/read/write behavior, breakpoint intent, correct stepping across mapping changes, and honest `Unknown` when metadata is insufficient.

**Compiler integration is orthogonal to machine mapping.** Oscar64, cc65, KickC and assemblers are artifact providers; do not hard-code 'Oscar64 = C64'. The supported compiler × machine matrix must be discovered from pinned real toolchain builds. Do not assert that Oscar64 currently supports Atari/NES without a verified target/toolchain invocation; adding support later must require only a new artifact adapter/configuration, not a redesigned debugger core. Test assembly fixtures first for each machine, then C fixtures wherever compiler targets actually exist.

**Parallel delivery gate:** RD-024/RD-027 define common contracts before RD-007/RD-013 integration; RD-025/026 and RD-028/029 can be developed concurrently by machine. P2/P3 acceptance requires real bank-aware behavior for all three families in supported emulator configurations, not just an NES-only abstraction.

**Hardware references:** [C64 memory management](https://codebase64.c64.org/doku.php?id=base:memory_management); [C64 6510 port and mapping](https://github.com/rhmoller/commodore64/blob/master/docs/appendix-b-memory-map.md); [Altirra Hardware Reference Manual](https://www.atari800xl.eu/docs/reference/altirra-hardware-reference-manual.pdf); [Atari800 cartridge types](https://github.com/atari800/atari800/blob/master/DOC/cart.txt); [Atari cartridge overview](https://www.atarimax.com/jindroush.atari.org/acarts.html).

---

## 4. Source document/view integration (requirement A)

### 4.1 Current-state integration

Retain the existing `Source Code` window/menu placement and debugger context. Replace its internal single-file display implementation with a reusable `SourceDocumentView` (or wrap the existing component if suitable).

**Inventory result (RD-001, #20.2):** the existing view is `CViewSourceCode` (`src/Views/CViewSourceCode.h/.cpp`), instantiated for C64 ("C64 Source code"), Atari ("Atari Assembler source") and NES ("NES Assembler source"); NES has no workspace layout slot. It draws with the engine font blitter (`CSlrFont::BlitTextColor`), borrowing the disassembly view's font, inside a custom `Render()`; there is no ImGui text widget and no text-editor component in the app. It follows `viewDisassembly->cursorAddress` (not the PC directly), shows only the file that contains the mapped line, centres it, and draws line numbers and the file path. Scrolling, keyboard, click-to-address, breakpoint markers, multi-file browsing and serialization are stubs. The address-to-line map it reads is `CDebugSymbolsSegment::codeSourceLineByMemoryAddress` (a flat per-segment array of `CDebugAsmSourceLine*`, which already carries line AND column ranges from the KickAssembler `.dbg` format). The new document component therefore replaces the rendering/interaction half and keeps feeding from the canonical database of #6 (which supersedes the flat array).

### 4.2 MVP read-only behavior

- Open source files referenced by debug metadata; support manually opening a file.
- UTF-8 loading, BOM handling, CRLF/LF, robust long lines, horizontal/vertical scrolling and line numbers.
- Highlight **current execution line** distinctly from **selected line** and **breakpoint line**; support multiple indicators on one line.
- Source line gutter with breakpoint markers, current PC marker and (later) coverage/diagnostic markers.
- Click line to select; double-click or command to navigate to corresponding disassembly addresses; if multiple mappings exist, show choices rather than choosing arbitrarily.
- Follow PC option (on/off); when off, execution updates do not steal scroll position or tab focus. Today the view follows the disassembly cursor; keep a "follow disassembly cursor" mode for parity and add a true "follow PC" mode fed from the symbols' CPU (the disassembly view already gets `SetCurrentPC` per UI frame from `CViewC64::Render`, there is no pause callback to subscribe to).
- Missing source files: show path-resolution UI, workspace-relative remapping and per-project source roots; do not crash or show stale content.
- On debug stop, show the best matching source file and location, and keep disassembly synchronized bidirectionally.
- Multiple open files internally from day one; tabs can ship in P1 or be exposed in P5.

### 4.3 Future editor compatibility

Keep **document model**, **syntax highlighting**, **decorations**, **rendering**, and **filesystem persistence** separate. Read-only is a document/view mode, not a different architecture. Future editing adds undo/redo, dirty buffers, save/save-as, conflict detection, selection, find/replace, and project file watchers without rewriting debug navigation. Do not automatically save edited buffers before builds without explicit policy.

### 4.4 Navigation semantics

`PC → candidate mapping(s) → selected source location → document/tab → highlight` and reverse `file:line[:column] → address ranges → disassembly navigation / breakpoint request`. If a line has no executable address, show a non-executable gutter state; optionally search nearby executable locations only via explicit user action.

---

## 5. Syntax highlighting (requirement B)

### 5.1 Language selection

Prefer explicit project language override, then extension (`.c`, `.h` → C; `.asm`, `.s`, `.a65`, `.inc` → assembler subject to dialect), then metadata/heuristics. Compiler identity and syntax language are separate: Oscar64 C is C; its generated `.asm` is 6502 assembly.

### 5.2 C highlighting

Keywords, types, qualifiers, operators, strings/escapes, character literals, numeric literals, line/block comments, preprocessor directives, macro invocations, function identifiers and diagnostics. Support C dialect extensions incrementally; highlight unknown compiler-specific constructs conservatively. Lexing must preserve multiline comment/string state across lines and update incrementally when editing arrives.

### 5.3 6502 assembly highlighting

Opcodes (`LDA`, `STA`, `JSR`, `RTS`, `RTI`, `JMP`, branches, etc.), registers, addressing syntax, hex (`$0400`, `0x0400` where accepted), binary, labels, directives, macros, comments, local labels and symbols. **Dialect profiles** for KickAssembler, 64tass, ca65 and Oscar64-generated assembler; avoid treating incompatible directive syntaxes as interchangeable. Allow a base 6502 profile and future 65C02/65816 profiles.

### 5.4 UI/technical requirements

- Theme-driven token categories; do not hard-code colors into parsers.
- Lexer errors must not break rendering; cap parsing work for very large files.
- Debug execution highlight takes precedence over lexical styling without destroying token colors (background/gutter/underline overlays).
- Test C and ASM golden snippets, escaped quotes, multiline comments, macros and large files.

---

## 6. Canonical debug information (requirement C)

### 6.1 Core entities

`CompilationUnit`, `SourceFile`, `Function`, `LexicalScope`, `Statement`, `SourceMapping`, `AddressRange`, `CallSite`, `InlineContext`, `Symbol`, `Variable`, `Type`, `Diagnostic`, `BuildIdentity`.

Minimum P2 database:

- normalized file identities + original compiler paths and remapping rules;
- final linked address ranges (exclusive end), including multiple non-contiguous ranges per source location;
- optional columns, statement IDs, function IDs and inline ancestry;
- function entry points and address coverage; zero/multiple entries supported;
- compiler/build provenance and metadata capability flags;
- binary checksum/build ID to prevent loading stale maps against another executable.

**Migration rule (1.4):** the P2 database supersedes `CDebugAsmSource` / `codeSourceLineByMemoryAddress`, it does not sit beside it. The KickAssembler `.dbg` loader becomes the first provider of the new model (its `<Segment>` becomes a physical-identity candidate, its `<Block>` a compilation-unit-like grouping, its `LINE1,COL1,LINE2,COL2` a column-aware mapping), and `CViewSourceCode`, the disassembly label/breakpoint lookups and the Remote/MCP `segment` endpoints keep working through it. One active `CDebugSymbolsSegment` per `CDebugSymbols` remains the breakpoint/label scope until RD-025 makes selection mapping-driven.

Optional P6: lexical scopes, variable types and location expressions (memory, registers, software stack, PC-dependent ranges), logical inline frames, optimized-out markers, relocation and overlay semantics.

### 6.2 Resolution rules

1. Resolve the instruction-fetch CPU address through the current machine mapper into stable physical code identity; match source ranges by physical identity and validated artifact layout. Treat mapping epoch as runtime context, not persistent bank identity.
2. Prefer exact mapped ranges; if multiple overlap, rank explicit statement and inline context mappings over line-only mapping; preserve all candidates.
3. Treat unmapped runtime/helper code as `Missing`, not as the preceding source line.
4. Source mapping may move backward (loops), remain on the same line across multiple instructions, and disappear during optimization.
5. A change in `file:line` alone is **not sufficient** to define a C statement boundary.
6. A `SourcePosition` is not a unique statement identity; column is useful but not guaranteed unique.
7. Source breakpoint resolution may produce several machine addresses; retain source intent and re-resolve after rebuilding.

### 6.3 Suggested internal API

```cpp
struct ResolvedLocation {
    std::vector<SourceMapping> candidates;
    MappingQuality quality = MappingQuality::Missing;
};

class IDebugDatabase {
public:
    virtual ~IDebugDatabase() = default;
    virtual ResolvedLocation resolve(const AddressContext& address) const = 0;
    virtual std::vector<AddressRange> resolveSource(const SourcePosition& source) const = 0;
    virtual std::optional<FunctionInfo> findFunction(const AddressContext& address) const = 0;
};
```

Build indexed interval lookup rather than linear-scanning metadata on every CPU instruction. Use immutable snapshot databases during a debug session; replace atomically after build/reload.

### 6.4 Capability negotiation

Each provider reports whether it has line mappings, columns, statement boundaries, function ranges, call sites, inline ancestry, variable locations, types, bank information. UI and stepping adapt accordingly; missing features are **not** silently synthesized as exact information.

---

## 7. Compiler adapters and Oscar64 extension

### 7.1 Oscar64 — first implementation target

**Input:** `.dbj` plus linked binary and optional `.map`/`.lbl`/`.asm`; exact generated filenames and invocation flags must be verified with the pinned compiler build. Parse `functions`, `lines`, `origins`, `variables`, `types`, `memory` where present; do not require optional sections for stepping.

**Verified source audit anchors in supplied Oscar64 snapshot:**

- `oscar64/Errors.h`: `Location` with file, line, column and `mFrom` provenance chain.
- `oscar64/InterCode.h`: `InterInstruction::mLocation`.
- `oscar64/NativeCodeGenerator.h`: `NativeCodeInstruction::mIns` pointer to IR instruction.
- `oscar64/InterCodeGenerator.cpp`: `MapLocation(...)` propagates source/inline location.
- `oscar64/NativeCodeGenerator.cpp`: `NativeCodeBasicBlock::PutLocation(...)` around line 5833, and final range coalescing around lines 72720–72740; adjacent ranges currently compare file+line, potentially losing column/inline distinctions.
- `oscar64/Compiler.cpp`: `.dbj` location emission around lines 1739–1769, including `mCodeOrigins` traversal.

**Recommended upstream patch series (separate PRs):**

1. Add regression fixtures and compare `.dbj` before/after.
2. Preserve column and full provenance when coalescing code-location ranges; compare full location chains by value/identity semantics, not just pointer equality. Be mindful of `weak` locations and optimizer-generated instructions.
3. Add optional `column` to `.dbj` line/origin records; keep old consumers working.
4. Export **structured** inline/provenance chains, distinguishing macro expansion from function inlining if the compiler can identify them; do not label every `mFrom` edge as inline.
5. Investigate stable statement IDs and semantic call-site tags; add only after validating where identity is preserved through IR/native optimization.

**Important:** preserving more ranges may increase `.dbj` size and can reveal mapping gaps. Verify actual emitted data, not only C++ fields. Any sample JSON in this spec is a **proposed schema**, not a claim about Oscar64's existing exact schema.

### 7.2 cc65

Load the linker `.dbg` output (file/line/span/scope/sym/csym/segment records as supported by the pinned toolchain). Respect multiple spans, segment relocations, C versus assembly lines, and lexical scopes. Map function scopes only when sufficiently supported; do not equate every symbol with a function. Test `cl65`/`ld65` debug options on the actual installed version. Initial support can be line-level without compiler patching; enhanced statement/call-site provenance is an optional upstream proposal.

### 7.3 KickC

Treat KickC as a separate compiler adapter and KickAssembler as a potentially separate assembler/link step. **Do not claim an address→C mapping format exists until verified** against a pinned build. Investigate compiler-generated ASM, labels, source annotations, KickAssembler symbol/debug outputs and final address assignment. Where only approximate line mapping exists, mark `Approximate`, limit stepping features, and create a documented proposal for source provenance export through optimizer and assembler stages. Keep compilation and debug-metadata adapters independently replaceable.

### 7.4 Assembly and legacy support

There is no TASM integration to continue. What exists: the KickAssembler `.dbg` XML loader (`CDebugSymbols::ParseSourceDebugInfo` → `CDebugAsmSource::ParseXML`, documented in `docs/README-RetroDebugger.txt` "KickAss debug symbols"), the embedded 64tass assembler used for in-memory assembly only (`CDebuggerApi::Assemble64Tass*`, no debug output), and a KickAssembler-via-Java invocation inside the CrtMaker plugin. Keep all three working; add ca65/64tass/Oscar64 assembly dialects through language profiles. Allow a generic assembler map provider and disassembly-only fallback. **Extension collision:** cc65/ld65 debug files and KickAssembler debug files are both named `.dbg`; loaders must sniff content (`<C64debugger` XML root vs. `version\tmajor=` text header), never dispatch on extension alone (RD-031). Source-level stepping in pure ASM should default to instruction semantics unless explicitly configured for source-line stepping.

---

## 8. Execution tracking and stepping engine

### 8.1 Required CPU/emulator events

Observe `beforeInstruction`, `afterInstruction`, call/return (decoded or derived), IRQ/NMI/BRK entry, RTI, authoritative instruction-fetch physical mapping, mapper/memory mapping changes, breakpoint/watchpoint hits, reset, pause/resume and debug session termination. Prefer authoritative emulator events over decoding an opcode at PC when an instruction may be intercepted or emulated specially.

6502 basics: `JSR` opcode `$20`, `RTS` `$60`, `RTI` `$40`, `BRK` `$00`. `JSR` alone does **not** imply a source-language function call. Do not rely solely on SP comparisons: hardware stack, compiler software stack and interrupts are different concepts.

**What the emulators provide today (RD-001, #20.3):** every CPU core has a hard-coded pre-instruction hook that receives the PC (`c64d_c64_check_pc_breakpoint`, `atrd_check_pc_breakpoint`, `nesd_check_pc_breakpoint`) and a post-execute cell marker; there is no generic callback registry, no RTS/RTI hook, no reset hook, and the only interrupt-entry signal is the VICE IRQ/NMI breakpoint check (which runs after the vector was taken). Stack pushes are annotated per byte (`CStackAnnotationData`: JSR_PCH/PCL, BRK, IRQ, NMI, origin PC) in all three cores; pops do not clear entries. These annotations are the seed of the physical execution tracker, not a replacement for it. VICE auto-pauses on `BRK` and has a single-step watchdog (more than 300 single-step re-arms under 5 ms apart force PAUSED), so a source step must execute as a policy evaluated **inside the emulation-thread hook** with an instruction budget, never as a UI loop of `RUN_ONE_INSTRUCTION` steps. Single-step parks after the instruction in VICE and before the next instruction in Atari800/Nestopia; the observable contract ("PC is the next instruction to execute") is the same, but the state machine must only evaluate at instruction boundaries, never from the VICE per-cycle hook. The 1541 drive CPU shares the C64 debug-mode word and can consume a `RUN_ONE_INSTRUCTION` when drive debugging is on; the step controller must own its own arming state.

### 8.2 Physical versus logical frames

- **Physical execution tracker:** tracks observed calls, returns and interrupt nesting, with confidence/unknown state. It is not guaranteed to reconstruct frames after attaching mid-execution, tail `JMP`, indirect jumps, custom trampolines or corrupted stacks.
- **Logical source frames:** combine function ranges, compiler call-site metadata and inline ancestry. Logical frames can exist without `JSR` (inlined functions).
- Treat IRQ/NMI/BRK as separate execution contexts; `RTI` restores the interrupted context. A source step initiated outside an interrupt normally skips interrupt handler code unless explicitly configured to stop on interrupts.

### 8.3 State machine

`Idle → Armed → Running → Paused(Completed | Breakpoint | Watchpoint | InterruptPolicy | Unmapped | BudgetExceeded | UserCancel | Error)`.

Capture at start: original PC/bank, selected source mapping, statement identity if available, logical frame identity, physical call depth, interrupt nesting and user stepping policy. Re-evaluate after **each executed instruction** and on debugger events. A user breakpoint/watchpoint always wins over an automatic step-completion decision (unless explicitly overridden by a documented policy).

### 8.4 Source Step Into

Execute until the next *distinct executable source statement* in the appropriate context, or enter a called user function with known source if the policy is `step into`. If the current statement calls an unmapped runtime helper, execute through it and resume source stepping on return. If there is no statement ID, use `file:line:column + logical frame + mapped address ranges` as a best-effort approximation. A same-line multi-call example (`a(); b();`) must be tested; line-only providers cannot promise separate stops.

### 8.5 Source Step Over

Finish the current source statement in the **starting logical frame**, ignoring mapped locations in deeper callees, then stop at the next executable statement in that frame. Do not implement this merely as `JSR depth++ / RTS depth--`: handle nested calls, IRQ/RTI, tail calls, indirect calls and control-flow exits. A temporary return-address breakpoint can be an optimization for a proven simple call, **not** the only correctness mechanism. The current `CViewDisassembly::StepOverJsr()` is exactly that shortcut (temporary breakpoint at `pc+3`, no depth check, not recursion-safe, not bank-aware, implemented in a view): it is replaced by the engine of this section and moved into `CDebugInterface` (RD-037), which also gives instruction-level Step Out for free. If the current frame exits before reaching another statement, stop at the caller with a reason such as `FrameExited`.

### 8.6 Source Step Out

Continue until the starting logical frame is exited, then stop at the first safe instruction in its caller. For an inlined logical frame, step out of its inline range/context without requiring an `RTS`. If frame reconstruction is unknown, expose a degraded mode or explanatory stop reason instead of running indefinitely.

### 8.7 Unmapped code and optimized code

Default: skip unmapped callees that are reached during a source step **when their return context is tracked reliably**. Do not skip arbitrary unmapped control flow forever. Apply configurable instruction/time budget and safe stop (`Unmapped`/`BudgetExceeded`) when no reliable return is known. Optimized-away lines are not executable and must not be fabricated; mark approximate mappings and document source-level stepping limits.

### 8.8 Breakpoints

- Source breakpoints bind to file+line(+column/statement) and resolve to zero, one or many machine addresses.
- Disassembly breakpoints bind to machine address context.
- Breakpoints survive recompilation by re-resolving source intent; stale binary/metadata must trigger warnings.
- Conditional source breakpoints require a separate expression evaluator and are out of MVP scope.
- Define priorities for user breakpoints, watchpoints, temporary internal step breakpoints and pause requests. Today the temporary breakpoint is a single `int` per `CDebugBreakpointsAddr`, its hit is checked ahead of the user breakpoint lookup in the VICE hooks (a user breakpoint at the same address is skipped), it creates no event-history entry and no remote broadcast, and the C64 PC/memory checks ignore the `breakOnPC`/`breakOnMemory` flags. The priority rules of this section fix all four.

### 8.9 Pseudocode (behavioral, not emulator API)

```cpp
void SourceStepController::onInstructionCompleted(const CpuEvent& event) {
    if (!activeStep) return;
    tracker.apply(event);

    if (event.userBreakpointHit || event.watchpointHit) {
        stop(StepStopReason::UserBreakpoint);
        return;
    }
    if (event.resetOccurred || event.sessionEnded || userRequestedCancel()) {
        stop(StepStopReason::Cancelled);
        return;
    }
    if (exceededInstructionBudget()) {
        stop(StepStopReason::BudgetExceeded);
        return;
    }

    const ResolvedLocation location = database.resolve(event.nextAddress);
    const StepDecision decision = policy.evaluate(startContext, tracker, location);
    if (decision.shouldStop) stop(decision.reason);
}
```

Avoid blocking the UI thread or recursively executing CPU instructions inside a GUI callback.

---

## 9. Test programs and compilation pipeline (requirement D)

### 9.1 Fixture repository layout

```text
tests/source-debug/
  fixtures/
    basic-flow/main.c
    same-line/main.c
    nested-calls/main.c
    recursion/main.c
    branches-loops/main.c
    interrupts/main.c
    indirect-calls/main.c
    optimized/main.c
    inline/main.c
    asm-only/main.asm
    banked/...
  toolchains/
    oscar64.yaml
    cc65.yaml
    kickc.yaml
    kickass.yaml
    64tass.yaml
  expectations/
    oscar64/*.json
    cc65/*.json
    kickc/*.json
  scripts/
    build_fixtures.py
    validate_debug_info.py
    run_stepping_tests.py
  artifacts/                 # gitignored
```

Keep one portable C subset across compilers plus compiler-specific fixture variants when startup, intrinsics, calling conventions or memory layout differ. Do not pretend that one C source can always be compiled unchanged by all three toolchains.

**Fixture tiers (1.4):** Tier 0 fixtures are synthesized in-process by the test itself with no external tool (the repo already does this: `CTestViceInstructionStepping` pokes opcodes and jumps, `CTestStackAnnotation` writes a NES ROM to a temp path, the embedded 64tass can assemble source into RAM); they carry the banking and stepping-engine tests. Tier 1 fixtures are committed golden artifacts (binary + debug metadata + the exact toolchain command and version) under `tests/data/source-debug/`, regenerated by `tests/source-debug/scripts/`; they run in the PR suite. Tier 2 rebuilds Tier 1 from pinned toolchains in the nightly job. `assets/tests/c64/dankwars.prg` + `dankwars.dbg` (an unused KickAssembler pair already in the repo) is the first Tier 1 fixture and the regression anchor for the existing loader.

### 9.2 Minimal shared test source

```c
volatile unsigned char result;

unsigned char add(unsigned char left, unsigned char right) {
    return left + right;
}

void doSomething(void) {
    result = add(result, 3);
}

int main(void) {
    result = 1;
    doSomething();
    result = add(result, 2);
    for (;;) { /* stable debugger stop point */ }
}
```

Add mandatory NES NROM and switchable-PRG-bank fixtures (see #3A.6), including the same CPU PC mapped to different physical PRG banks. Add tests for `a(); b();` on one line, multiple statements with different columns, nested calls, calls to runtime helpers, repeated calls from the same line, `if/else`, `for`, `while`, `switch`, `return`, recursion (where supported), indirect function calls (where supported), inlining, optimized-out statements, IRQ/NMI, and bank switches. For `printf`, use compiler-specific tests because library and startup behavior differ.

### 9.3 Toolchain configuration

External commands must be configurable as executable + argument arrays, working directory, environment and timeout, **not shell-concatenated strings**. Record compiler path/version/hash, source hashes, flags, output binary and metadata paths. Separate `compile`, `assemble/link`, `parse diagnostics`, `load binary`, and `load debug info` stages. KickC may need explicit KickAssembler configuration. Build adapters should discover tool versions and fail with actionable errors.

**Illustrative configuration, not validated command-line flags:**

```yaml
toolchain: oscar64
compiler: /path/to/oscar64
workingDirectory: ${workspaceFolder}
arguments:
  - ${sourceFile}
outputs:
  binary: ${buildDir}/program.prg
  debugInfo: ${buildDir}/program.dbj
```

Pin working toolchain versions/commits in CI; verify each command and generated filename empirically before publishing presets. Use an offline/locally installed toolchain option; do not require network downloads at test runtime.

### 9.4 Automated test layers

1. **Unit:** lexer snapshots; path normalization; address interval queries; parser validation; overlapping/non-contiguous spans; inline chain parsing; breakpoint resolution.
2. **Metadata contract:** compile fixture; verify linked binary and debug map correspond; compare normalized canonical JSON snapshots; check `file:line[:column]` on known addresses.
3. **CPU stepping integration:** load PRG/ROM into headless emulator, stop at fixture start, issue Step Into/Over/Out, assert `(PC, function, file, line, frame depth, stop reason)`.
4. **Negative/fault:** missing source file, corrupted debug file, stale binary, unresolvable function, interrupt during step, infinite loop, unmapped runtime and budget exhaustion.
5. **UI smoke:** open Source Code view, syntax language, current-line decoration, click navigation, tab switching and follow-PC toggle.
6. **Regression:** disassembly stepping, existing breakpoints, KickAssembler `.dbg` loading (`assets/tests/c64/dankwars.dbg`), VICE `.vs`/64tass labels, HJSON symbol persistence, the `-symbols`/`-debuginfo`/`-segment` command line and performance must not regress.

### 9.5 Expected-step trace format

```json
{
  "fixture": "nested-calls",
  "compiler": "oscar64",
  "optimization": "debug",
  "steps": [
    {
      "action": "sourceOver",
      "start": {"file": "main.c", "line": 11},
      "expected": {"file": "main.c", "line": 12},
      "expectedReason": "StepCompleted"
    }
  ]
}
```

**Example only:** expected line numbers and compiler flags must be generated from actual pinned fixture builds. Do not hard-code machine addresses across compiler versions unless testing a frozen golden binary.

### 9.6 CI strategy

- Fast PR suite: parser/stepping unit tests + checked-in golden binaries/maps (subject to licensing and size review). **Today only `build-macos.yml` runs `tests/run_test.sh`; Linux installs xvfb but runs no tests and Windows runs none.** The three-machine matrix needs the headless suite on Linux (at least) before P2/P3 acceptance can be measured (RD-034); `AGENTS.md` already requires all three platforms green on both log legs.
- Nightly or optional full suite: compile with pinned Oscar64/cc65/KickC versions and run headless integration tests.
- Matrix: compiler × optimization level × fixture × target machine; mark unsupported capabilities as `skip with reason`, not pass.
- Produce JUnit/XML or equivalent test report, logs, machine/source step traces and failing metadata artifacts.
- Require deterministic timeouts and execution instruction budgets; record seed/interrupt schedule for repeatability.

### 9.7 Manual verification checklist

Open C64 fixture; load matching PRG and debug metadata; inspect highlighted C line; open disassembly and verify PC mapping; Step Into user function; Step Over `printf`/runtime helper; Step Out to caller; step through loop and branch; interrupt during step; navigate to ASM; test same-line statements; change source path; load stale metadata; verify stop reasons. Record screen/video and trace for any discrepancy.

---

## 10. IDE evolution: workspace, tabs, builds

### 10.1 Workspace model

`Workspace` has a root folder, optional project configuration, source roots, path mappings, toolchain profile, build configurations and target emulator. A folder explorer displays hierarchical files and folders; supports expand/collapse, filtering, open-in-tab and file refresh. Do not require a project file merely to open a folder.

### 10.2 Tabs and documents

Each file URI maps to one document model with multiple possible views. Tabs show basename, dirty state (future), language and close actions. Support switching between C, ASM, generated ASM and disassembly. Generated build artifacts may be read-only. Preserve navigation history (back/forward) and last source location.

### 10.3 Build/run/debug

- Toolchain registry for the built-in 64tass (`CDebuggerApi::Assemble64Tass*`), the KickAssembler-via-Java pattern already used by the CrtMaker plugin (`javaPath`, `kickAssJarPath`, `kickAssParams`, mtime-based rebuild skip) and external Oscar64, cc65, KickC/KickAssembler and future tools.
- Configurable build task: executable, arguments, cwd, env, outputs, diagnostic parser, pre/post steps.
- Build console with stdout/stderr and clickable compiler diagnostics.
- On successful build, atomically load matching executable + debug info; optional reset/run-to-entry policy.
- On failed build, keep previous debug session clearly marked as stale; do not silently associate old symbols with new source.
- Workspace-relative configurations with variable substitution; allow user-specific tool paths outside version control.
- Later: hotkeys for Build, Rebuild, Run, Debug, Stop, Step Into/Over/Out, and open declaration.

### 10.4 Security and robustness

Explicit consent before running arbitrary workspace build commands; do not automatically execute a newly opened project's scripts. Escape/process arguments correctly (the only process launcher the app uses today is the engine's `SYS_ExecSystemCommand(const char *shellString, ...)`, which takes a shell string; build tasks use the engine's argv-based `CProcessSpawner::Run` instead, see open decision 11 and RD-042); prevent path traversal in artifact resolution; cap external tool runtime; redact sensitive environment variables in logs; handle untrusted malformed metadata safely.

---

## 11. UX and keyboard model

Suggested actions (adapt to existing RetroDebugger shortcuts): `Step Instruction`, `Step Source Into`, `Step Source Over`, `Step Source Out`, `Toggle Breakpoint`, `Follow PC`, `Open Source`, `Open Disassembly`, `Open Folder` (later), `Build` (later). Clearly distinguish instruction-level and source-level modes in toolbar/status bar. Show current compiler, loaded debug file, mapping confidence, active frame, and a warning when source stepping is approximate or unavailable.

Recommended source line decorations: current execution (gutter arrow + background), breakpoint (gutter icon), selected line (subtle selection), diagnostic (underline), inlined location (breadcrumb/tooltip). Avoid permanently forcing focus to the source window when the user is inspecting disassembly.

---

## 12. Risks and mitigations

| Risk | Mitigation |
|---|---|
| Source metadata is incomplete/ambiguous | Capability flags, mapping quality and disassembly fallback |
| `JSR` used for runtime helpers | Semantic call-site data where available; track frame transitions, not just opcode |
| Tail calls/indirect jumps | Track actual control flow and function context; unknown-state safe stop |
| IRQ/NMI/BRK during stepping | Separate interrupt frames and `RTI` handling; deterministic tests |
| Optimization/inlining | Optional inline provenance, logical frames, explicitly approximate stepping |
| Same-line C statements | Columns/statement IDs when available; document line-only limitations |
| Bank switching/overlays | Physical code identity and authoritative mapper translation from P0/P2; mapping epochs only for runtime snapshots; NES multi-bank regression fixtures |
| Stale debug metadata | Binary build identity/hash checks |
| Unmapped loops hang the UI | Instruction/time budgets, cancellation, asynchronous state machine |
| Toolchain CLI/format changes | Version-pinned integration tests and provider-specific adapters |
| Editor rewrite blocks future IDE | Reusable document model and separated view/lexer/decoration layers |

---

## 13. Acceptance criteria

**P1:** Current Source Code window can open C and ASM files, highlight tokens and PC line, display breakpoints, and navigate bidirectionally to disassembly. The document model can support multiple files without being replaced.

**P2 banking gate:** The canonical resolver differentiates two NES PRG banks mapped at the same CPU address and never claims an exact wrong-bank source mapping; mapper translation and ROM artifact identity are verified.

**P2:** For a pinned Oscar64 fixture, source addresses and function ranges resolve correctly; non-contiguous ranges, ambiguous mappings, missing files and mismatched binaries are handled explicitly. cc65 and KickC providers expose real, tested capabilities rather than assumed ones.

**P3 banking gate:** Source stepping and source/physical breakpoints survive PRG bank switches and bank-switch trampolines; CPU-address-only breakpoints retain explicitly distinct semantics.

**P3:** Source Into enters mapped user calls; Source Over skips their bodies; Source Out returns to the caller; user breakpoints/watchpoints interrupt stepping; IRQ does not corrupt call depth; unmapped code cannot cause an unbounded hang; source and instruction stepping remain separate.

**P4:** Automated compilation/metadata/stepping tests run against Oscar64, cc65 and KickC where supported, with recorded toolchain versions and explicit skips for missing metadata capabilities.

**P5:** A workspace folder opens into a file explorer, files open in tabs, and an external compiler can be invoked via a configured build task with clickable diagnostics and matching debug artifacts.

### Definition of done (cross-cutting)

Tests are repeatable; logs explain stop reasons; no regression in existing KickAssembler `.dbg`/label/disassembly features; documentation includes setup, supported compiler versions, limitations and sample workflows; code reviews validate lifetime/threading safety between UI and emulator.

---

## 14. Concrete engineering backlog

| ID | Phase | Priority | Task | Depends on |
|---|---|---|---|---|
| RD-001 | P0 | core | Inventory existing source view, debugger hooks, breakpoint engine, symbol loaders, 64tass/KickAssembler integrations and C64 PLA/6510, NES mapper, and Atari PIA/cartridge translation APIs. **Done 2026-10-17, results in #20.** | — |
| RD-002 | P0 | core | Add portable C/ASM fixtures and pin compiler versions; verify the compiler x machine matrix with real invocations | — |
| RD-038 | P0 | core | Regression test for the existing KickAssembler `.dbg` path (`assets/tests/c64/dankwars.dbg`) and VICE `.vs` loader before any refactor | RD-001 |
| RD-024 | P0 | core | Define generic CPU/logical versus physical code identity and machine mapping provider contract, including mapper capability discovery | RD-001 |
| RD-027 | P0 | core | Audit C64 PLA/6510, NES mapper and Atari 400/800/XL/XE/XEGS emulator memory hooks, including fetch/read/write and bus-master semantics; implement shared mapping event contracts; add the missing emulator-core accessors (#20.4) | RD-001, RD-024 |
| RD-003 | P1 | ui | Introduce document registry and read-only source view (replaces `CViewSourceCode` rendering/interaction, keeps its placement) | RD-001, RD-038 |
| RD-004 | P1 | ui | Implement C lexer and 6502 assembler dialect profiles | RD-003 |
| RD-005 | P1 | ui | Add PC/source decorations and bidirectional navigation (first against the migrated `.dbg` map, then RD-007) | RD-003, RD-006 |
| RD-006 | P2 | core | Define canonical debug database and capability model; migrate `CDebugAsmSource` onto it | RD-001, RD-038 |
| RD-031 | P2 | core | **Must fix.** Debug-artifact loader registry with content sniffing (KickAss `.dbg` XML vs cc65 `.dbg` text vs Oscar64 `.dbj` JSON), replacing extension dispatch in `CMainMenuBar`, `CMainMenuHelper::LoadLabelsAndWatches` and `C64CommandLine` | RD-006 |
| RD-032 | P2 | core | Fix symbol lifetime hazards: segments never deleted, dangling `codeSourceLineByMemoryAddress` after `.dbg` re-import, segment destructor double delete, `.vs` `D:` labels ignored, newer `<Watchpoints>` layout misparsed | RD-006 |
| RD-007 | P2 | core | Implement physical-bank-aware address indexes, source remapping and binary/cartridge identity checks | RD-006, RD-024 |
| RD-008 | P2 | core | Implement Oscar64 `.dbj` adapter and fixture contract tests | RD-002, RD-006, RD-031 |
| RD-009 | P2 | core | Implement cc65 `.dbg` adapter and fixture contract tests | RD-002, RD-006, RD-031 |
| RD-010 | P2 | core | Audit KickC artifacts and implement supported adapter subset | RD-002, RD-006, RD-031 |
| RD-011 | P2 | core | Add emulator execution/interrupt events and instruction-fetch mapping snapshot hooks (generic callback at the existing pre/post-instruction hook points; RTS/RTI/reset/interrupt-entry signals) | RD-001, RD-024 |
| RD-028 | P2 | c64 | Implement C64 memory mapping provider: 6510 DDR/port, PLA, ROM-under-RAM, I/O and cartridge/Ultimax identities (unify the `CViewC64MemoryBank` table; add VICE cart-bank accessors; fix the Ultimax uninitialized-read path in `c64d_peek_c64`) | RD-024, RD-027 |
| RD-040 | P2 | nes | Implement NES memory mapping provider: mapper-authoritative PRG identity via the board's `Memory::GetBank`, iNES/NES 2.0 header → ROM file offset, PRG-RAM, aliases; fix `nesd_peek_safe_io` out-of-bounds RAM reads | RD-024, RD-027 |
| RD-029 | P2 | atari | Implement Atari 8-bit provider: machine variants, PIA PORTB, BASIC/OS ROM, 130XE extended RAM (`MEMORY_xe_bank`), ANTIC view and cartridge mappings (`CARTRIDGE_main`) | RD-024, RD-027 |
| RD-037 | P2 | core | Move step-over out of `CViewDisassembly::StepOverJsr` into `CDebugInterface`; add instruction-level Step Out; make the temporary breakpoint a proper breakpoint object with event/broadcast and correct priority vs user breakpoints | RD-011 |
| RD-012 | P3 | core | Implement physical/logical execution context tracker (seeded from `CStackAnnotationData`) | RD-011, RD-006 |
| RD-013 | P3 | core | Implement asynchronous source-step state machine and stop reasons, evaluated in the emulation-thread hook | RD-007, RD-012, RD-037 |
| RD-025 | P3 | core | Implement bank-aware CPU/source/physical breakpoints and labels; preserve source identity across mapper changes; mapping-driven segment activation | RD-007, RD-011, RD-024 |
| RD-014 | P3 | core | Implement source breakpoints and re-resolution after rebuild | RD-007, RD-013, RD-025 |
| RD-015 | P3 | tests | Add headless step trace runner and golden expectations | RD-002, RD-013 |
| RD-026 | P3 | tests | Add NROM, UxROM and one MMC1/MMC3 bank-switching fixture with golden physical-fetch traces and source-step assertions | RD-002, RD-024, RD-025, RD-040 |
| RD-030 | P3 | tests | Add C64 and Atari banked fixture suites and the shared three-machine headless stepping/breakpoint test matrix | RD-002, RD-025, RD-028, RD-029 |
| RD-034 | P0 | ci | **Must fix.** Run the headless suite on Linux CI (xvfb is already installed) so the three-machine matrix is measured on more than macOS; keep both log legs | — |
| RD-036 | P3 | remote | Remote/MCP endpoints: `step/source/{into,over,out}`, `step/out`, `source/location`, `symbols/load`, and the MCP skill doc update | RD-013, RD-031 |
| RD-033 | P3 | core | Generalize symbol loading entry points: `-debuginfo`/autoload for NES (today C64 or Atari only), unreachable `nes` CLI branch, missing `-ines` handler, NES `source_code` workspace slot | RD-031 |
| RD-016 | P4 | compiler | Patch Oscar64 range coalescing/column export with regression tests (upstream) | RD-008 |
| RD-017 | P4 | compiler | Add structured inline provenance support and logical inline frames | RD-016, RD-012 |
| RD-035 | P4 | ci | Toolchain provisioning for the nightly matrix: cc65 (distro package), Oscar64 (pinned commit, built from source), KickC + KickAssembler (Java; licensing review for the jar), with version recording | RD-002 |
| RD-043 | P4 | compiler | Embedded 64tass emits symbols and an address→source-line map on every in-process assembly (`CDebuggerApi::Assemble64Tass*`, MCP `retro_assemble`, plugins), loaded straight into the canonical database; see #20.9 | RD-006, RD-031 |
| RD-044 | P4 | c64 | 1541 drive CPU source debugging: per-CPU wiring of database/resolver/observer/step controller on `symbolsDrive1541`, drive mapping provider (RAM/VIA/ROM, no banking), `D:` labels, project-level routing of `.dbg` segments to the drive, drive stack as tracker seed; see #20.8 | RD-013, RD-025, RD-032 |
| RD-045 | P5 | c64 | C64 Ultimate source support in observed mode: source view, labels and mapping provider from polled `$01`/EXROM/GAME, `C64UTraceBuffer` replayed into the execution observer after the fact (source-level trace, last executed source lines); stepping/source breakpoints gated by `CC64BackendCapabilities`; see #20.8 | RD-011, RD-028, RD-003 |
| RD-018 | P5 | ui | Add workspace explorer and multi-file tabs | RD-003 |
| RD-042 | P0 (engine) / P5 (use) | core | **Must fix, decided: engine.** Extend the engine's existing argv-based `CProcessSpawner` with `Run` (cwd/env/PATH/timeout/cancel/captured output), consumed by build tasks in P5; replaces shell-string `SYS_ExecSystemCommand` | — |
| RD-019 | P5 | ui | Add toolchain registry, external build tasks and diagnostics | RD-018, RD-002, RD-042 |
| RD-020 | P5 | core | Add build/run/debug orchestration with artifact consistency checks | RD-019, RD-007 |
| RD-021 | P5 | ui | Enable editing, save, undo/redo, search and file watchers (no file watcher exists today) | RD-003, RD-018 |
| RD-022 | P6 | core | Extend debug model for variables/types, advanced optimized stepping | RD-017 |
| RD-023 | P6 | research | Research C symbols, labels, variable-to-memory mapping and integration with existing Memory Viewer and Watch; produce a follow-up implementation proposal only | RD-008, RD-009, RD-010, RD-020 |

**Recommended critical path:** RD-001 (done) → RD-038/002 → RD-024/027/006 → RD-031/032/007/011/037 + RD-028/040/029 + RD-008 → RD-012/013/025/014/015/026/030 (+ RD-034 so the matrix is measured) → RD-003/004/005 integration (can start earlier against the migrated `.dbg` map) → RD-036 → compiler expansion (RD-009/010/016/017, RD-035) → RD-043/044 → IDE (RD-018..021, RD-042 consumption) → RD-045 → RD-022/023. RD-034 and the RD-042 engine proposal are filed first (must-fix, M0). The `Phase` column is the delivery phase of #2; `Priority` is the delivery track (core / ui / c64 / nes / atari / tests / ci / remote / compiler / research). Keep the UI and core debugger changes separately testable. The live status of every item is tracked in `TRACKER.md` next to this file.

---

## 15. Open decisions / discovery questions

1. Which existing RetroDebugger UI text-rendering/editor component can be reused without introducing an incompatible dependency? **Answered (1.4):** none beyond `CSlrFont` blitting; the app has no text-editor widget and the engine (MTEngineSDL, which must not be modified from this repo) offers `CGuiEditBoxText`/`CGuiEditHex` single-line editors only, as far as the app's includes show. The document component is app-side code rendering with ImGui draw lists or the existing font blitter.
2. Which exact CPU execution callbacks and threading model exist for C64 and other emulators? **Answered (1.4):** #20.3.
3. Does the current debugger have reliable breakpoint hooks before each instruction, including IRQ entry and bank mapping changes? **Answered (1.4):** before each instruction yes (all three cores); IRQ entry only on VICE and only as a post-vector breakpoint check; bank mapping change notifications: none, the provider must re-resolve per fetch (#20.4).
4. Which exact Oscar64, cc65 and KickC versions/flags are the initial supported baseline?
5. C64 may be the first integrated runtime, but C64/Atari/NES must share a bank-aware canonical debug-address API immediately; identify which mapper hooks are already exposed per emulator. **Answered (1.4):** none are exposed to debugger code; VICE has `mem_get_current_bank_config()` and the cart bank variables, Nestopia has `Memory::GetBank` behind a protected `Board::prg`, Atari800 has `MEMORY_xe_bank`/`PIA_PORTB`/`CARTRIDGE_main`; all need accessors (RD-027).
6. Should a source step into a function without source metadata fall back to disassembly or skip that call by default? Proposed default: skip tracked runtime calls; expose an option to enter disassembly.
7. Which compiler optimizations are officially supported for *precise* source stepping? Proposed initial baseline: debug/unoptimized builds; optimized mode best-effort with visible warning.
8. What existing assembler build/project configuration must remain backward-compatible? **Answered (1.4):** there is no TASM integration; backward compatibility covers the KickAssembler `.dbg` loader and its documented format, the `-debuginfo`/`-symbols`/`-segment`/`-pass` command line, the `.labels`/`.vs`/`.watch`/`.dbg` autoload next to a PRG/XEX, the HJSON symbol persistence and the Remote/MCP `segment` endpoints.

9. Which NES mapper families and ROM/linker artifact formats can be exercised in headless integration tests initially?
10. How does the existing emulator expose physical PRG identity and fetch mapping, including mapper-specific aliases and PRG-RAM? **Answered (1.4):** it does not; #20.4 lists the core accessors to add (RD-040).
11. Which argv-based process launcher is used for build tasks? **Decided (maintainer, 2026-10-17): engine.** The engine already ships an argv-based `CProcessSpawner` (spawn/kill/try-wait, used by NetGame); RD-042 adds `CProcessSpawner::Run` there with working directory, captured stdout/stderr, PATH lookup, timeout and process-tree cancellation (engine branch `c-source-debugging`), and the app consumes it once merged into engine `devel`. The engine's `SYS_ExecSystemCommand` takes a shell string and is unsuitable for #9.3/#10.4.
12. Does source-level debugging cover the C64 Ultimate (`CDebugInterfaceC64U`) and the 1541 drive CPU? **Decided (maintainer, 2026-10-17): both are must-have, delivered in later phases** (1541: RD-044 in P4; C64U observed mode: RD-045 in P5). The compatibility review in #20.8 lists what P0-P3 must keep open so neither needs a redesign.
13. Which default Atari configuration do fixtures assume? The debugger's default is 800XL with the 1088 KB RAM option (`c64SettingsAtariMachineType=4`, `c64SettingsAtariRamSizeOption=9`), so an extended-RAM window is active by default; fixtures must set machine type and RAM size explicitly.
14. Embedded 64tass (vendored 1.46, 2012): capture symbols/listing from the vendored version through callbacks (RD-043, recommended first step) or upgrade the vendored assembler to a current 64tass first? The upgrade is larger (new source tree in three build systems, syntax differences for plugin-generated code) and is not required for RD-043.

Resolve these during RD-001/RD-002; none requires postponing the canonical model or the Oscar64 adapter.

---

## 16. Source basis and verification status

This specification consolidates the architecture and stepping discussion in the conversation and the **user-supplied Oscar64 source snapshot** (`oscar64-source.zip`, archive revision `a92fd0cd4f9a12dd5340f57da768fc5a8dc1dd63`). Relevant Oscar64 anchors are listed in #7.1. The source audit supports the existence of source locations, IR/native linkage and provenance data, but **does not establish that every optimizer transformation preserves exact statement identity**.

cc65/KickC integration details remain **implementation hypotheses pending compilation of pinned fixture programs**; their precise output formats, flags and capabilities must be established by RD-002/RD-009/RD-010. The proposed APIs, JSON, YAML and class names are design proposals, not claims about existing RetroDebugger interfaces. No RetroDebugger source repository was provided for revisions 1.0-1.3; revision 1.4 performed that reconciliation against the repository (branch `c-source-debugging`, base `devel` 778ef3a) and records it in #20. Where #20 and earlier sections disagree, #20 wins.


---

## 17. Final research spike — C symbols, labels, Memory Viewer and Watch integration (future work)

**Classification:** Research / feasibility spike only. **Timing:** Final stage, after the source display, debug metadata adapters, stepping, tests and build workflow have reached their acceptance criteria. **Not in implementation scope for this release.**

### 17.1 Context and intent

RetroDebugger already provides a **Memory Viewer** and a **Watch** facility capable of inspecting memory locations. The long-term opportunity is to connect those existing tools to C-level symbol and variable metadata instead of building separate memory-inspection UI. This spike must establish what can be derived reliably from compiler artifacts and what would require compiler extensions or runtime context.

### 17.2 Research questions

1. **Symbols and labels:** Inventory existing RetroDebugger label/symbol loaders and internal label storage. Determine how C global variables, functions, static symbols, assembler labels, compiler-generated aliases and linker symbols could resolve to the same canonical symbol model without duplicate or misleading names.
2. **Compiler metadata:** Compare the actual symbol, type, scope and storage-location information produced by pinned **Oscar64**, **cc65** and **KickC** builds, plus assembler maps. Record unsupported capabilities rather than inventing them.
3. **Address mapping:** Determine whether each variable has a fixed address, a PC-dependent storage location, a register or software-stack location, an optimized-out value, multiple live ranges, or no reliable mapping. Account for banked memory, overlays, zero page and shadowed names.
4. **Memory Viewer integration:** Explore navigation from a C declaration, function, label or selected expression to the resolved memory address/range, with byte size, type and a clear validity/confidence indication. Reuse the current viewer wherever possible.
5. **Watch integration:** Explore creating watches from C identifiers/expressions and displaying values using compiler type metadata (integer widths/signs, pointers, arrays, structures) where supported. Distinguish raw address watches from expressions that require execution-context-aware evaluation.
6. **Execution context:** Assess how current PC, logical/physical frame, function scope and build identity affect variable resolution; a local variable name alone must not imply a stable address.
7. **User experience and safety:** Define behavior for ambiguous or unavailable variables, stale binary/debug pairs, changed memory banking, pointer dereferencing and invalid ranges. Never present guessed values as exact.
8. **Testing:** Propose fixtures for globals, statics, locals, shadowed names, arrays, structs, pointers, zero-page allocation, inlining, optimization and bank switching, including comparisons against runtime memory.

### 17.3 Deliverables and exit criteria

- A compiler-by-compiler **capability matrix** for symbol names, addresses, types, scopes, storage locations and live ranges.
- An inventory of existing **Memory Viewer**, **Watch** and label APIs suitable for reuse, with gaps and ownership boundaries.
- A proposed **symbol/variable resolution model** and a UX sketch for `Go to Memory`, `Add to Watch` and typed value display.
- A small reproducible **proof of concept** only if necessary to validate feasibility; no production integration is expected.
- A separate, prioritized **follow-up implementation specification/backlog** with dependencies, risks and test acceptance criteria.

**Explicit non-goals for this spike:** implementing a new variable inspector, replacing existing Memory Viewer/Watch, shipping C expression evaluation, or committing to full optimized-local-variable support. No implementation work should be scheduled before the research results are reviewed.

---

## 18. Revision 1.2 — NES banking decision

Bank-aware physical code identity, mapper-backed instruction-fetch resolution, and physical/source breakpoint semantics are mandatory early architecture and test requirements (#3A; RD-024–026), **not** deferred to P6. The final C variable/Watch/Memory Viewer research spike (RD-023) remains last and research-only. Existing C64 behavior must remain compatible.

---

## 19. Revision 1.3 — parallel C64/NES/Atari banking decision

**Decision:** No C64-first / NES-later architecture. Implement generic physical memory identity and fetch/read/write mapping contracts immediately; build C64, NES, and Atari adapters and test fixtures in parallel. C64 banking is governed by 6510 `$0000/$0001`, PLA and `/GAME`/`/EXROM`, with hidden RAM and access-specific routing. Atari banking is model-dependent, including XL/XE `PORTB`, 130XE extended RAM, ANTIC versus CPU views, and cartridge-specific `$D5xx` switching. NES continues to use mapper-authoritative PRG translation. RD-027–030 implement this decision; RD-023 remains the final research-only C-symbol/Watch/Memory Viewer spike.

## 20. Revision 1.4 — reconciliation with the RetroDebugger codebase

Revision 1.4 is the first revision written with the repository at hand. It executes the RD-001 inventory against `devel` (778ef3a) and corrects the sections above where they assumed something the code does not have, or missed something it already has. All paths are relative to `src/` unless stated otherwise; the engine sibling `../MTEngineSDL` was not available, so engine-side widgets are described only through the app's includes.

### 20.1 Summary of corrections

| Area | Revisions 1.0-1.3 assumed | What the code has | Consequence |
|---|---|---|---|
| Assembler integration | "existing TASM integration" | No TASM anywhere. A KickAssembler `.dbg` XML loader with segments, blocks and line+column ranges; the embedded 64tass used for in-memory assembly only; a KickAssembler-via-Java call in the CrtMaker plugin | Replace every "TASM" reference; the `.dbg` loader is the regression anchor and the first provider of the new model |
| Address-to-source map | To be created in P2 | Exists: `CDebugSymbolsSegment::codeSourceLineByMemoryAddress[]` filled by `CDebugAsmSource::ParseXML` | P2 migrates, it does not greenfield; P1 decorations can ship against the migrated map |
| Segments / banks | Nothing bank-like exists | KickAssembler `<Segment>` = one `CDebugSymbolsSegment` with its own labels, watches, breakpoints and source map; exactly one is active, selected by hand (menu, `;`/`'`, `-segment`, remote `segment/write`) | A segment is already a "physical code identity candidate"; RD-025 makes activation mapping-driven instead of manual |
| Stepping API | Unknown | Modes are `#define`s (RUNNING/PAUSED/RUN_ONE_CYCLE/RUN_ONE_INSTRUCTION/SHUTDOWN); step-over lives in `CViewDisassembly::StepOverJsr` as a temp breakpoint at `pc+3`; no step-out, no run-to-cursor | RD-037 moves stepping into the interface before the source engine is built on it |
| Execution hooks | Unknown | Pre-instruction PC hook in all three cores, hard-coded to breakpoint checks; stack-push annotation; VICE-only IRQ breakpoint checks; no RTS/RTI/reset hooks; VICE single-step watchdog | Source step controller runs inside the emulation-thread hook with a budget (#8.1) |
| Bank state exposure | Unknown | None reaches debugger code; VICE, Nestopia and Atari800 each have the state internally without accessors | RD-027 adds accessors in the embedded cores (allowed; they are not MTEngineSDL); RD-040 added for the NES provider, which had no backlog item |
| CPU view vs raw RAM | Not discussed | C64 and 1541 have CPU-view and direct-RAM adapters (`SwitchIsDataDirectlyFromRam`, marked "TODO: proper banking"); Atari "RAM" is the CPU view; NES has CPU view only | #3B.1's "CPU read view / write destination / physical backing" has a C64 head start and no Atari/NES start |
| Tests / CI | Nightly + PR suites to be designed | In-process `CTest` suite (`tests/run_test.sh`), run in CI on macOS only; no external toolchains; `dankwars.prg/.dbg` and `labelstest1.vs` fixtures unused | RD-034 (Linux suite), RD-035 (toolchains), RD-038 (regression anchor) |
| Process launching | argv arrays required | The app uses only `SYS_ExecSystemCommand(shellString)`; the engine's argv `CProcessSpawner` lacked cwd/capture/timeout | Open decision 11, RD-042 (`CProcessSpawner::Run`) |
| Section markers | the section-sign character | Repo convention forbids it (CLAUDE.md) | All cross-references use `#N` |

### 20.2 Source view, symbols and loaders (facts)

- **View:** `CViewSourceCode` (`Views/CViewSourceCode.h:23`, 447-line `.cpp`), constructed with `(name, ..., CDebugInterface*, CDebugSymbols*, CViewDisassembly*)`. Instances: C64 "C64 Source code" (`Screens/CViewC64.cpp:1148`), Atari "Atari Assembler source" (`:1581`), NES "NES Assembler source" (`:1698`), all hidden by default and added through `AddEmulatorMenuItemView`. Workspace slot `DefaultWorkspaceSlot_SourceCode = 13`, Ctrl+Shift+F3 (`Views/CDefaultWorkspaceLayouts.cpp:405`); C64 and Atari have layouts and view ids (`c64.source_code`, `atari.source_code`), NES has neither.
- **Rendering:** custom `Render()` (`CViewSourceCode.cpp:113-316`) blitting with `CSlrFont::BlitTextColor`, font and size borrowed from `viewDisassembly` each frame; `RenderImGui` only wraps pre/post. Each frame copies `cursorAddress = viewDisassembly->cursorAddress` and looks up `currentSegment->codeSourceLineByMemoryAddress[cursorAddress]`; the file containing that line is drawn centred, white only, with line numbers (`showLineNumbers`) and the path (`showFilePath`) and a green highlight on the centre line. Column information is never used. `DoScrollWheel`/`ScrollUp`/`ScrollDown` only set `changedByUser`; `KeyDown` returns false; `DoTap` handles only the segment-name area (`SelectNextSegment`); `Serialize`/`Deserialize` are empty. No breakpoint markers, no file switching, no click-to-address.
- **Symbol model:** `CDebugSymbols` (`DebugInterface/Symbols/CDebugSymbols.h:26-118`) owns `dataAdapter`, `asmSource`, `segments`, `currentSegment`, `memory`. `CDebugSymbolsSegment` owns `codeSourceLineByMemoryAddress` (flat array sized `AdapterGetDataLength()`), `blocks`, `breakpointsByType` with `breakpointsPC`/`breakpointsData` (C64 adds raster, VIC/CIA/NMI IRQ flags; 1541 adds VIA/IEC), `codeLabelByAddress` (`CDebugSymbolsCodeLabel` with a `u16 address`, no source position), `watches`. `CDebugAsmSourceLine` carries `codeLineNumberStart/End`, `codeColumnNumberStart/End`, `memoryAddressStart/End` (`CDebugAsmSource.h:42-55`). No reverse file:line → address index, no bank key anywhere; `CDebugMemoryCell.h:8` says so explicitly.
- **One `CDebugSymbols` per CPU/address space:** VICE main CPU `symbolsC64`, `symbolsDrive1541`, `symbolsDrive1541DiskContents`, `symbolsCartridgeC64` (`Emulators/vice/ViceInterface/CDebugInterfaceVice.cpp:176-216`); Atari one generic (`CDebugInterfaceAtari.cpp:89`); NES `symbols` + `symbolsPpuNmt` (`CDebugInterfaceNes.cpp:78-89`); C64U its own.
- **Loaders:** VICE `.vs` + 64tass labels in `CDebugSymbols::ParseSymbols` (`CDebugSymbols.cpp:117-339`, `D:` drive labels ignored with a TODO); text breakpoints (`:378-631`) and watches (`:687-931`); KickAss `.dbg` in `ParseSourceDebugInfo` (`:936-1005`) → `CDebugAsmSource::ParseXML` (`CDebugAsmSource.cpp:67-694`, pugixml; `<Sources>`, `<Segment>`/`<Block>` with `START,END,FILE_IDX,LINE1,COL1,LINE2,COL2`, `<Labels>`, `<Watchpoints>`, `<Breakpoints>`; source files read from disk by absolute path, tabs to 4 spaces, UTF-16 lines, 1-based). `ParseOldFormat` is dead code. HJSON labels/watches/breakpoints persist per segment (`:1008-1436`), source map is not serialized ("TODO" at `:1105`). Nothing parses cc65, Oscar64, KickC or Atari `.lbl`.
- **Entry points:** menu import by extension (`Views/CMainMenuBar.cpp:5694-5718`, `.dbg` re-import does not call `DeleteAllSymbols`); autoload next to PRG/XEX of `.labels`, `.vs`, `.watch`, `.dbg` (`Screens/CMainMenuHelper.cpp:1655-1747`, C64 and Atari only, gated by `c64SettingsLoad*`); command line `-breakpoints`, `-symbols`/`-vicesymbols`/`-vs`, `-watch`, `-debuginfo`, `-segment` applied to C64 else Atari, never NES (`Tools/C64CommandLine.cpp:651-694`, `:926-1052`, "GENERALIZE ME"); `-pass` shared-memory path (`:1412-1450`). Remote `segment/read|write` and MCP `retro_segment_read|write` exist; there is no remote "load symbols" or "source location".
- **Known defects to fix on the way (RD-032/033):** segments are never deleted on purpose ("THE BUG" comments, `CDebugSymbols.cpp:63-67, 109-111`); the segment destructor would double-delete lines also owned by `CDebugAsmSourceFile` (`CDebugSymbolsSegment.cpp:50-55`); `.dbg` re-import deletes `asmSource` and leaves old segments with dangling pointers (`CDebugSymbols.cpp:992-996`); `SetSegment` bypasses `ActivateSegment` (`:1567`); newer KickAss `<Watchpoints>` `ADDRESS1,ADDRESS2,ARGUMENT` rows read ADDRESS2 as the value count; `"nes"` is matched twice in the CLI so the NES path branch is unreachable, and the drag-in `-ines` has no handler.

### 20.3 Execution control (facts)

- **Modes:** `DEBUGGER_MODE_RUNNING 0x00`, `PAUSED 0x80`, `RUN_ONE_CYCLE 0x81`, `RUN_ONE_INSTRUCTION 0x82`, `SHUTDOWN 0xFF` (`DebugInterface/DebuggerDefs.h:15-19`). `SetDebugMode` writes an atomic and notifies `pauseCV` (`CDebugInterface.cpp:499-517`); emulator overrides mirror it into `c64d_debug_mode` / `atrd_debug_mode` / `nesd_debug_mode`. `PauseEmulationBlockedWait` polls up to 2 s. `StepOverInstruction` is a single instruction step; `StepOneCycle` is a no-op on Atari and NES; `StepOverSubroutine` delegates to `viewDisassembly->StepOverJsr()` (`Views/CViewDisassembly.cpp:3224-3261`): opcode `$20` → `SetTemporaryBreakpointPC(pc+3)` + RUNNING, else one instruction. `RunEmulationForOneFrame` exists (tests only). No run-to-cursor (`MakeJMPToCursor` sets PC), no step-out. The VICE monitor has opcode-counting step/next/return (`monitor/monitor.c:2564-2618`, `:2893-2954`) reachable only from the monitor console; usable as a reference, not as the engine.
- **Hooks, VICE (`Emulators/vice/c64/c64cpusc.c`):** after alarms and `DO_INTERRUPT`, IRQ/NMI/VIC/CIA breakpoint checks (`:3283-3313`, after the vector was taken, `reg_pc` is the handler); pre-execute `c64d_c64_check_pc_breakpoint(reg_pc)` + `c64d_debug_pause_check(1)` unless single-stepping (`:3352-3376`); `BRK` auto-pauses (`:3457-3465`); post-execute `c64d_mark_c64_cell_execute` (`:4417`), RUN_ONE_INSTRUCTION → PAUSED (`:4428-4431`), `c64d_debug_pause_check(0)` (`:4447`); per-cycle `c64d_c64_do_cycle` (`:4551`) drains tasks and allows mid-instruction pauses; `c64d_set_debug_mode` watchdog (`:4473`, >300 re-arms <5 ms apart force PAUSED). JSR/RTS only annotate the stack and call Champ-profiler stubs (`USE_CHAMP_PROFILER` undefined); RTI has no hook; no reset hook. The 1541 CPU shares `c64d_debug_mode` and can consume a single step ("drive1541 stealing RUN_ONE_INSTRUCTION", `drive/drivecpu.c:3697-3711`).
- **Hooks, Atari800 (`Emulators/atari800/cpu.c:762-808`):** top-of-instruction `atrd_mark_atari_cell_execute`, task drain, `atrd_check_pc_breakpoint` + `atrd_debug_pause_check`, then RUN_ONE_INSTRUCTION → PAUSED; pause loop polls with `Sleep(10)` (`AtariInterface/AtariWrapper.cpp:194-240`). JSR/NMI/IRQ annotate only. `MONITOR_ret_nesting` is compiled out.
- **Hooks, Nestopia (`Emulators/nestopiaue/core/NstCpu.cpp:2083-2136`):** `nesd_update_cpu_pc_by_emulator`, `nesd_mark_cell_execute`, `nesd_check_pc_breakpoint`, `nesd_debug_pause_check(1)`, opcode, `nesd_debug_pause_check(0)`, RUN_ONE_INSTRUCTION → PAUSED. `DoISR` annotates only. `nesdMainCpuCycle` counts instructions, not cycles.
- **Breakpoints:** `CDebugBreakpointAddr{int addr; actions; data}`, `CDebugBreakpointData` adds range/access/comparison, `CDebugBreakpointRasterLine`; containers `CDebugBreakpointsAddr` (`std::map<int,...>` + render copy) and `CDebugBreakpointsData` (linear sweep); evaluated only on `symbols->currentSegment` (`ViceWrapper.cpp:853-868`, `AtariWrapper.cpp:112-153`, `NesWrapper.cpp:1676-1689`); hit = `SetDebugMode(PAUSED)` from the emulation thread + `CDebugEventsHistory::CreateEventBreakpoint` + WebSocket "breakpoint" broadcast; views are never notified, they poll (`CViewC64::Render` ~`:2370-2445` calls `SetCurrentPC`). The temporary breakpoint is one `int temporaryBreakpointPC` per `CDebugBreakpointsAddr`, checked before user breakpoints in VICE, no event, no broadcast; cleared by step/continue/frame-step/reset. All checks are suppressed during snapshot restore (`IsPerformingSnapshotRestore`), every forward step calls `snapshotsManager->CancelRestore()`.
- **Threads and tasks:** one emulation thread per emulator (`CEmulationThreadC64/Atari/Nes::ThreadRun`); `cpuDebugInterruptTasks` drained in the pause check / snapshot-restore check; task classes are joystick/keyboard/reset/cartridge/resource only, there is no stepping task. Mutexes: `breakpointsMutex` (`LockMutex`), `renderScreenMutex`, `ioMutex`, `tasksMutex`, `pauseMutex`+`pauseCV` (VICE only), `frameStepMutex`+`frameStepCV`.
- **Call tracking:** none in the debugger layer; `CStackAnnotationData` (`DebugInterface/CStackAnnotation.h`: `entryTypes[256]`, `irqSources[256]`, `originPC[256]`) is written on pushes by all three cores and shown by `CViewStack`; pops do not clear; cleared on snapshot restore.
- **Timeline:** `CSnapshotsManager` backstep = restore an earlier snapshot and run forward to a target cycle; C64-only frame navigation documented in `docs/mcp/timeline-navigation.md`. A source step must call `CancelRestore` like the instruction step and stay inert during replay.

### 20.4 Memory mapping per machine (facts)

- **Adapters:** base `CDebugDataAdapter` (`DebugInterface/CDebugDataAdapter.h:19`). VICE: `CDataAdapterViceC64` (CPU view via `c64d_peek_c64`, bus write via `c64d_mem_write_c64`; used by disassembly, memory map and hex dump), `CDataAdapterViceC64DirectRam` (`mem_ram[]`; used by monitor and stack views and by `SwitchIsDataDirectlyFromRam`, `Screens/CViewC64.cpp:2830-2856`, "TODO: proper banking"), 1541 CPU/direct/minimal adapters, `CDataAdapterViceC64Cartridge` (hard-coded 512 KB, ROML only through `c64d_peek_cart_roml`, type enum unused, cells never marked), `CDataAdapterViceC64Reu` stub. Atari: single `CDataAdapterAtari` over `MEMORY_SafeGetByte`/`MEMORY_PutByte`. NES: `CDataAdapterNesRam` over `nesd_peek_safe_io` and `CDataAdapterNesPpuNmt`; PPU/CHR/OAM/palette adapters are uninstantiated stubs.
- **C64:** `c64d_peek_c64` (`Emulators/vice/c64/c64memsc.c:1208-1398`) is a hand-written decoder from `pport.dir/data/data_read` and `export.exrom/game` reading `mem_ram`, the ROM arrays, `c64d_peek_io` and `c64io_de00/df00_peek`; cartridge windows go through `generic_peek_mem` (`cart/c64-generic.c:370`) at the current `roml_bank`/`romh_bank`, bypassing per-cartridge `cartridge_peek_mem_slotmain`; Ultimax open areas can leave the result uninitialized. VICE's `mem_get_current_bank_config()` (`:228`) and the cart bank variables (`cart/c64cartsystem.h:144-148`) have no debugger accessor. Exposed today: `c64d_get_exrom_game`, `c64d_get_ultimax_phi`, `c64d_peek_memory0001`, `GetC64CartridgeState`, and the remote CPU status fields `memory0001/exrom/game`. `CViewC64MemoryBank` holds a 32-entry table indexed by `(exrom<<4)|(game<<3)|(port&7)` with RAM/BASIC/KERNAL/CHAR/IO/CART_LO/CART_HI/OPEN (duplicated in `CVicEditorLayerMemoryAccess.cpp`); it shows no cart bank number and no write routing. RAM-under-ROM is not modelled; writes just follow VICE's write table.
- **NES:** `nesd_peek_safe_io` (`NestopiaInterface/NesWrapper.cpp:1049`) peeks the mapper's current PRG window for `$0001-$1FFF` and `$5001-$FFFF`, and reads `ram[addr]` (a 2 KB array) for `$0000`, `$2000-$4FFF`, out of bounds. Writes are bus pokes (mapper registers). PRG bank accessors exist in the core (`Memory::GetBank<SIZE>(addr)`, `core/NstMemory.hpp:450-468`) behind protected `Board::prg` and private `Cartridge::board`; `Cartridge::GetProfile()` / `Api::Cartridge::Profile` (header, `prgRom`, `trainer`) exist but are unused by the debugger. 199 board implementations, 236 distinct mapper numbers. No PRG bank display anywhere.
- **Atari:** `MEMORY_mem` is the CPU-visible image; OS/BASIC/cart/XE banks are `memcpy`'d in by `MEMORY_HandlePORTB` (`Emulators/atari800/memory.c:729-908`), RAM under the OS lives in static `under_atarixl_os[]`, `under_cart809F`/`under_cartA0BF`, banked RAM in static `atarixe_memory`; `MEMORY_PutByte` drops writes to ROM. `MEMORY_xe_bank` (`memory.c:82`) is unused by the debugger; `PIA_PORTB` is shown as a register only. `CARTRIDGE_main{type,state,size,image}` with `CARTRIDGE_LAST_SUPPORTED = 70` is never referenced by debugger code. Machine presets in `atariMachinesDefs` (400/16K, 800/48K, 1200XL, 600XL, 800XL, 130XE, XEGS, 5200); default 800XL with RAM option 9 (`MEMORY_ram_size = 1088`).
- **Keying:** one `CDebugMemoryCell` per CPU address per adapter (`Symbols/CDebugMemory.cpp:16`), marked by `c64d_/atrd_/nesd_mark_*cell*` on the CPU symbols only; breakpoints, labels, watches and source lines are `std::map<int,...>` / flat arrays per segment. Nothing carries PRG offset, ROM file offset or bank; the only bank arithmetic in the app is the CrtMaker Magic Desk builder.

### 20.5 Tests, CI and tooling (facts)

- `src/Tests/`: ~70 `CTest` subclasses (`GetName`, `Run(ITestCallback*)`, `Cancel`, `StepCompleted`, `TestCompleted`, `ResolveProjectPath`), registered in `CTestSuiteRegisterRetroDebuggerTests()` (`src/Tests/CTestSuiteRetroDebugger.cpp:95-221`); ImGui tests in `CImGuiTests.cpp` under `MT_ENABLE_IMGUI_TEST_ENGINE`. Patterns to copy: `CTestViceInstructionStepping` (poke + `MakeJmpC64` + RUN_ONE_INSTRUCTION polling), `CTestViceBreakpoints` (`currentSegment->AddBreakpointPC`), `CTestStackAnnotation` (C64 + Atari + a synthesized NES ROM), `CTestNesInputReplay` (`InsertCartridge`). No test loads symbols, `.dbg` or source; no NES/Atari stepping or breakpoint test.
- `tests/run_test.sh`: builds unless `--skip-build`, `--imgui`, `--imgui-test`, `--timeout` (900 s default), results in `tests/results/last_run.txt` (`RESULT: P/T`); `run_suite_isolated.sh` one process per test; `test-skipped-tests.sh` keeps `tests/SKIPPED_TESTS.md` honest.
- CI: `build-linux.yml` builds and runs `test_mtcaps.py` only (xvfb installed, suite not run); `build-macos.yml` builds and runs `tests/run_test.sh --skip-build`; `build-windows.yml` builds only; `disk-contents.yml` runs a standalone test. No toolchain is downloaded anywhere.
- External tools: 64tass vendored in `src/Tools/64tass` (also `src/Tools/asm6`), used by `CDebuggerApi::Assemble64Tass*` and the `retro_assemble` tool; KickAssembler via `SYS_ExecSystemCommand("java -jar ...")` in `Plugins/CrtMaker/CViewC64CrtMaker.cpp:395-430` with `javaPath`/`kickAssJarPath`/`kickAssParams` and an mtime-based skip; ca65 only in `tools/Exomizer-Decrunch/build-decrunch.sh`. No file watcher of any kind.
- Remote/MCP: stepping endpoints `step/cycle|instruction|subroutine`, `continue`, `pause`, `cpu/jump`; breakpoint PC/memory add/remove/list; `segment/read|write`; `retro_disassemble` with `includeLabels`. Missing: step-out, source stepping, symbol loading, source location queries.
- Build hygiene: `tools/add_source_file.py` registers a new file in CMake, Xcode and the vcxproj/filters at once; every new file must go through it or the three lists drift (CLAUDE.md).

### 20.6 Decisions taken in 1.4

1. The canonical database (#6) is a migration of `CDebugAsmSource`/`CDebugSymbolsSegment`, keeping KickAssembler segments as physical-identity candidates and the one-active-segment scope until RD-025; the `.dbg` path gets a regression test first (RD-038).
2. Loader dispatch moves to a content-sniffing registry (RD-031) because cc65 and KickAssembler share the `.dbg` extension.
3. Step-over/step-out move from `CViewDisassembly` into `CDebugInterface` with a real temporary-breakpoint object and documented priorities (RD-037) before the source-step engine (RD-013) is built on them.
4. The source-step controller is evaluated inside the per-instruction emulation-thread hook (RD-011 adds the generic callback at the existing hook points), with instruction budget and cancellation; it never drives `RUN_ONE_INSTRUCTION` from the UI.
5. The NES mapping provider gets its own backlog item (RD-040); the emulator-core accessor patches for all three machines are part of RD-027/028/029/040 and are made in the embedded cores under `src/Emulators/`, never in MTEngineSDL and never in the `[C64D-REFERENCE-ONLY]` reference copies.
6. The headless suite must run on Linux CI (RD-034) before the three-machine matrix is used as an acceptance gate; toolchain provisioning is nightly-only (RD-035).
7. C64 Ultimate and the 1541 drive CPU are must-have, delivered as RD-044 (P4) and RD-045 (P5); P0-P3 follow the compatibility guards in #20.8. Atari fixtures must set machine type and RAM size explicitly (open decision 13).
9. RD-034 (Linux CI suite), RD-031 (content-sniffing loaders) and the RD-042 engine proposal are must-fix items scheduled in M0/M1, not deferred.
10. The embedded 64tass emits symbols and a source map on every assembly (RD-043, P4); every assembly the debugger performs feeds the canonical database.
8. Documentation in this folder: this spec, `REVIEW-v1.3.md` (the review that produced 1.4), `TRACKER.md` (live backlog status), `IMPLEMENTATION-PLAN.md` (milestones and sequencing).

### 20.7 The existing address→source map versus the bank-aware requirements

The map that exists (`codeSourceLineByMemoryAddress`, filled from KickAssembler `.dbg`) is compared below with what #3A, #3B and #6 require. "Partial" means the data model can be migrated without loss; "No" means new data is required.

| Requirement | Existing map | Verdict |
|---|---|---|
| Address → file:line | Yes, per address | OK |
| Column ranges (#6.1, same-line statements) | Yes, `LINE1,COL1,LINE2,COL2` parsed into `CDebugAsmSourceLine` but never displayed | OK, data present |
| Half-open ranges, multiple non-contiguous ranges per line | Inclusive `START,END` expanded to one pointer per address; several rows may point at the same line | Partial: migrate to ranges, keep many-to-many |
| Reverse index file:line → addresses (#4.4, #8.8) | No | No: built in RD-006 |
| Physical code identity per range (#3A.2) | No. A KickAssembler `<Segment>` is a *name*; the `.dbg` carries no bank/physical identity, and the parser ignores the segment `dest` attribute | No: RD-007 adds a project-level "segment name → physical identity" table; without it segments are `Ambiguous`, never `Exact` |
| CPU address vs mapping epoch vs ROM file offset (#3A.2) | CPU address only; no epoch, no offset | No: RD-024/007 |
| Fetch/read/write distinction (#3A.2, #3B.1) | Not applicable to the map; the CPU-view adapter follows VICE's tables for reads and writes | No: providers RD-028/040/029 |
| Automatic selection of the right map for the current mapping (#3A.4) | Manual segment switch only; emulator hooks evaluate `currentSegment` | No: RD-025 mapping-driven activation |
| Breakpoint intents CPU-only / physical / source (#3A.4) | Per-segment PC breakpoints only (= "CPU address, within the chosen segment") | Partial: RD-025/014 |
| Mapping quality / capability flags (#6.4) | None, everything is treated as exact | No: RD-006 |
| Binary identity / stale-map detection (#6.1) | None | No: RD-007 |
| Banked NES / Atari artifacts | Nothing; `.dbg` loading never reaches NES (CLI/autoload) | No: RD-033, RD-026, RD-030 |

Conclusion: the existing map is a correct *line-level, single-identity* seed and is not in conflict with the spec; the bank-aware requirements are additions on top of it. Nothing in the spec needs to change because of it, and the migration rule in #6.1 stands.

### 20.8 Compatibility review: 1541 drive CPU and C64 Ultimate (must-have, later phases)

**1541 drive CPU (RD-044, P4).** What exists: its own `CDebugSymbols` (`symbolsDrive1541`, `CDebugSymbolsSegmentDrive1541` with VIA1/VIA2/IEC IRQ breakpoints), its own data adapters, a pre-instruction hook with PC breakpoint check (`drive/drivecpu.c:2592-2598`), post-instruction check (`:3697-3711`), JSR stack annotation into `CDebugInterfaceC64::driveStack`, labels with the `D:` device prefix (ignored today). The address space is flat (RAM `$0000-$07FF`, VIA1/VIA2, ROM `$C000-$FFFF`), so its mapping provider is trivial. The one real obstacle is that the drive shares `c64d_debug_mode` with the C64 CPU and "steals" `RUN_ONE_INSTRUCTION`. The source-step engine of #8 does not use `RUN_ONE_INSTRUCTION` (it runs with `RUNNING` and decides in its own per-CPU observer), so the sharing problem disappears for source stepping as long as the arming state is per CPU. Pausing stays machine-wide (both CPUs stop), which is the expected behaviour.

**C64 Ultimate (RD-045, P5).** `CDebugInterfaceC64U` derives from `CDebugInterfaceC64` and publishes `CC64BackendCapabilities` (`src/DebugInterface/C64/CC64BackendCapabilities.h`, levels `NONE/DIRECT/DERIVED/OBSERVED/HELPER`): mapped memory read/write and screen stream `DIRECT`, while `currentPcTracking`, `breakpointsPC`, `stepInstruction` and `stepCycle` are `NONE`. It has a trace mode (`C64UTraceBuffer`, `DrainTraceBusData`) that streams bus activity. Therefore on C64U: symbols, source view, navigation and the mapping provider (from polled `$01`/EXROM/GAME) work as on VICE; live stepping and source breakpoints are unavailable unless the firmware adds them; the trace buffer can be replayed into the execution observer *after the fact* to show the last executed source lines and a source-level trace. The existing capability-level pattern is exactly the negotiation mechanism #6.4 asks for, generalized to execution control.

**Guards that P0-P3 must respect so RD-044/045 are additions, not redesigns:**

1. Every new object is owned per `CDebugSymbols` (= per CPU / address space), never per `CDebugInterface` or as a global: the canonical database (RD-006), the mapping resolver (RD-024), the execution observer list and events (RD-011, every event carries the originating `CDebugSymbols*`/CPU id), the execution tracker (RD-012), the source-step controller and its arming state (RD-013), source breakpoints (RD-014). `CDebugInterface` only aggregates them.
2. Execution-control capabilities are part of capability negotiation: extend the `EC64CapabilityLevel` pattern into a machine-independent `CDebugBackendCapabilities` (`instructionHook`, `stepInstruction`, `stepOut`, `sourceStep`, `fetchMapping`, `physicalIdentity`, `executionTrace`), published by every `CDebugInterface`; UI and remote endpoints grey out or reject what is `NONE` instead of assuming VICE.
3. The execution observer accepts events from two sources with the same interface: live hooks (VICE/Atari800/Nestopia) and replayed traces (C64U trace buffer; later any recorded trace). Observers must not assume they can stop the machine synchronously; the stop request is a return value the live source honours and the replay source ignores.
4. The loader registry and the `.dbg`/`.vs` loaders route symbols by device: `D:` labels go to `symbolsDrive1541`; a project-level table assigns `.dbg` segments to a CPU (default: main CPU). KickAssembler itself has no device notion, so this stays a project setting.
5. The source view, document registry and decorations take a `CDebugSymbols*`, so a "1541 Source code" instance is a constructor call, not a new class; the NES/Atari instances already prove the pattern.
6. Tests for the stepping engine are written against an abstract fake CPU (events in, stop decisions out) in addition to the VICE integration tests, so the engine is proven independent of which core feeds it.

With these guards, RD-044 is wiring plus a trivial provider plus `D:` label routing, and RD-045 is a trace-replay event source plus capability gating; neither touches the core contracts.

### 20.9 Embedded 64tass as a symbol and source-map producer (RD-043)

The vendored assembler (`src/Tools/64tass/`, version 1.46 by Soci/Singular, 2012) is driven through `assemble_64tass(userData, text, size, &start, &size)` (`64tass.c:3076`) and writes bytes back through `c64debugger_set_assemble_result_to_memory`. It already contains `labelprint()` (`:2978`, the `-l` label file) and a second listing pass with source lines (`arguments.list`/`arguments.source`, `:2913-2970`), both writing to `FILE*`. RD-043 adds two more callbacks next to the existing one, `c64debugger_assemble_label(userData, name, address, size)` and `c64debugger_assemble_source_range(userData, startAddr, endAddr, line, column?)`, fed from the label table and the listing pass without touching files, and makes `CDebuggerApi::Assemble64Tass*` return a small `CDebugDatabase` fragment (labels + line ranges for the assembled text, identity = the CPU address space it was written to) that is merged into the active symbols. Consumers: the MCP `retro_assemble` tool, plugins using the `A(...)` macro, future build tasks. The in-memory source text is registered as a synthetic document so the source view can show it. Decision 14 covers whether to upgrade the vendored 64tass before or after this.
