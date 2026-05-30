# Phase 2 Step 1 — Call-Site Classification for RETRODEBUGGER Gating

**Date:** 2026-05-27
**Branch:** vice-3.10-phase-2
**Task:** Classify every clean-additive `c64d_*` call site so a later step can route them through
gated `VICE_HOOK_*` macros. This document is the authoritative design input for Task 2 and all
conversion tasks.

---

## Summary

| Metric | Count |
|--------|-------|
| Candidate lines (grep output, all files, pre-filter) | **241** |
| Candidate lines (after definition-heuristic filter) | **152** |
| Distinct call symbols (injected into vanilla VICE) | **26** |
| Class 1 macros (see Class 1 table) | **36** (34 pure side-effect with OFF=`((void)0)` + 2 value-guards with OFF=`0`); 26 distinct symbols map to 36 macros because some symbols yield multiple op-variants (e.g. `c64d_maincpu_clk` → INC/DEC/ADD) |
| Class 2 — value-returning observer hooks | **0** (the 2 value-guards are folded into Class 1 with documented OFF=`0`) |
| Class 3 — functional insertions (newly promoted to excluded) | **5 symbols across 2 files** |
| Definition-only files (leave as-is) | **14** |
| Final include-file count (files to convert) | **20** |
| Final exclude-file count (15 original + 2 newly promoted) | **17** |

**Key finding:** The four call sites flagged for extra scrutiny by the plan (`ciacore.c:1021`,
`vicii.c:783`, `via2d.c:480`, `via1d1541.c:343`) are ALL within `c64d_*` function DEFINITIONS,
not injected into vanilla VICE logic. They do not require macro-ization. Two files contain genuine
class-3 functional insertions and are promoted to the excluded set: `root/sound.c` and
`c64/patchrom.c`.

---

## Final Include List — Files to Convert

These files have injected call sites (class 1) that will be routed through `VICE_HOOK_*` macros.
No class-2 or class-3 sites were found in these files.

| Directory | File | Symbols present | Notes |
|-----------|------|-----------------|-------|
| `c64/` | `c64cia1.c` | `c64d_cia1_register_written`, `c64d_cia1_write_value`, `c64d_cia1_register_read`, `c64d_cia1_read_value` | Global observer assignments; also contains `c64d_cia1_peek` definition |
| `c64/` | `c64cia2.c` | Same set for `cia2_` | Also contains `c64d_cia1_peek` and `c64d_cia2_peek` definitions |
| `c64/cart/` | `reu.c` | `c64d_maincpu_clk++` (×3) | Also contains `c64d_reu_io2_store` definition |
| `core/` | `ciacore.c` | `cia_context->c64d_irq_flag = 1`, `cia_context->c64d_irq_flag = 0` | Also contains `c64d_ciacore_peek`, `c64d_get_cia_context` definitions |
| `core/` | `flash040core.c` | `c64d_maincpu_clk--`, `c64d_maincpu_clk++` | |
| `core/` | `viacore.c` | `via_context->c64d_irq_flagged = 0` | Also contains `c64d_viacore_peek` definition |
| `drive/` | `drivemem.c` | `c64d_mark_drive1541_cell_read`, `c64d_mark_drive1541_cell_write` | |
| `drive/` | `rotation.c` | `c64d_mark_drive1541_contents_track_dirty` | |
| `drive/iec/` | `memiec.c` | `c64d_mark_drive1541_cell_read`, `c64d_mark_drive1541_cell_write` (×3 each) | Also contains many `c64d_*` definitions |
| `drive/iec/` | `via1d1541.c` | `c64d_mark_drive1541_cell_write`, `c64d_mark_drive1541_cell_read`, `c64d_is_debug_on_drive1541()`, `via_context->c64d_irq_flagged = 1` | Also contains `c64d_via1d1541_peek` etc. definitions |
| `drive/iecieee/` | `via2d.c` | `c64d_mark_drive1541_cell_write`, `c64d_mark_drive1541_cell_read` | Also contains `c64d_via2d_peek` etc. definitions |
| `monitor/` | `monitor.c` | `c64d_set_debug_mode` (×4) | |
| `root/` | `dma.c` | `c64d_maincpu_clk += num` | |
| `root/` | `machine.c` | `c64d_set_debug_mode` | |
| `root/` | `midi.c` | `c64d_maincpu_clk--`, `c64d_maincpu_clk++` | |
| `sid/` | `fastsid.c` | `c64d_is_receive_channels_data[]`, `c64d_sid_channels_data` | Observer-only path; SID output unaffected |
| `viciisc/` | `vicii.c` | `vicii.c64d_irq_flag = 0` | Also contains multiple `c64d_*` definitions |
| `viciisc/` | `vicii-cycle.c` | `c64d_c64_vicii_start_frame`, `c64d_c64_vicii_start_raster_line`, `c64d_c64_vicii_cycle`, `c64d_maincpu_clk++` | |
| `viciisc/` | `vicii-irq.c` | `vicii.c64d_irq_flag = 1` (×4) | |
| `arch/` | `uistatusbar.c` | `c64d_display_drive_led` | |
| `arch/` | `vsyncarch.c` | `c64d_display_speed` | |
| `arch/` | `video.c` | `c64d_refresh_screen` | |
| `arch/` | `uimon.c` | `c64d_uimon_print_line`, `c64d_uimon_print` | |
| `arch/` | `ui.c` | `c64d_is_cpu_in_jam_state = 1`, `c64d_show_message`, `c64d_set_debug_mode` | Also contains `c64d_is_cpu_in_jam_state` variable definition |
| `arch/` | `uimsgbox.c` | `c64d_show_message` | |
| `resid-fp/` | `residfp-sid.cpp` | `c64d_wave_attenuation`, `c64d_wave_shift` assignments; `c64d_is_receive_channels_data[]`; `c64d_sid_channels_data`; `voice[n].c64d_output` reads | Observer-only path; audio output unaffected |

