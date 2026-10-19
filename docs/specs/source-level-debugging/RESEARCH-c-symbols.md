# C Symbols and Watches

The committed fixtures show three compilers with three different levels of variable information. **Oscar64 1.32.273** (`.dbj`) names every global with an exact address range and a type id. It also names parameters and locals per function in three location forms: an absolute zero-page register, a static-stack address valid while the function's lexical scope is active, and an offset from the software stack pointer `SP`. It gives a minimal type table (`uint`, `ptr`, `array` with sizes; no names, no signedness seen, no function types). **cc65 2.19** (`ld65 --dbgfile`) names functions and `auto` parameters and locals with a frame offset, but has no C type information: every `csym` has `type=0`, which is `"00"`. It has no `csym` records for global data at all; globals appear only as assembler labels (`_result`) without a size. The frame offset of an `auto` depends on the PC, because cc65 moves `sp` inside a function and records no stack depth per PC. **KickC 0.8.6** (through KickAssembler `-debugdump`) gives no variable model: globals become `.label` values, locals live in CPU registers, and locals that are folded to constants are still emitted as labels. One such label is `v` at `$0001`, which is a constant, not an address. KickAssembler and 64tass give assembler labels only. As a result, only global data can be resolved to memory exactly by every producer. Locals can be resolved exactly only for Oscar64, and only when the stop is inside the function body. The existing Watch and Memory Viewer can show such a resolved global with almost no change. Locals need a new, context-aware resolution layer between the debug database and those views, not a new viewer.

Status: research, for maintainer review (RD-023)

Scope: #17 of `source-level-debugging-ide-spec.md`. No code was written. Every claim below is grounded in an artifact or source file in this repository, cited by path. Paths are relative to the repository root.

---

## 1. Evidence base

| Producer | Version and flags (from `manifest.json`) | Artifacts used | Machines |
|---|---|---|---|
| Oscar64 | 1.32.273, `oscar64 -tm=<m> -n -g -O0` | `main.dbj`, `main.asm`, `main.map`, `main.lbl` | c64, atari, nes |
| cc65 | 2.19, `cc65 -g`, `ca65 -g`, `ld65 --dbgfile main.dbg -Ln main.lbl -m main.map` | `main.dbg`, `main.s`, `main.lbl`, `main.map` | c64, atari, nes, c64-magicdesk, nes-mmc1, nes-uxrom, atari-xegs |
| KickC | 0.8.6, `kickc -t=<m> -Sl`, then `kickass main.asm -debugdump` | `main.dbg` (XML), `main.asm` | c64, atari, nes |
| KickAssembler | hand-written `kickass-c64.asm` | `main.dbg` | c64 |
| 64tass | hand-written `64tass-c64.asm` | `main.vs`, `main.lst` | c64 |

