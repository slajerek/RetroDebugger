# Vendored emulator files that are REFERENCE, not live code

**140 files** under `src/Emulators/atari800/sdl/` and `src/Emulators/vice/arch/`.
**139 of them contain zero live SDL library calls.** The single exception is
`src/Emulators/atari800/sdl/a-input.c`.

Note the two axes, which are easy to conflate: *live SDL calls* (one file) and
*live code* (two files — see "Live-exception files" below). A file can be
compiled-and-called live code and still contain no SDL call at all.

Modelled on `~/develop/MTEngineSDL/src/Engine/Libs/imgui/MTENGINE_PATCHES.md`,
which earned its keep during the S-1 ImGui upgrade by predicting exactly which
local patches a wholesale file replacement would destroy.

---

## Why these files are here

They are kept **deliberately**, as reference for features the original
emulators have and RetroDebugger does not yet:

- printer / RS232 — `rs232.c`, `menu_rs232.c`
- drive settings — `menu_drive.c`
- cartridge menus — `menu_c64cart.c`, `menu_vic20cart.c`
- the virtual keyboard — `vkbd.c`

Deleting them to tidy a `grep` would throw away the map for work not yet done.
Their SDL bodies are already commented out; RetroDebugger draws its own ImGui
UI.

## What to do at the next emulator upgrade

**This is the reason the file exists.** When atari800 or VICE is next bumped,
nobody should have to re-derive what was derived here:

1. **Diff the reference set against the new upstream release.** These files are
   vendored copies; the diff tells you what moved.
2. **Re-check the live/reference split rather than assuming it held.** A new
   upstream may add SDL calls to a file everybody believes is inert. That is
   exactly what `tests/test-reference-files.sh` checks, and why it fails a
   marked file that grows a live SDL call.
3. **Do not remove them from the build.** They compile to almost nothing, and
   some still provide symbols the emulator cores reference (`PLATFORM_Initialise`
   has 3 callers). Wrapping bodies in `#if 0` is likewise attractive and
   likewise deferred: it must be proven per file, not assumed.

## How the split was measured, so it can be reproduced

A raw `grep -c SDL_` over `src/Emulators/` returns **2026** and is **worthless**.
It counts VICE's menu macros (`SDL_MENU_LIST_END` x197,
`SDL_MENU_ITEM_SEPARATOR` x179) and atari800's own module globals
(`SDL_VIDEO_screen` x121, `SDL_VIDEO_SW_MapRGB`, `SDL_INPUT_Mouse`,
`SDL_PALETTE_buffer`, ...) — atari800's SDL port prefixes every symbol in the
module `SDL_`.

The giveaway was that the "SDL calls" included `SDL_SetVideoMode`,
`SDL_VIDEORESIZE`, `SDL_OPENGL` and `SDL_FULLSCREEN` — **all SDL 1.2 API that
does not exist in SDL2 at all**. Code calling them could not have compiled.
Every one turns out to be inside a comment or commented out.

Measured properly:

1. **strip comments** before matching;
2. **exclude the emulators' own `SDL_<MODULE>_Name` identifiers**;
3. count `SDL_CamelCase(` calls.

Result: **`SDL_GetTicks` x12, in one file.** That is the whole genuine SDL
library surface of the entire emulator tree.

## The one exception (SDL calls)

| File | Why it is live |
|---|---|
| `src/Emulators/atari800/sdl/a-input.c` | **12 live `SDL_GetTicks()` calls.** It carries a different marker (`[C64D-REFERENCE-LIVE-EXCEPTION]`) and **is** ported when SDL changes — e.g. SDL3 widened `SDL_GetTicks()` from `Uint32` to `Uint64`. |

## Live-exception files (compiled and called)

These carry `[C64D-REFERENCE-LIVE-EXCEPTION]` instead of `[C64D-REFERENCE-ONLY]`.
They are exempt from the SDL-call check in `tests/test-reference-files.sh` and
must be maintained like ordinary source, not like reference material.

| File | Why it is live |
|---|---|
| `src/Emulators/atari800/sdl/a-input.c` | The SDL exception above. |
| `src/Emulators/vice/arch/socketimpl.h` | Socket platform header (`SOCKET`, `TIMEVAL`, `closesocket`, `INVALID_SOCKET`, `ARCHDEP_SOCKET_ERROR`). `root/socket.c` includes it whenever `HAVE_NETWORK` is defined — which it is, since the IDE64 USB server was enabled (2026-09). Contains no SDL calls. |