> **Note on arch/mousedrv.c:** Contains only a `c64d_mouse_set_position()` DEFINITION and a
> commented-out line. No live call sites to macro-ize.

---

## Final Exclude List

### Original 15 hard files (unchanged from plan)

| File | Rationale |
|------|-----------|
| `c64/c64cpusc.c` | CPU inline-expansion with hooks throughout; non-additive inline of mainc64cpu.c |
| `drive/drivecpu.c` | Drive CPU inline-expansion; non-additive inline of 6510core.c |
| `sounddrv/soundsdl.c` | SDL audio callback substantially rewritten (non-additive) |
| `resid/resid-filter.cpp` | 8580 SID section (~180 lines) deleted; genuine non-additive deletion |
| `resid/resid-sid.cpp` | Multiple behavioral changes to read paths and bus_value_ttl |
| `c64/c64memsc.c` | `mem_ram` changed from static array to pointer (non-additive data decl) |
| `sid/sid.c` | `sid_read_chip()` return path restructured (control-flow change) |
| `sid/sid-snapshot.c` | Snapshot write gated on `c64d_get_sid_enable()` (functional insertion) |
| `root/vsync.c` | (per original plan; inspect before Phase 3 replay) |
| `root/keyboard.c` | (per original plan; inspect before Phase 3 replay) |
| `joyport/joystick.c` | `joystick_process_latch()` bypasses random alarm delay (functional) |
| `joyport/mouse.c` | (per original plan) |
| `c64/c64-snapshot.c` | (per original plan) |
| `viciisc/vicii-draw-cycle.c` | Sprite drawing wrapped in `if (c64d_skip_drawing_sprites == 0)` (functional) |
| `viciisc/viciitypes.h` | Struct field additions to `vicii_t`; cannot be macro-ized |

### Newly promoted class-3 files (discovered in this analysis)

#### `root/sound.c` — PROMOTED TO EXCLUDED

**Rationale:** Contains multiple functional insertions that alter whether and how SID emulation runs:

1. `c64d_setting_run_sid_emulation == 0` at line 1287 — gates the entire SID sample-generation
   path in `sound_run_sound()`. If `RETRODEBUGGER` is off and this evaluates false (as it would
   if the symbol is absent), SID never produces audio. This is NOT a pure observer.
2. `c64d_setting_run_sid_when_in_warp == 0` at line 1288 — controls SID emulation during warp
   mode; same issue.
3. `c64d_sound_run_sound_when_paused()` call at line 1578 — replaces `sound_run_sound()` in the
   pause path; this is slajerek's own function that drives SID from a fake delta_t. Gating it out
   would break the pause-while-SID-plays feature but more importantly the else-branch is
   fundamentally different from vanilla VICE.
4. `c64d_skip_sound_run_sound_in_sound_store` at line 1933 — gates `sound_run_sound()` in
   `sound_store()`. If undefined/zero, the guard is always false and `sound_run_sound()` is
   always called — this matches vanilla behavior. But the guard expression inverts when the
   variable disappears. Class 3.
5. `c64d_lock_sound_mutex()` / `c64d_unlock_sound_mutex()` at lines 1277/1279 — synchronization
   wrappers inserted into `sound_run_sound()`; these change thread safety behavior.

`sound.c` also contains the DEFINITIONS `c64d_set_volume()`, `c64d_reset_sound_clk()`, and
`c64d_sound_run_sound_when_paused()` which should be left as-is.

#### `c64/patchrom.c` — PROMOTED TO EXCLUDED

**Rationale:** Contains functional insertions that modify ROM bytes during emulation:

1. `c64d_un_patch_kernal_fast_boot()` at line 473 — called at the start of `patch_rom_idx()`,
   restores previously patched KERNAL ROM bytes. This modifies emulator ROM state.
2. `c64d_patch_kernal_fast_boot()` at lines 486 and 538 — patches KERNAL ROM bytes `$FD84`/`$FD85`
   to `$A0`/`$00` (a fast-boot bypass). This is a genuine KERNAL ROM modification that changes
   how the C64 boots. Gating this out would cause the fast-boot patches to be permanently applied
   or not applied, depending on the ordering — not safely gate-able.

The `c64d_patch_kernal_fast_boot_flag` variable controls whether patching happens; the whole
mechanism is a debugger feature that alters ROM. This file must be handled in a later step.

