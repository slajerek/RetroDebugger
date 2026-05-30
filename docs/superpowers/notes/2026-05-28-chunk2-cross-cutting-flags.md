# Chunk 2 cross-cutting flags (live, append as discovered)

Flags that span multiple batches / matter to the deferred HEAVY phase. Lead tracks these.

## From SID batch (sidw, task #1) — committed 519ff7b

1. **SID engine-slot collision (HEAVY sid.c).** RD `SID_ENGINE_RESID_FP == 7` collides
   with 3.10's new `SID_ENGINE_USBSID == 7`. Kept RESID_FP=7 (faithful re-apply; symbol
   used by CDebugInterfaceVice.cpp, arch/menu_sid.c, sid.c). Combined-model macros do NOT
   collide. Bare-engine `case` labels (sid.c has `case SID_ENGINE_USBSID:`; RD adds
   `case SID_ENGINE_RESID_FP:`) would be duplicate-case-7 ONLY if both HAVE_RESID_FP and
   HAVE_USBSID defined. RD doesn't build USBSID → latent. ACTION (sid.c phase): confirm
   root config never defines HAVE_USBSID; else move RESID_FP to freed slot (3.10 dropped
   SSI2001=5) — but that changes the persisted SidEngine resource value.

2. **SID_MODEL_6581R4 removed by 3.10.** Dropped from sid.h (vanilla-3.1, not an RD delta).
   `c64/c64scmodel.c` (task #5) has `case SID_MODEL_6581R4:` → MUST take 3.10's form or it
   won't compile. RELAYED to task-#5 owner (sidw).

3. **fastsid SOUND_SYSTEM_FLOAT path has no RD channel hook** (latent). RD's voiceMask-mute
   + VICE_HOOK_SID_CHANNELS_DATA live only in the int16_t `#else` path (faithful to RD's
   original placement). If ever built with SOUND_SYSTEM_FLOAT, fastsid debugger channel-data
   won't fire. Flag only; not fixing (would be new work).

## From CART batch (cartw, task #2) — committed 8e9e2d4

4. **Aux-core inline convention (BUILD-LIST impact).** 3.10 extracted the Z80 core to a new
   standalone `src/z80core.c` that `cpmcart.c` `#include`s mid-file. RD's established
   convention is to INLINE such cross-dir .c includes (verified: live `digimax.c` and
   `userport_digimax.c` already inline `digimaxcore.c` with `/// digimaxcore.c starts/ends`
   markers + commented `//#include`). DECISION: inlined 3.10 z80core.c into cpmcart.c, and
   3.10 digimaxcore.c into shortbus_digimax.c. **ACTION (build-list phase): do NOT add
   src/z80core.c or src/digimaxcore.c as standalone build units** — they're inlined. (3.1
   had no standalone z80core.c either; this matches history.)

5. **digimax.c / userport_digimax.c already inlined** in commit 957b4eb (the 21 clean
   merges) — verified >=1 (benign). No re-inlining needed. MISC task #7's userport_digimax.c
   is verify-only.

## From DRIVE batch (drivew, task #3) — committed 8c85900

7. **drivemem.h extern types (RESOLVED by lead).** drivew migrated memiec.c/drivemem.c
   defs to diskunit_context_t/uint8_t/uint16_t but drivemem.h (not in any batch — lead
   omission) still had 3.1-typed externs. Lead fixed: retyped drive_read_rom /
   drive_peek_free / drive_read_rom_ds1216 externs to 3.10 types, and DROPPED the stale
   `drive_peek_rom` extern (vanilla 3.10 + RD both keep it file-local static; verified it
   is referenced nowhere outside memiec.c). drive_read_rom/peek_free/read_rom_ds1216 are
   genuine RD cross-file exports (memieee.c uses shared drive_read_rom).

