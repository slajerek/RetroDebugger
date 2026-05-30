# Chunk 2 CPU-core re-merge plan (reassessment, 2026-05-28)

Triggered by discovering drivecpu.c's inlined 6502 core is stale 3.1. This doc maps ALL
stale inlined cores and the remaining Chunk 2 surface, with a validated methodology and
recommended sequencing. The cycle-exact core re-merges are the genuine hard core of the
VICE 3.1->3.10 upgrade (and the source of the 1541-accuracy goal).

## Architecture: who inlines what

RD inlines cross-dir core `.c` files into their host CPU/device file (aux-core inline
convention). Discovered inline relationships:

- `drivecpu.c`      inlines `6510core.c`     (markers lines 496-3693)  -> drive 6502
- `drivecpu65c02.c` inlines `65c02core.c`    (//#include line 410)     -> drive 65C02  [DONE]
- `c64cpusc.c`      inlines `mainc64cpu.c`   (lines 217-4356), which itself inlines
                    `6510dtvcore.c` (nested, //#include at line 1299)  -> MAIN C64 CPU
- `c64acia1.c`      inlines `aciacore.c`     (lines 96-1322)           -> ACIA 6551
- `cpmcart.c`       inlines `z80core.c`      [DONE, CART batch]
- `digimax.c`/`shortbus_digimax.c`/`userport_digimax.c` inline `digimaxcore.c` [DONE]

NOTE: main CPU core is `6510dtvcore.c` (handles both 6510 and DTV via #ifdef C64DTV), NOT
`6510core.c`. Drive core is `6510core.c`. They are SEPARATE files, each with its own
inlined copy. No sharing -> each needs its own re-merge.

## Stale-core inventory (cycle-exact 3-way re-merges needed)

| Core (host)                         | upstream Δ 3.1->3.10 | _DUMMY 3.1->3.10 | isolated merge conflicts | status |
|-------------------------------------|----------------------|------------------|--------------------------|--------|
| 6510core.c (drivecpu.c)             | 1304 lines           | 0 -> 107         | 68                       | STALE  |
| 6510dtvcore.c (c64cpusc.c, nested)  | 521 lines            | 12 -> 34         | TBD                      | STALE  |
| mainc64cpu.c (c64cpusc.c)           | 317 lines            | n/a              | TBD                      | STALE  |
| aciacore.c (c64acia1.c)             | 413 lines            | n/a              | 30                       | STALE  |
| 65c02core.c (drivecpu65c02.c)       | 130 lines            | n/a              | -                        | DONE   |

Staleness evidence: drive 6510core has 0 `_DUMMY` (3.10 has 107); c64cpusc.c has
`#include "clkguard.h"` + `clk_guard_t *maincpu_clk_guard` (3.10 removed clkguard); its 12
`_DUMMY` are 3.1's dtvcore dummies, not 3.10. RD instrumentation is PERVASIVE in 6510core
(~hooks woven through nearly every instruction).

## VALIDATED methodology: isolated per-core 3-way merge

The whole-file diff3 on drivecpu.c is CORRUPTED (the inlined core shifts everything ->
3 spurious drivecpu_execute defs). But merging the ISOLATED inlined-core region aligns
cleanly (6510core -> 68 well-formed conflicts). So:

1. Extract RD's inlined core region (between the `/// X.c starts/ends here` markers) -> ours.
2. base = vanilla 3.1 core, theirs = vanilla 3.10 core (both sed `types.h`->`vicetypes.h`).
3. `git merge-file -p --diff3 ours base theirs` -> conflicts only where RD hooks meet 3.10 changes.
4. Resolve each: take 3.10 cycle-exact logic as base truth, re-apply RD's debugger-hook delta
   on top (same principle as every prior merge). Highest care: a misplaced hook = wrong
   drive/CPU timing. The >-gate canNOT validate cycle placement -> manual review of each
   resolved instruction hook.
5. >-gate: resolved core vs vanilla 3.10 core (sed vicetypes->types). `>`=0 (no dropped 3.10
   cycle logic, except documented intentional); `<`=RD hooks (eyeball each).
6. Re-insert between the host file's `/// X.c starts/ends here` markers.
7. Apply the host-file scaffolding 3.10 delta separately (drivecpu.c/c64cpusc.c wrapper:
   loop condition, CLOCK tcycles, ORIGIN_MEMSPACE/CPU_LOG_ID/CPU_IS_JAMMED, JAM rename, etc.).

## Full remaining Chunk 2 surface (honest inventory)

A. CYCLE-EXACT CORE RE-MERGES (the delicate heart):
   A1. drivecpu.c inlined 6510core.c    (68 conflicts) -> drive 6502 / 1541 accuracy
   A2. c64cpusc.c inlined mainc64cpu.c + nested 6510dtvcore.c -> main C64 CPU
   A3. c64acia1.c inlined aciacore.c    (30 conflicts) -> ACIA 6551 (also clkguard straggler)

B. DRIVE-CLUSTER type/clkguard migration (mechanical; drivecpu.c part done on /tmp working copy):
   B1. drivecpu.c scaffolding: drive_context_t->diskunit_context_t, drop clkguard +
       drivecpu_prevent_clk_overflow, 3.10 scaffolding delta (field map in flags-doc DECISION 11).
   B2. drive.c: diskunit migration (58 refs) + drop prevent_clk_overflow call at :514.
   B3. drive-overflow.c/.h: DELETE (gone in 3.10).

C. CLKGUARD-removal stragglers:
   C1. datasette.c: clkguard removal.
   C2. clkguard.c / clkguard.h: DELETE from tree + Xcode build-list.
   (c64acia1.c clkguard handled with A3; c64cpusc.c clkguard handled with A2.)

D. POST-MERGE: build-list reconciliation (add new 3.10 .c units, drop GONE files) + link +
   re-enable WebSocket regression suite (28 pass / 1 skip / 1 xfail) as the acceptance gate.

## Recommended sequencing

1. drivecpu.c FULL: 6510core.c re-merge (A1) + scaffolding (B1). Self-contained (drive-only),
   validates the methodology on the biggest core, delivers the headline 1541-accuracy goal.
2. drive.c (B2) + delete drive-overflow.c (B3). Completes + makes the drive subsystem
   type-coherent (the approved cluster).
3. c64cpusc.c (A2 + scaffolding): mainc64cpu.c + nested 6510dtvcore.c re-merge. The main CPU;
   hardest extraction (nested inline). Includes its clkguard removal.
4. clkguard stragglers: c64acia1.c (A3 aciacore re-merge + clkguard), datasette.c (C1),
   delete clkguard.c/.h (C2).
5. Build-list + link + WebSocket suite (D).

Status when this plan was written: monitor.c committed (00f5e94). drivecpu.c type-migration
(B1 field/type sed) staged on /tmp/dcpu_work only — NOT applied to live; live drivecpu.c is
still untouched RD 3.1. No core re-merge started yet (paused here to reassess per user).

## PROGRESS UPDATE (2026-05-28, after user approved "proceed with recommended sequence")

### A1 + B1 DONE (drivecpu.c) — installed to LIVE, NOT yet committed (cluster pending drive.c):
- 6510core.c isolated 3-way re-merge: 68 conflicts -> 59 auto (RD reindent-only, took 3.10) +
  9 manual hook merges (DO_INTERRUPT, BRK, JSR, FETCH_OPCODE, CPU-emulated header, interrupt
  processing, FETCH jam, opcode switch, header marker). Method: /tmp/resolve_6510.py inserted RD
  hooks (C64D_DRIVE_ANNOTATE_PUSH, IRQ-source/breakpoint checks, PC-override, VICE_DEBUG) onto
  3.10's cycle-exact base. Core gate (diff -w vs vanilla 3.10): 0 dropped cycle logic (17 > all
  intentional: 3 VICE_DEBUG renames + conflict-8 C64D_FETCH_OPCODE_LOAD replacement + blanks).
  _DUMMY count now 107 (== 3.10; was 0 stale) — cycle-exact dummy reads landed. drive_context[0]
  -> diskunit_context[0]. jam recovery (lastop/SET_OPCODE) + CPU_IS_JAMMED preserved.
- Scaffolding B1: includes (-clkguard +mainlock +uiapi), diskunit_clk[NUM_DISK_UNITS],
  setup_context (mem_bank_list_nos/mem_bank_poke, %u, -clk_guard_new), LOAD/STORE +6 host DUMMY
  macros, JUMP uint types, cpu_reset (ui_display_reset + dual rotation_reset), drivecpu_shutdown
  (-clk_guard_destroy), drivecpu_init (drivemem_init(drv)), REMOVED drivecpu_prevent_clk_overflow,
  drive_trap_handler uint types, drivecpu_execute (CLOCK tcycles, 64-bit loop cond, ORIGIN_MEMSPACE/
  CPU_LOG_ID/CPU_IS_JAMMED, JAM->drivecpu_jam), -MSVC pragmas, drivecpu_jam (rename, DRIVE_TYPE_9000,
  machine_jam->drive_jam(drv->mynumber,...), JAM_RESET_CPU/JAM_POWER_CYCLE, MACHINE_RESET_MODE_*),
  snapshot (SNAP_MINOR 3, SMW/SMR_CLOCK, +cpu_last_data, uint casts). Scaffolding gate: 11 > all
  diff-alignment artifacts from RD relocating reg-defines to file scope (all 11 verified PRESENT).
  Whole file: 4511 lines, 0 markers, braces 367/367, 0 drive_context_t, 0 clkguard.
- NOTE: uint8/uint16 (non-_t) in c64d_get_drivecpu_regs_internal are an established RD alias used
  by committed iecbus.c/memiec.c — left as-is (not a regression).

### B2 drive.c DONE: 3 conflicts resolved (took 3.10 dual-drive nested loops + re-added RD snapshot
fields GCR_dirty_track_for_snapshot/needs_refresh, P64_dirty_for_snapshot; dropped clock_frequency
[now diskunit-level] + rotation_reset [3.10 moved it]). Residual c64d helpers migrated:
drive_context_t->diskunit_context_t, drive_context[]->diskunit_context[], drv->drive->drv->drives[0]
(RD drive-0 assumption). Preserved 3.10's drive->drive index field (drive_s.drive). drive_jam() ext
fn present (drivecpu.c calls it). Gate: 0 dropped 3.10 lines, 133 RD additions, braces 166/166.
NOTE: BSD sed lacks \b -> used Python regex for word-bounded migration.
### B3 DONE: drive-overflow.c/.h deleted (3.10 removed; only exported drive_overflow_init, 0 callers).

### DRIVE CLUSTER COMPLETE (commit 4874d68): drivecpu.c + drive.c + delete drive-overflow.c/.h.

## A2 c64cpusc.c DONE (installed to LIVE, this session) — the MAIN C64 CPU, second big core.
Structure: c64cpusc.c = 3 vanilla files. (1) wrapper c64/c64cpusc.c (192 ln), (2) inlined
mainc64cpu.c (795->988), (3) nested-inlined 6510dtvcore.c (2556->2799). RD's nested-core markers:
`//#include "6510dtvcore.c"` opens, `/// end of 6510dtvcore.c` closes. Did THREE isolated 3-way
merges, then reassembled by swapping the 4 content regions into the original (preserving RD's exact
inline markers).

### Wrapper merge: 0 conflicts. Adopted 3.10 deltas: memmap_mem_update(addr,0,0) signature +
BYTE/WORD/DWORD->uint8/16/32_t. RD's CLK_INC override (c64d_c64_do_cycle) + profiler/includes kept.

### mainc64cpu.c merge: 26 conflicts -> 7 reindent(->3.10) + 19 hooks. KEY resolutions:
- DEBUG->VICE_DEBUG renames folded with 3.10's `#if defined(DEBUG)||defined(FEATURE_CPUMEMHISTORY)`.
- mem-access layer (FEATURE_CPUMEMHISTORY is LIVE, vice-config.h:69): adopted 3.10 dummy infra
  (memmap_mem_store_dummy/read_dummy, STORE_DUMMY/LOAD_DUMMY, REU-in-STORE) + kept RD cell-marking
  on the REAL accessors only (added c64d_mark_c64_cell_read to the split-out real memmap_mem_read;
  dummies stay unmarked). RD's c64d_mem_* API funcs preserved.
- 2 DIFF MISALIGNMENTS (RD reorganized init): 3.10's 6 new mem_bank fields (current_bank_index,
  mem_bank_list_nos, mem_bank_index_from_bank, mem_bank_flags_from_bank, mem_peek_with_config,
  mem_bank_poke) folded into monitor_interface_get; maincpu_log=log_open folded into early_init.
  Symbols verified: fields exist in root/monitor.h (3.10), funcs in c64memsc.c (012ad85).