`archdep_network_init()` / `archdep_network_shutdown()` are NOT here: each
platform's own `platform/<OS>/src.<OS>/archdep.c` has always defined them
(unconditionally, WSAStartup included on Windows). A vendored copy under
`arch/` would be a duplicate symbol — this was tried and reverted 2026-09.

---

## The registry

Every file below carries a `[C64D-REFERENCE-ONLY]` header marker, except the
two live-exception files listed above (`a-input.c`, `socketimpl.h`), which
carry `[C64D-REFERENCE-LIVE-EXCEPTION]`.
`tests/test-reference-files.sh` fails if a marker and this table ever disagree.

Rows are sorted by path; any later addition is **appended** rather than
inserted, because inserting alphabetically would renumber every following row
for no benefit. Do not re-sort.

| # | File | Vendored from |
|---|---|---|
| 1 | `src/Emulators/atari800/sdl/a-init.c` | atari800 |
| 2 | `src/Emulators/atari800/sdl/a-init.h` | atari800 |
| 3 | `src/Emulators/atari800/sdl/a-input.c` | atari800 |
| 4 | `src/Emulators/atari800/sdl/a-input.h` | atari800 |
| 5 | `src/Emulators/atari800/sdl/a-main.c` | atari800 |
| 6 | `src/Emulators/atari800/sdl/a-palette.c` | atari800 |
| 7 | `src/Emulators/atari800/sdl/a-palette.h` | atari800 |
| 8 | `src/Emulators/atari800/sdl/a-sdl-sound.c` | atari800 |
| 9 | `src/Emulators/atari800/sdl/a-video.c` | atari800 |
| 10 | `src/Emulators/atari800/sdl/a-video.h` | atari800 |
| 11 | `src/Emulators/atari800/sdl/a-video_gl.c` | atari800 |
| 12 | `src/Emulators/atari800/sdl/a-video_gl.h` | atari800 |
| 13 | `src/Emulators/atari800/sdl/a-video_sw.c` | atari800 |
| 14 | `src/Emulators/atari800/sdl/a-video_sw.h` | atari800 |
| 15 | `src/Emulators/vice/arch/archdep.h` | VICE |
| 16 | `src/Emulators/vice/arch/archdep_defs.h` | VICE |
| 17 | `src/Emulators/vice/arch/archdep_dir.h` | VICE |
| 18 | `src/Emulators/vice/arch/archdep_exit.h` | VICE |
| 19 | `src/Emulators/vice/arch/archdep_file_io.c` | VICE |
| 20 | `src/Emulators/vice/arch/archdep_sleep.h` | VICE |
| 21 | `src/Emulators/vice/arch/archdep_tick.c` | VICE |
| 22 | `src/Emulators/vice/arch/archdep_tick.h` | VICE |
| 23 | `src/Emulators/vice/arch/archdep_unix.h` | VICE |
| 24 | `src/Emulators/vice/arch/archdep_win32.h` | VICE |
| 25 | `src/Emulators/vice/arch/blockdev.c` | VICE |
| 26 | `src/Emulators/vice/arch/c64-hardsid.c` | VICE |
| 27 | `src/Emulators/vice/arch/catweaselmkiii.c` | VICE |
| 28 | `src/Emulators/vice/arch/console.c` | VICE |
| 29 | `src/Emulators/vice/arch/coproc.c` | VICE |
| 30 | `src/Emulators/vice/arch/coproc.h` | VICE |
| 31 | `src/Emulators/vice/arch/dynlib.c` | VICE |
| 32 | `src/Emulators/vice/arch/fullscreen.c` | VICE |
| 33 | `src/Emulators/vice/arch/fullscreenarch.h` | VICE |
| 34 | `src/Emulators/vice/arch/joy.c` | VICE |
| 35 | `src/Emulators/vice/arch/joy.h` | VICE |
| 36 | `src/Emulators/vice/arch/kbd.c` | VICE |
| 37 | `src/Emulators/vice/arch/kbd.h` | VICE |
| 38 | `src/Emulators/vice/arch/lightpendrv.c` | VICE |
| 39 | `src/Emulators/vice/arch/lightpendrv.h` | VICE |
| 40 | `src/Emulators/vice/arch/menu_c64_common_expansions.c` | VICE |
| 41 | `src/Emulators/vice/arch/menu_c64_common_expansions.h` | VICE |
| 42 | `src/Emulators/vice/arch/menu_c64_expansions.c` | VICE |
| 43 | `src/Emulators/vice/arch/menu_c64_expansions.h` | VICE |
| 44 | `src/Emulators/vice/arch/menu_c64cart.c` | VICE |
| 45 | `src/Emulators/vice/arch/menu_c64cart.h` | VICE |
| 46 | `src/Emulators/vice/arch/menu_c64hw.c` | VICE |
| 47 | `src/Emulators/vice/arch/menu_c64hw.h` | VICE |
| 48 | `src/Emulators/vice/arch/menu_c64model.c` | VICE |
| 49 | `src/Emulators/vice/arch/menu_c64model.h` | VICE |
| 50 | `src/Emulators/vice/arch/menu_common.c` | VICE |
| 51 | `src/Emulators/vice/arch/menu_common.h` | VICE |
| 52 | `src/Emulators/vice/arch/menu_debug.c` | VICE |
| 53 | `src/Emulators/vice/arch/menu_debug.h` | VICE |
| 54 | `src/Emulators/vice/arch/menu_drive.c` | VICE |
| 55 | `src/Emulators/vice/arch/menu_drive.h` | VICE |
| 56 | `src/Emulators/vice/arch/menu_drive_rom.c` | VICE |
| 57 | `src/Emulators/vice/arch/menu_drive_rom.h` | VICE |
| 58 | `src/Emulators/vice/arch/menu_ffmpeg.c` | VICE |
| 59 | `src/Emulators/vice/arch/menu_ffmpeg.h` | VICE |
| 60 | `src/Emulators/vice/arch/menu_help.c` | VICE |
| 61 | `src/Emulators/vice/arch/menu_help.h` | VICE |
| 62 | `src/Emulators/vice/arch/menu_joystick.c` | VICE |
| 63 | `src/Emulators/vice/arch/menu_joystick.h` | VICE |
| 64 | `src/Emulators/vice/arch/menu_lightpen.c` | VICE |
| 65 | `src/Emulators/vice/arch/menu_lightpen.h` | VICE |
| 66 | `src/Emulators/vice/arch/menu_midi.c` | VICE |
| 67 | `src/Emulators/vice/arch/menu_midi.h` | VICE |
| 68 | `src/Emulators/vice/arch/menu_mouse.c` | VICE |
| 69 | `src/Emulators/vice/arch/menu_mouse.h` | VICE |
| 70 | `src/Emulators/vice/arch/menu_network.c` | VICE |
| 71 | `src/Emulators/vice/arch/menu_network.h` | VICE |
| 72 | `src/Emulators/vice/arch/menu_printer.c` | VICE |
| 73 | `src/Emulators/vice/arch/menu_printer.h` | VICE |
| 74 | `src/Emulators/vice/arch/menu_ram.c` | VICE |
| 75 | `src/Emulators/vice/arch/menu_ram.h` | VICE |
| 76 | `src/Emulators/vice/arch/menu_reset.c` | VICE |
| 77 | `src/Emulators/vice/arch/menu_reset.h` | VICE |
| 78 | `src/Emulators/vice/arch/menu_rom.c` | VICE |
| 79 | `src/Emulators/vice/arch/menu_rom.h` | VICE |
| 80 | `src/Emulators/vice/arch/menu_rs232.c` | VICE |
| 81 | `src/Emulators/vice/arch/menu_rs232.h` | VICE |
| 82 | `src/Emulators/vice/arch/menu_screenshot.c` | VICE |
| 83 | `src/Emulators/vice/arch/menu_screenshot.h` | VICE |
| 84 | `src/Emulators/vice/arch/menu_settings.c` | VICE |
| 85 | `src/Emulators/vice/arch/menu_settings.h` | VICE |
| 86 | `src/Emulators/vice/arch/menu_sid.c` | VICE |
| 87 | `src/Emulators/vice/arch/menu_sid.h` | VICE |
| 88 | `src/Emulators/vice/arch/menu_snapshot.c` | VICE |
| 89 | `src/Emulators/vice/arch/menu_snapshot.h` | VICE |
| 90 | `src/Emulators/vice/arch/menu_sound.c` | VICE |
| 91 | `src/Emulators/vice/arch/menu_sound.h` | VICE |
| 92 | `src/Emulators/vice/arch/menu_speed.c` | VICE |
| 93 | `src/Emulators/vice/arch/menu_speed.h` | VICE |
| 94 | `src/Emulators/vice/arch/menu_tape.c` | VICE |
| 95 | `src/Emulators/vice/arch/menu_tape.h` | VICE |
| 96 | `src/Emulators/vice/arch/menu_tfe.c` | VICE |
| 97 | `src/Emulators/vice/arch/menu_tfe.h` | VICE |
| 98 | `src/Emulators/vice/arch/menu_vic20cart.c` | VICE |
| 99 | `src/Emulators/vice/arch/menu_vic20cart.h` | VICE |
| 100 | `src/Emulators/vice/arch/menu_vic20hw.c` | VICE |
| 101 | `src/Emulators/vice/arch/menu_vic20hw.h` | VICE |
| 102 | `src/Emulators/vice/arch/menu_video.c` | VICE |
| 103 | `src/Emulators/vice/arch/menu_video.h` | VICE |
| 104 | `src/Emulators/vice/arch/mousedrv.c` | VICE |
| 105 | `src/Emulators/vice/arch/mousedrv.h` | VICE |
| 106 | `src/Emulators/vice/arch/parsid.c` | VICE |
| 107 | `src/Emulators/vice/arch/rawnetarch.c` | VICE |
| 108 | `src/Emulators/vice/arch/rawnetarch.h` | VICE |
| 109 | `src/Emulators/vice/arch/rs232.c` | VICE |
| 110 | `src/Emulators/vice/arch/rs232dev.c` | VICE |
| 111 | `src/Emulators/vice/arch/rs232dev.h` | VICE |
| 112 | `src/Emulators/vice/arch/rs232net.c` | VICE |
| 113 | `src/Emulators/vice/arch/rs232net.h` | VICE |
| 114 | `src/Emulators/vice/arch/signals.c` | VICE |
| 115 | `src/Emulators/vice/arch/socketimpl.h` | VICE (**live exception** — socket platform header, see above) |
| 116 | `src/Emulators/vice/arch/ui.c` | VICE |
| 117 | `src/Emulators/vice/arch/ui.h` | VICE |
| 118 | `src/Emulators/vice/arch/uicmdline.c` | VICE |
| 119 | `src/Emulators/vice/arch/uifilereq.c` | VICE |
| 120 | `src/Emulators/vice/arch/uifilereq.h` | VICE |
| 121 | `src/Emulators/vice/arch/uihotkey.c` | VICE |
| 122 | `src/Emulators/vice/arch/uihotkey.h` | VICE |
| 123 | `src/Emulators/vice/arch/uimenu.c` | VICE |
| 124 | `src/Emulators/vice/arch/uimenu.h` | VICE |
| 125 | `src/Emulators/vice/arch/uimon.c` | VICE |
| 126 | `src/Emulators/vice/arch/uimsgbox.c` | VICE |
| 127 | `src/Emulators/vice/arch/uimsgbox.h` | VICE |
| 128 | `src/Emulators/vice/arch/uipause.c` | VICE |
| 129 | `src/Emulators/vice/arch/uipoll.c` | VICE |
| 130 | `src/Emulators/vice/arch/uipoll.h` | VICE |
| 131 | `src/Emulators/vice/arch/uistatusbar.c` | VICE |
| 132 | `src/Emulators/vice/arch/uistatusbar.h` | VICE |
| 133 | `src/Emulators/vice/arch/vice310_stubs.c` | VICE |
| 134 | `src/Emulators/vice/arch/video.c` | VICE |
| 135 | `src/Emulators/vice/arch/videoarch.h` | VICE |
| 136 | `src/Emulators/vice/arch/vkbd.c` | VICE |
| 137 | `src/Emulators/vice/arch/vkbd.h` | VICE |
| 138 | `src/Emulators/vice/arch/vsidui_sdl.h` | VICE |
| 139 | `src/Emulators/vice/arch/vsyncarch.c` | VICE |
| 140 | `src/Emulators/vice/arch/x64sc_ui.c` | VICE |

**Total: 140 files.**
