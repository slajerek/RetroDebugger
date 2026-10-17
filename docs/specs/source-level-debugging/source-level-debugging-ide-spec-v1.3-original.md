# RetroDebugger — Source-Level Debugging & IDE Evolution

**Technical specification / implementation plan**  
**Status:** Proposed · **Version:** 1.3 · **Date:** 2026-10-17  
**Parallel implementation targets from P0:** C64 (6510/PLA), NES (cartridge mappers), Atari 8-bit (400/800/XL/XE/XEGS) · **Shared core:** generic bank-aware 6502-family debugging · **Compiler targets:** Oscar64, cc65, KickC · **Assembler targets:** existing TASM integration, KickAssembler and other external tools

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

- Preserve current disassembly stepping and existing assembler workflows.
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
| **P0 — Discovery & fixtures** | Inventory current Source Code view, CPU stepping API, symbol loaders, TASM integration, and NES mapper/memory translation hooks; compile sample fixtures | Current interfaces documented; golden binaries and metadata retained |
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
  │                    └─ TASM / custom commands
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

`resolveInstructionFetch` must use the **actual fetch mapping**; read, write, and instruction-fetch routing must remain distinct. On C64, a write under BASIC/KERNAL ROM normally targets hidden RAM even when reads/fetches see ROM; on Atari XL/XE, ROM overlays generally block write-through. NES mapper behavior also depends on access type. `resolveCpuWindows` may return zero, one, or several currently mapped aliases. Expose an explicit `Unknown`/`Unmapped` result; never silently fall back to an arbitrary bank. The exact API must be adapted to existing emulator interfaces after RD-001.

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
| `100` (`$34`) | RAM | I/O | RAM |
| `011` (`$33`) | BASIC ROM | character ROM | KERNAL ROM |
| `010` (`$32`) | RAM | character ROM | KERNAL ROM |
| `001` (`$31`) | RAM | character ROM | RAM |
| `000` (`$30`) | RAM | RAM | RAM |

The table shows typical `$3x` values with the usual DDR setup; low-bit combinations, not literal register values alone, define the mapping. BASIC visibility requires both LORAM and HIRAM. `CHAREN=0` selects character ROM only when LORAM or HIRAM is active; otherwise `$D000–DFFF` can expose RAM. **Read and write routing are different:** in normal C64 ROM-overlay modes, writes to `$A000–BFFF` or `$E000–FFFF` go to underlying RAM while CPU reads/fetches see ROM. When I/O is visible at `$D000–DFFF`, writes target I/O registers, not underlying RAM. Cartridge configurations and Ultimax mode alter these rules; do not generalize ordinary ROM write-through to every cartridge/Ultimax window.

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

Retain the existing `Source Code` window/menu placement and debugger context. Replace its internal single-file display implementation with a reusable `SourceDocumentView` (or wrap the existing component if suitable). Before changes, inventory its rendering technology, file-loading path, line-number support, scrolling, selection, fonts and synchronization with emulator execution.

### 4.2 MVP read-only behavior

- Open source files referenced by debug metadata; support manually opening a file.
- UTF-8 loading, BOM handling, CRLF/LF, robust long lines, horizontal/vertical scrolling and line numbers.
- Highlight **current execution line** distinctly from **selected line** and **breakpoint line**; support multiple indicators on one line.
- Source line gutter with breakpoint markers, current PC marker and (later) coverage/diagnostic markers.
- Click line to select; double-click or command to navigate to corresponding disassembly addresses; if multiple mappings exist, show choices rather than choosing arbitrarily.
- Follow PC option (on/off); when off, execution updates do not steal scroll position or tab focus.
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

Opcodes (`LDA`, `STA`, `JSR`, `RTS`, `RTI`, `JMP`, branches, etc.), registers, addressing syntax, hex (`$0400`, `0x0400` where accepted), binary, labels, directives, macros, comments, local labels and symbols. **Dialect profiles** for TASM, KickAssembler, ca65 and Oscar64-generated assembler; avoid treating incompatible directive syntaxes as interchangeable. Allow a base 6502 profile and future 65C02/65816 profiles.

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