8. **iecbus/iecbus.c (task #7) drive_context_t forward decls.** L234-239 forward decls of
   c64d_*_drive_internal use drive_context_t → must become diskunit_context_t to match
   memiec.c defs (keep BYTE/uint8/uint16 per the actual sig). drivew owns task #7 — handles
   this itself.

9. **drive_snapshot_read_module(s, read_roms, read_disks)** sig preserved; callers in
   c64-snapshot.c L217/277 (task #5) already pass these args — VERIFY consistency when #5
   lands. No change expected.

10. **interrupt.h CLOCK_MAX (task #6).** 3.10 introduced CLOCK_MAX; interrupt.h uses it.
    Verify RD's vicetypes.h (64-bit CLOCK) defines CLOCK_MAX or interrupt.h takes 3.10's
    form. Check when #6 lands.

## ARCHITECTURE DECISION: threading model for vsync/sound/keyboard (task #6 deferred 6)

**RD does NOT define USE_VICE_THREAD** (confirmed absent from root/vice-config.h and the
Xcode pbxproj; RD uses USE_SDLUI + USE_SDL_AUDIO). 3.10's vice-thread/mainlock model is
entirely `#ifdef USE_VICE_THREAD` (mainlock.c fully wrapped; vsync.c gates 8 sites). So
for RD, 3.10 runs SINGLE-THREADED exactly like 3.1 — there is no thread model to adopt.

DECISION (keep RD's existing model; faithful, lowest-risk):
- Take 3.10's vsync/sound/keyboard *bodies* (SID 2-8, float sound, keyboard queue). Leave
  the `#ifdef USE_VICE_THREAD` branches in place (inert) — do not delete them.
- KEEP RD's extended signatures that thread isPaused, because RD's own callers pass it:
  ViceWrapper.cpp:1397/1408 and vicii.c:477/498 call
  `vsync_do_vsync(canvas, been_skipped, isPaused)`; vsync.c passes isPaused to
  `sound_flush(int isPaused)` (RD returns double for sound_delay).
  - vsync.c: keep `int vsync_do_vsync(video_canvas_s*, int been_skipped, int isPaused)`;
    retain `if (isPaused == 0)` gating of vsync_hook + c64d_lock/unlock_sound_mutex around
    sound_flush.
  - sound.c: keep `double sound_flush(int isPaused)`; retain isPaused gating,
    c64d_sound_run_sound_when_paused, sound mutex, c64d_set_volume, c64d_reset_sound_clk,
    warp/emulation gating. Verify which sound-system path (SOUND_SYSTEM_FLOAT vs int16) RD
    actually compiles and place hooks there.
  - keyboard.c: take 3.10 queue model; preserve c64d_keyboard_init / key_up_latch /
    keymap_clear, exposed keyboard_parse_set_pos_row/neg_row, and the `return latch`.
- CROSS-FILE: vicii.c (task #7, drivew) call sites to vsync_do_vsync MUST keep the 3-arg
  RD form (NOT 3.10's 1-arg). Alerted drivew.

## ARCHITECTURE DECISION 2: vsync/raster frame-timing API (RESOLVES the vsync.c blocker)

cartw found the tree is mid-migration on the vsync/raster skip API; my earlier "keep
vicii.c 3-arg" instruction was WRONG and is RETRACTED. Evidence:
- 3.10 split the old `int vsync_do_vsync(c, been_skipped, isPaused)` into a new API:
  `void vsync_do_vsync(c)` + `vsync_should_skip_frame(c)` + `vsync_do_end_of_line()` +
  `vsync_on_vsync_do(...)` + `vsync_set/get_warp_mode()`.
- The chunk1 vendor drop ALREADY brought in 3.10 consumers of the new API:
  raster/raster-canvas.c:114 calls vsync_should_skip_frame (committed); root/screenshot.c
  and viciisc/vicii-resources.c call vsync_on_vsync_do. So the 3.10 skip model is already
  LIVE; keeping RD's 3.1 return-based skip would double-count.

DECISION = Option A (full 3.10 vsync migration; "do it right" — Option B reverts committed
3.10 code = scope-cut, rejected):
- vsync.c provides 3.10's full API (void 1-arg vsync_do_vsync + the new functions).
- RD's isPaused is RE-THREADED AS A QUERY, not a parameter: `isPaused` is provably
  equivalent to `c64d_debug_mode == DEBUGGER_MODE_PAUSED` at BOTH live call sites
  (vicii.c passes 0 when running; ViceWrapper.cpp's c64d_debug_pause_check busy-loop passes
  1 while mode==PAUSED). `extern volatile int c64d_debug_mode` is in ViceWrapper.h:173.
  So inside vsync_do_vsync / sound_flush, replace the isPaused param with that query.
- Keep RD's sound-mutex bracketing + warp integration (reconcile c64d_get_warp_mode with
  3.10 vsync_get_warp_mode).
- vicii.c (drivew #7): take 3.10's form — vsync_do_vsync(canvas) 1-arg; skip handled by
  raster-canvas.c. Do NOT keep the 3-arg call (retraction).
- ViceWrapper.cpp c64d_debug_pause_check: change its 2 `vsync_do_vsync(canvas,0,1)` calls
  to `vsync_do_vsync(canvas)` (mode==PAUSED during that loop -> correct paused behavior).
- screenshot.c (already 3.10) + vicii-resources.c: consume new API -> fine, no special work.

## UNIFYING PRINCIPLE (vsync / sound / keyboard / main / threading)

RD is a SINGLE-THREADED debugger with its OWN UI. So:
- Where 3.10's change is threading/async/native-UI machinery (USE_VICE_THREAD, the async
  keyboard queue+alarm, the vice-thread vsync, the native VICE menu UI) -> RD keeps its
  synchronous/direct model; take 3.10's code but leave that machinery inert / keep RD's
  direct path.
- Where 3.10's change is FUNCTIONAL (chip accuracy, new features, bug fixes, or a new API
  symbol that already-committed 3.10 code REQUIRES) -> adopt it.
- The deciding test: does a committed 3.10 file REQUIRE the new API? If yes, adopt the
  3.10 base. If it's purely threading/UI decoupling with no other consumer, keep RD's model.

## DECISION 3: keyboard.c/.h (task #6) — 3.10 base + RD synchronous key_pressed/released

NOT a clean "keep 3.1": c64/c64cia1.c (committed) calls `keyboard_get_shiftlock()`, a
3.10-NEW function -> keyboard.c MUST provide the 3.10 shiftlock mechanism. But the ONLY
caller of keyboard_key_pressed/released is RD glue CDebugInterfaceVice.cpp:1217/1234
(1-arg, uses the synchronous `== 1` return); arch/kbd.c calls are commented out; nothing
references 3.10's internal kbd_queue. 3.10's rewrite makes key_pressed/released `void`
2-arg async (queue+alarm) — which would BREAK RD's synchronous consumed-check.

DECISION (REFINED — simpler than taking 3.10 base): The ONLY 3.10-new keyboard symbol any
committed file needs is keyboard_get_shiftlock (c64cia1.c:371). 3.10's impl is a trivial
3-liner `{ return keyboard_shiftlock; }`, and RD's 3.1 keyboard.c ALREADY HAS
`int keyboard_shiftlock = 0;` (line 80) maintained identically. So: KEEP RD's 3.1
keyboard.c/.h essentially as-is (synchronous 1-arg keyboard_key_pressed/released + latch
return; async queue avoided entirely; RD glue unchanged) and just ADD
`int keyboard_get_shiftlock(void) { return keyboard_shiftlock; }` + its prototype. No 3.10
body, no 2-arg signature, no glue edit. cartw executing.

## DECISION 4: main.c (task #4) — host-driven, USE_VICE_THREAD off (CONFIRMED)

vice_main_program = 3.10 full init + RD deltas, returns 0 after c64model_set; keep RD's
vice_main_loop_run -> maincpu_mainloop (CDebugInterfaceVice.cpp ~486 `while(isRunning)`
drives it); 3.10 pthread fns inert under #ifdef USE_VICE_THREAD. rootw executing.

## DECISION 5: sound.c (task #6) — proceed, manual placement review

Clean public contract (RD glue only calls signature-compatible sound_init/suspend/resume +
RD pure-additions; sound_flush(int isPaused) is internal to batch #6, called only by
vsync.c). Take 3.10's int16 sound engine (SOUND_SYSTEM_FLOAT undefined tree-wide), re-place
RD's c64d_lock/unlock_sound_mutex + isPaused-gating (-> c64d_debug_mode query) +
c64d_sound_run_sound_when_paused/run-SID-in-warp + c64d_set_volume/reset_sound_clk per RD's
live sites. NOTE: the >-gate CANNOT validate mutex placement (a misplaced mutex = deadlock/
audio corruption, still passes the gate). So cartw enumerates each hook/mutex placement in
the report and the lead eyeballs them manually. Pairs with vsync (shared isPaused/mutex).

## DECISION 6: main.c (task #4) — keep RD-aligned (arch-layer boundary)

3.10's main_program init calls tick_init() (no tick.c in RD tree) and ui_init_with_args()
(RD arch/ui.c provides ui_init(int*,char**)+ui_init_finish, not those) -> taking 3.10's
init = undefined symbols. arch/ is RD platform glue (DO-NOT-TOUCH), out of scope. main.c is
entry-orchestration matched to that arch layer; VICE-core accuracy/features live in the
subsystems we ARE upgrading, not main.c's init. So LIVE main.c kept essentially as-is
(working entry point) — a legitimate scope boundary, NOT a scope-cut. Revisit only if/when
the arch UI/tick layer is migrated (separate effort). rootw to mark #4 complete.

## DECISION 7: gifdrv.c (task #7) — keep RD's commented-out GIF impl

RD vice-config.h:247 DOES `#define HAVE_GIF`, and 3.10 gifdrv.c is HAVE_GIF-gated — BUT
libgif is NOT linked in the Xcode project (0 refs in pbxproj). So taking 3.10's gif impl
wholesale -> undefined giflib symbols at link. RD's commented-out impl is necessary/correct.
(Potential future enhancement: link giflib + take 3.10 gif. Out of scope now.)

## SID engine struct alignment (from sid.c d814f77 -> affects resid task #11 + fastsid)

3.10's `sid_engine_t` (in sid.h, committed): DROPPED the `prevent_clk_overflow` member,
ADDED `set_voice_mask` (11 members now). sid.c (committed) + fastsid.c (committed, task #1)
+ fakesid (committed in sid.c) all align. STILL-3.1 and MUST align when task #11 merges
resid: resid.cpp's resid_hooks (resid.cpp:382) and resid-fp.cpp's residfp_hooks
(resid-fp.cpp:320) currently initialize `*_prevent_clk_overflow` and LACK set_voice_mask ->
when merging to 3.10, DROP the prevent_clk_overflow initializer (take 3.10's resid hooks)
and ADD a set_voice_mask entry (RD's resid voice-mask hook, mirroring fastsid_set_voice_mask)
so the hooks struct matches the 11-member sid_engine_t. Otherwise: too-few/wrong
initializers = compile error. (resid.cpp/resid-fp.cpp are .cpp, part of the resid DSP task.)

## DECISION 8: resid DSP (task #11) — FULL engine upgrade (scope expanded)

rootw found RD's reSID is an OLDER engine; .cc and .h are one unit (3.10 sid.cc uses
scaleFactor/fir_beta/databus_ttl/raw_debug_output + int output() absent from RD's
resid-sid.h; 3.10 filter.h diverges 152 lines). And RD's reSID hooks live in the glue:
sid/resid.cpp set_chip_number(chipNo) + SID(void *c64d_waveform_callback) ctor (RD-specific;
3.10 is SID()). voice_mask/set_voice_mask are VANILLA 3.10 (free). So .cc-only scope can't
stand. DECISION = option (a) full engine upgrade (vendor-keeping = scope-cut, rejected per
mandate). Task #11 scope EXPANDED to: resid/*.cc + resid/*.h (all 11 headers) + sid/resid.cpp
+ sid/resid-fp.cpp. Take 3.10's reSID .cc/.h verbatim (DSP math + class defs), keep RD's
"resid-X.h" include naming, re-apply RD hooks (c64d_waveform_callback in filter+sid,
set_chip_number/chipNo, SID(void*) ctor), align resid_hooks/residfp_hooks to the 11-member
3.10 sid_engine_t (drop prevent_clk_overflow, add set_voice_mask). rootw authorized to edit
those files. Flag unclear hook placements vs 3.10's filter/DAC refactor.

## DECISION 9: monitor.c (task #10) — keep RD wrapper API, graft 3.10 internals

monitor.c collides with the DO-NOT-TOUCH wrapper CDebugInterfaceVice.cpp, which calls
`int exit_mon = monitor_process(cmd)` (RD de-static'd it; 3.10 made it static void),
`void monitor_close(int check)` (3.10: bool check_exit), and relies on RD's
monitor_trap_triggered entry flow (3.10 REMOVED it). DECISION (same principle as
keyboard/vsync — keep RD's integration surface, take 3.10 functional internals): KEEP RD's
wrapper-facing API — `int monitor_process(char*)` returning exit_mon, `void
monitor_close(int)`, and monitor_trap_triggered (RD's GUI monitor-entry trap, retained even
though 3.10 dropped it). Graft 3.10's command-processing internals where they don't alter
that API surface; map 3.10's `enum exit_mon` values to RD's int usage. Do NOT edit
CDebugInterfaceVice.cpp. Re-apply the c64d lifecycle hooks (VICE_HOOK_LIFECYCLE_DEBUG_MODE,
c64d_set_debug_mode). Highest-care; flag specific spots rather than guessing the entry/exit
flow. (Lead reviews closely.)

## DECISION 10: generated monitor mon_parse.c/mon_lex.c (task #10)

RD ships the GENERATED files directly (no .y/.l in tree -> no regeneration; the .c are
authoritative) and has 2 real hand-edits: mon_parse.c yyerror stderr-print commented out
(GUI monitor suppresses parser errors); mon_lex.c added `if (*s==0) goto send;` null-guard.
APPROACH: copy 3.10's generated mon_parse.c/mon_lex.c -> live, apply types->vicetypes rename,
re-apply ONLY those 2 meaningful RD edits (drop RD's cosmetic reformatting, take 3.10's).
Verify >-count = 0 except those 2 edits.

## BUILD-LIST TRACKER (for the post-merge Xcode/build-list phase)

ADD new 3.10 compile units to the Xcode project:
- joyport/mouse_1351.c, mouse_neos.c, mouse_paddle.c, mouse_quadrature.c (mouse split, e807815)
- the .c counterparts of the ~57 new-3.10 headers added in chunk1 (carts, userport devices,
  mainlock.c, sha1.c, profiler.c, uiactions.c, etc.) — enumerate when reconciling.
DO NOT add (inlined, not standalone): src/z80core.c (inlined into cpmcart.c),
src/digimaxcore.c (inlined into digimax.c/shortbus_digimax.c/userport_digimax.c).
REMOVE GONE files from build: clkguard.c, patchrom.c, platform/* , translate.c (when the
translation subsystem is removed).

## monitor.c RESOLUTION PLAN (12 conflicts, mapped; execute as ONE atomic careful pass)

Live monitor.c is untouched (RD 3.1). 3-way merge mapped 12 conflicts. Resolutions
(keep RD wrapper-facing API + GUI-driven monitor model; graft 3.10 internals; drop
vice-thread UI). CAUTION: diff3 mis-aligns a few regions (3.10's ui_pause_active/
should_pause_on_exit_mon block can appear as "common" post-conflict — resolve against the
FULL 3.10 monitor_startup, not the raw diff3). RD's monitor_startup fundamentally COMMENTS
OUT the native monitor loop (monitor_open + for(;;)) and instead fires
VICE_HOOK_LIFECYCLE_DEBUG_MODE(DEBUGGER_MODE_PAUSED) — the RD GUI drives the monitor. Keep
that.

1. (incl) keep RD `#include "DebuggerDefs.h"` + `vice_debugger_hook.h`; DROP RD's
   `#ifndef HAVE_STPCPY stpcpy` block (3.10 removed it / unused); take 3.10's `/*#define
   DEBUG_MONITOR*/`.
2. (mon_jump / G) keep RD's `execute_monitor_command_jump_trap` fn + `monitor_command_jump_
   addr`; in mon_jump take 3.10's `(uint16_t)` cast + `exit_mon = exit_mon_change_flow`,
   then keep RD's trap tail (monitor_command_jump_addr=addr; interrupt_maincpu_trigger_trap).
3. exit_mon = exit_mon_change_flow (take 3.10 enum).
4. (monitor_open) keep RD `void monitor_open(void)` (non-static) + take 3.10's TTY_COLUMNS/
   TTY_ROWS additions inside.
5. (monitor_startup reset) `inside_monitor = true;` (3.10) + KEEP RD `monitor_trap_triggered
   = FALSE;`.
6. KEEP RD `int monitor_process(char *cmd)` (non-static, returns int).
7. keep RD `LOGD("monitor_process: cmd=%s", cmd);`.
8. `void monitor_close(int check_exit)` — RD's non-static + int type, 3.10's param NAME
   check_exit (so 3.10 body compiles).
9. keep RD `LOGTODO("monitor_close: exit(0)");`.
10. (monitor_startup) keep RD `LOGD("monitor_startup")` + RD console_mode FIXME branch +
    3.10 `DBG(...)` + 3.10 `if(inside_monitor){...return;}` recursion guard; then RD's
    `VICE_HOOK_LIFECYCLE_DEBUG_MODE(PAUSED)` + RD's commented-out native loop. DROP 3.10's
    `if(ui_pause_active()){should_pause_on_exit_mon=...}` (vice-thread UI pause).
11. `if (exit_mon != exit_mon_no)` (take 3.10 enum).
12. keep RD's commented-out `}p monitor_close(1);*/` block + `monitor_close(1)`; DROP 3.10's
    `pause_on_exit_mon { ui_pause_enable/loop }` block (vice-thread UI; RD has it off).
Verify: markers 0; >-gate = only intentional RD deltas; monitor_trap_triggered re-grafted
(static int + the double-arm guard in monitor_startup_trap + reset in monitor_startup);
exit_mon enum internal, int at the monitor_process return. Wrapper CDebugInterfaceVice.cpp
only LOGs monitor_process's return (never branches) so int-vs-enum value is safe.

## More cross-cutting

6. **cartridge.h <-> c64carthooks.c snapshot-sig dependency.** c64carthooks.c (cart, done)
   calls `cartridge_snapshot_write_modules(s)` etc. and threads store_reu_data/save_cart_roms
   into easyflash/reu modules; the extended declarations live in `root/cartridge.h` (task #6
   ROOT-IO). VERIFY when #6 lands that cartridge.h's sigs match. easyflash.c uses RD's
   pre-existing `save_cart_roms ? SMW_BA(...) : 1 < 0` idiom (functionally correct: SMW_BA
   nonzero=error in the `||` chain; false branch `1<0`=0); preserved verbatim from live.

## monitor.c (task #10) — COMPLETE. 12 conflicts + 3 cross-cutting spots resolved.

Final resolution (executed solo; gate: 0 markers, braces 569/569, >-gate=7 all intentional,
<-gate=56 all RD additions; hook placement manually verified):
- Wrapper-facing API kept non-static per CDebugInterfaceVice.cpp:3088-3091:
  `void monitor_open(void)`, `void make_prompt(char*)`, `int monitor_process(char*)` (returns
  exit_mon; wrapper only LOGs it), `void monitor_close(int check_exit)` (int param, 3.10 body
  uses check_exit). monitor_process/open/close/make_prompt de-static = the 4 intentional >-sig lines.
- exit_mon enum (montypes.h, committed) taken internally: mon_jump=exit_mon_change_flow,
  mon_go/mon_exit/mon_instructions_*=exit_mon_continue, checks vs exit_mon_no/exit_mon_quit_vice.
- **monitor_trap_triggered DROPPED** (correction to the original plan): it is vanilla-3.1 (NOT an
  RD addition), removed by 3.10, with ZERO external consumers (grep: only monitor.c). 3.10 supersedes
  it with `inside_monitor` guards in monitor_startup/_trap, which auto-merged cleanly (ours==base there).
- **monitor_startup** = RD's proven GUI-driven structure: LOGD + DBG + console_mode FIXME guard +
  memspace + `VICE_HOOK_LIFECYCLE_DEBUG_MODE(PAUSED)` then RETURN. 3.10's native blocking loop
  (monitor_open + for(;;) + monitor_close + the inside_monitor recursion guard + ui_pause blocks)
  kept VERBATIM under `#if 0` (not `/* */` — 3.10's loop contains nested comments). Did NOT add
  3.10's `if(inside_monitor)return;` to the ACTIVE path: the wrapper calls monitor_open() (sets
  inside_monitor=true) and rarely monitor_close(), so an active guard would suppress the PAUSED hook
  on later breakpoints. Breakpoints call monitor_startup() directly (CPU cores), so the hook must
  fire unconditionally — matches RD's shipping 3.1 behavior.
- **RUNNING hooks** re-applied at end of mon_instructions_step/next/return (the resume-exec cmds).
- **mon_jump**: kept RD's `execute_monitor_command_jump_trap` + `monitor_command_jump_addr` +
  `interrupt_maincpu_trigger_trap` tail; took 3.10's (uint16_t) cast + exit_mon_change_flow.
- **ui_pause_*/pause_on_exit_mon**: native-UI/vice-thread machinery; ui_pause_* declared only in
  arch/{sdl,gtk3}/ui.h (NOT in RD). Left inert: commented the `|| ui_pause_active()` calls in
  mon_go/mon_exit (2 intentional >-lines; should_pause_on_exit_mon stays false so the blocks are
  dead-but-harmless); the monitor_startup ui_pause/pause_on_exit_mon blocks are inside the #if 0.
- **monitor_close exit path**: took 3.10 `archdep_vice_exit(0)` (available tree-wide in RD) over RD's
  `exit(0)`; kept RD's LOGTODO (message updated to match).
- stpcpy block (RD/3.1 `#ifndef HAVE_STPCPY`) dropped — 3.10 removed it; RD's 2 includes kept.
- Active 3.10 symbols verified present in RD before adopting: lib_strdup, lib_strdup_trimmed,
  log_printf, archdep_vice_exit, uimon_window_open(bool) [root/uimon.h], 5-field console_t
  [root/console.h byte-identical to 3.10]. (arch/uimon.c still no-arg impl = separate arch-layer
  issue, out of scope per DECISION 6.)
- mon_parse.c/mon_lex.c already merged in 597009e (DECISION 10). monitor.c is the last monitor file.

## DECISION 11: drive cluster + clkguard subsystem removal (SCOPE CORRECTION)

Discovered while starting drivecpu.c (task #13): the "3 remaining files" model was incomplete.
Audit of drive/ shows STILL-3.1 files: **drivecpu.c (43 old refs), drive.c (58), drive-overflow.c (4)**
— everything else in drive/ is committed-migrated (diskunit_context_t). These three are INTERLOCKED:
- `drive_context_t` no longer exists (committed drivetypes.h has only `diskunit_context_t`); drive.c
  still passes the old `drive_context[]` array into drivecpu.c's functions.
- 3.10 DELETED clkguard.c/.h AND drive-overflow.c/.h (64-bit CLOCK obviates them) and removed
  `drivecpu_prevent_clk_overflow`. That fn is still CALLED at drive.c:514. drivecpu.h (committed)
  already dropped its prototype, so drive.c is already in limbo.
- clkguard still referenced by: drive.c, drive-overflow.c, datasette.c, c64acia1.c, c64cpusc.c.

USER DECISION (asked 2026-05-28): **migrate the drive cluster together** — drivecpu.c + drive.c in one
coherent pass + DELETE drive-overflow.c (+ .h), so drive/ converges to 3.10 (diskunit + clkguard-free)
as one reviewable unit. (Tree already doesn't fully build per chunk1 error landscape, so this is
sequencing, not regressing.)

TRUE remaining Chunk 2 surface after this: [drive cluster: drivecpu.c + drive.c + delete drive-overflow.c],
[clkguard stragglers: datasette.c, c64acia1.c, + delete clkguard.c/.h from tree & build-list],
[c64cpusc.c — also has clkguard]. drivecpu65c02.c is the DONE sister = structural template.

### drivecpu.c field-migration map (authoritative, from vanilla 3.1->3.10 delta + diskunit struct):
drive_context_t->diskunit_context_t; struct drive_context_s->struct diskunit_context_s;
drive_clk[DRIVE_NUM]->diskunit_clk[NUM_DISK_UNITS]; drivecpu_int_status_ptr[DRIVE_NUM]->[NUM_DISK_UNITS].
Field flatten (now in diskunit): drv->drive->{type,drive_ram,log,trap,trapcont,idling_method,enable}
-> drv->{...}. Stays per-drive: drv->drive->byte_ready_edge -> drv->drives[0]->byte_ready_edge;
rotation_rotate_disk(drv->drive)->...(drv->drives[0]); rotation_reset(drv->drive)-> reset drives[0]+drives[1].
Removed: clkguard.h include, clk_guard_new/destroy, whole drivecpu_prevent_clk_overflow fn, MSVC pragmas.
Added (3.10): mainlock.h+uiapi.h includes (keep RD via.h), mi->mem_bank_poke/mem_bank_list_nos,
LOAD_DUMMY/STORE_DUMMY macros, ORIGIN_MEMSPACE/CPU_LOG_ID/CPU_IS_JAMMED, ui_display_reset in cpu_reset,
drivemem_init(drv) [no type], DRIVE_TYPE_9000 jam case. Renames: drive_jam->drivecpu_jam (static),
machine_jam->drive_jam(drv->mynumber,...), JAM_RESET->JAM_RESET_CPU, JAM_HARD_RESET->JAM_POWER_CYCLE,
MACHINE_RESET_MODE_SOFT->RESET_CPU, _HARD->POWER_CYCLE. Snapshot: SNAP_MINOR 1->3, SMW/SMR_DW(clock)->
SMW/SMR_CLOCK, add cpu_last_data SMW_B/SMR_B, WORD/BYTE/DWORD->uint16_t/uint8_t/uint32_t. Execute loop:
`while((int)(*clk_ptr-stop_clk)<0)` -> `while(*drv->clk_ptr < cpu->stop_clk)`; int tcycles->CLOCK tcycles.
NOTE: diff3 is CORRUPTED for drivecpu.c (drivecpu_execute misaligns -> 3 spurious defs); build from RD
live + apply this delta, NOT from the diff3 merge.
