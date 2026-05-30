# Phase 2 Step 2 — Functional-file gating completion summary

**Date:** 2026-05-28
**Branch:** `vice-3.10-phase-2-step2`
**Plan:** `docs/superpowers/plans/2026-05-28-vice-3.10-phase-2-step2-functional-gating.md`

## Goal

Gate slajerek's *functional* `c64d_*` modifications in the 11 Tier A+B files behind
`#ifdef RETRODEBUGGER`, so the ON build behaves exactly as before (regression suite green)
and the OFF build reverts toward vanilla VICE 3.1 (clean hook-free bisection build) for the
cleanly-gateable changes.

## Scope

- **Included (11 files):** `joyport/joystick.c`, `joyport/mouse.c`, `viciisc/viciitypes.h`,
  `viciisc/vicii-draw-cycle.c`, `c64/patchrom.c`, `root/sound.c`, `sid/sid-snapshot.c`,
  `root/keyboard.c`, `root/vsync.c`, `sid/sid.c`, `c64/c64-snapshot.c`.
- **Deferred to the 3.10 vendor swap (Tier C/D):** `resid/resid-filter.cpp`,
  `resid/resid-sid.cpp`, `sounddrv/soundsdl.c` (C++/audio rewrites) and
  `c64/c64cpusc.c`, `drive/drivecpu.c` (CPU inline-expansion). These get re-done against
  3.10 source, so gating the 3.1 copies would be throwaway.
- **Untouched:** the pervasive `types.h`→`vicetypes.h` rename (mechanical, handled at the swap).

## Gating patterns used

- **Pure additions** (slajerek added a call/def/block): `#ifdef RETRODEBUGGER … #endif`.
- **Replacements** (slajerek changed vanilla code): `#ifdef RETRODEBUGGER <slajerek> #else <verbatim vanilla> #endif` — every `#else` copied verbatim from `~/vice-ref/3.1/src/`.
- **Conditional guard:** `vicii-draw-cycle.c` sprite-draw uses `VICE_HOOK_VIC_DRAW_SPRITES()`
  (added to `vice_debugger_hook.h`): ON = `(c64d_skip_drawing_sprites == 0)`, OFF = `1`
  (vanilla always draws sprites).
- **Storage class:** `static` removals gated `#ifdef RETRODEBUGGER`(non-static)`#else`(static).

## Commits

| Commit | Content |
|--------|---------|
| `3f778d4` | Step 2 plan |
| `d9ed0ed` | Batch 1: 7 non-signature files (mouse, viciitypes, vicii-draw-cycle, patchrom, joystick, sound, sid-snapshot) |
| `68835f1` | Batch 2: 4 signature files (keyboard, vsync, sid, c64-snapshot) + joystick LOGD cleanup |

## Verification

- **ON build:** clean-from-scratch Release build succeeds; suite `28 passed, 1 skipped, 1 xfailed`, stable across 3 runs. ON behavior is unchanged because each `#ifdef RETRODEBUGGER` arm is exactly slajerek's prior code.
- **OFF correctness (the suite can't test this):** verified with `unifdef -URETRODEBUGGER`
  per file, diffing the OFF path against the vanilla reference. Result: **no RETRODEBUGGER/`c64d_`
  functional behavior leaks into the OFF path.** Files with zero functional OFF residual:
  `mouse.c`, `patchrom.c`, `sid-snapshot.c`, `viciitypes.h`, `joystick.c` (after the LOGD cleanup),
  `sound.c` (behavior; signature exception below). `vicii-draw-cycle.c` is vanilla-by-construction
  (guard macro OFF=1).
- **Flag-OFF syntactic:** the new `VICE_HOOK_VIC_DRAW_SPRITES()` guard compiles with
  `RETRODEBUGGER` undefined (statement + `if(...)` contexts).

## OFF-purity exceptions (documented, accepted)

These are intentional limits on the "OFF == vanilla" goal. ON-build correctness is unaffected.

### Retained signatures (gating a signature ripples to all callers — body gated instead)

| Function | File | Vanilla → Slajerek | Note |
|---|---|---|---|
| `keyboard_key_pressed` / `keyboard_key_released` | `root/keyboard.c` | `void`→`int` return | body `return` values gated; signature kept |
| `vsync_do_vsync` | `root/vsync.c` | +`isPaused` param | body behavior gated; call sites gated |
| `sound_flush` | `root/sound.c` | `()`→`(int isPaused)` | paired with vsync call site (gated); signature kept |
| `sid_sound_machine_open` engine hook | `sid/sid.c` | `open(d)`→`open(d, chipno)` | engine vtable; call site gated |
| `c64_snapshot_write` / `c64_snapshot_read` | `c64/c64-snapshot.c` | extended params (+ callee ripples) | bodies/new funcs gated; signatures kept |
| various `static`→external | keyboard/vsync/sid/sound | linkage only | non-functional |

### Non-hook slajerek divergences left for the 3.10 reconciliation (not debugger hooks, out of scope)

- `sid/sid.c`: ReSID-FP engine support (guarded by its own `HAVE_RESID_FP` flag), `byte`→`value`
  param rename, added `LOGD`/`log.h`.
- General: brace-style reformatting, added comments/LOGD lines across files.

These are pre-existing slajerek-vs-vanilla differences unrelated to `c64d_*` debugger hooks; they
will be reconciled when slajerek's tree is merged onto vanilla VICE 3.10.

## Net result

Every cleanly-gateable debugger functional change in the Tier A+B files is now behind
`RETRODEBUGGER`. Combined with Step 1 (the 36 `VICE_HOOK_*` side-effect macros), the debugger
hook surface across all non-Tier-C/D files is consolidated and gated. Remaining hook work is the
Tier C/D files, folded into Phase 3 (the VICE 3.10 vendor swap).