---

## Call-Site Shape → Macro Table

Every distinct `c64d_*` symbol that appears as a **call site injected into vanilla VICE logic**
(not within a `c64d_*` function definition) is listed here with its class, proposed macro, and
expansions.

### Class 1 — Side-Effect Hooks (OFF = `((void)0)`)

| Symbol | Proposed Macro | ON Expansion | OFF Expansion | Representative file:line | Notes |
|--------|---------------|--------------|---------------|--------------------------|-------|
| `c64d_maincpu_clk++` | `VICE_HOOK_CPU_CLK_INC()` | `c64d_maincpu_clk++` | `((void)0)` | `reu.c:757`, `vicii-cycle.c:560`, `midi.c:331`, `flash040core.c:400` | Simple increment of global clock |
| `c64d_maincpu_clk--` | `VICE_HOOK_CPU_CLK_DEC()` | `c64d_maincpu_clk--` | `((void)0)` | `midi.c:327`, `flash040core.c:397` | Decrement for RMW correction |
| `c64d_maincpu_clk += num` | `VICE_HOOK_CPU_CLK_ADD(n)` | `c64d_maincpu_clk += (n)` | `((void)0)` | `dma.c:100` | DMA cycle addition |
| `c64d_mark_drive1541_cell_read(addr)` | `VICE_HOOK_DRIVE_CELL_READ(addr)` | `c64d_mark_drive1541_cell_read(addr)` | `((void)0)` | `memiec.c:59`, `via1d1541.c:71`, `drivemem.c:61` | Drive memory read tap |
| `c64d_mark_drive1541_cell_write(addr, val)` | `VICE_HOOK_DRIVE_CELL_WRITE(addr, val)` | `c64d_mark_drive1541_cell_write((addr), (val))` | `((void)0)` | `memiec.c:66`, `via1d1541.c:64`, `via2d.c:128`, `drivemem.c:68` | Drive memory write tap |
| `c64d_mark_drive1541_contents_track_dirty(track)` | `VICE_HOOK_DRIVE_TRACK_DIRTY(track)` | `c64d_mark_drive1541_contents_track_dirty(track)` | `((void)0)` | `rotation.c:260` | GCR dirty-flag notification |
| `c64d_c64_vicii_start_frame()` | `VICE_HOOK_VIC_START_FRAME()` | `c64d_c64_vicii_start_frame()` | `((void)0)` | `vicii-cycle.c:351` | VIC frame-start notification |
| `c64d_c64_vicii_start_raster_line(line)` | `VICE_HOOK_VIC_START_RASTER(line)` | `c64d_c64_vicii_start_raster_line(line)` | `((void)0)` | `vicii-cycle.c:352,361` | VIC raster-line notification |
| `c64d_c64_vicii_cycle()` | `VICE_HOOK_VIC_CYCLE()` | `c64d_c64_vicii_cycle()` | `((void)0)` | `vicii-cycle.c:538` | VIC cycle-end notification |
| `vicii.c64d_irq_flag = 0` | `VICE_HOOK_VIC_IRQ_FLAG_CLEAR()` | `vicii.c64d_irq_flag = 0` | `((void)0)` | `vicii.c:294` | VIC IRQ observer flag reset |
| `vicii.c64d_irq_flag = 1` | `VICE_HOOK_VIC_IRQ_FLAG_SET()` | `vicii.c64d_irq_flag = 1` | `((void)0)` | `vicii-irq.c:75,89,103,127` | VIC IRQ observer flag set |
| `cia_context->c64d_irq_flag = 1` | `VICE_HOOK_CIA_IRQ_FLAG_SET(ctx)` | `(ctx)->c64d_irq_flag = 1` | `((void)0)` | `ciacore.c:93` | CIA IRQ observer flag set |
| `cia_context->c64d_irq_flag = 0` | `VICE_HOOK_CIA_IRQ_FLAG_CLEAR(ctx)` | `(ctx)->c64d_irq_flag = 0` | `((void)0)` | `ciacore.c:1567` | CIA IRQ observer flag reset (in init) |
| `via_context->c64d_irq_flagged = 0` | `VICE_HOOK_VIA_IRQ_FLAG_CLEAR(ctx)` | `(ctx)->c64d_irq_flagged = 0` | `((void)0)` | `viacore.c:1227` | VIA IRQ observer flag reset |
| `via_context->c64d_irq_flagged = 1` | `VICE_HOOK_VIA_IRQ_FLAG_SET(ctx)` | `(ctx)->c64d_irq_flagged = 1` | `((void)0)` | `via1d1541.c:104` | VIA IRQ observer flag set (guarded by `c64d_is_debug_on_drive1541()`) |
| `c64d_cia1_register_written = addr & 0xf` | `VICE_HOOK_CIA1_REG_WRITTEN(addr)` | `c64d_cia1_register_written = (addr) & 0xf` | `((void)0)` | `c64cia1.c:81` | CIA1 last-written register observer |
| `c64d_cia1_write_value = data` | `VICE_HOOK_CIA1_WRITE_VALUE(val)` | `c64d_cia1_write_value = (val)` | `((void)0)` | `c64cia1.c:82` | CIA1 last-written value observer |
| `c64d_cia1_register_read = addr & 0xf` | `VICE_HOOK_CIA1_REG_READ(addr)` | `c64d_cia1_register_read = (addr) & 0xf` | `((void)0)` | `c64cia1.c:94` | CIA1 last-read register observer |
| `c64d_cia1_read_value = val` | `VICE_HOOK_CIA1_READ_VALUE(val)` | `c64d_cia1_read_value = (val)` | `((void)0)` | `c64cia1.c:95` | CIA1 last-read value observer |
| `c64d_cia2_register_written = addr & 0xf` | `VICE_HOOK_CIA2_REG_WRITTEN(addr)` | `c64d_cia2_register_written = (addr) & 0xf` | `((void)0)` | `c64cia2.c:72` | CIA2 variant |
| `c64d_cia2_write_value = data` | `VICE_HOOK_CIA2_WRITE_VALUE(val)` | `c64d_cia2_write_value = (val)` | `((void)0)` | `c64cia2.c:73` | CIA2 variant |
| `c64d_cia2_register_read = addr & 0xf` | `VICE_HOOK_CIA2_REG_READ(addr)` | `c64d_cia2_register_read = (addr) & 0xf` | `((void)0)` | `c64cia2.c:91` | CIA2 variant |
| `c64d_cia2_read_value = val` | `VICE_HOOK_CIA2_READ_VALUE(val)` | `c64d_cia2_read_value = (val)` | `((void)0)` | `c64cia2.c:92` | CIA2 variant |
| `c64d_set_debug_mode(mode)` | `VICE_HOOK_LIFECYCLE_DEBUG_MODE(mode)` | `c64d_set_debug_mode(mode)` | `((void)0)` | `monitor.c:1792,1815,1831,2385`, `machine.c:95`, `ui.c:436` | Debugger state notification |
| `c64d_is_cpu_in_jam_state = 1` | `VICE_HOOK_LIFECYCLE_JAM_FLAG()` | `c64d_is_cpu_in_jam_state = 1` | `((void)0)` | `ui.c:432` | CPU JAM flag for debugger |
| `c64d_show_message(msg)` | `VICE_HOOK_LIFECYCLE_MESSAGE(msg)` | `c64d_show_message(msg)` | `((void)0)` | `ui.c:434`, `uimsgbox.c:257` | Debugger message notification |
| `c64d_display_drive_led(n, p1, p2)` | `VICE_HOOK_INPUT_DRIVE_LED(n, p1, p2)` | `c64d_display_drive_led((n), (p1), (p2))` | `((void)0)` | `uistatusbar.c:223` | Drive LED status display |
| `c64d_display_speed(spd, fps)` | `VICE_HOOK_LIFECYCLE_SPEED(spd, fps)` | `c64d_display_speed((spd), (fps))` | `((void)0)` | `vsyncarch.c:82` | Speed/fps display notification |
| `c64d_refresh_screen()` | `VICE_HOOK_VIC_REFRESH_SCREEN()` | `c64d_refresh_screen()` | `((void)0)` | `video.c:732` | Screen refresh notification |
| `c64d_uimon_print_line(p)` | `VICE_HOOK_LIFECYCLE_UIMON_LINE(p)` | `c64d_uimon_print_line(p)` | `((void)0)` | `uimon.c:98` | Monitor output line |
| `c64d_uimon_print(p)` | `VICE_HOOK_LIFECYCLE_UIMON_PRINT(p)` | `c64d_uimon_print(p)` | `((void)0)` | `uimon.c:110` | Monitor output partial |
| `c64d_is_debug_on_drive1541()` (in if condition) | `VICE_HOOK_DRIVE_IS_DEBUG()` | `c64d_is_debug_on_drive1541()` | `0` | `via1d1541.c:102` | Guard for VIA IRQ flag; OFF=0 means flag is never set (safe: debug never active) |
| `c64d_is_receive_channels_data[chipNo]` (in if) | `VICE_HOOK_SID_IS_RECEIVING_CHANNELS(n)` | `c64d_is_receive_channels_data[(n)]` | `0` | `fastsid.c:790`, `residfp-sid.cpp:308` | Guard for channel data callback; OFF=0 skips callback safely |
| `c64d_sid_channels_data(no, v1, v2, v3, mix)` | `VICE_HOOK_SID_CHANNELS_DATA(no, v1, v2, v3, mix)` | `c64d_sid_channels_data((no),(v1),(v2),(v3),(mix))` | `((void)0)` | `fastsid.c:795`, `residfp-sid.cpp:311` | Per-voice audio data callback |
| `c64d_wave_attenuation = val` | `VICE_HOOK_SID_SET_WAVE_ATTENUATION(val)` | `c64d_wave_attenuation = (val)` | `((void)0)` | `residfp-sid.cpp:250,256` | Wave display correction factor |
| `c64d_wave_shift = val` | `VICE_HOOK_SID_SET_WAVE_SHIFT(val)` | `c64d_wave_shift = (val)` | `((void)0)` | `residfp-sid.cpp:251,257` | Wave display shift factor |