Continue existing TASM integration; add ca65/KickAssembler/Oscar64 assembly dialects through language profiles. Allow a generic assembler map provider and disassembly-only fallback. Source-level stepping in pure ASM should default to instruction semantics unless explicitly configured for source-line stepping.

---

## 8. Execution tracking and stepping engine

### 8.1 Required CPU/emulator events

Observe `beforeInstruction`, `afterInstruction`, call/return (decoded or derived), IRQ/NMI/BRK entry, RTI, authoritative instruction-fetch physical mapping, mapper/memory mapping changes, breakpoint/watchpoint hits, reset, pause/resume and debug session termination. Prefer authoritative emulator events over decoding an opcode at PC when an instruction may be intercepted or emulated specially.

6502 basics: `JSR` opcode `$20`, `RTS` `$60`, `RTI` `$40`, `BRK` `$00`. `JSR` alone does **not** imply a source-language function call. Do not rely solely on SP comparisons: hardware stack, compiler software stack and interrupts are different concepts.

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

Finish the current source statement in the **starting logical frame**, ignoring mapped locations in deeper callees, then stop at the next executable statement in that frame. Do not implement this merely as `JSR depth++ / RTS depth--`: handle nested calls, IRQ/RTI, tail calls, indirect calls and control-flow exits. A temporary return-address breakpoint can be an optimization for a proven simple call, **not** the only correctness mechanism. If the current frame exits before reaching another statement, stop at the caller with a reason such as `FrameExited`.

### 8.6 Source Step Out

Continue until the starting logical frame is exited, then stop at the first safe instruction in its caller. For an inlined logical frame, step out of its inline range/context without requiring an `RTS`. If frame reconstruction is unknown, expose a degraded mode or explanatory stop reason instead of running indefinitely.

### 8.7 Unmapped code and optimized code

Default: skip unmapped callees that are reached during a source step **when their return context is tracked reliably**. Do not skip arbitrary unmapped control flow forever. Apply configurable instruction/time budget and safe stop (`Unmapped`/`BudgetExceeded`) when no reliable return is known. Optimized-away lines are not executable and must not be fabricated; mark approximate mappings and document source-level stepping limits.

### 8.8 Breakpoints

- Source breakpoints bind to file+line(+column/statement) and resolve to zero, one or many machine addresses.
- Disassembly breakpoints bind to machine address context.
- Breakpoints survive recompilation by re-resolving source intent; stale binary/metadata must trigger warnings.
- Conditional source breakpoints require a separate expression evaluator and are out of MVP scope.
- Define priorities for user breakpoints, watchpoints, temporary internal step breakpoints and pause requests.

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
    tasm.yaml
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

Add mandatory NES NROM and switchable-PRG-bank fixtures (see §3A.6), including the same CPU PC mapped to different physical PRG banks. Add tests for `a(); b();` on one line, multiple statements with different columns, nested calls, calls to runtime helpers, repeated calls from the same line, `if/else`, `for`, `while`, `switch`, `return`, recursion (where supported), indirect function calls (where supported), inlining, optimized-out statements, IRQ/NMI, and bank switches. For `printf`, use compiler-specific tests because library and startup behavior differ.

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
6. **Regression:** disassembly stepping, existing breakpoints, TASM build and performance must not regress.

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

- Fast PR suite: parser/stepping unit tests + checked-in golden binaries/maps (subject to licensing and size review).
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

- Toolchain registry for built-in TASM and external Oscar64, cc65, KickC/KickAssembler and future tools.
- Configurable build task: executable, arguments, cwd, env, outputs, diagnostic parser, pre/post steps.
- Build console with stdout/stderr and clickable compiler diagnostics.
- On successful build, atomically load matching executable + debug info; optional reset/run-to-entry policy.
- On failed build, keep previous debug session clearly marked as stale; do not silently associate old symbols with new source.
- Workspace-relative configurations with variable substitution; allow user-specific tool paths outside version control.
- Later: hotkeys for Build, Rebuild, Run, Debug, Stop, Step Into/Over/Out, and open declaration.