- mainloop cluster (RD moved CPU regs to FILE SCOPE for debugger + uses o_bank_base pointers +
  `while(c64d_vice_run_emulation)`): KEPT RD's organization, DROPPED 3.10 bank_base_ready refactor,
  INJECTED 3.10 macros (CPU_LOG_ID, ANE/LXA_LOG_LEVEL, CPU_IS_JAMMED, ORIGIN_MEMSPACE) +
  MODE_SOFT->MODE_RESET_CPU.
- JAM: 3.10 JAM_RESET_CPU/JAM_POWER_CYCLE/machine_powerup + RD LOGError.
- suffix: RD per-instruction debug-hook block + 3.10 maincpu_log/archdep_vice_exit/autostart_advance.
- snapshot: full 3.10 (SMR/SMW_CLOCK 64-bit + ane_log_level/lxa_log_level/maincpu_jammed fields +
  interrupt_read/write _snapshot/_new_snapshot/_sc_snapshot, SNAP 1.4). Gate: 31 > all intentional
  (VICE_DEBUG, marking-routed STOREs, file-scope regs, o_bank_base, while-cond, FIXME comments,
  mainloop/ORIGIN_MEMSPACE alignment artifacts), 669 <.

### 6510dtvcore.c merge (THE HEADLINE): 45 conflicts -> 35 reindent + 10 hooks. _DUMMY 12->34 (==3.10
exactly) -> cycle-exact dummy reads landed (main-CPU analogue of drive 6510core's 0->107). ALL
cycle-critical tokens == 3.10: CLK_INC 88, LOAD_DUMMY 12, DO_IRQBRK 3, LOCAL_SET_INTERRUPT 7,
CHECK_PROFILE_INTERRUPT 5. PROFILING ADOPTED (consistent w/ drivecpu.c; profiler.c added in step D;
main CPU has DRIVE_CPU undefined so #if !defined(DRIVE_CPU) blocks active). Hooks: DO_IRQBRK +
DO_INTERRUPT (C64D_ANNOTATE_PUSH x10 stack annotation + IRQ/NMI source tracking via
vicii/cia1/cia2 c64d_irq_flag + c64d_irqbrk source), BRK/JSR/RTS (RD annotate + CHAMP c64d_profiler_*
kept ALONGSIDE 3.10 CHECK_PROFILE_*), per-instruction block (RD breakpoint checks
c64d_c64_check_irq{nmi,vic,cia}_breakpoint + snapshot mgr + 3.10 JAMMED-recovery HACK), FETCH (RD
bank_base==NULL JAM guard + 3.10 lastop/SET_OPCODE jam recovery + profile_sample_start). Gate -w:
11 > all intentional (VICE_DEBUG + cosmetic 3.10 paren-conditional formatting), 144 <.
Assembled c64cpusc.c: 4785 lines, braces 437/437, 0 markers, _DUMMY 54 total.

## A3 c64acia1.c DONE + datasette.c DONE (clkguard stragglers) — this session.
- c64acia1.c (c64/cart/, inlines aciacore.c): aciacore 3.1->3.10 isolated merge = 30 conflicts, ALL
  reindent (RD never instrumented the 6551 ACIA -> 0 hooks). Wrapper c64acia1.c merge: 0 conflicts
  (RD added nothing to the wrapper). RD's only ACIA change = inline + reindent + DEBUG->VICE_DEBUG.
  aciacore clkguard auto-dropped (3.1 had 3 refs, 3.10 0). Gate: aciacore 1 intentional > (VICE_DEBUG),
  wrapper 1 blank line. Assembled 1809 lines, braces 189/189, 0 markers, 0 clkguard.
- datasette.c (RD root/datasette.c, 3.10 moved to datasette/datasette.c): PURE 3.1 (only rename, 0
  c64d hooks) -> merged = identical to vanilla 3.10 + rename, clkguard removed.
- CLKGUARD now has ZERO functional users tree-wide (c64cpusc/drivecpu/c64acia1/datasette all clean;
  only arch/vicetypes.h has a COMMENT mentioning it). clkguard.c/.h are orphaned -> DELETE in step D
  (with the Xcode build-list edit).

### NEXT: D build-list reconciliation (ADD profiler.c + joyport/mouse_* + new headers' .c; REMOVE
clkguard.c/translate.c/patchrom.c) + translation removal + link + WebSocket suite (28/1/1) gate.
This is the FINAL Chunk 2 sub-task — all heavy/core subsystem merges are now done.