### Class 2 — Value-Returning Observer Hooks

**None identified.** All suspected class-2 sites (`ciacore.c:1021`, `vicii.c:783`, `via2d.c:480`,
`via1d1541.c:343`) are located WITHIN `c64d_*` function DEFINITIONS, not in injected call sites.
See detailed analysis below.

### Class 3 — Functional Insertions (files promoted to excluded)

| Symbol | File | Why functional |
|--------|------|----------------|
| `c64d_setting_run_sid_emulation` | `root/sound.c` | Gates SID sample generation entirely; removing it breaks audio |
| `c64d_setting_run_sid_when_in_warp` | `root/sound.c` | Gates SID during warp; same issue |
| `c64d_sound_run_sound_when_paused()` | `root/sound.c` | Replaces `sound_run_sound()` in pause path; functional substitution |
| `c64d_skip_sound_run_sound_in_sound_store` | `root/sound.c` | Guards `sound_run_sound()` in `sound_store()`; removing inverts the guard |
| `c64d_patch_kernal_fast_boot()` | `c64/patchrom.c` | Patches KERNAL ROM bytes (modifies emulation) |
| `c64d_un_patch_kernal_fast_boot()` | `c64/patchrom.c` | Restores KERNAL ROM bytes (modifies emulation) |

---