### 10.4 Security and robustness

Explicit consent before running arbitrary workspace build commands; do not automatically execute a newly opened project's scripts. Escape/process arguments correctly; prevent path traversal in artifact resolution; cap external tool runtime; redact sensitive environment variables in logs; handle untrusted malformed metadata safely.

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

Tests are repeatable; logs explain stop reasons; no regression in existing TASM/disassembly features; documentation includes setup, supported compiler versions, limitations and sample workflows; code reviews validate lifetime/threading safety between UI and emulator.

---

## 14. Concrete engineering backlog

| ID | Priority | Task | Depends on |
|---|---|---|---|
| RD-001 | P0 | Inventory existing source view, debugger hooks, breakpoint engine, TASM integration and C64 PLA/6510, NES mapper, and Atari PIA/cartridge translation APIs | — |
| RD-002 | P0 | Add portable C/ASM fixtures and pin compiler versions | — |
| RD-003 | P1 | Introduce document registry and read-only source view | RD-001 |
| RD-004 | P1 | Implement C lexer and 6502 assembler dialect profiles | RD-003 |
| RD-005 | P1 | Add PC/source decorations and bidirectional navigation | RD-003, RD-007 |
| RD-006 | P1 | Define canonical debug database and capability model | RD-001 |
| RD-007 | P1 | Implement physical-bank-aware address indexes, source remapping and binary/cartridge identity checks | RD-006, RD-024 |
| RD-008 | P1 | Implement Oscar64 `.dbj` adapter and fixture contract tests | RD-002, RD-006 |
| RD-009 | P1 | Implement cc65 `.dbg` adapter and fixture contract tests | RD-002, RD-006 |
| RD-010 | P1 | Audit KickC artifacts and implement supported adapter subset | RD-002, RD-006 |
| RD-011 | P1 | Add emulator execution/interrupt events and instruction-fetch mapping snapshot hooks | RD-001, RD-024 |
| RD-012 | P1 | Implement physical/logical execution context tracker | RD-011, RD-006 |
| RD-013 | P1 | Implement asynchronous source-step state machine and stop reasons | RD-007, RD-012 |
| RD-014 | P1 | Implement source breakpoints and re-resolution after rebuild | RD-007, RD-013 |
| RD-015 | P1 | Add headless step trace runner and golden expectations | RD-002, RD-013 |
| RD-016 | P2 | Patch Oscar64 range coalescing/column export with regression tests | RD-008 |
| RD-017 | P2 | Add structured inline provenance support and logical inline frames | RD-016, RD-012 |
| RD-018 | P2 | Add workspace explorer and multi-file tabs | RD-003 |
| RD-019 | P2 | Add toolchain registry, external build tasks and diagnostics | RD-018, RD-002 |
| RD-020 | P2 | Add build/run/debug orchestration with artifact consistency checks | RD-019, RD-007 |
| RD-021 | P3 | Enable editing, save, undo/redo, search and file watchers | RD-003, RD-018 |
| RD-022 | P3 | Extend debug model for variables/types, advanced optimized stepping | RD-017 |
| RD-024 | P0 / core | Define generic CPU/logical versus physical code identity and machine mapping provider contract, including mapper capability discovery | RD-001 |
| RD-025 | P1 / core | Implement bank-aware CPU/source/physical breakpoints and labels; preserve source identity across mapper changes | RD-007, RD-011, RD-024 |
| RD-026 | P1 / tests | Add NROM, UxROM and one MMC1/MMC3 bank-switching fixture with golden physical-fetch traces and source-step assertions | RD-002, RD-024, RD-025 |
| RD-027 | P0 / core | Audit C64 PLA/6510 and Atari 400/800/XL/XE/XEGS emulator memory hooks, including fetch/read/write and bus-master semantics; implement shared mapping event contracts | RD-001, RD-024 |
| RD-028 | P1 / core | Implement C64 memory mapping provider: 6510 DDR/port, PLA, ROM-under-RAM, I/O and cartridge/Ultimax identities | RD-024, RD-027 |
| RD-029 | P1 / core | Implement Atari 8-bit provider: machine variants, PIA PORTB, BASIC/OS ROM, 130XE extended RAM, ANTIC view and cartridge mappings | RD-024, RD-027 |
| RD-030 | P1 / tests | Add C64 and Atari banked fixture suites and shared three-machine headless stepping/breakpoint test matrix | RD-002, RD-025, RD-028, RD-029 |
| RD-023 | P6 / final research spike | Research C symbols, labels, variable-to-memory mapping and integration with existing Memory Viewer and Watch; produce a follow-up implementation proposal only | RD-008, RD-009, RD-010, RD-020 |

