# Phase 3 Chunk 1 — Task 1: file-fate map + merge scoping

**Date:** 2026-05-28
**Branch:** `vice-3.10-phase-3-vendor-swap`
**Data:** `phase3-data/chunk1-filemap.tsv` (1155 rows: `<live-path>\t<vanilla-3.10-rel>\t<fate>`)

## What this is

Maps every source/header file in `src/Emulators/vice/` (excluding `ViceInterface/` and our
`vice_debugger_hook.h`) to its vanilla VICE 3.10 equivalent, to drive the Chunk 1 3-way merge.
Path-mapping rules applied: slajerek `root/<f>` → vanilla top-level `<f>`; resid
`resid-<x>.cpp` → `resid/<x>.cc`; resid-fp `residfp-<x>.cpp` → `resid-fp/<x>.cc`.

## Category counts (1155 files total)

| Fate | Count | Meaning | Merge handling |
|------|-------|---------|----------------|
| **SAME** | 858 | same relative path in 3.10 | mechanical 3-way merge (most are unhooked → content + `vicetypes.h` rename) — Task 2 |
| **MOVED** | 154 | found in 3.10 at a different path (basename match) | path-aware mechanical merge — Task 2 (verify each basename match is the real file, not a namesake) |
| **GONE** | 142 | no 3.10 file by that basename | triage below — Task 4 |
| OURS | 1 | `vice_debugger_hook.h` (ours) | leave |

## GONE triage: only ~32 are actually COMPILED (the real breakage set)

Of the 142 GONE files, only ~32 are referenced in the build; the other ~110 are non-compiled /
vestigial (headers, unused platform variants) and can be ignored. The compiled-GONE set, grouped:

| Group | Files | Disposition |
|-------|-------|-------------|
| `platform/` subsystem | `platform.c`, `platform_*_runtime_os.c` (×9), `platform_x86_runtime_cpu.c` | 3.10 REMOVED the platform-detection subsystem. Drop all from the build; confirm nothing references `platform_get_*`. |
| translation / legacy `root/` | `root/translate.c`, `root/libm_math.c`, `root/embedded.c`, `root/ioutil.c` | 3.10 removed translation; `ioutil`/`embedded`/`libm_math` reorganized. Drop + reconcile any callers. |
| `root/clkguard.c` | 1 | DELETED upstream; CLOCK-overflow handling redesigned. Remove + fix the 9 includers (Task 4 Step 1). |
| video render | `video/render1x1crt.c`, `render1x2crt.c`, `render2x2crt.c`, `render2x4crt.c`, `renderyuv.c` | 3.10 restructured `video/`. Adopt 3.10's video-render file set (these specific files replaced). |
| monitor | `monitor/mon_ui.c` | removed in 3.10's monitor rewrite. Drop; monitor replay is a later chunk anyway. |
| c64 | `c64/patchrom.c`, `c64/c64embedded.c` | both gone in 3.10. patchrom fast-boot was `#ifdef RETRODEBUGGER`-gated → OFF-safe to drop. |
| sid/resid | `sid/resid.cpp` (+ resid/resid-fp renames) | resid file naming/structure changed; reconcile in the resid handling. |
| arch SDL-UI | `arch/blockdev.c`, `arch/menu_lightpen.c`, `arch/menu_tfe.c` | SDL-menu/arch files; likely shouldn't be in RetroDebugger's build. Verify + drop. |
| misc drivers | `gfxoutputdrv/doodledrv.c`, `jpegdrv.c`, `diskimage/rawimage.c`, `tapeport/tapelog.c` | individual files; check for 3.10 equivalents (may be renamed/moved) or drop. |

## Implication for the rest of Chunk 1

- **Task 2 (mechanical):** ~1000 SAME+MOVED files, the bulk unhooked → content replace + `vicetypes.h` rename. Delegatable, but MOVED (154) need per-file path verification (basename match could be a namesake).
- **Task 3 (delicate):** ~60 hooked files (the catalog's 64 minus CPU-cores `c64cpusc.c`/`drivecpu.c` and Tier C resid/sound, all deferred to Chunk 2) → hand 3-way merge preserving gated hooks.
- **Task 4 (infra/GONE):** the ~32 compiled-GONE groups above + clkguard/diskunit/mainlock reconciliation.
- **Tasks 5–6:** build-list update + error-landscape capture.

This is a large multi-session operation (~1000+ files). The mechanical bulk is low-risk; the risk concentrates in the ~60 hooked merges + the GONE/infra reconciliation + (Chunk 2) the CPU-core re-inline and monitor.