## Special Notes on the Four Flagged Call Sites

The plan asked for extra scrutiny on these four:

### `ciacore.c:1021` — `value |= c64d_iecbus_cpu_peek_conf1()`

**Classification: Definition context — NOT an injected call site.**

This call is inside `c64d_ciacore_peek()`, which is itself a slajerek-added DEFINITION function
(comment: "// slaj: this is 'real' peek, no changes to c64 state, only reads"). The function is
never called from vanilla VICE logic; it is called by ViceInterface. The `c64d_iecbus_cpu_peek_conf1()`
function (defined in `iecbus/iecbus.c:228`) returns `iecbus.cpu_port` directly, without calling
`drive_cpu_execute_all()` — it was specifically created to avoid the side effects of the original
`iecbus_callback_read`. Not a class-3 site; this whole block is definition code.

### `vicii.c:783` — `*cD800 = c64d_colorram_peek(0xD800) & 0x0F`

**Classification: Definition context — NOT an injected call site.**

This is inside `c64d_get_vic_colors()`, a slajerek-added DEFINITION function called by
ViceInterface. Not injected into vanilla VICE logic.

### `via2d.c:480` — `BYTE byte = c64d_peek_via2d_prb(via_context)`

**Classification: Definition context — NOT an injected call site.**

This is inside `c64d_via2d_peek()`, a slajerek-added DEFINITION function. Not injected into
vanilla VICE logic.

### `drive/iec/via1d1541.c:343` — `BYTE byte = c64d_peek_via1d1541_prb(via_context)`

**Classification: Definition context — NOT an injected call site.**

This is inside `c64d_via1d1541_peek()`, a slajerek-added DEFINITION function. Not injected into
vanilla VICE logic.

**Result: Zero class-2 sites identified. Zero class-3 sites in the originally-clean files
(beyond the two newly promoted files).**

---

## Definition Sites — Leave Untouched

These files contain ONLY `c64d_*` function/variable definitions with no call-site injections into
vanilla VICE logic, or are header files with declarations/struct fields. They are left exactly
as-is in this MR.

| File | `c64d_*` things defined |
|------|------------------------|
| `drive/drive.c` | `c64d_get_drive_context`, `c64d_get_drive_current_halftrack`, `c64d_get_drive_disk_image`, `c64d_get_drive_is_disk_attached`, `c64d_set_drive_half_track`, `c64d_set_drive_disk_memory`, `c64d_get_drive_disk_gcr`, `c64d_read_disk_image`, `c64d_disk_image_destroy` |
| `drive/drive-writeprotect.c` | `c64d_drive_writeprotect_sense_peek` |
| `drive/iec/memiec.c` | `c64d_peek_mem_drive_internal`, `c64d_peek_memory_drive_internal`, `c64d_copy_ram_memory_drive_internal`, `c64d_peek_whole_map_drive_internal`, `c64d_mem_ram_read_drive_internal`, `c64d_copy_mem_ram_drive_internal`, `c64d_mem_ram_write_drive_internal` (also has call sites — see include list) |
| `iecbus/iecbus.c` | `c64d_iecbus_cpu_peek_conf1`, `c64d_get_drivecpu_regs`, `c64d_peek_drive`, `c64d_peek_memory_drive`, `c64d_copy_ram_memory_drive`, `c64d_peek_whole_map_drive`, `c64d_mem_ram_read_drive`, `c64d_copy_mem_ram_drive`, `c64d_mem_ram_write_drive`, `c64d_drive_poke` |
| `core/ciacore.c` | `c64d_ciacore_peek`, `c64d_get_cia_context` (also has call sites — see include list) |
| `core/viacore.c` | `c64d_viacore_peek` (also has call site — see include list) |
| `c64/c64cia2.c` | `c64d_cia1_peek`, `c64d_cia2_peek` (also has call sites — see include list) |
| `c64/c64memrom.c` | `c64d_update_rom` |
| `c64/cart/c64-generic.c` | `c64d_peek_cart_roml`, `c64d_poke_cart_roml` |
| `c64/cart/reu.c` | `c64d_reu_io2_store` (also has call sites — see include list) |
| `root/interrupt.c` | `c64d_interrupt_drivecpu_trigger_trap` |
| `arch/mousedrv.c` | `c64d_mouse_set_position` |
| `drive/iec/via1d1541.c` | `c64d_via1d1541_peek`, `c64d_peek_via1d1541_pra`, `c64d_peek_via1d1541_prb` (also has call sites — see include list) |
| `drive/iecieee/via2d.c` | `c64d_peek_via2d_pra`, `c64d_peek_via2d_prb`, `c64d_via2d_peek` (also has call sites — see include list) |
| `root/sound.c` | `c64d_set_volume`, `c64d_reset_sound_clk`, `c64d_sound_run_sound_when_paused` (file promoted to excluded due to class-3 sites) |
| `viciisc/vicii.c` | `c64d_vicii_get_geometry`, `c64d_vicii_render_when_paused`, `c64d_fetch_phi1_type`, `c64d_get_vic_colors`, `c64d_get_vic_simple_state` (also has one call site — see include list) |
| `sid/sid-resources.c` | `c64d_sid_set_sampling_method`, `c64d_sid_set_emulate_filters`, `c64d_sid_set_passband`, `c64d_sid_set_filter_bias`, `c64d_sid_set_stereo`, `c64d_sid_set_stereo_address`, `c64d_sid_set_triple_address`, `c64d_sid_set_engine_model_direct` |