**Recommended critical path:** RD-001/002 → RD-024/027/006 → RD-007/011/025 + RD-028/029 + RD-008 → RD-012/013/015/026/030 → RD-003/004/005 integration → compiler expansion. Keep the UI and core debugger changes separately testable.

---

## 15. Open decisions / discovery questions

1. Which existing RetroDebugger UI text-rendering/editor component can be reused without introducing an incompatible dependency?
2. Which exact CPU execution callbacks and threading model exist for C64 and other emulators?
3. Does the current debugger have reliable breakpoint hooks before each instruction, including IRQ entry and bank mapping changes?
4. Which exact Oscar64, cc65 and KickC versions/flags are the initial supported baseline?
5. C64 may be the first integrated runtime, but C64/Atari/NES must share a bank-aware canonical debug-address API immediately; identify which mapper hooks are already exposed per emulator.
6. Should a source step into a function without source metadata fall back to disassembly or skip that call by default? Proposed default: skip tracked runtime calls; expose an option to enter disassembly.
7. Which compiler optimizations are officially supported for *precise* source stepping? Proposed initial baseline: debug/unoptimized builds; optimized mode best-effort with visible warning.
8. What existing TASM build/project configuration must remain backward-compatible?

9. Which NES mapper families and ROM/linker artifact formats can be exercised in headless integration tests initially?
10. How does the existing emulator expose physical PRG identity and fetch mapping, including mapper-specific aliases and PRG-RAM?

Resolve these during RD-001/RD-002; none requires postponing the canonical model or the Oscar64 adapter.

---

## 16. Source basis and verification status

This specification consolidates the architecture and stepping discussion in the conversation and the **user-supplied Oscar64 source snapshot** (`oscar64-source.zip`, archive revision `a92fd0cd4f9a12dd5340f57da768fc5a8dc1dd63`). Relevant Oscar64 anchors are listed in §7.1. The source audit supports the existence of source locations, IR/native linkage and provenance data, but **does not establish that every optimizer transformation preserves exact statement identity**.

cc65/KickC integration details remain **implementation hypotheses pending compilation of pinned fixture programs**; their precise output formats, flags and capabilities must be established by RD-002/RD-009/RD-010. The proposed APIs, JSON, YAML and class names are design proposals, not claims about existing RetroDebugger interfaces. No RetroDebugger source repository was provided for this specification, so its internal integration points must be confirmed in RD-001.


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

Bank-aware physical code identity, mapper-backed instruction-fetch resolution, and physical/source breakpoint semantics are mandatory early architecture and test requirements (§3A; RD-024–026), **not** deferred to P6. The final C variable/Watch/Memory Viewer research spike (RD-023) remains last and research-only. Existing C64 behavior must remain compatible.

---

## 19. Revision 1.3 — parallel C64/NES/Atari banking decision

**Decision:** No C64-first / NES-later architecture. Implement generic physical memory identity and fetch/read/write mapping contracts immediately; build C64, NES, and Atari adapters and test fixtures in parallel. C64 banking is governed by 6510 `$0000/$0001`, PLA and `/GAME`/`/EXROM`, with hidden RAM and access-specific routing. Atari banking is model-dependent, including XL/XE `PORTB`, 130XE extended RAM, ANTIC versus CPU views, and cartridge-specific `$D5xx` switching. NES continues to use mapper-authoritative PRG translation. RD-027–030 implement this decision; RD-023 remains the final research-only C-symbol/Watch/Memory Viewer spike.