The artifacts are under `tests/data/source-debug/<producer>/<machine>/<fixture>/`, and the C sources are under `tests/source-debug/fixtures/<fixture>/main.c`. The fixtures contain one-byte `unsigned char` globals, parameters, block locals, recursion, a function-pointer table, and an interrupt-only function. They contain **no** signed, 16/32-bit, struct, static-local, shadowed or optimized (`-O1`+) case, so this research cannot establish those capabilities (see #9). `tests/data/source-debug/cc65/drive1541/drive1541/` is empty in this checkout.

## 2. Capability matrix

Key: **yes** means present and observed in the fixtures. **partial** means present but incomplete, with the limit stated. **no** means absent from the artifact. **n/o** means the format may support it, but no fixture exercises it.

| Capability | Oscar64 `.dbj` | cc65 `.dbg` (+`.lbl`) | KickC (`.dbg` + `.asm`) | KickAss / 64tass |
|---|---|---|---|---|
| Function names and code ranges | yes (`functions[].start/end`, `xname` signature) | yes (`csym sc=ext` -> `sym` -> `scope` spans) | partial (labels; `{}` scopes in `.asm`) | labels only |
| Global variable names | yes (`variables[]`) | **no C record**; asm label `_result` only | partial (`.label result = 2`) | labels only |
| Global address | yes, exact | yes, through the asm label `val` | yes, label value | yes, label value |
| Global byte size | yes (`end - start`, type `size`) | **no** (`_result` has no `size=`; only the segment size) | no | no |
| Type information | partial: `uint`/`ptr`/`array` with `size`, `eid`; empty names | **no** (`type=0` -> `val="00"` for every csym) | partial: an **asm comment** only, e.g. `// __register(A) char leaf(__register(A) char v)` | no |
| Parameters and locals | yes, per function `variables[]` | yes, `sc=auto` with `offs` | no records; a register in the comment | no |
| Local storage location | yes: zp register, static stack, or `SP`-relative (`base`) | `offs` relative to the frame; base `sp` zp address only from `.lbl` | register (A/X) or folded constant | n/a |
| Lexical scope | partial: `enter`/`leave` **source lines** for static-stack locals | function scope only (`scope=`), no blocks | no | no |
| Live range per PC | **no** | **no** | no | no |
| Optimized-out marker | no (only `-O0` built) | no | no; dead variables silently vanish | n/a |
| Bank of data | no | segment `ooffs` for ROM; BSS has none | no | no |

### 2.1 Oscar64 `.dbj`

The top-level keys are exactly `memory`, `variables`, `functions` and `types` (the union over all 21 fixtures). The union of local-variable keys is `name, start, end, typeid, enter, leave, base, references`. The local variables have three shapes:

```
add(u8,u8):     {"name":"left","start":13,"end":14,"typeid":6}            (c64 basic-flow; asm: "LDA P0 ; (left + 0)")
main() local i: {"name":"i","start":40959,"end":40960,"enter":20,"leave":32,"typeid":4}   (c64 branches-loops)
sum(u8) n:      {"name":"n","start":7,"end":8,"base":35,"typeid":5}       (c64 recursion; asm: "LDA (SP + 0),y" with SP=$23)
```

1. **Zero-page parameter register.** There is no `base` and no `enter`. `start` is an absolute zp address: `P0`=$0D on C64 and `P0`=$8B on Atari (`atari/basic-flow/main.asm`: `STA P0` at `$8B`). The value is valid at the function entry. After that the compiler may reuse the register. Oscar64 gives non-overlapping registers along the call chain: in `nested-calls`, `top.v` is 15, `middle.v` is 14 and `leaf.v` is 13. Nothing states that this holds in general.
2. **Static stack.** The variable has `enter`/`leave`. `start` is an absolute address in the `SSTACK` section, and `main.map` names the frame: `9fff - a000 : SSTACK, main@stack` and `9fff - a000 : i, BSS:main@stack`. `enter 20`/`leave 32` are the declaration line and the closing-brace line of `main` (`branches-loops/main.c`), so they are **source lines, not PCs**. The address is fixed, but its content is meaningful only while `main` is active (the function is not reentrant). The functions that use the static stack in these fixtures are not recursive.
3. **Frame-relative.** `base` is the zp address of the 16-bit software stack pointer: `SP`=$23 on C64 and `SP`=$A1 on Atari (`atari/recursion/main.asm`: `STA SP + 0` at `$a1`). The variable lives at `word(base) + start`. The offset is valid **after the prologue only**. `sum` does `SBC #$05` on `SP` at `089c..08a5`. The caller stores the argument at `(SP),2` (`LDY #$02 / STA (SP + 0),y / JSR $089c`), so before the prologue `n` is at offset 2, not 7. The prologue `089c..08ad` has no `lines` entry (`sum` lines start at `0x8ad`). The last line range `0x8c6..0x8d8` (line 10) contains the epilogue `ADC #$05`. Oscar64 uses this form for recursive functions (`sum`) and for functions called through pointers (`twice`, `plusOne` in `indirect-calls`).

`types` entries look like `{"typeid":3,"size":4,"type":"array","eid":10}` and `{"typeid":10,"size":2,"type":"ptr","eid":11}`. `name` is always `""`, so the typedef `operation` is lost. Function types appear as `{"size":0}` with no `type`. The return and parameter types survive only in `xname` (`add(u8,u8)->u8`, `main()->i16`). `volatile` is not represented.

The `memory` entries duplicate the globals and functions with a section kind (`NATIVE_CODE`, `DATA`, `BSS`, `START`, `END`). Linker markers are also listed as `variables` (`BSSStart`, `BSSEnd`, `ZeroStart`, `StackEnd`, `spentry`), and they share addresses with real data. In `c64/basic-flow`, `BSSStart` and `result` are both at 2225. In the `.lbl` file, the static-stack local appears as a **global, unqualified** label (`al 9fff .i`) next to `al 9fff .StackEnd`.

### 2.2 cc65 `.dbg`

```
csym	id=0,name="add",scope=1,type=0,sc=ext,sym=13
csym	id=1,name="left",scope=1,type=0,sc=auto,offs=1
csym	id=2,name="right",scope=1,type=0,sc=auto
csym	id=4,name="w",scope=2,type=0,sc=auto,offs=-1        (nested-calls, middle)
sym	id=14,name="_result",addrsize=absolute,scope=0,def=21,ref=47+43,val=0x9A7,seg=2,type=lab
sym	id=15,name="sp",addrsize=zeropage,scope=0,def=36,ref=39+35+15,type=imp
type	id=0,val="00"
```

- Every `csym` in all cc65 fixtures is either `sc=ext` (a function, with `sym=`) or `sc=auto`. No `sc=static`/`sc=reg` record occurs, and **no global variable has a csym**. In `main.s`, the compiler itself emits `.dbg func, "add", "00", extern, "_add"` and `.dbg sym, "left", "00", auto, 1`. It emits nothing for `_result`. The type is the literal `"00"` at the source.
- `offs` is omitted when it is 0. It is relative to the frame as set by the function's own pushes. In `middle()`, the prologue `jsr pusha` pushes `v` (offs 0). Line 13 pushes `w` with a second `jsr pusha`, after which `v` is at `(sp),1` and `w` at `(sp),0`. In other words, `offs` is relative to `sp` *after the parameters are pushed*: at line 14, `sp` = entry−1, so `w` = `sp`+0. Argument pushes for calls (`pushax`) move `sp` again in mid-statement. The `.dbg` records no stack depth per PC, so the address is exact only where the stack depth can be reconstructed.
- A fastcall parameter (the last one, `right` or `v`) is in A at the entry PC. It is in memory only after `jsr pusha`.
- `sp` is only an import in the main module (`type=imp`, no `val`). Its address comes from `main.lbl`: `al 000002 .sp` on c64, nes and magicdesk, and `al 000082 .sp` on atari.
- The `type id=1..` records (`val="800120"` and similar) are referenced by `line ... type=1`. They describe assembler spans, not C types.
- In banked fixtures, labels of different banks share CPU addresses: `al 009000 ._bankA` and `al 009000 ._bankB` (`c64-magicdesk/banked/main.lbl`). `BSS` has no `ooffs`, because RAM data has no image identity.

### 2.3 KickC

The KickAssembler XML has `<Sources>`, `<Segment>/<Block>`, `<Labels>`, `<Breakpoints>` and `<Watchpoints>`. `<Labels>` holds every `.label`, whether it is an address or a constant:

```
Basic,$0002,result,1,13,10,13,15
Code,$0001,v,1,39,12,39,12            (nested-calls; asm: ".label v = 1" ... "lda #top.v")
```

`top.v` was constant-propagated: `.label v = 1` is used as an immediate (`lda #top.v`), so the label `v` at `$0001` names the 6510 port register by accident. Locals are allocated to registers: `branches-loops` keeps `i` in X (`tax` / `cpx #3`). Register allocation is visible only in the comment lines that `-Sl` writes before each function: `// __register(A) char add(__register(X) char left, __register(A) char right)`. In `kickc/c64/interrupts/main.asm`, `ticks`, `tick()` and `interruptWork` are gone entirely, so there is no trace of them. Globals sit in zero page (`result = 2` on c64/nes, `$80` on atari). No `recursion` or `indirect-calls` KickC fixtures exist.

### 2.4 Assemblers

KickAssembler `<Labels>` (`Default,$083a,result,...`, `$083b,tmp`) and 64tass VICE labels (`al 837 .result`, `al 838 .tmp`) give names and addresses only, with no size or type. They are the label tier that C globals would merge with.

## 3. What the loaders keep today

From `src/DebugInterface/Symbols/CDebugInfoLoader.cpp`:

| Loader | Keeps | Drops |
|---|---|---|
| `DebugInfoLoadOscar64` | `functions[]` -> `CDebugFunction` (name, `xname` as `signature`, range, definition line); `lines[]` -> mappings; a label for each function start and each `variables[].start` | `origins`, `types`, `memory`, `variables[].end` (size), `typeid`, `references`, the whole per-function `variables[]` (locals and params) |
| `DebugInfoLoadCc65` | `csym sc=ext` -> functions through `scope`; `sym type=lab` -> labels, skipping generated `Lxxxx` (`IsCc65GeneratedLabel`) | all `sc=auto` csyms, `type` records; `sp` (an import without `val`); the `.lbl` file is not read |
| `DebugInfoLoadKickAss` (+`AddKickCSourceMappings`) | `<Labels>` -> `AddCodeLabel` for every row, constants included (`v` at `$0001`) | the KickC `__register` comments |

`CDebugDatabase.h` models files, regions, blocks, mappings and functions, plus image-offset (bank) resolution. `CDebugDatabaseCapabilities` has `lineMappings, columns, functions, statements, physicalIdentity`. It has **no variables, types, scopes or location expressions**. #6.1 lists these as optional P6.

Every loader collapses labels into one `CDebugSymbolsSegment` named "Default". `CDebugSymbolsSegment::AddCodeLabel` keeps **one label per address**: a second label at the same address deletes the first (`codeLabelByAddress` erase, `CDebugSymbolsSegment.cpp:126-147`). The index by text is a hash of the name, so a second label with the same name replaces the first in `codeLabelByTextHashCode`. As a result:
- Oscar64's `result` wins over `BSSStart` only because it comes later in `variables[]`.
- A banked `_bankA`/`_bankB` pair at `$9000` keeps one name.
- Two functions with a static-stack local named `i` would both become `.i` (in `.lbl`), and one would be lost.

## 4. Reusable APIs and their gaps

| Facility | Where | Reusable as is | Gap for C variables |
|---|---|---|---|
| Label store | `CDebugSymbolsSegment` (`AddCodeLabel`, `FindLabel`, `FindLabelByText`, `FindNearLabel`, `codeLabelsArray`), `CDebugSymbolsCodeLabel` | name to address for globals; the label column in the Watch view; label hints in the Data Dump (`CViewDataDump.cpp:1999`) | one name per address, no size, no kind (code / data / linker marker / constant), no bank, no scope |
| Watches | `CDebugSymbolsDataWatch` (`address, watchName, representation, numberOfValues`), `CDebugSymbolsSegment::watches` (`std::map<int, ...>`), `AddWatch`/`FindWatch`/`RemoveWatch`, HJSON (de)serialization | a fixed-address watch with a C name and an integer representation (`WatchRepresentation` in `DebuggerDefs.h`: hex/unsigned/signed 8/16/32 LE/BE, bin, text) | keyed by address (only one watch per address); no source-variable identity, so it cannot follow a rebuild; no pointer, array or struct types; no frame/PC context; `WATCH_REPRESENTATION_TEXT` is a TODO in `CViewDataWatch.cpp`; the label column shows `FindLabel(address)`, not the watch's own name |
| Watch view | `CViewDataWatch` (one view per machine: "C64 Data watch", "Atari Watches", "NES Watches", "1541 Data watch", "C64U Watch" in `CViewC64.cpp`) | the add-by-label popup (`ComboFilter` over `codeLabelsArray`, `FindLabelByText`); access-fade colors from `CDebugMemory` cells | no validity state, no row for "unavailable", no typed rendering |
| Memory viewer | `CViewDataDump::ScrollToAddress(int)` / `ScrollToAddress(int, bool)`, `CViewDataMap` (already calls `viewDataDump->ScrollToAddress`) | the "Go to Memory" target for an absolute CPU address | no range or size highlight; no way to say which mapping or bank the address was resolved for |
| Debugger API | `CDebuggerApi::AddWatch(...)` (three overloads), `GetDataAdapterMemoryWithIO/DirectRAM`, `GetCallStackJson()` | scripted watches and memory reads | no symbol lookup by name |
| Remote/MCP | `CDebuggerServerApi.cpp` endpoints `watch/list`, `watch/add`, `watch/remove`, `cpu/memory/readBlock`, `ram/readBlock`, `memory/search`, `cpu/callstack`, `source/location`, `source/resolve`; MCP tools `retro_watch_add/list/remove`, `retro_memory_read`, `retro_callstack`, `retro_source_location` (`CMCPServer.cpp`) | building blocks for a future `symbols/resolve` | `watch/add` always passes representation 0 (`AddWatch(address, nameBuf, 0, numValues)`); there is no label or variable query endpoint |
| Memory identity | `CDebugMemoryMappingResolver::ResolveRead/ResolveWrite`, `CDebugPhysicalAddress` (`CDebugMemoryMapping.h`) | resolving a data address into the current bank or RAM view | data variables carry no physical identity in any artifact |
| Execution context | `CDebugExecutionContextTracker`, `CDebugPhysicalFrame` (`fromPc, entryPc, spBefore, returnPc, seeded`), `DebugSeedFramesFromStack` | the active function per hardware frame (through `FindFunctionImage`); the callstack endpoint adds `caller` source | tracks the **6502 hardware stack** only; the C software stack (`sp`/`SP` in zp) is not tracked per frame |

Ownership: the variable and type model belongs to `CDebugDatabase` (loader-written, immutable after `Finalize`, #6). Watches and labels stay in `CDebugSymbolsSegment` (user state, serialized). A resolver sits between them and is the only place that combines the database, PC, frames and live memory.

## 5. Location classes

Every variable that a loader produces should be assigned exactly one location class. The class decides what the UI may claim about the variable.

| Class | Address rule | Valid when | Observed in |
|---|---|---|---|
| `Fixed` | absolute address and size | always (subject to the bank, #6) | Oscar64 globals; cc65 and KickC globals through labels (cc65: size unknown) |
| `FixedScoped` | absolute address | the owning function is on the stack **and** the PC is inside its `enter..leave` lines | Oscar64 static-stack locals (`main@stack`) |
| `ZpParam` | absolute zp address | at the function entry; later only if the compiler does not reuse the register (no proof) | Oscar64 `P0..` parameters |
| `FrameRelative` | `word(zp base) + offs` | PC in the function body, past the prologue and before the epilogue, with stack depth known | Oscar64 `base` locals; cc65 `auto` (base `sp`, depth unknown at mid-statement pushes) |
| `Register` | CPU register | PC-dependent and not described | KickC `__register` comments; cc65 fastcall parameter before `pusha` |
| `Constant` | none | never a memory location | KickC folded `.label v = 1` |
| `Absent` | none | never | KickC dead variables, for example `ticks` |

These rules are the minimum: a local's name alone never implies a stable address (#17.2 Q6). For a `FrameRelative` local, the frame comes from the **logical** frame. The innermost one comes from the current PC through `FindFunctionImage`. Outer ones come from `CDebugPhysicalFrame.returnPc` mapped back to functions. For outer frames, the software stack pointer value is not recorded. Oscar64 restores `SP` symmetrically (`ADC #$05` in the epilogue), so an outer frame's `SP` = inner `SP` + frame size. Neither artifact gives the frame size, so this would have to be decoded from the prologue (`SBC #$05`), which is a heuristic. Until a producer records the frame size, only the innermost frame's `FrameRelative` locals can be exact. Recursion (`sum`) shows why a static address cannot be used: every instance of `n` has a different address.

## 6. Proposed resolution model

The model adds to the canonical database (#6.1 `Variable`, `Type`, `LexicalScope`). It does not replace the label store.

- **`CDebugType`**: `id`, `kind` (`Unknown, UInt, SInt, Pointer, Array, Struct, Function`), `size`, `elementType`, `count`, `name`. Oscar64 fills `UInt/Pointer/Array`. cc65 and KickC produce only `Unknown` with a size when one is known. This must never be guessed from the name.
- **`CDebugVariable`**: `name`, `scope` (global, or function id with optional `enterLine..leaveLine`), `typeId`, `locationClass` (#5), `address` or `offset + baseZp`, `size`, `regionId`, `origin` (which artifact and record), `declLine`.
- **New capabilities** on `CDebugDatabaseCapabilities`: `variables`, `localVariables`, `types`, `scopes`, `liveRanges`, matching the `variable locations, types` in #6.4.
- **The symbol merge rule** (#17.2 Q1): a C variable record, the asm label of the same object (`_result`, `.result`) and the linker markers (`BSSStart`) are separate kinds. The C record is the one shown. A label is linked to it only when the address **and** the name (after the producer's mangling: cc65 adds a leading `_`) agree. Linker markers and `Constant` labels are never offered as variables. `.lbl`/`.vs` stay a fallback tier marked `Approximate`.
- **The resolver**: `ResolveVariable(name or id, context) -> {address, size, type, quality, reason}`, with context = (database snapshot, PC fetch identity, logical frame index, live zp reads). `quality` uses the same values as `DebugMappingQuality` (`Exact, Approximate, Ambiguous, Missing`). `reason` is a short text such as "prologue: frame not set up" or "two candidates: i in main, i in loop".
- **Build identity**: a resolution is tied to the database snapshot. A reload invalidates every context-dependent watch, and `DebugInfoCheckStale` marks the snapshot as stale (#17.2 Q7).

## 7. UX sketch

**Go to Memory.** This is available on a variable or function name in `CViewSourceCode`, on a watch row, and on a symbol hint. It resolves the name with the current context and calls `CViewDataDump::ScrollToAddress` on the machine's dump view. It highlights `size` bytes and shows a status line: `result  $08D9  1 byte  unsigned char  Exact`. If the resolution is not `Exact`, the status line says why and nothing is highlighted as fact. For `Register`/`Constant`/`Absent`, the command is disabled with the reason in a tooltip.

**Add to Watch.** This has two forms:
- **Address watch**: a `Fixed` variable becomes an ordinary `CDebugSymbolsDataWatch`. Its representation comes from the type (UInt size 1 -> `UNSIGNED_DEC_8`, Pointer size 2 -> `HEX_16_LITTLE_ENDIAN`). The source name is stored so the address can be resolved again after a rebuild.
- **Context watch**: other classes become a watch kept by name, which is resolved again on every stop. Its row shows `<not in scope>`, `<frame not set up>` or `<register, not readable>` instead of a value, and it never falls back to stale memory.

This needs one change: watches can no longer be keyed only by address.

**Typed display.** Integers are shown in the declared width and signedness. Unknown signedness is shown as unsigned with an `?` badge. A pointer is shown as `$xxxx`, with "Go to Memory" on the target and no automatic dereference. An array expands to elements up to `size`. A struct is shown only once some producer emits fields (none does today). Every value cell carries the quality badge. A value read under a different bank than the variable's region is flagged.

## 8. Relation to stepping and banking

- The source stepper stops at line starts (`CSourceStepController`). At those PCs Oscar64's `FrameRelative` offsets are valid: the prologue is unmapped and the epilogue sits at the end of the last line. The resolver can therefore offer `Exact` at statement-boundary stops, and `Approximate` (with a warning) at a PC stepped by instruction.
- For cc65, the stack depth must be reconstructed by decoding the function's `pusha`/`pushax`/`decsp`/`incsp` calls from entry to PC (calls to known runtime helpers). This is feasible but heuristic, and it should report `Approximate` unless the decoding has been verified against fixtures.
- Banked code (`ResolveImage`, `FindFunctionImage`) selects the function, and therefore the locals. Data in RAM has no bank in any artifact. Data in a banked RAM window (the Atari `PORTB`, the 130XE) cannot be resolved without new producer data. For such data, Go to Memory must use `ResolveRead` and show the current mapping, not claim a bank.

## 9. Test proposals (#17.2 Q8)

New fixtures in `tests/source-debug/fixtures/`, built for all three compilers where the language allows. Each test compares the resolved value with a runtime memory read at a source breakpoint (the trace runner already stops at lines).

| Fixture | Covers | Expected result |
|---|---|---|
| `vars-globals` | `char`, `signed char`, `int`, `long`, an array, a pointer, a struct, a `static` global | Oscar64 types and sizes are exact. cc65: address only, size from the next label or the segment, marked `Approximate`. KickC: label value |
| `vars-locals` | block locals, a shadowed `i` in two blocks and two functions, a static local | the shadowed name resolves per PC; `.lbl` duplicates are not trusted |
| `vars-recursion` | `n` at depth 1..3 | Oscar64: the innermost frame is `Exact`, outer frames are `Missing` until the frame size is known |
| `vars-cc65-stack` | locals read during argument pushes | the cc65 depth reconstruction matches memory at every line start |
| `vars-optimized` | the same sources at `-O2` (Oscar64), `-Oirs` (cc65), KickC default | records which variables disappear; no guessed values |
| `vars-banked` | a global in banked RAM (Atari XL/XE `PORTB`, NES with WRAM) | `ResolveRead` mapping shown; no bank claimed |
| `vars-kickc-const` | KickC constant-folded locals | `Constant` class; never offered for memory navigation |

## 10. Follow-up backlog (proposal)

The numbers continue after RD-045. All rows come after the RD-023 review (#17.3 non-goals).

| ID | Item | Depends on | Risk | Acceptance |
|---|---|---|---|---|
| RD-046 | `CDebugVariable`/`CDebugType`/scope entities and capabilities in `CDebugDatabase` | RD-006 | model churn | unit test: build, finalize and query by name and by PC; capability flags reported |
| RD-047 | Oscar64 loader: `variables[]` (with size and type), per-function locals in the three classes, `types` | RD-046 | `.dbj` changes between Oscar64 versions | every fixture variable loads with the class in #2.1; matches `main.map` addresses |
| RD-048 | cc65 loader: `auto` csyms (`offs`), `sp` from `.lbl`, globals from `_`-prefixed data labels with `Approximate` size | RD-046 | no types, stack depth | `n`, `left`, `right`, `w` load with offsets as in #2.2; `_result` -> `result` |
| RD-049 | KickC: classify `.label` rows into address / constant / register using `.asm`; remove constants from the label store | RD-046 | relies on comment text | `v` at `$0001` is not a label; `result` is `Fixed` |
| RD-050 | Label store: kind and scope per label, several labels per address, bank-aware names | RD-046 | touches all label UIs and serialization | `_bankA`/`_bankB` both kept; `BSSStart` not shown as a variable |
| RD-051 | Variable resolver with quality and reason; innermost-frame locals; stale-build invalidation | RD-047/048, RD-012 | wrong `Exact` claims | each `vars-*` fixture: resolved value equals a runtime read at every stop marked `Exact` |
| RD-052 | Go to Memory from the source view, watches and hints (range highlight, status line) | RD-051 | low | ImGui test: navigate to `result`, the dump scrolls and highlights 1 byte |
| RD-053 | Watches by source name, typed representation, unavailable states | RD-051 | watch serialization compatibility | old HJSON watch files load; a context watch shows `<not in scope>` outside its function |
| RD-054 | Remote/MCP: `symbols/resolve`, `watch/add` with a representation and variable name | RD-051 | API surface | endpoint tests for all classes and qualities |
| RD-055 | cc65 stack-depth reconstruction; Oscar64 outer-frame `SP` from prologue decoding | RD-051 | heuristic | `vars-recursion` and `vars-cc65-stack` pass at every line start, otherwise `Approximate` |
| RD-056 | Producer requests: Oscar64 frame size and PC live ranges; cc65 C types and global csyms | none | upstream timing | issue filed with the fixture evidence of #2 |

## 11. Open questions for the maintainer

1. Should context watches live in `CDebugSymbolsSegment` beside address watches, or in a separate list owned by the source-debug layer? **Decided (2026-10-19): a separate list owned by `CDebugSymbols`**, so a debug info reload (which replaces the segments) keeps them, saved in its own HJSON section. The maintainer also asked that watches support banking (today a watch is a CPU address read through the CPU's current mapping); see RD-053 in TRACKER.md.
2. Is the cc65 stack-depth decoding (RD-055) worth doing, or should cc65 locals stay `Approximate` until cc65 emits more?
3. Should the Oscar64 `.lbl` and cc65 `.lbl` be read in addition to the debug file? `.lbl` is the only source of the `sp` address for cc65. **Settled by RD-022:** the cc65 loader reads `main.lbl` for `sp`.