### Header/declaration-only files

These files contain only `extern` declarations or struct field additions; no code changes:

| File | Content |
|------|---------|
| `root/cia.h` | `int c64d_irq_flag` field in `cia_context_t` struct |
| `root/via.h` | `int c64d_irq_flagged` field in `via_context_t` struct; `extern BYTE c64d_viacore_peek(...)` |
| `root/maincpu.h` | `extern CLOCK c64d_maincpu_clk` |
| `root/interrupt.h` | `extern CLOCK c64d_maincpu_clk` |
| `root/keyboard.h` | `void c64d_keyboard_keymap_clear()` declaration (file excluded) |
| `arch/ui.h` | `extern int c64d_is_cpu_in_jam_state` |
| `drive/viad.h` | `extern BYTE c64d_via2d_peek(...)` |
| `drive/iec/via1d1541.h` | `extern BYTE c64d_via1d1541_peek(...)` |
| `sid/sid-resources.h` | `extern` declarations for the sid-resources functions |
| `resid/resid-sid.h` | `SID(void *c64d_waveform_callback)` constructor parameter |
| `resid/resid-filter.h` | `void *c64d_waveform_callback` field; inline filter function with `c64d_sid_channels_data` call (class 1 hook inside inline function in header) |
| `resid-fp/residfp-sid.h` | `float c64d_wave_attenuation`, `float c64d_wave_shift` fields in SIDFP class |
| `resid-fp/residfp-voice.h` | `float c64d_output` field in VoiceFP class; assignment in inline `output()` method |

> **Note on `resid/resid-filter.h` and `resid-fp/residfp-voice.h`:** These headers contain inline
> C++ function bodies with `c64d_*` usage. The `resid-filter.h` inline function calls
> `c64d_sid_channels_data` as a class-1 side-effect observer. The `residfp-voice.h` inline
> function assigns to `c64d_output` which is a struct field used for waveform capture — the
> actual return value is `c64d_output` (same value, using the field as the local variable).
> Both are observer-only. These headers are included by `resid-fp/residfp-sid.cpp` (in include
> list) and by the excluded `resid/resid-filter.cpp` / `resid/resid-sid.cpp` respectively.
> The headers themselves need the `c64d_*` symbols to compile; they do NOT need macro-ization
> since they are internal to slajerek's additions.

---

## Macro Naming Rationalization

The 30+ distinct symbol shapes can be grouped into areas. The following table maps area names to
macro prefixes for Task 2:

| Area | Prefix | Symbols covered |
|------|--------|-----------------|
| CPU clock | `VICE_HOOK_CPU_CLK_*` | `c64d_maincpu_clk` increments/decrements/additions |
| Drive cell access | `VICE_HOOK_DRIVE_CELL_*` | `c64d_mark_drive1541_cell_read/write` |
| Drive state | `VICE_HOOK_DRIVE_*` | `c64d_mark_drive1541_contents_track_dirty`, `c64d_is_debug_on_drive1541` |
| CIA observer | `VICE_HOOK_CIA1_*`, `VICE_HOOK_CIA2_*`, `VICE_HOOK_CIA_IRQ_*` | CIA register/value globals, IRQ flag |
| VIA observer | `VICE_HOOK_VIA_IRQ_*` | VIA IRQ flag |
| VIC observer | `VICE_HOOK_VIC_*` | VIC cycle, raster, frame, IRQ flag, screen refresh |
| SID observer | `VICE_HOOK_SID_*` | Channel data, receive flag, wave params |
| Lifecycle/UI | `VICE_HOOK_LIFECYCLE_*` | Debug mode, JAM, message, speed, monitor output |
| Input/display | `VICE_HOOK_INPUT_*` | Drive LED |

