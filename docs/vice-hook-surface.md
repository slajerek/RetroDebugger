# VICE Hook Surface Catalog — Phase 1

**Date:** 2026-05-27  
**Branch:** vice-3.10-upgrade  
**Baseline:** vanilla VICE 3.1 (`~/vice-ref/3.1/src/`)  
**Modified tree:** `src/Emulators/vice/` (slajerek's fork)  
**Excludes:** `src/Emulators/vice/ViceInterface/` — slajerek's own bridge code, not patched VICE  

This document is the complete map of every modification slajerek made to vanilla VICE 3.1 inside
`src/Emulators/vice/`. It is the authoritative reference for Phase 2 macro design and Phase 4
replay planning.

---

## Summary

| Metric | Count |
|--------|-------|
| Modified files with `c64d_` markers (full tree sweep) | **64** |
| Total `c64d_` reference lines (call sites + definitions) | **633** |
| Distinct `c64d_*` symbol names | **282** |
| `c64d_*` function definition lines (grep for definition forms) | **312** |
| Files with genuine non-additive patches (deletions beyond header/static noise) | **~14** (see table below) |

**Prior finding upheld with corrections:** The Phase 0 finding that "modifications are essentially
all additive" is **substantially correct** for the majority of files (~50 of 64). However, the
full sweep reveals six categories of genuine non-additive changes that go beyond the known
header-rename / static-removal noise:

1. `c64cpusc.c` and `drivecpu.c`: vanilla files include `mainc64cpu.c` / `6510core.c` via
   `#include` at the bottom; slajerek's versions have **inlined** those files and inserted hooks
   throughout. The diff shows large deletion blocks, but no logic was removed — it is an
   inline-expansion with hooks inserted. This is non-additive in diff form but not in substance.

2. `soundsdl.c`: SDL audio callback was substantially **rewritten** to support multi-channel
   (mono/stereo) output. Vanilla callback logic was replaced, not augmented.

3. `resid/resid-filter.cpp`: The **8580 SID model section** (~180 lines of commentary and filter
   coefficient tables) was deleted. Only 6581 filter remains.

4. `resid/resid-sid.cpp`: Multiple behavioral changes — `SID::read()` POT/OSC/ENV paths no longer
   update `bus_value_ttl`; write pipeline behavior changed (no 8580 fake pipeline delay);
   `databus_ttl` initialization changed to a fixed `0x4000`.

5. `c64memsc.c`: `mem_ram[C64_RAM_SIZE]` (static array) changed to `BYTE *mem_ram` (pointer),
   externally initialized via `c64d_init_memory()` to allow memory-mapped file backing.

6. `joystick.c`: `joystick_process_latch()` bypasses the random alarm delay (sets `delay=0` and
   calls `joystick_latch_matrix(0)` directly), and `keyboard_key_pressed()` return type changed
   from `void` to `int`.

---

## Pervasive non-`c64d_*` noise (present in virtually every modified file)

Two changes appear in essentially all 64 modified files and are mechanical, not logical:

- **Header rename:** `#include "types.h"` → `#include "vicetypes.h"` (must be replayed as a
  mechanical substitution on every patched file when targeting VICE 3.10).
- **`static` removal:** Internal functions promoted to external linkage (removing `static`) so
  ViceInterface can call them. See the "Static promotions" subsection in each category below.

---

## Hook Categories

### Category 1: CPU Step / Cycle Accounting

**Purpose:** Track the C64 main CPU and drive CPU at per-cycle granularity for the debugger,
profiler, and step/trace modes.

**Files involved:**
- `c64/c64cpusc.c` (283 hits — primary definition site)
- `drive/drivecpu.c` (97 hits — drive CPU variant)
- `c64/cart/reu.c` (4 hits — REU DMA cycles)
- `core/flash040core.c` (2 hits — flash cycle adjustment)
- `root/dma.c` (1 hit — DMA cycle accounting)
- `root/midi.c` (2 hits — MIDI cycle correction)
- `root/maincpu.h`, `root/interrupt.h` (extern declarations)
- `viciisc/vicii-cycle.c` (1 hit — raster cycle increment)
- `sid/sid.c` (4 hits — SID cycle correction)

**Representative symbols:**
- `c64d_maincpu_clk` — global CLOCK counter incremented alongside `maincpu_clk` in the `CLK_INC()` macro; used throughout as the debugger's authoritative clock
- `c64d_maincpu_current_instruction_clk` — clock value at start of current instruction
- `c64d_maincpu_previous_instruction_clk`, `c64d_maincpu_previous2_instruction_clk` — ring of saved per-instruction clocks
- `c64d_c64_instruction_cycle` — cycle counter within current instruction
- `c64d_c64_do_cycle()` — hook injected into `CLK_INC()` macro; returns BA-low flags
- `c64d_do_cycle` — drive CPU equivalent
- `c64d_reset_counters` — resets cycle counters
- `c64d_get_maincpu_clock` — returns current `c64d_maincpu_clk`
- `c64d_get_vice_maincpu_clk`, `c64d_get_vice_maincpu_current_instruction_clk` — accessors

**Style:** Mixed. `c64d_maincpu_clk` increment is inserted into the `CLK_INC()` macro body
(control-flow change to the macro); all functions are additive.

**Hook count (distinct symbols):** ~15

**3.10 impact:** `c64cpusc.c` still exists in 3.10 (192 lines, same structure — `#include
"../mainc64cpu.c"` pattern). `drivecpu.c` still exists in 3.10 at `drive/drivecpu.c` (715
lines, same structure — `#include "6510core.c"` pattern). The inline-expansion approach slajerek
used means replay requires re-inlining mainc64cpu.c and re-inserting all hooks. **Risk: HIGH** —
both files require full re-examination since vanilla 3.10's versions differ from 3.1's.
`clkguard.c/.h` is present in slajerek's tree (`root/clkguard.c`) but is REMOVED from 3.10;
several files in the slajerek tree include it.

---

### Category 2: Memory Read Taps (C64 side)

**Purpose:** Intercept every C64 memory read for the debugger's memory access visualizer and
breakpoint system.

**Files involved:**
- `c64/c64cpusc.c` (primary — `c64d_mark_c64_cell_read` injected at read points)
- `c64/c64memsc.c` (34 hits — peek functions, init, RAM accessor)

**Representative symbols:**
- `c64d_mark_c64_cell_read(addr)` — callback on every C64 read
- `c64d_peek_c64(addr)` — side-effect-free peek (does not advance state)
- `c64d_peek_c64_for_cycle(addr, mem0001, exrom, game)` — cycle-accurate peek
- `c64d_peek_io(addr)` — peek into I/O region
- `c64d_peek_memory_c64(buf, start, end)` — bulk range peek
- `c64d_peek_whole_map_c64(buf)` — full 64K map with banking
- `c64d_peek_memory0001()` — read port register $01
- `c64d_mem_ram_read_c64(addr)` — raw RAM read (no banking)
- `c64d_colorram_peek(addr)` — color RAM read

**Style:** Additive — all new functions appended or inline hooks at read sites.

**Notable structural change:** `mem_ram` in `c64memsc.c` changed from `BYTE mem_ram[C64_RAM_SIZE]`
(static array) to `BYTE *mem_ram` (pointer), initialized via `c64d_init_memory(uint8 *c64memory)`.
This allows memory-mapped file backing. This is a genuine non-additive change to a data declaration
that affects all consumers of `mem_ram`.

**Hook count (distinct symbols):** ~10

**3.10 impact:** `c64memsc.c` still exists in 3.10 but has structural changes (added
`c64-memory-hacks.h`, `c64model.h` includes; `mem_ram` is `uint8_t mem_ram[C64_RAM_SIZE]` — a
**static array again**, which conflicts with slajerek's pointer patch). **Risk: HIGH** — the
`mem_ram` pointer change must be carefully re-evaluated for 3.10.

---

### Category 3: Memory Write Taps (C64 side)

**Purpose:** Intercept C64 memory writes for the debugger's write-access tracking and
mem-store breakpoints.

**Files involved:**
- `c64/c64cpusc.c` (primary — `c64d_mark_c64_cell_write` at STORE macro sites)
- `c64/c64memsc.c` (write-RAM accessors)

**Representative symbols:**
- `c64d_mark_c64_cell_write(addr, value)` — callback on every C64 write
- `c64d_mark_c64_cell_execute(addr, opcode)` — callback on every C64 instruction fetch
- `c64d_mem_write_c64(addr, value)` — write to C64 memory (honors banking)
- `c64d_mem_write_c64_no_mark(addr, value)` — write without triggering mark
- `c64d_mem_store`, `c64d_mem_store_zero` — store wrappers for debugger
- `c64d_mem_ram_write_c64(addr, value)` — raw RAM write
- `c64d_mem_ram_fill_c64(addr, size, value)` — bulk RAM fill
- `c64d_copy_ram_memory_c64(buf, start, end)` — copy RAM range to buffer
- `c64d_copy_whole_mem_ram_c64(buf)` — copy all RAM
- `c64d_mem_access_addr`, `c64d_mem_access_value`, `c64d_mem_access_is_write`, `c64d_mem_access_watch` — per-access state variables

**Style:** Additive. Hooks are inserted immediately before/after the vanilla STORE macro body
in the inlined `mainc64cpu.c` expansion.

**Hook count (distinct symbols):** ~15

**3.10 impact:** Same as Category 2 — the STORE macro sites must be re-identified in the 3.10
inlined CPU core. Risk: MEDIUM (write hooks are structurally parallel to read hooks).

---

### Category 4: Drive State Observer

**Purpose:** Track 1541 drive CPU state, memory access, and disk state for the drive debugger.

**Files involved:**
- `drive/drivecpu.c` (97 hits — drive CPU primary)
- `drive/iec/memiec.c` (18 hits — IEC memory R/W hooks)
- `drive/iecieee/via2d.c` (10 hits — VIA-2 read/write)
- `drive/iec/via1d1541.c` (12 hits — VIA-1 read/write/peek)
- `drive/drive.c` (12 hits — GCR dirty flags)
- `drive/drive-writeprotect.c` (1 hit — sense peek)
- `drive/drivemem.c` (2 hits — RAM read/write hooks)
- `drive/rotation.c` (2 hits — track dirty flag)
- `drive/viad.h`, `drive/iec/via1d1541.h` (extern declarations)
- `iecbus/iecbus.c` (25 hits — IEC bus peek and drive accessors)

**Representative symbols:**
- `c64d_mark_drive1541_cell_read(addr)` / `c64d_mark_drive1541_cell_write(addr, val)` / `c64d_mark_drive1541_cell_execute(addr, op)` — access markers
- `c64d_peek_drive(driveNum, addr)` — side-effect-free peek
- `c64d_peek_mem_drive_internal(drv, addr)` — internal peek by context
- `c64d_peek_memory_drive_internal(drv, buf, start, end)` — bulk peek
- `c64d_peek_whole_map_drive(driveNum, buf)` / `c64d_peek_whole_map_drive_internal(drv, buf)` — full drive memory map
- `c64d_get_drivecpu_regs(driveNum, a, x, y, p, sp, pc)` — CPU register readout
- `c64d_set_drivecpu_pc_no_trap(drv, pc)` — set PC without triggering trap
- `c64d_set_drivecpu_regs_no_trap(drv, ...)` — set all regs
- `c64d_mem_ram_read_drive(driveNum, addr)` / `c64d_mem_ram_write_drive(driveNum, addr, val)` — raw RAM access
- `c64d_copy_ram_memory_drive(driveNum, buf, start, end)` / `c64d_copy_mem_ram_drive(driveNum, buf)` — bulk copy
- `c64d_get_drive_disk_gcr()` / `c64d_get_drive_disk_image()` / `c64d_get_drive_is_disk_attached()` — disk state queries
- `c64d_get_drive_current_halftrack()` / `c64d_set_drive_half_track(track)` — track position
- `c64d_mark_drive1541_contents_track_dirty(track)` — GCR dirty flag
- `c64d_is_drive_dirty_for_snapshot()` / `c64d_is_drive_dirty_and_needs_refresh()` etc. — dirty state flags
- `c64d_drive1541_check_pc_breakpoint(pc)` — PC breakpoint check (drive)
- `c64d_drive1541_check_irqvia1_breakpoint()` / `c64d_drive1541_check_irqvia2_breakpoint()` / `c64d_drive1541_check_irqiec_breakpoint()` — IRQ breakpoints
- `c64d_peek_via1d1541_pra/prb` / `c64d_via1d1541_peek(ctxptr, addr)` — VIA-1 side-effect-free reads
- `c64d_via2d_peek(ctxptr, addr)` / `c64d_peek_via2d_pra/prb` — VIA-2 reads
- `c64d_peek_via_context` / `c64d_viacore_peek` — generic VIA core peek
- `c64d_interrupt_drivecpu_trigger_trap` — inject trap into drive CPU
- `c64d_iecbus_cpu_peek_conf1()` — IEC bus CPU port peek
- `c64d_set_drive_disk_memory(drv, buf)` — inject disk memory
- `c64d_drive_poke(driveNum, addr, val)` — write via store function table

**Style:** Mostly additive. VIA read hooks (`c64d_mark_drive1541_cell_read/write`) are inserted
immediately before/after the original read/write in `memiec.c`, `via1d1541.c`, `via2d.c`,
`drivemem.c` (control-flow: one line before original action, one after — genuine non-additive
modification). Drive peek functions are all new.

**Static promotions in this category:**
- `memiec.c`: `drive_read_rom`, `drive_read_rom_ds1216` — promoted to external
- `iecieee/via2d.c`: multiple via store/read functions promoted

**Hook count (distinct symbols):** ~55

**3.10 impact:** `drive/drivecpu.c`, `drive/drive.c`, `drive/drivemem.c`, `drive/rotation.c`
all exist in 3.10. `drive/iec/memiec.c` and `drive/iec/via1d1541.c` exist in 3.10 at the same
paths. `drive/iecieee/via2d.c` exists in 3.10. **Risk: MEDIUM** — file paths are stable, but
the 6510core.c inline expansion in drivecpu.c requires the same re-inlining approach.

---

### Category 5: REU DMA Taps

**Purpose:** Track REU DMA cycles so `c64d_maincpu_clk` stays in sync during DMA transfers.

**Files involved:**
- `c64/cart/reu.c` (4 hits)
- `c64/cart/c64-generic.c` (2 hits — cart ROML peek/poke)

**Representative symbols:**
- `c64d_maincpu_clk++` — inserted at three DMA cycle-bump sites in `reu_dma_host_to_reu()` and related functions
- `c64d_reu_io2_store(addr, value)` — public wrapper for `reu_io2_store()`
- `c64d_peek_cart_roml(addr)` — peek into cartridge ROML
- `c64d_poke_cart_roml(addr, value)` — poke cartridge ROML

**Style:** Additive. The `c64d_maincpu_clk++` insertions are pure additions within unchanged logic.
Two `assert()` calls were commented out (minor non-additive: `//assert(...)`).

**Static promotions:** `reu_io2_store`, `reu_read_without_sideeffects`, `set_reu_enabled`,
`set_reu_size`, `set_reu_filename` all had `static` removed. `reu_enabled` changed from
`static int` to `volatile int`.

**Hook count (distinct symbols):** ~5

**3.10 impact:** `c64/cart/reu.c` exists in 3.10. `c64/cart/c64-generic.c` exists in 3.10.
**Risk: LOW** — the DMA tap approach will still work; only the cycle-bump insertion points may
have shifted if 3.10 restructured the DMA loop.

---

### Category 6: CIA / VIA Observer

**Purpose:** Capture every CIA1/CIA2 register read/write and surface side-effect-free peek for
the debugger's chip inspector and IRQ breakpoints.

**Files involved:**
- `c64/c64cia1.c` (8 hits)
- `c64/c64cia2.c` (13 hits)
- `core/ciacore.c` (6 hits — `c64d_ciacore_peek` and IRQ flag)
- `root/cia.h` (1 hit — struct field `c64d_irq_flag`)
- `core/viacore.c` (2 hits — `c64d_viacore_peek`)
- `root/via.h` (2 hits — struct field `c64d_irq_flagged`)
- `drive/iec/via1d1541.c` (IRQ flag set on via interrupt)
- `viciisc/vicii-irq.c` (4 hits — VIC IRQ flag)
- `viciisc/viciitypes.h` (1 hit — struct field `c64d_irq_flag`)

**Representative symbols:**
- `c64d_cia1_peek(addr)` / `c64d_cia2_peek(addr)` — side-effect-free CIA register reads
- `c64d_ciacore_peek(cia_context, addr)` — core CIA peek implementation
- `c64d_cia1_register_read` / `c64d_cia1_register_written` / `c64d_cia1_read_value` / `c64d_cia1_write_value` — per-access state variables (extern globals)
- Same set for `cia2_`
- `c64d_viacore_peek(via_context, addr)` — VIA core peek
- `c64d_get_cia_context(num)` — return CIA context pointer
- `c64d_refresh_cia` — force CIA state refresh
- `c64d_c64_check_irqcia_breakpoint()` / `c64d_c64_check_irqnmi_breakpoint()` — IRQ breakpoint checks
- `c64d_c64_is_checking_irq_breakpoints_enabled()` / `c64d_drive1541_is_checking_irq_breakpoints_enabled()` — predicate
- `c64d_irq_flag` struct field on `cia_context_t` (set when CIA triggers IRQ)
- `c64d_irq_flagged` struct field on `via_context_t`
- `c64d_irq_flag` / `c64d_irq_flag_needs_clear` / `c64d_irqbrk_irq_source` — VIC IRQ observation globals
- `c64d_irqbrk_entry_type_base` — IRQ entry type classification

**Style:** Additive for peek functions. The register-read/write observation is a non-additive
insertion: in `c64cia1.c` and `c64cia2.c`, both `ciacore_store()` and `ciacore_read()` calls
are wrapped so the observed address/value is captured before/after the real call.
`ciacore.c:ciacore_store()` had its parameter renamed from `byte` to `value` (non-additive: both
original and new logic remain but the variable name changed throughout). `cia_update_ta` /
`cia_update_tb` had `static` removed.

**Hook count (distinct symbols):** ~25

**3.10 impact:** `c64cia1.c` and `c64cia2.c` exist in 3.10. `core/ciacore.c` exists in 3.10.
`viciisc/vicii-irq.c` exists in 3.10. **Risk: LOW-MEDIUM** — file paths stable; the observation
wrappers need re-insertion at the same call sites.

---

### Category 7: VIC-II State Observer

**Purpose:** Capture VIC-II raster state, color registers, border mode, and frame events for the
debugger's VIC viewer and cycle-accurate state snapshots.

**Files involved:**
- `viciisc/vicii.c` (8 hits)
- `viciisc/vicii-cycle.c` (5 hits)
- `viciisc/vicii-draw-cycle.c` (5 hits)
- `viciisc/vicii-irq.c` (4 hits)
- `viciisc/viciitypes.h` (1 hit — struct field additions)

**Representative symbols:**
- `c64d_c64_vicii_cycle()` — called at end of every `vicii_cycle()` 
- `c64d_c64_vicii_start_frame()` — called at start-of-frame detection
- `c64d_c64_vicii_start_raster_line(line)` — called at start of each raster line
- `c64d_c64_set_vicii_record_state_mode(mode)` / `c64d_vicii_record_state_mode` — state recording control
- `c64d_c64_vicii_store_state_to_bytebuffer()` / `c64d_c64_vicii_restore_state_from_bytebuffer()` — snapshot helpers
- `c64d_get_vicii_state_for_raster_cycle()` / `c64d_get_vicii_state_for_raster_line()` — state readouts
- `c64d_vicii_copy_state()` / `c64d_vicii_copy_state_data` — copy current VIC state
- `c64d_set_color_register(regNum, val)` — write color register (injected in draw-cycle)
- `c64d_get_vic_colors()` / `c64d_get_vic_simple_state()` — bulk readouts
- `c64d_get_vicii_border_mode()` / `c64d_set_vicii_border_mode(mode)` — border control
- `c64d_fetch_phi1_type(addr)` — classify VIC phi-1 fetch (RAM / charROM / cart)
- `c64d_vicii_render_when_paused()` — force render update while paused
- `c64d_vicii_get_geometry(...)` — canvas geometry readout
- `c64d_skip_drawing_sprites` — flag to suppress sprite rendering
- `c64d_refresh_dbuf()` — force display buffer refresh when paused
- `c64d_refresh_previous_lines` / `c64d_refresh_screen` / `c64d_refresh_screen_no_callback`
- Struct field additions to `vicii_t` in `viciitypes.h`: `c64d_irq_flag`, `register_written`,
  `prev_register_written`, `register_read`, `prev_register_read`

**Style:** Mixed. Most new symbols are additive. The sprite drawing block in
`vicii-draw-cycle.c` is wrapped in `if (c64d_skip_drawing_sprites == 0)` — genuine control-flow
change. The `VICE_DEBUG` vs `DEBUG` conditional change in `vicii-cycle.c` is a non-additive rename.

**Static promotions in this category:**
- `vicii.c`: `fetch_phi1_type` promoted to external linkage

**Hook count (distinct symbols):** ~20

**3.10 impact:** `viciisc/vicii.c`, `vicii-cycle.c`, `vicii-draw-cycle.c`, `vicii-irq.c` all
exist in 3.10. 3.10's monitor is in a `monitor/` subdirectory (existing in 3.1 as well).
**Risk: MEDIUM** — file paths stable; the hooks inside the raster loop are insertion-point
sensitive.

---

### Category 8: SID Observer

**Purpose:** Capture SID register reads/writes, provide voice masking, waveform callbacks, and
per-channel audio data for the debugger's SID viewer and audio analyzer.

**Files involved:**
- `sid/sid.c` (17 hits)
- `sid/sid-snapshot.c` (10 hits)
- `sid/sid-resources.c` (8 hits) + `sid/sid-resources.h` (8 hits)
- `sid/fastsid.c` (2 hits)
- `resid/resid-sid.cpp` (3 hits) + `resid/resid-sid.h` (1 hit) + `resid/resid-filter.h` (5 hits) + `resid/resid-filter.cpp` (1 hit)
- `resid-fp/residfp-sid.cpp` (10 hits) + `resid-fp/residfp-sid.h` (2 hits) + `resid-fp/residfp-voice.h` (3 hits)

**Representative symbols:**
- `c64d_sid_register_read` / `c64d_sid_read_value` — last-read register address/value (extern globals)
- `c64d_sid_register_written` / `c64d_sid_write_value` — last-written register address/value
- `c64d_sid_voiceMask` — bitmask for voice enable/disable (per chip)
- `c64d_store_sid_data(buf, sidNum)` — copy SID register array to buffer
- `c64d_sid_read(addr)` / `c64d_sid_write(addr, val)` — instrumented SID access
- `c64d_get_sid_enable()` — predicate for SID enable status
- `c64d_sid_set_engine_model_direct(engine, model)` — force-set engine and model
- `c64d_sid_set_emulate_filters(val)` — filter emulation toggle
- `c64d_sid_set_sampling_method(method)` — sampling method control
- `c64d_sid_set_passband(val)` / `c64d_sid_set_filter_bias(val)` — filter parameters
- `c64d_sid_set_stereo(stereo)` / `c64d_sid_set_stereo_address(addr)` / `c64d_sid_set_triple_address(addr)` — multi-SID config
- `c64d_is_receive_channels_data[chipNo]` — flag array: whether per-channel data is requested
- `c64d_sid_channels_data(chipNo, v1, v2, v3, mixed)` — per-voice audio data callback
- `c64d_waveform_callback` — C++ constructor parameter / function pointer for waveform data
- `c64d_wave_attenuation` / `c64d_wave_shift` — waveform scaling parameters (ReSID-FP)
- `c64d_output` (field on `Voice`) — per-voice sample capture in ReSID-FP
- `c64d_lock_sound_mutex(who)` / `c64d_unlock_sound_mutex(who)` — threaded snapshot protection
- `c64d_sound_init()` / `c64d_sound_pause()` / `c64d_sound_resume()` — sound lifecycle
- `c64d_sound_run_sound_when_paused` / `c64d_skip_sound_run_sound_in_sound_store` — control flags
- `c64d_setting_run_sid_emulation` / `c64d_setting_run_sid_when_in_warp` — SID run flags
- `c64d_reset_sound_clk` — reset sound clock on pause
- `c64d_store_vicii_state_with_snapshot` — save VIC state alongside SID snapshot

**Genuine non-additive changes:**
- `sid.c`: `sid_read_chip()` return path restructured to always capture into `val`, then
  observe before returning (control-flow change — else-if chain changed).
- `sid-snapshot.c`: snapshot write function gated with `c64d_get_sid_enable()` check.
- `resid/resid-filter.cpp`: 8580 SID section (180+ lines) **deleted**.
- `resid/resid-sid.cpp`: `SID::read()` no longer updates `bus_value_ttl`; `SID::write()` uses
  fixed `bus_value_ttl = 0x4000` instead of `databus_ttl`; write pipeline behavior changed.

**Static promotions in this category:**
- `sid.c`: `sid_peek_chip`, `sid_store_chip` promoted
- `sid-resources.c`: `set_sid_engine`, `set_sid_model` promoted
- `fastsid.c`: `fastsid_set_voice_mask` added (new function)

**Hook count (distinct symbols):** ~35

**3.10 impact:** `sid/sid.c`, `sid/sid-snapshot.c`, `sid/fastsid.c`, `sid/sid-resources.c`
exist in 3.10. ReSID is at `resid/` in 3.10 (same path); ReSID-FP (`resid-fp/`) exists in 3.10
(this is a slajerek addition not in vanilla). The 8580 filter deletion in
`resid/resid-filter.cpp` means 3.10's filter.cc has the 8580 code again — this must be
re-deleted or gated. **Risk: MEDIUM-HIGH** for ReSID (behavioral changes are subtle).

---

### Category 9: Sound Driver (SDL Audio)

**Purpose:** Rewrite the SDL audio callback to support multi-channel output (mono/stereo/triple
SID) with the RetroDebugger audio pipeline.

**Files involved:**
- `sounddrv/soundsdl.c` (4 hits)

**Representative symbols:**
- `sidNumChannels` — global for active channel count
- `c64d_set_volume(float)` — volume control (wraps `set_volume`)
- `c64d_wave_attenuation`, `c64d_wave_shift` — referenced via extern

**Genuine non-additive changes:**
- The entire `sdl_callback()` function body was replaced: parameter type changed
  (`Uint8 *stream, int len` → `uint8 *stream, int numSamples`), logic now branches on
  `sidNumChannels` for mono vs stereo handling, doing explicit per-sample interleaving.
- `USE_SDL_AUDIO` / `ANDROID_COMPILE` conditional blocks removed.
- `static SDL_AudioSpec sdl_spec` commented out.

**Hook count (distinct symbols):** ~3 (plus the rewrite)

**3.10 impact:** In 3.10, `soundsdl.c` moved to `arch/shared/sounddrv/soundsdl.c`. The 3.10
version is structurally closer to the vanilla 3.1 version (SDL callback still mono). The
slajerek rewrite will need to be replayed at the new path, against a 3.10 callback that already
differs from 3.1. **Risk: HIGH** — path moved, and 3.10 version already diverged from 3.1.

---

### Category 10: Monitor Integration

**Purpose:** Integrate VICE's built-in monitor with the RetroDebugger step/trace/run commands.

**Files involved:**
- `monitor/monitor.c` (5 hits)
- `arch/uimon.c` (4 hits)

**Representative symbols:**
- `c64d_set_debug_mode(mode)` — called from `mon_instructions_step()`, `mon_instructions_next()`,
  `mon_instruction_return()` to transition to RUNNING after a step
- `execute_monitor_command_jump_trap` / `c64d_execute_monitor_command_jump_trap` — trampoline
  injected into `mon_jump()` for async jump execution
- `c64d_uimon_buf` / `c64d_uimon_bufpos` — output buffer for monitor text
- `c64d_uimon_print(str)` / `c64d_uimon_print_line(str)` — write to monitor buffer
- `c64d_console_log` — `console_t` struct for monitor window routing

**Genuine non-additive changes:**
- `monitor.c`: A closing `}` + `monitor_close(1)` call restructured around the
  `execute_monitor_command_jump_trap` trampoline (the original call is conditional on whether it
  returns a non-jump result). `make_prompt`, `monitor_open`, `monitor_process`, `monitor_close`
  had `static` removed.
- `arch/uimon.c`: Completely new file (not in vanilla SDL arch) — provides the `uimon_*` console
  interface routing output to `c64d_uimon_buf`.

**Hook count (distinct symbols):** ~8

**3.10 impact:** `monitor/monitor.c` exists in 3.10 inside `monitor/` directory (as in 3.1).
3.10's `monitor.c` is significantly larger (3421 vs 2389 lines) — substantial new code. The
`static` promotions and `c64d_set_debug_mode` injection points may have shifted considerably.
**Risk: HIGH** — monitor grew substantially; injection points need to be re-identified.

---

### Category 11: Keyboard & Input

**Purpose:** Provide latch-based keyboard/joystick injection for the debugger API.

**Files involved:**
- `root/keyboard.c` (6 hits)
- `root/keyboard.h` (1 hit)
- `joyport/joystick.c` (10 hits)
- `joyport/mouse.c` (2 hits)
- `arch/mousedrv.c` (2 hits)

**Representative symbols:**
- `c64d_keyboard_init()` — initialize keyboard matrix constants (lshift/rshift row/col)
- `c64d_keyboard_key_down_latch(keycode)` / `c64d_keyboard_key_up_latch(keycode)` — latch key events
- `c64d_keyboard_force_key_up_latch(keycode)` — force key release
- `c64d_keyboard_keymap_clear()` — clear keymap
- `c64d_joystick_key_down(key, joyport)` / `c64d_joystick_key_up(key, joyport)` — joystick input
- `c64d_joystick_latch_matrix_workaround` — workaround flag
- `c64d_mouse_set_type(mt)` — set mouse type
- `c64d_mouse_type_to_joyportid(mouseType)` — map mouse type to joyport ID
- `c64d_mouse_set_position(x, y)` / `c64d_set_mouse_pos(x, y)` — mouse position injection

**Genuine non-additive changes:**
- `keyboard.c`: `keyboard_key_pressed()` return type changed from `void` to `int` (returns 0/1
  to indicate success). `keyboard_set_latch_keyarr()`, `keyboard_key_pressed_matrix()` had
  `static` removed.
- `joystick.c`: `joystick_process_latch()` bypass — `delay` forced to 0 and `alarm_set()` is
  replaced by direct `joystick_latch_matrix(0)` call (the random delay was removed).

**Hook count (distinct symbols):** ~12

**3.10 impact:** `keyboard.c`, `joystick.c`, `mouse.c` exist in 3.10 at same paths.
**Risk: LOW-MEDIUM** — the return type change on `keyboard_key_pressed()` must be tracked;
joystick latch change may conflict with 3.10's joystick handling.

---

### Category 12: Snapshot / State Manager

**Purpose:** Extended snapshot system: in-memory snapshots, screen save, REU/cart data inclusion,
per-frame snapshot manager, and SID enable guard.

**Files involved:**
- `c64/c64-snapshot.c` (6 hits)
- `sid/sid-snapshot.c` (see Category 8)
- `c64/c64cpusc.c` (snapshot manager functions)

**Representative symbols:**
- `c64d_snapshot_write_module(s, save_screen)` / `c64d_snapshot_read_module(s)` — write/read the
  RetroDebugger-specific snapshot module
- `c64_snapshot_write()` — signature extended with `save_reu_data`, `save_cart_roms`, `save_screen` params
- `c64_snapshot_write_in_memory()` — new function: create snapshot in memory buffer
- `c64d_start_frame_for_snapshots_manager()` — called each frame to manage auto-snapshots
- `c64d_check_snapshot_interval()` / `c64d_check_snapshot_restore()` — interval-based snapshot trigger
- `c64d_check_cpu_snapshot_manager_store()` / `c64d_check_cpu_snapshot_manager_restore()` — CPU-side snapshot hooks
- `c64d_is_performing_snapshot_restore` — flag during restore
- `c64d_store_vicii_state_with_snapshot` — co-save VIC state

**Genuine non-additive changes:**
- `c64-snapshot.c`: `c64_snapshot_write()` signature changed (new params); `snapshot_create()`
  call gains new `0` param (snapshot format version). `c64_snapshot_write_module()` call gains
  three new params. A large new function `c64_snapshot_write_in_memory()` is appended.

**Hook count (distinct symbols):** ~12

**3.10 impact:** `c64/c64-snapshot.c` exists in 3.10 but with structural changes (SNAP_MAJOR
changed from 2 to 1 in 3.1→3.10? No — 3.10 uses 1.1 vs 3.1 uses 1.1 as well but with different
module list). The signature changes to `c64_snapshot_write()` must be replayed carefully.
**Risk: MEDIUM** — snapshot API evolved between versions.

---

### Category 13: Profiler

**Purpose:** Built-in CHAMP-style profiler for C64 code: JSR/RTS call stack tracking, cycle-per-
function accounting, watchpoints.

**Files involved:**
- `c64/c64cpusc.c` (profiler implementation — all under `#ifdef USE_CHAMP_PROFILER` / `#ifndef`)

**Representative symbols:**
- `c64d_profiler_init()` / `c64d_profiler_activate(file, cycles, pauseWhenDone)` / `c64d_profiler_deactivate()`
- `c64d_profiler_jsr(target)` / `c64d_profiler_rts()` — JSR/RTS tracking
- `c64d_profiler_start_handle_cpu_instruction()` / `c64d_profiler_end_cpu_instruction()` — per-instruction callbacks
- `c64d_profiler_is_active` — flag
- `c64d_profiler_cpu_total_cycles` / `c64d_profiler_cycles_per_function[]` / `c64d_profiler_calls_per_function[]` — counters
- `c64d_profiler_trace_stack[N]` / `c64d_profiler_trace_stack_pointer` / `c64d_profiler_trace_stack_function[]` — call stack
- `c64d_profiler_watches[]` / `c64d_profiler_watch_count` / `c64d_profiler_watches_allocated` / `c64d_profiler_watch_offset_for_pc_and_post()` — watchpoints
- `c64d_profiler_last_cycles` / `c64d_profiler_old_pc` — per-instruction state
- `c64d_profiler_file_out` — output FILE pointer
- `c64d_activate_profiler` — flag

**Style:** Entirely additive. All profiler code is appended to `c64cpusc.c` with `#ifdef
USE_CHAMP_PROFILER` guard. Stub implementations provided under `#ifndef USE_CHAMP_PROFILER`.

**Hook count (distinct symbols):** ~18

**3.10 impact:** Profiler is self-contained in `c64cpusc.c` additions. The injection points
(profiler calls inside the CPU instruction loop) depend on the inlined `mainc64cpu.c` structure.
3.10's `mainc64cpu.c` exists at `src/mainc64cpu.c` (same path as 3.1). **Risk: LOW** — profiler
is additive and self-contained.

---

### Category 14: System Lifecycle / Arch Callbacks

**Purpose:** Machine lifecycle hooks, display callbacks, mutex management, and arch-level
integration (JAM, speed, LED, screen refresh).

**Files involved:**
- `arch/ui.c` (5 hits)
- `arch/uimon.c` (4 hits — see Category 10)
- `arch/video.c` (1 hit — screen refresh)
- `arch/vsyncarch.c` (1 hit — speed display)
- `arch/uistatusbar.c` (1 hit — drive LED)
- `arch/uimsgbox.c` (1 hit — message display)
- `arch/mousedrv.c` (2 hits — mouse position)
- `root/vsync.c` (5 hits — vsync/sound mutex)
- `root/machine.c` (2 hits — machine pause)
- `root/sound.c` (23 hits — sound lifecycle)

**Representative symbols:**
- `c64d_set_debug_mode(mode)` — central debug mode state machine (defined in `c64cpusc.c`,
  declared/called widely)
- `c64d_debug_mode` — global mode variable
- `c64d_debug_pause_check()` — called in tight loops to check for pause requests
- `c64d_stop_vice_emulation()` — halt the emulation loop
- `c64d_vice_run_emulation()` — start/resume emulation
- `c64d_is_debug_on_c64()` / `c64d_is_debug_on_drive1541()` — mode predicates
- `c64d_lock_mutex(who)` / `c64d_unlock_mutex(who)` — main emulator mutex
- `c64d_lock_sound_mutex(who)` / `c64d_unlock_sound_mutex(who)` — sound thread mutex
- `c64d_refresh_screen()` / `c64d_refresh_screen_no_callback()` — force screen update
- `c64d_clear_screen()` — blank screen buffer
- `c64d_display_speed(speed, fps)` — update speed display
- `c64d_display_drive_led(drive, pwm1, pwm2)` — drive LED state
- `c64d_show_message(msg)` — show UI message
- `c64d_output(str)` — console output
- `c64d_console_log` — console_t routing struct
- `c64d_get_frame_num()` — frame counter
- `c64d_screen_num_skip_top_lines` — display crop control
- `c64d_setting_render_transparent_screen` — transparency flag
- `c64d_update_c64_model()` / `c64d_update_c64_machine_from_model_type()` / `c64d_update_c64_screen_height_from_model_type()` — model management
- `c64d_is_cpu_in_jam_state` — CPU JAM flag set in `arch/ui.c`

**Genuine non-additive changes in this category:**
- `root/vsync.c`: `vsync_do_vsync()` signature changed: new `isPaused` parameter;
  `vsync_hook()` call gated on `!isPaused`; `sound_flush()` call wrapped in mutex;
  `sound_flush()` itself gains `isPaused` parameter; `set_warp_mode()` had `static` removed.
- `root/sound.c`: `set_volume()` had `static` removed; `ui_error()` call replaced with
  `LOGError()` (no UI popup on sound error); `sound_open()` loop adds `// LOGD` traces but
  original loop logic unchanged.

**Hook count (distinct symbols):** ~25

**3.10 impact:** `arch/` files are slajerek's own (no vanilla counterpart). `root/vsync.c` maps
to `vsync.c` in 3.10 at root level. `vsync_do_vsync()` signature change must be tracked in
3.10's version. `root/sound.c` maps to `sound.c` in 3.10 (still at root, still present).
**Risk: MEDIUM** — vsync and sound changes are sensitive to 3.10 API changes.

---

### Category 15: ROM / KERNAL Patching

**Purpose:** Fast-boot KERNAL patch and ROM update hooks.

**Files involved:**
- `c64/c64memsc.c` (2 hits — patch/unpatch implementation)
- `c64/c64memrom.c` (1 hit — ROM update)
- `c64/patchrom.c` (7 hits — trigger patching on ROM refresh)

**Representative symbols:**
- `c64d_patch_kernal_fast_boot()` — patch KERNAL ROM for fast boot
- `c64d_un_patch_kernal_fast_boot()` — restore KERNAL ROM
- `c64d_patch_kernal_fast_boot_flag` — enable/disable flag
- `c64d_update_rom()` — copy ROM to trap ROM copy after ROM load
- `c64d_trigger_update_roms()` / `c64d_update_roms` — trigger ROM refresh
- `c64d_update_rom` — callback registered with ViceInterface

**Style:** Additive. ROM patch calls are inserted in `patchrom.c` around the existing
`c64memrom_patch_kernal()` logic.

**Hook count (distinct symbols):** ~6

**3.10 impact:** `c64/patchrom.c` and `c64/c64memrom.c` exist in 3.10. **Risk: LOW**.

---

## Non-Additive Patches Table

The following files have genuine logic changes beyond header-rename and `static`-removal noise.

| File | Genuine deletions | What changed | Inferred why |
|------|-------------------|--------------|--------------|
| `c64/c64cpusc.c` | Structural | `#include "../mainc64cpu.c"` replaced by inlined content + hooks | Required to inject hooks at exact points in instruction fetch/execute loop; can't inject into a separately compiled TU |
| `drive/drivecpu.c` | Structural | `#include "6510core.c"` replaced by inlined content + hooks | Same reason as c64cpusc.c — drive 6510 needed per-cycle hooks |
| `sounddrv/soundsdl.c` | ~100 lines | SDL callback completely rewritten for mono/stereo/triple SID support | Needed multi-channel audio routing for SID waveform capture |
| `resid/resid-filter.cpp` | ~261 lines | 8580 SID filter section deleted; only 6581 filter kept | slajerek targets only 6581 emulation (ReSID-FP handles 8580 path) |
| `resid/resid-sid.cpp` | ~89 lines | `SID::read()` POT/OSC/ENV path bypasses `bus_value_ttl` update; write pipeline behavior changed; `SID()` constructor gains `c64d_waveform_callback` parameter | Waveform callback injection; TTL behavior simplification |
| `c64/c64memsc.c` | 1 line | `BYTE mem_ram[C64_RAM_SIZE]` → `BYTE *mem_ram` (pointer externally set) | Allows memory-mapped file backing for the RAM buffer |
| `sid/sid.c` | ~15 lines | `sid_read()` restructured to always capture return value before returning (added else-if chain); `sid_sound_machine_open()` gains `chipno` parameter; SID engine open call updated | Needed to observe the actual returned SID read value |
| `sid/sid-snapshot.c` | ~15 lines | Snapshot write gated on `c64d_get_sid_enable()`; mutex wraps around `sound_open()` call | Prevent crash when SID is disabled; thread safety for snapshot |
| `root/vsync.c` | ~9 lines | `vsync_do_vsync()` gains `isPaused` parameter; `vsync_hook()` gated; `sound_flush()` gains `isPaused` param | Suppress vsync side-effects (sound events) when emulator is paused |
| `root/keyboard.c` | ~15 lines | `keyboard_key_pressed()` return type `void` → `int` | Return value needed by ViceInterface to check if key was accepted |
| `joyport/joystick.c` | ~3 lines | `joystick_process_latch()` bypass of random delay | Avoid timing non-determinism in debugger-driven key injection |
| `joyport/mouse.c` | ~2 lines | `joyport_mouse_enable()` / `set_mouse_enabled()` static removal | ViceInterface needs to call these directly |
| `c64/c64-snapshot.c` | ~9 lines | `c64_snapshot_write()` signature extended; `snapshot_create()` call updated; large new `c64_snapshot_write_in_memory()` function added | Extended snapshot capability needed by state manager |
| `viciisc/vicii-draw-cycle.c` | ~14 lines | Sprite draw block wrapped in `c64d_skip_drawing_sprites` condition | Allow suppressing sprites in debugger rendering |
| `viciisc/viciitypes.h` | ~5 lines | New struct fields added to `vicii_t`; some debug `#define` guards changed from `/* #define */` to active/commented | Need per-cycle register observation fields |

**Note:** Files with only header-rename (`types.h` → `vicetypes.h`) and `static` removal are NOT
listed here. Those changes affect nearly all 64 modified files. They must be replayed as a
mechanical step during Phase 4.

---

## Top Hotspot Files

| Rank | File | `c64d_` hits | Primary categories |
|------|----|---|---|
| 1 | `c64/c64cpusc.c` | 283 | CPU Step, Memory R/W taps, Profiler, Snapshot, Lifecycle |
| 2 | `drive/drivecpu.c` | 97 | Drive State Observer |
| 3 | `c64/c64memsc.c` | 34 | Memory Read/Write taps |
| 4 | `iecbus/iecbus.c` | 25 | Drive State Observer (wrapper dispatch) |
| 5 | `root/sound.c` | 23 | SID Observer, Sound Driver |
| 6 | `drive/iec/memiec.c` | 18 | Drive State Observer (IEC memory) |
| 7 | `sid/sid.c` | 17 | SID Observer |
| 8 | `c64/c64cia2.c` | 13 | CIA/VIA Observer |
| 9 | `drive/iec/via1d1541.c` | 12 | Drive State Observer (VIA-1) |
| 10 | `drive/drive.c` | 12 | Drive State Observer (GCR dirty) |
| 11 | `sid/sid-snapshot.c` | 10 | SID Observer, Snapshot |
| 12 | `resid-fp/residfp-sid.cpp` | 10 | SID Observer (ReSID-FP) |
| 13 | `joyport/joystick.c` | 10 | Keyboard & Input |
| 14 | `drive/iecieee/via2d.c` | 10 | Drive State Observer (VIA-2) |
| 15 | `viciisc/vicii.c` | 8 | VIC-II State Observer |

---

## Per-Directory Summary

| Count | Directory | Primary category |
|-------|-----------|-----------------|
| 12 | `root/` | CPU clock, sound, vsync, keyboard |
| 10 | `drive/` | Drive State Observer |
| 9 | `c64/` | CPU step, memory, CIA, snapshot, ROM |
| 8 | `arch/` | System lifecycle callbacks |
| 5 | `viciisc/` | VIC-II State Observer |
| 5 | `sid/` | SID Observer |
| 4 | `resid/` | SID Observer (ReSID backend) |
| 3 | `resid-fp/` | SID Observer (ReSID-FP backend) |
| 3 | `core/` | CIA/VIA Observer (ciacore, viacore, flash040) |
| 2 | `joyport/` | Keyboard & Input |
| 1 | `sounddrv/` | Sound Driver (SDL) |
| 1 | `monitor/` | Monitor Integration |
| 1 | `iecbus/` | Drive State Observer (wrapper) |

---

## Symbol-to-File Index

All 282 distinct `c64d_*` symbols. Format: **symbol** — defined in `file` — called in `files`.
Where a symbol is a global variable (not a function), "defined" means the declaration with initializer.

### CPU / Clock

| Symbol | Defined in | Called/referenced in |
|--------|-----------|---------------------|
| `c64d_maincpu_clk` | `c64/c64cpusc.c` | `c64/cart/reu.c`, `core/flash040core.c`, `root/dma.c`, `root/midi.c`, `root/interrupt.h`, `root/maincpu.h`, `viciisc/vicii-cycle.c`, `sid/sid.c`, `c64/c64cpusc.c` (CLK_INC macro) |
| `c64d_maincpu_current_instruction_clk` | `c64/c64cpusc.c` | `c64/c64cpusc.c` |
| `c64d_maincpu_previous_instruction_clk` | `c64/c64cpusc.c` | `c64/c64cpusc.c` |
| `c64d_maincpu_previous2_instruction_clk` | `c64/c64cpusc.c` | `c64/c64cpusc.c` |
| `c64d_c64_instruction_cycle` | `c64/c64cpusc.c` | `c64/c64cpusc.c` |
| `c64d_c64_do_cycle` | `c64/c64cpusc.c` | `c64/c64cpusc.c` (CLK_INC macro) |
| `c64d_do_cycle` | `drive/drivecpu.c` | `drive/drivecpu.c` |
| `c64d_get_maincpu_clock` | `c64/c64cpusc.c` | ViceInterface |
| `c64d_get_vice_maincpu_clk` | `c64/c64cpusc.c` | ViceInterface |
| `c64d_get_vice_maincpu_current_instruction_clk` | `c64/c64cpusc.c` | ViceInterface |
| `c64d_reset_counters` | `c64/c64cpusc.c` | ViceInterface |
| `c64d_reset_sound_clk` | `root/sound.c` | ViceInterface |

### CPU Registers / Control

| Symbol | Defined in | Called/referenced in |
|--------|-----------|---------------------|
| `c64d_get_maincpu_regs` | `c64/c64cpusc.c` | ViceInterface |
| `c64d_set_maincpu_regs` | `c64/c64cpusc.c` | ViceInterface |
| `c64d_set_maincpu_regs_no_trap` | `c64/c64cpusc.c` | ViceInterface |
| `c64d_set_c64_pc` | `c64/c64cpusc.c` | ViceInterface |
| `c64d_set_maincpu_set_a/x/y/p/sp` | `c64/c64cpusc.c` | ViceInterface |
| `c64d_maincpu_make_basic_run` | `c64/c64cpusc.c` | ViceInterface |
| `c64d_get_drivecpu_regs` | `iecbus/iecbus.c` | ViceInterface |
| `c64d_get_drivecpu_regs_internal` | `drive/drivecpu.c` | `iecbus/iecbus.c` |
| `c64d_set_drivecpu_pc_no_trap` | `drive/drivecpu.c` | ViceInterface |
| `c64d_set_drivecpu_regs_no_trap` | `drive/drivecpu.c` | ViceInterface |
| `c64d_set_drive_register_a/x/y/p/sp` | `drive/drivecpu.c` | ViceInterface |
| `c64d_set_drive_pc` | `drive/drivecpu.c` | ViceInterface |
| `c64d_is_cpu_in_jam_state` | `arch/ui.c` | ViceInterface |
| `c64d_interrupt_drivecpu_trigger_trap` | `drive/drivecpu.c` | ViceInterface |

### Debug Mode / Lifecycle

| Symbol | Defined in | Called/referenced in |
|--------|-----------|---------------------|
| `c64d_set_debug_mode` | `c64/c64cpusc.c` | `monitor/monitor.c`, `arch/ui.c`, `root/machine.c`, `root/midi.c` |
| `c64d_debug_mode` | `c64/c64cpusc.c` | `root/sound.c`, `viciisc/vicii-draw-cycle.c`, `resid/resid-sid.cpp` |
| `c64d_debug_pause_check` | `c64/c64cpusc.c` | ViceInterface |
| `c64d_stop_vice_emulation` | `c64/c64cpusc.c` | ViceInterface |
| `c64d_vice_run_emulation` | ViceInterface def | Called from ViceInterface |
| `c64d_is_debug_on_c64` | `c64/c64cpusc.c` | `drive/iec/via1d1541.c` |
| `c64d_is_debug_on_drive1541` | `drive/drivecpu.c` | `drive/iec/via1d1541.c` |
| `c64d_shutdown_vice` | `c64/c64cpusc.c` | ViceInterface |
| `c64d_get_warp_mode` | `root/vsync.c` | ViceInterface |
| `c64d_get_frame_num` | ViceInterface def | ViceInterface |
| `c64d_lock_mutex` / `c64d_unlock_mutex` | ViceInterface def | `joyport/joystick.c` |
| `c64d_lock_sound_mutex` / `c64d_unlock_sound_mutex` | ViceInterface def | `root/vsync.c`, `sid/sid-snapshot.c` |

### Memory Access — C64

| Symbol | Defined in | Called/referenced in |
|--------|-----------|---------------------|
| `c64d_mark_c64_cell_read` | ViceInterface def | `c64/c64cpusc.c` |
| `c64d_mark_c64_cell_write` | ViceInterface def | `c64/c64cpusc.c` |
| `c64d_mark_c64_cell_execute` | ViceInterface def | `c64/c64cpusc.c` |
| `c64d_init_memory` | `c64/c64memsc.c` | ViceInterface |
| `c64d_peek_c64` | `c64/c64memsc.c` | ViceInterface, `c64/c64cpusc.c` |
| `c64d_peek_c64_for_cycle` | `c64/c64memsc.c` | ViceInterface |
| `c64d_peek_io` | `c64/c64memsc.c` | ViceInterface |
| `c64d_peek_memory0001` | `c64/c64memsc.c` | ViceInterface |
| `c64d_peek_memory_c64` | `c64/c64memsc.c` | ViceInterface |
| `c64d_peek_whole_map_c64` | `c64/c64memsc.c` | ViceInterface |
| `c64d_copy_ram_memory_c64` | `c64/c64memsc.c` | ViceInterface |
| `c64d_copy_whole_mem_ram_c64` | `c64/c64memsc.c` | ViceInterface |
| `c64d_mem_ram_read_c64` | `c64/c64memsc.c` | ViceInterface |
| `c64d_mem_ram_write_c64` | `c64/c64memsc.c` | ViceInterface |
| `c64d_mem_ram_fill_c64` | `c64/c64memsc.c` | ViceInterface |
| `c64d_mem_write_c64` | `c64/c64cpusc.c` | ViceInterface |
| `c64d_mem_write_c64_no_mark` | `c64/c64cpusc.c` | ViceInterface, `c64/c64cpusc.c` |
| `c64d_mem_store` / `c64d_mem_store_zero` | `c64/c64cpusc.c` | ViceInterface |
| `c64d_mem_access_addr` / `c64d_mem_access_value` / `c64d_mem_access_is_write` / `c64d_mem_access_watch` | `c64/c64cpusc.c` | ViceInterface |
| `c64d_peek_cart_roml` | `c64/cart/c64-generic.c` | ViceInterface |
| `c64d_poke_cart_roml` | `c64/cart/c64-generic.c` | ViceInterface |
| `c64d_colorram_peek` | `c64/c64memsc.c` | ViceInterface |
| `c64d_get_exrom_game` | `c64/c64memsc.c` | ViceInterface |
| `c64d_get_ultimax_phi` | `c64/c64memsc.c` | ViceInterface |

### Memory Access — Drive

| Symbol | Defined in | Called/referenced in |
|--------|-----------|---------------------|
| `c64d_mark_drive1541_cell_read` | ViceInterface def | `drive/iec/memiec.c`, `drive/drivemem.c`, `drive/iecieee/via2d.c`, `drive/iec/via1d1541.c` |
| `c64d_mark_drive1541_cell_write` | ViceInterface def | `drive/iec/memiec.c`, `drive/drivemem.c`, `drive/iecieee/via2d.c`, `drive/iec/via1d1541.c` |
| `c64d_mark_drive1541_cell_execute` | ViceInterface def | `drive/drivecpu.c` |
| `c64d_peek_drive` | `iecbus/iecbus.c` | ViceInterface |
| `c64d_peek_mem_drive_internal` | `drive/iec/memiec.c` | `iecbus/iecbus.c` |
| `c64d_peek_memory_drive` | `iecbus/iecbus.c` | ViceInterface |
| `c64d_peek_memory_drive_internal` | `drive/iec/memiec.c` | `iecbus/iecbus.c` |
| `c64d_peek_whole_map_drive` | `iecbus/iecbus.c` | ViceInterface |
| `c64d_peek_whole_map_drive_internal` | `drive/iec/memiec.c` | `iecbus/iecbus.c` |
| `c64d_copy_ram_memory_drive` | `iecbus/iecbus.c` | ViceInterface |
| `c64d_copy_ram_memory_drive_internal` | `drive/iec/memiec.c` | `iecbus/iecbus.c` |
| `c64d_copy_mem_ram_drive` | `iecbus/iecbus.c` | ViceInterface |
| `c64d_copy_mem_ram_drive_internal` | `drive/iec/memiec.c` | `iecbus/iecbus.c` |
| `c64d_mem_ram_read_drive` | `iecbus/iecbus.c` | ViceInterface |
| `c64d_mem_ram_read_drive_internal` | `drive/iec/memiec.c` | `iecbus/iecbus.c` |
| `c64d_mem_ram_write_drive` | `iecbus/iecbus.c` | ViceInterface |
| `c64d_mem_ram_write_drive_internal` | `drive/iec/memiec.c` | `iecbus/iecbus.c` |
| `c64d_drive_poke` | `iecbus/iecbus.c` | ViceInterface |
| `c64d_iecbus_cpu_peek_conf1` | `iecbus/iecbus.c` | `core/ciacore.c` |

### Drive State

| Symbol | Defined in | Called/referenced in |
|--------|-----------|---------------------|
| `c64d_get_drive_context` | `drive/drivecpu.c` | ViceInterface |
| `c64d_get_drive_disk_gcr` | `drive/drivecpu.c` | ViceInterface |
| `c64d_get_drive_disk_image` | `drive/drivecpu.c` | ViceInterface |
| `c64d_get_drive_is_disk_attached` | `drive/drivecpu.c` | ViceInterface |
| `c64d_get_drive_current_halftrack` | `drive/drivecpu.c` | ViceInterface |
| `c64d_set_drive_half_track` | `drive/drivecpu.c` | ViceInterface |
| `c64d_read_disk_image` | `drive/drivecpu.c` | ViceInterface |
| `c64d_set_drive_disk_memory` | `drive/drivecpu.c` | ViceInterface |
| `c64d_mark_drive1541_contents_track_dirty` | `drive/rotation.c` | `drive/rotation.c` |
| `c64d_is_drive_dirty_for_snapshot` | `drive/drivecpu.c` | ViceInterface |
| `c64d_is_drive_dirty_and_needs_refresh` | `drive/drivecpu.c` | ViceInterface |
| `c64d_clear_drive_dirty_for_snapshot` | `drive/drivecpu.c` | ViceInterface |
| `c64d_clear_drive_dirty_needs_refresh_flag` | `drive/drivecpu.c` | ViceInterface |
| `c64d_set_drive_dirty_needs_refresh_flag` | `drive/drivecpu.c` | ViceInterface |
| `c64d_disk_image_destroy` | `drive/drivecpu.c` | ViceInterface |
| `c64d_drive_writeprotect_sense_peek` | `drive/drive-writeprotect.c` | `drive/iecieee/via2d.c` |
| `c64d_drive_cpu_stack_entry_types` / `c64d_drive_cpu_stack_irq_sources` / `c64d_drive_cpu_stack_origin_pc` | `drive/drivecpu.c` | ViceInterface |
| `c64d_drive_irqbrk_irq_source` | `drive/drivecpu.c` | ViceInterface |
| `c64d_drive1541_check_pc_breakpoint` | `drive/drivecpu.c` | ViceInterface |
| `c64d_drive1541_check_irqvia1_breakpoint` / `c64d_drive1541_check_irqvia2_breakpoint` / `c64d_drive1541_check_irqiec_breakpoint` | `drive/drivecpu.c` | ViceInterface |
| `c64d_drive1541_is_checking_irq_breakpoints_enabled` | `drive/drivecpu.c` | `drive/drivecpu.c` |

### CIA / VIA / VIC-II Observer

| Symbol | Defined in | Called/referenced in |
|--------|-----------|---------------------|
| `c64d_cia1_peek` / `c64d_cia2_peek` | `c64/c64cia2.c` | `c64/c64memsc.c`, ViceInterface |
| `c64d_ciacore_peek` | `core/ciacore.c` | `c64/c64cia2.c` |
| `c64d_cia1_register_read` / `c64d_cia1_read_value` | `c64/c64cpusc.c` (extern) | `c64/c64cia1.c` (set), ViceInterface (read) |
| `c64d_cia1_register_written` / `c64d_cia1_write_value` | `c64/c64cpusc.c` (extern) | `c64/c64cia1.c` (set), ViceInterface (read) |
| `c64d_cia2_register_read/written` / `c64d_cia2_read/write_value` | `c64/c64cpusc.c` (extern) | `c64/c64cia2.c` (set), ViceInterface (read) |
| `c64d_get_cia_context` | `core/ciacore.c` | ViceInterface |
| `c64d_refresh_cia` | ViceInterface def | — |
| `c64d_viacore_peek` | `core/viacore.c` | `drive/iec/via1d1541.c`, `drive/iecieee/via2d.c` |
| `c64d_via1d1541_peek` | `drive/iec/via1d1541.c` | `drive/iec/memiec.c` |
| `c64d_peek_via1d1541_pra/prb` | `drive/iec/via1d1541.c` | `drive/iec/via1d1541.c` |
| `c64d_via2d_peek` | `drive/iecieee/via2d.c` | `drive/iec/memiec.c` |
| `c64d_peek_via2d_pra/prb` | `drive/iecieee/via2d.c` | `drive/iecieee/via2d.c` |
| `c64d_peek_via_context` / `c64d_viacore_peek` | `core/viacore.c` | various via peek functions |
| `c64d_c64_check_irqcia_breakpoint` / `c64d_c64_check_irqnmi_breakpoint` / `c64d_c64_check_irqvic_breakpoint` | `c64/c64cpusc.c` | ViceInterface |
| `c64d_c64_is_checking_irq_breakpoints_enabled` | `c64/c64cpusc.c` | `c64/c64cpusc.c` |
| `c64d_irq_flag` (struct field on `cia_context_t`) | `root/cia.h` | `core/ciacore.c`, ViceInterface |
| `c64d_irq_flagged` (struct field on `via_context_t`) | `root/via.h` | `drive/iec/via1d1541.c` |
| `c64d_irqbrk_irq_source` / `c64d_irqbrk_entry_type_base` | `c64/c64cpusc.c` | ViceInterface |
| `c64d_main_cpu_stack_entry_types` / `c64d_main_cpu_stack_irq_sources` / `c64d_main_cpu_stack_origin_pc` | `c64/c64cpusc.c` | ViceInterface |
| `c64d_irq_flag` (VIC) | `viciisc/viciitypes.h` field | `viciisc/vicii-irq.c` (set), ViceInterface (read) |
| `c64d_irq_flag_needs_clear` / `c64d_irqbrk_irq_source` (VIC) | `c64/c64cpusc.c` | ViceInterface |
| `c64d_c64_check_raster_breakpoint` / `c64d_c64_check_pc_breakpoint` | `c64/c64cpusc.c` | ViceInterface |

### VIC-II State

| Symbol | Defined in | Called/referenced in |
|--------|-----------|---------------------|
| `c64d_c64_vicii_cycle` | ViceInterface def | `viciisc/vicii-cycle.c` |
| `c64d_c64_vicii_start_frame` | ViceInterface def | `viciisc/vicii-cycle.c` |
| `c64d_c64_vicii_start_raster_line` | ViceInterface def | `viciisc/vicii-cycle.c` |
| `c64d_c64_set_vicii_record_state_mode` | ViceInterface def | ViceInterface |
| `c64d_vicii_record_state_mode` | ViceInterface def | `viciisc/vicii-cycle.c` (extern) |
| `c64d_c64_vicii_store_state_to_bytebuffer` | ViceInterface def | ViceInterface |
| `c64d_c64_vicii_restore_state_from_bytebuffer` | ViceInterface def | ViceInterface |
| `c64d_get_vicii_state_for_raster_cycle` / `c64d_get_vicii_state_for_raster_line` | ViceInterface def | ViceInterface |
| `c64d_vicii_copy_state` / `c64d_vicii_copy_state_data` | ViceInterface def | ViceInterface |
| `c64d_set_color_register` | `viciisc/vicii-draw-cycle.c` | ViceInterface |
| `c64d_get_vic_colors` | ViceInterface def | ViceInterface |
| `c64d_get_vic_simple_state` | ViceInterface def | ViceInterface |
| `c64d_get_vicii_border_mode` / `c64d_set_vicii_border_mode` | ViceInterface def | ViceInterface |
| `c64d_fetch_phi1_type` | `viciisc/vicii.c` | ViceInterface |
| `c64d_vicii_render_when_paused` | `viciisc/vicii.c` | ViceInterface |
| `c64d_vicii_get_geometry` | `viciisc/vicii.c` | ViceInterface |
| `c64d_skip_drawing_sprites` | ViceInterface def | `viciisc/vicii-draw-cycle.c` |
| `c64d_refresh_dbuf` | ViceInterface def | `viciisc/vicii-draw-cycle.c` |
| `c64d_refresh_previous_lines` | ViceInterface def | ViceInterface |
| `c64d_refresh_screen` / `c64d_refresh_screen_no_callback` | ViceInterface def | `arch/video.c` |
| `c64d_screen_num_skip_top_lines` | ViceInterface def | (extern) |
| `c64d_setting_render_transparent_screen` | ViceInterface def | (extern) |

### SID / Sound

| Symbol | Defined in | Called/referenced in |
|--------|-----------|---------------------|
| `c64d_sid_register_read` / `c64d_sid_read_value` | `c64/c64cpusc.c` (extern) | `sid/sid.c` (set), ViceInterface (read) |
| `c64d_sid_register_written` / `c64d_sid_write_value` | `c64/c64cpusc.c` (extern) | `sid/sid.c` (set), ViceInterface (read) |
| `c64d_sid_voiceMask` | `sid/sid.c` | `sid/sid.c`, ViceInterface |
| `c64d_store_sid_data` | `sid/sid.c` | ViceInterface |
| `c64d_get_sid_enable` | `sid/sid-snapshot.c` | `sid/sid-snapshot.c` |
| `c64d_sid_set_engine_model_direct` | ViceInterface def | ViceInterface |
| `c64d_sid_set_emulate_filters` / `c64d_sid_set_sampling_method` / `c64d_sid_set_passband` / `c64d_sid_set_filter_bias` | ViceInterface def | ViceInterface |
| `c64d_sid_set_stereo` / `c64d_sid_set_stereo_address` / `c64d_sid_set_triple_address` | ViceInterface def | ViceInterface |
| `c64d_is_receive_channels_data` | ViceInterface def | `sid/fastsid.c`, `resid/resid-sid.cpp`, `resid-fp/residfp-sid.cpp` |
| `c64d_sid_channels_data` | ViceInterface def | `sid/fastsid.c`, `resid/resid-sid.cpp`, `resid-fp/residfp-sid.cpp` |
| `c64d_waveform_callback` | ViceInterface def | `resid/resid-filter.h`, `resid/resid-sid.cpp`, `resid-fp/residfp-sid.cpp` |
| `c64d_wave_attenuation` / `c64d_wave_shift` | ViceInterface def | `resid-fp/residfp-sid.cpp` |
| `c64d_output` (Voice field) | `resid-fp/residfp-voice.h` | `resid-fp/residfp-sid.cpp` |
| `c64d_sound_init` / `c64d_sound_pause` / `c64d_sound_resume` | ViceInterface def | ViceInterface |
| `c64d_sound_run_sound_when_paused` | ViceInterface def | `root/sound.c` |
| `c64d_skip_sound_run_sound_in_sound_store` | ViceInterface def | `root/sound.c` |
| `c64d_setting_run_sid_emulation` / `c64d_setting_run_sid_when_in_warp` | ViceInterface def | `root/sound.c` |
| `c64d_set_volume` | `root/sound.c` | ViceInterface |
| `c64d_store_vicii_state_with_snapshot` | ViceInterface def | `root/sound.c` |
| `c64d_sid_read_value` | `c64/c64cpusc.c` (extern) | `sid/sid.c`, ViceInterface |

### Snapshot / State Manager

| Symbol | Defined in | Called/referenced in |
|--------|-----------|---------------------|
| `c64d_snapshot_write_module` / `c64d_snapshot_read_module` | ViceInterface def | `c64/c64-snapshot.c` |
| `c64d_start_frame_for_snapshots_manager` | ViceInterface def | `c64/c64cpusc.c` |
| `c64d_check_snapshot_interval` | ViceInterface def | `c64/c64cpusc.c` |
| `c64d_check_snapshot_restore` | ViceInterface def | `c64/c64cpusc.c` |
| `c64d_check_cpu_snapshot_manager_store` / `c64d_check_cpu_snapshot_manager_restore` | `c64/c64cpusc.c` | ViceInterface |
| `c64d_is_performing_snapshot_restore` | ViceInterface def | `c64/c64cpusc.c` (extern) |

### Profiler

| Symbol | Defined in | Called/referenced in |
|--------|-----------|---------------------|
| `c64d_profiler_init` / `c64d_activate_profiler` | `c64/c64cpusc.c` | ViceInterface |
| `c64d_profiler_activate` / `c64d_profiler_deactivate` | `c64/c64cpusc.c` | ViceInterface |
| `c64d_profiler_jsr` / `c64d_profiler_rts` | `c64/c64cpusc.c` | `c64/c64cpusc.c` |
| `c64d_profiler_start_handle_cpu_instruction` / `c64d_profiler_end_cpu_instruction` | `c64/c64cpusc.c` | `c64/c64cpusc.c` |
| `c64d_profiler_is_active` / `c64d_profiler_file_out` / `c64d_profiler_cpu_total_cycles` | `c64/c64cpusc.c` | `c64/c64cpusc.c` |
| `c64d_profiler_cycles_per_function` / `c64d_profiler_calls_per_function` | `c64/c64cpusc.c` | ViceInterface |
| `c64d_profiler_trace_stack` / `c64d_profiler_trace_stack_pointer` / `c64d_profiler_trace_stack_function` | `c64/c64cpusc.c` | `c64/c64cpusc.c` |
| `c64d_profiler_watches` / `c64d_profiler_watch_count` / `c64d_profiler_watches_allocated` | `c64/c64cpusc.c` | `c64/c64cpusc.c` |
| `c64d_profiler_watch_offset_for_pc_and_post` | `c64/c64cpusc.c` | `c64/c64cpusc.c` |
| `c64d_profiler_last_cycles` / `c64d_profiler_old_pc` | `c64/c64cpusc.c` | `c64/c64cpusc.c` |

### System Lifecycle / Display

| Symbol | Defined in | Called/referenced in |
|--------|-----------|---------------------|
| `c64d_display_speed` | ViceInterface def | `arch/vsyncarch.c` |
| `c64d_display_drive_led` | ViceInterface def | `arch/uistatusbar.c` |
| `c64d_show_message` | ViceInterface def | `arch/ui.c`, `arch/uimsgbox.c` |
| `c64d_output` | ViceInterface def | `arch/ui.c` |
| `c64d_console_log` | `arch/uimon.c` | `arch/uimon.c` |
| `c64d_uimon_buf` / `c64d_uimon_bufpos` | ViceInterface def | `arch/uimon.c` |
| `c64d_uimon_print` / `c64d_uimon_print_line` | ViceInterface def | `arch/uimon.c` |
| `c64d_clear_screen` | ViceInterface def | ViceInterface |
| `c64d_update_c64_model` / `c64d_update_c64_machine_from_model_type` / `c64d_update_c64_screen_height_from_model_type` | ViceInterface def | ViceInterface |
| `c64d_update_rom` / `c64d_update_roms` / `c64d_trigger_update_roms` | `c64/c64memrom.c`, `c64/c64cpusc.c` | ViceInterface |

### Keyboard / Input / Mouse

| Symbol | Defined in | Called/referenced in |
|--------|-----------|---------------------|
| `c64d_keyboard_init` | `root/keyboard.c` | ViceInterface |
| `c64d_keyboard_keymap_clear` | `root/keyboard.h` | ViceInterface |
| `c64d_keyboard_key_down_latch` / `c64d_keyboard_key_up_latch` / `c64d_keyboard_force_key_up_latch` | ViceInterface def | ViceInterface, `root/keyboard.c` |
| `c64d_joystick_key_down` / `c64d_joystick_key_up` | `joyport/joystick.c` | ViceInterface |
| `c64d_joystick_latch_matrix_workaround` | ViceInterface def | (extern flag) |
| `c64d_mouse_set_type` / `c64d_mouse_type_to_joyportid` | `joyport/mouse.c` | ViceInterface |
| `c64d_mouse_set_position` | `arch/mousedrv.c` | ViceInterface |
| `c64d_set_mouse_pos` | ViceInterface def | ViceInterface |

### ROM Patching

| Symbol | Defined in | Called/referenced in |
|--------|-----------|---------------------|
| `c64d_patch_kernal_fast_boot` / `c64d_un_patch_kernal_fast_boot` | `c64/c64memsc.c` | `c64/patchrom.c` |
| `c64d_patch_kernal_fast_boot_flag` | ViceInterface def | `c64/patchrom.c`, `c64/c64memsc.c` |
| `c64d_prev_val_of_FD84` / `c64d_prev_val_of_FD85` | ViceInterface def | (extern, snapshot state) |

### REU / DMA

| Symbol | Defined in | Called/referenced in |
|--------|-----------|---------------------|
| `c64d_reu_io2_store` | `c64/cart/reu.c` | ViceInterface |
| `c64d_maincpu_clk` (REU bump) | (call site) | `c64/cart/reu.c`, `core/flash040core.c`, `root/dma.c`, `root/midi.c` |

### Palette / Colors

| Symbol | Defined in | Called/referenced in |
|--------|-----------|---------------------|
| `c64d_palette_red` / `c64d_palette_green` / `c64d_palette_blue` | ViceInterface def | ViceInterface |
| `c64d_float_palette_red` / `c64d_float_palette_green` / `c64d_float_palette_blue` | ViceInterface def | ViceInterface |
| `c64d_set_palette` / `c64d_set_palette_vice` | ViceInterface def | ViceInterface |
| `c64d_set_color_register` | `viciisc/vicii-draw-cycle.c` | ViceInterface |

---

## 3.10 Migration Risk Assessment

Ranked by estimated replay difficulty:

### Critical (replay requires significant investigation)

1. **CPU inline expansion (`c64cpusc.c`, `drivecpu.c`) — Risk: CRITICAL**
   - Both files use a unique pattern: vanilla includes `mainc64cpu.c` / `6510core.c` at end.
     Slajerek inlined the included file and inserted hooks throughout.
   - 3.10 maintains the same `#include` pattern (`c64cpusc.c` = 192 lines, includes
     `../mainc64cpu.c`). The 3.10 versions of both included files differ from 3.1.
   - Phase 4 must: (a) inline 3.10's `mainc64cpu.c` into 3.10's `c64cpusc.c`, (b) diff slajerek's
     expanded version against the 3.1 expansion to identify insertion points, (c) map those
     insertion points to their equivalents in the 3.10 expansion.
   - `clkguard.c/.h` is referenced by `c64cpusc.c` and `drivecpu.c` in the slajerek tree but
     is **removed from 3.10**. All `clkguard` usage must be eliminated or replaced.

2. **`soundsdl.c` rewrite — Risk: HIGH**
   - Moved to `arch/shared/sounddrv/soundsdl.c` in 3.10.
   - 3.10 version differs from both vanilla 3.1 and slajerek's rewrite.
   - The rewrite must be replayed against 3.10's version (which is closer to vanilla 3.1 mono
     callback style) at the new path.

3. **Monitor (`monitor/monitor.c`) — Risk: HIGH**
   - 3.10's monitor is 3421 lines vs 3.1's 2389 lines (+1000 lines of new features).
   - `static` promotions (`make_prompt`, `monitor_open`, etc.) and `c64d_set_debug_mode` injection
     points must be re-identified in substantially reworked code.

### High (require careful line-by-line mapping)

4. **`c64memsc.c` `mem_ram` pointer change — Risk: HIGH**
   - 3.10 retains `uint8_t mem_ram[C64_RAM_SIZE]` as a static array. Slajerek's pointer-based
     approach must be re-evaluated or the memory-mapped-file approach reimplemented for 3.10.

5. **ReSID backend (`resid/resid-sid.cpp`, `resid/resid-filter.cpp`) — Risk: MEDIUM-HIGH**
   - 8580 filter deletion, constructor signature change, behavioral changes to `read()` and
     `write()`. 3.10's resid/ is unchanged from 3.1 (same files, same structure). The deletions
     and behavioral changes must be replayed exactly.

6. **Snapshot system (`c64/c64-snapshot.c`) — Risk: MEDIUM-HIGH**
   - 3.10's `c64-snapshot.c` (154 lines) differs structurally from 3.1 (149 lines).
     The extended `c64_snapshot_write()` signature and new `c64_snapshot_write_in_memory()` must
     be replayed against a changed base.

### Medium (structural changes but file paths stable)

7. **VIC-II hooks (`viciisc/vicii-cycle.c`, `vicii-draw-cycle.c`, `vicii.c`) — Risk: MEDIUM**
   - Files exist in 3.10 at same paths. Insertion points are in the raster cycle loop; must
     verify loop structure unchanged in 3.10.

8. **SID observation (`sid/sid.c`, `sid-snapshot.c`) — Risk: MEDIUM**
   - Files exist in 3.10 at same paths. The `sid_read()` restructuring is subtle and must be
     verified against 3.10's SID read logic.

9. **vsync (`root/vsync.c`) — Risk: MEDIUM**
   - 3.10's `vsync.c` likely differs; `vsync_do_vsync()` signature change must be replayed.
     `sound_flush()` also gains `isPaused` parameter which may conflict with 3.10's sound API.

### Low (additive, file paths stable)

10. **CIA/VIA observer (`core/ciacore.c`, `c64cia1/2.c`) — Risk: LOW-MEDIUM**
11. **Keyboard/Input (`root/keyboard.c`, `joyport/joystick.c`) — Risk: LOW-MEDIUM**
12. **ROM patching (`c64/patchrom.c`, `c64/c64memrom.c`) — Risk: LOW**
13. **Profiler (all in `c64cpusc.c`) — Risk: LOW** (once CPU inline expansion is done)
14. **REU DMA taps (`c64/cart/reu.c`) — Risk: LOW**
15. **Drive memory access marking — Risk: LOW** (additive insertions, stable file paths)

---

## Phase 2 Design Seed: Proposed `VICE_HOOK_*` Macro Families

Based on the category analysis above, Phase 2 should define the following macro families to
replace the inline `c64d_*` calls with version-portable hooks:

### `VICE_HOOK_CLK_INC()`
Wraps the `CLK_INC()` macro expansion in both `c64cpusc.c` and `drivecpu.c`.  
Replaces: `c64d_maincpu_clk++`, `c64d_c64_do_cycle()` call in CLK_INC.  
Note: will need two variants — `VICE_HOOK_CLK_INC_C64()` and `VICE_HOOK_CLK_INC_DRIVE()`.

### `VICE_HOOK_CPU_INSTRUCTION_START(pc, opcode)`
Called at the start of each 6510 instruction decode.  
Replaces: `c64d_c64_instruction_cycle = 0`, `c64d_maincpu_previous_instruction_clk` saves,
`c64d_profiler_start_handle_cpu_instruction()`.

### `VICE_HOOK_CPU_INSTRUCTION_END(pc, opcode, cycles)`
Called at end of each 6510 instruction.  
Replaces: `c64d_profiler_end_cpu_instruction()`, `c64d_mark_c64_cell_execute()`.

### `VICE_HOOK_MEM_READ_C64(addr)` / `VICE_HOOK_MEM_WRITE_C64(addr, value)`
Injected at LOAD/STORE macro sites in the inlined mainc64cpu.c.  
Replaces: `c64d_mark_c64_cell_read()`, `c64d_mark_c64_cell_write()`.

### `VICE_HOOK_MEM_READ_DRIVE(addr)` / `VICE_HOOK_MEM_WRITE_DRIVE(addr, value)`
Injected at LOAD/STORE macro sites in the inlined 6510core.c.  
Replaces: `c64d_mark_drive1541_cell_read()`, `c64d_mark_drive1541_cell_write()`.

### `VICE_HOOK_CIA_STORE(cia_num, addr, value)` / `VICE_HOOK_CIA_READ(cia_num, addr, result)`
Injected at `ciacore_store()` and `ciacore_read()` call sites in `c64cia1.c`/`c64cia2.c`.  
Replaces: `c64d_cia1_register_written`, `c64d_cia1_read_value` etc.

### `VICE_HOOK_SID_STORE(addr, value)` / `VICE_HOOK_SID_READ(addr, result)`
Injected at SID store/read sites in `sid/sid.c`.  
Replaces: `c64d_sid_register_written`, `c64d_sid_read_value` etc.

### `VICE_HOOK_VICII_CYCLE(raster_line, raster_cycle)`
Called at end of `vicii_cycle()`.  
Replaces: `c64d_c64_vicii_cycle()`, `c64d_c64_vicii_start_raster_line()`, `c64d_c64_vicii_start_frame()`.

### `VICE_HOOK_VSYNC_FRAME(is_paused)`
Called in `vsync_do_vsync()`.  
Replaces: `c64d_start_frame_for_snapshots_manager()`, `c64d_check_snapshot_interval()`.

### `VICE_HOOK_DRIVE_MEM_READ(drv, addr)` / `VICE_HOOK_DRIVE_MEM_WRITE(drv, addr, value)`
Injected at drive memory read/write functions in `memiec.c`, `drivemem.c`, VIA stores.  
Replaces: `c64d_mark_drive1541_cell_read()`, `c64d_mark_drive1541_cell_write()`.

### `VICE_HOOK_SOUND_OPEN(chip_num)` / `VICE_HOOK_SOUND_FRAME(is_paused)`
Injected at `sound_machine_open()` and `sound_flush()` in `sound.c`.  
Replaces: `c64d_sound_init()`, `c64d_sound_run_sound_when_paused` checks.

### Non-hookable (must remain direct calls)
- `c64d_set_debug_mode()` — state machine transitions; must remain direct calls
- `c64d_lock_mutex()` / `c64d_unlock_mutex()` — threading primitives; cannot be macros
- All `c64d_peek_*` functions — complex accessor logic; stay as function calls via ViceInterface

---

## Completeness Verification

The following files were cross-checked to confirm all 64 modified files are referenced in this
catalog. Files not present in vanilla 3.1 (`arch/*`, `resid/resid-*.cpp/h`, `resid-fp/*`) are
noted as new additions in their respective sections.

All 64 files from `/tmp/c64d-per-file.txt` are accounted for in the categories above.

```
src/Emulators/vice/arch/mousedrv.c        → Category 11
src/Emulators/vice/arch/ui.c              → Category 14
src/Emulators/vice/arch/ui.h              → Category 14
src/Emulators/vice/arch/uimsgbox.c        → Category 14
src/Emulators/vice/arch/uimon.c           → Categories 10, 14
src/Emulators/vice/arch/uistatusbar.c     → Category 14
src/Emulators/vice/arch/video.c           → Category 14
src/Emulators/vice/arch/vsyncarch.c       → Category 14
src/Emulators/vice/c64/c64-snapshot.c     → Category 12
src/Emulators/vice/c64/c64cia1.c          → Category 6
src/Emulators/vice/c64/c64cia2.c          → Category 6
src/Emulators/vice/c64/c64cpusc.c         → Categories 1, 2, 3, 6, 12, 13
src/Emulators/vice/c64/c64memrom.c        → Category 15
src/Emulators/vice/c64/c64memsc.c         → Categories 2, 3, 15
src/Emulators/vice/c64/cart/c64-generic.c → Category 5
src/Emulators/vice/c64/cart/reu.c         → Categories 1, 5
src/Emulators/vice/c64/patchrom.c         → Category 15
src/Emulators/vice/core/ciacore.c         → Category 6
src/Emulators/vice/core/flash040core.c    → Category 1
src/Emulators/vice/core/viacore.c         → Category 6
src/Emulators/vice/drive/drive-writeprotect.c → Category 4
src/Emulators/vice/drive/drive.c          → Category 4
src/Emulators/vice/drive/drivecpu.c       → Categories 1, 4
src/Emulators/vice/drive/drivemem.c       → Category 4
src/Emulators/vice/drive/iec/memiec.c     → Category 4
src/Emulators/vice/drive/iec/via1d1541.c  → Category 4
src/Emulators/vice/drive/iec/via1d1541.h  → Category 4
src/Emulators/vice/drive/iecieee/via2d.c  → Categories 3, 4
src/Emulators/vice/drive/rotation.c       → Category 4
src/Emulators/vice/drive/viad.h           → Category 4
src/Emulators/vice/iecbus/iecbus.c        → Category 4
src/Emulators/vice/joyport/joystick.c     → Category 11
src/Emulators/vice/joyport/mouse.c        → Category 11
src/Emulators/vice/monitor/monitor.c      → Category 10
src/Emulators/vice/resid-fp/residfp-sid.cpp → Category 8
src/Emulators/vice/resid-fp/residfp-sid.h → Category 8
src/Emulators/vice/resid-fp/residfp-voice.h → Category 8
src/Emulators/vice/resid/resid-filter.cpp → Category 8
src/Emulators/vice/resid/resid-filter.h   → Category 8
src/Emulators/vice/resid/resid-sid.cpp    → Category 8
src/Emulators/vice/resid/resid-sid.h      → Category 8
src/Emulators/vice/root/cia.h             → Category 6
src/Emulators/vice/root/dma.c             → Category 1
src/Emulators/vice/root/interrupt.c       → Category 1
src/Emulators/vice/root/interrupt.h       → Category 1
src/Emulators/vice/root/keyboard.c        → Category 11
src/Emulators/vice/root/keyboard.h        → Category 11
src/Emulators/vice/root/machine.c         → Category 14
src/Emulators/vice/root/maincpu.h         → Category 1
src/Emulators/vice/root/midi.c            → Category 1
src/Emulators/vice/root/sound.c           → Categories 8, 14
src/Emulators/vice/root/via.h             → Category 6
src/Emulators/vice/root/vsync.c           → Category 14
src/Emulators/vice/sid/fastsid.c          → Category 8
src/Emulators/vice/sid/sid-resources.c    → Category 8
src/Emulators/vice/sid/sid-resources.h    → Category 8
src/Emulators/vice/sid/sid-snapshot.c     → Category 8
src/Emulators/vice/sid/sid.c              → Category 8
src/Emulators/vice/sounddrv/soundsdl.c    → Category 9
src/Emulators/vice/viciisc/vicii-cycle.c  → Category 7
src/Emulators/vice/viciisc/vicii-draw-cycle.c → Category 7
src/Emulators/vice/viciisc/vicii-irq.c    → Categories 6, 7
src/Emulators/vice/viciisc/vicii.c        → Category 7
src/Emulators/vice/viciisc/viciitypes.h   → Categories 6, 7
```