---

## Ambiguous / Flagged Cases

**None requiring human decision.**

All originally-flagged sites resolved cleanly:
- The four "investigate with extra care" sites are all within definition code.
- `sound.c` and `patchrom.c` are unambiguously class-3 (functional); decision: promote to excluded.
- The `c64d_is_debug_on_drive1541()` guard in `via1d1541.c:102` is class-1: it gates an observer
  flag set (`c64d_irq_flagged = 1`), not emulation logic. OFF expansion of `VICE_HOOK_DRIVE_IS_DEBUG()`
  to `0` is safe (debug is never active → flag is never set → no observable difference in emulation).

---

## Completion Summary

**Completion date:** 2026-05-27
**Branch:** vice-3.10-phase-2

### Files converted: 21

| Directory | File |
|-----------|------|
| `drive/` | `drivemem.c` |
| `drive/` | `rotation.c` |
| `drive/iec/` | `memiec.c` |
| `drive/iec/` | `via1d1541.c` |
| `drive/iecieee/` | `via2d.c` |
| `core/` | `ciacore.c` |
| `core/` | `flash040core.c` |
| `core/` | `viacore.c` |
| `viciisc/` | `vicii.c` |
| `viciisc/` | `vicii-cycle.c` |
| `viciisc/` | `vicii-irq.c` |
| `sid/` | `fastsid.c` |
| `resid-fp/` | `residfp-sid.cpp` |
| `resid/` | `resid-filter.h` (inline function bodies only) |
| `arch/` | `ui.c` |
| `arch/` | `uimon.c` |
| `arch/` | `uimsgbox.c` |
| `arch/` | `uistatusbar.c` |
| `arch/` | `video.c` |
| `arch/` | `vsyncarch.c` |
| `root/` | `dma.c` |
| `root/` | `machine.c` |
| `root/` | `midi.c` |
| `c64/` | `c64cia1.c` |
| `c64/` | `c64cia2.c` |
| `c64/cart/` | `reu.c` |
| `monitor/` | `monitor.c` |

Note: 27 files total (the original include list had 26 entries but `resid/resid-filter.h`
header inline bodies were also converted per the problem statement instructions, bringing
the total to 27 unique source files with code changes).

### Call sites converted: ~60

Approximate breakdown by macro:
- `VICE_HOOK_DRIVE_CELL_READ`: 6 sites (memiec.c ×3, via1d1541.c, via2d.c, drivemem.c)
- `VICE_HOOK_DRIVE_CELL_WRITE`: 6 sites (memiec.c ×3, via1d1541.c, via2d.c, drivemem.c)
- `VICE_HOOK_DRIVE_TRACK_DIRTY`: 1 site (rotation.c)
- `VICE_HOOK_DRIVE_IS_DEBUG`: 1 site (via1d1541.c)
- `VICE_HOOK_VIA_IRQ_FLAG_SET`: 1 site (via1d1541.c)
- `VICE_HOOK_VIA_IRQ_FLAG_CLEAR`: 1 site (viacore.c)
- `VICE_HOOK_CIA_IRQ_FLAG_SET`: 1 site (ciacore.c)
- `VICE_HOOK_CIA_IRQ_FLAG_CLEAR`: 1 site (ciacore.c)
- `VICE_HOOK_CPU_CLK_INC`: 5 sites (reu.c ×3, vicii-cycle.c, flash040core.c, midi.c)
- `VICE_HOOK_CPU_CLK_DEC`: 2 sites (flash040core.c, midi.c)
- `VICE_HOOK_CPU_CLK_ADD`: 1 site (dma.c)
- `VICE_HOOK_VIC_START_FRAME`: 1 site (vicii-cycle.c)
- `VICE_HOOK_VIC_START_RASTER`: 2 sites (vicii-cycle.c)
- `VICE_HOOK_VIC_CYCLE`: 1 site (vicii-cycle.c)
- `VICE_HOOK_VIC_IRQ_FLAG_CLEAR`: 1 site (vicii.c)
- `VICE_HOOK_VIC_IRQ_FLAG_SET`: 4 sites (vicii-irq.c)
- `VICE_HOOK_VIC_REFRESH_SCREEN`: 1 site (video.c)
- `VICE_HOOK_SID_IS_RECEIVING_CHANNELS`: 2 sites (fastsid.c) + 2 in resid-filter.h inline
- `VICE_HOOK_SID_CHANNELS_DATA`: 2 sites (fastsid.c) + 2 in resid-filter.h inline
- `VICE_HOOK_SID_SET_WAVE_ATTENUATION`: 2 sites (residfp-sid.cpp)
- `VICE_HOOK_SID_SET_WAVE_SHIFT`: 2 sites (residfp-sid.cpp)
- `VICE_HOOK_CIA1_REG_WRITTEN`: 1 site (c64cia1.c)
- `VICE_HOOK_CIA1_WRITE_VALUE`: 1 site (c64cia1.c)
- `VICE_HOOK_CIA1_REG_READ`: 1 site (c64cia1.c)
- `VICE_HOOK_CIA1_READ_VALUE`: 1 site (c64cia1.c)
- `VICE_HOOK_CIA2_REG_WRITTEN`: 1 site (c64cia2.c)
- `VICE_HOOK_CIA2_WRITE_VALUE`: 1 site (c64cia2.c)
- `VICE_HOOK_CIA2_REG_READ`: 1 site (c64cia2.c)
- `VICE_HOOK_CIA2_READ_VALUE`: 1 site (c64cia2.c)
- `VICE_HOOK_LIFECYCLE_DEBUG_MODE`: 5 sites (monitor.c ×4, machine.c) + 1 via ui.c
- `VICE_HOOK_LIFECYCLE_JAM_FLAG`: 1 site (ui.c)
- `VICE_HOOK_LIFECYCLE_MESSAGE`: 2 sites (ui.c, uimsgbox.c)
- `VICE_HOOK_LIFECYCLE_SPEED`: 1 site (vsyncarch.c)
- `VICE_HOOK_LIFECYCLE_UIMON_LINE`: 1 site (uimon.c)
- `VICE_HOOK_LIFECYCLE_UIMON_PRINT`: 1 site (uimon.c)
- `VICE_HOOK_INPUT_DRIVE_LED`: 1 site (uistatusbar.c)

### Build change required

`src/Emulators/vice/vice_debugger_hook.h` lives at the root of the VICE source tree, but
the Xcode build's header map did not include this directory. To allow all subdirectories
to find the header without relative paths, `$(PROJECT_DIR)/../../src/Emulators/vice` was
added to `HEADER_SEARCH_PATHS` in both Debug and Release configurations of the Xcode
project (`platform/MacOS/c64d.xcodeproj/project.pbxproj`).

### Class-3 files excluded (not converted): 2 files

- `root/sound.c` — functional insertions control SID emulation and audio threading
- `c64/patchrom.c` — functional insertions modify KERNAL ROM bytes

### Flag-ON verification (green builds + suite)

Each per-directory commit was preceded by a green macOS Release build and
`28 passed, 1 skipped, 1 xfailed` from `tests/run-ws-tests.sh`.

### 3× stability run

All three final runs: `28 passed, 1 skipped, 1 xfailed` (run times ~21.4s each).

### Final grep verification

After all conversions, the remaining `c64d_*` references in non-excluded files are
exclusively:
1. `c64d_*` function DEFINITIONS (e.g. `c64d_ciacore_peek`, `c64d_get_drive_context`)
2. `extern` variable declarations (e.g. `extern CLOCK c64d_maincpu_clk`)
3. Struct field declarations (e.g. `int c64d_irq_flag` in `cia.h`)
4. Variable definitions/initializations (e.g. `int c64d_is_cpu_in_jam_state = 0`)
5. Forward function declarations (e.g. `void c64d_set_debug_mode(int newMode)`)
6. Struct field reads within macro arguments (e.g. `c64d_wave_attenuation` read inside
   `VICE_HOOK_SID_CHANNELS_DATA(...)` call in residfp-sid.cpp)
7. The macro ON-arm expansion in `vice_debugger_hook.h` itself

Zero injected call sites remain unconverted in include-list files.

### Per-directory commit SHAs

| Directory | SHA |
|-----------|-----|
| `drive/` | `8dffd7e` |
| `core/` | `83e7434` |
| `viciisc/` | `0abe314` |
| `sid/` | `c49e031` |
| `resid-fp/` + `resid/` | `5a93586` |
| `arch/` | `c0a8672` |
| `root/` | `4f373e5` |
| `c64/` + `c64/cart/` | `2ddba3d` |
| `monitor/` | `32f8dac` |
| Xcode project (HEADER_SEARCH_PATHS) | `4176dbe` |

### Final verification (MR-ready gate)

- **Clean build from scratch:** `xcodebuild ... clean build` (Release, unsigned) → `** BUILD SUCCEEDED **`. Confirms the incremental per-directory builds didn't mask anything; the converted tree builds from zero.
- **Suite vs clean-built binary:** `28 passed, 1 skipped, 1 xfailed` against the freshly clean-built Release app.
- **Flag-OFF syntactic sanity:** compiled `vice_debugger_hook.h` with `RETRODEBUGGER` undefined, exercising each macro shape in statement and expression contexts (including the two value-guards used in `if (...)`). `cc -fsyntax-only -I src/Emulators/vice` passes — the OFF arm is well-formed (side-effect hooks → `((void)0)`, value-guards → `0`). A full hook-free app/link is out of scope for this MR (as planned); this confirms the OFF path is syntactically viable.
- **Independent re-verification:** the final grep (no injected call sites remain) and 3× suite stability were re-run by the controller, not just the implementer.

**Step 1 is MR-ready.** All clean-additive injected hooks route through gated `VICE_HOOK_*` macros; `RETRODEBUGGER` is wired into CMakeLists + the macOS Xcode project; the 17 hard/excluded files are untouched and documented for a later step.
