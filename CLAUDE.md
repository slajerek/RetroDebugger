# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with
code in this repository. It is deliberately **not** the maintainer's private working-tree guidance
file — the private checkout keeps its own separate `CLAUDE.md` covering
internal machinery this repo does not have. Agent notes about
CI state and workflows live in `AGENTS.md` (read it before pushing/fetching
CI advice).

## Git & GitHub hygiene

**Never push AI-coding artifacts to GitHub.** Implementation plans, design
specs, and any AI-assistant working files — `.claude/`, `.superpowers/`,
`claude/`, `docs/superpowers/` — must NOT be committed to this repository.
They are gitignored; keep it that way. `CLAUDE.md` / `AGENTS.md` are the only
AI-related files intentionally tracked.

**No attribution trailers in commits.** A commit message here carries no
`Co-Authored-By:` and no "Generated with ..." line.

Commit locally; the maintainer pushes.

## Project Overview

Retro Debugger (v1) is a real-time debugger for 8-bit computers:
Commodore 64 (and C64 Ultimate), Atari XL/XE, and NES. It embeds full emulator
engines (VICE v3.10-WIP, Atari800, NestopiaUE) and provides cycle-accurate
debugging through an ImGui-based interface. Previously known as
"C64 65XE NES Debugger".

## Logging

MTEngineSDL's one logger, behaviour decided by the BUILD, not by source edits:

- `LOGD()`, `LOGM()`, `LOGVD()`, ... compile to `do {} while (0)` no-ops unless
  the engine was built with `MT_DEBUG_LOGS=1` (a compiler define).
- `LOGFatal()` / `LOGError()` / `LOGNError()` are ALWAYS compiled in — error
  and fatal levels reach stderr and the log file on every build.
- `MT_DEBUG_LOGS` is decided by the capability/build channel, not by editing
  headers: `./build-macos.sh --logs on|off` (dev builds default on; `--prod`
  defaults off unless `--logs on`). A verbose-log change is a rebuild, never
  an edit to `src/Engine/Core/DBG_Log.h` (engine) or any platform `DBG_Log.*`.
- Log files default to `~/Library/Caches/RetroDebugger-*.txt` on macOS;
  `--log-dir /tmp` redirects them at runtime (the test runner already does).

## Build Commands

The root `build-<os>.sh` / `build-windows.ps1` are app **stubs**: the app owns
`mtengine.caps` (what capabilities to enable), `mtengine-app.conf` (what to
build) and `MTENGINE_REF` (the pinned engine revision); the stub clones
MTEngineSDL at that pin when it is absent, then hands over to the engine's
appbuild driver (`MTEngineSDL/tools/appbuild/app-build-macos.sh` etc.), which
owns the whole flow — capability resolution into `MT_ENABLE_*` /
`MT_DEBUG_LOGS` compiler defines, keyed dependency staging, the engine target,
then the app target. Do not re-implement that flow by hand.

- Capability semantics and the full `MT_CAP_*` table: generated reference in
  the engine mirror —
  [docs/CAPABILITIES.md](https://github.com/slajerek/MTEngineSDL/blob/devel/docs/CAPABILITIES.md).
- Why there are two build channels (script vs IDE), what each supplies, and
  how dev/build caches interact:
  [docs/build-channels.md](https://github.com/slajerek/MTEngineSDL/blob/devel/docs/build-channels.md).
- Headless test procedure:
  [docs/testing.md](https://github.com/slajerek/MTEngineSDL/blob/devel/docs/testing.md).

### macOS
```bash
./build-macos.sh                # development build, incremental (--prod packages a release)
./build-macos.sh --logs off     # verbose log macros compiled out
```
Produces `build-macos/Build/Products/Release/Retro Debugger.app`.

**Do NOT build with a bare `xcodebuild -project platform/MacOS/c64d.xcodeproj`.**
The wrapper first stages the vendored uSockets dependency for the current
engine revision and resolves `mtengine.caps` into the `MT_ENABLE_*` defines
that BOTH the engine and app targets must be compiled with; a bare `xcodebuild`
builds the engine with engine-defaults while linking against the
capability-keyed libraries this manifest produced, and the link fails on
missing symbols with nothing to suggest why. Run `./build-macos.sh` once first,
then Xcode's own Build of the "Retro Debugger" scheme (the scheme phases
supply the same channel) is the supported IDE path.

### Linux (CMake)
```bash
./build-linux.sh        # clones MTEngineSDL + uSockets, builds everything
# After it has run once, an incremental `make -C build retrodebugger` works.
```
A bare `cmake` configure cannot resolve `mtengine.caps`; use the driver.

### Windows
```powershell
.\build-windows.ps1              # development build; -Prod for a package
```
Or open `platform/Windows/c64d.sln` in Visual Studio 2019/2022 (F5 starts in
the git root; the tests run under Git Bash:
`bash tests/run_test.sh --binary platform/Windows/bin/x64/Release/c64d.exe`).

### Keeping Build Projects in Sync
**IMPORTANT:** when adding, removing, or renaming source files in the Xcode
project (`platform/MacOS/c64d.xcodeproj`), you MUST also update the Linux
CMake (`CMakeLists.txt`) and the Windows Visual Studio
(`platform/Windows/c64d/c64d.vcxproj` + `.vcxproj.filters`) projects. All
three build systems use explicit file lists — there is no auto-discovery.
Add each file EXACTLY ONCE per project; a duplicate `ClCompile` entry makes
MSBuild warn MSB8027 about two outputs racing to one `.obj` — treat that
warning as an error. The same rule applies to MTEngineSDL's three projects.

### Critical dependency
**MTEngineSDL** must exist at `../MTEngineSDL` relative to this repo (the
stubs clone it at the revision pinned by `MTENGINE_REF`). It provides SDL3 +
ImGui integration, the GUI framework (`CGuiView`, `guiMain`), and all platform
abstractions. Repo: https://github.com/slajerek/MTEngineSDL

**IMPORTANT: do NOT modify MTEngineSDL directly.** It is an external library.
If new functionality is needed in MTEngineSDL, propose it in the engine repo
(via PR/issue) or flag it to the maintainer; never land app work as engine
commits.

**Do not create git worktrees; they are not supported by c64d or MTEngineSDL.**

## Architecture

### Emulator Abstraction Layer
`CDebugInterface` (in `src/DebugInterface/`) is the abstract base class all
emulators implement. Concrete implementations:
- `CDebugInterfaceVice` (C64, inherits `CDebugInterfaceC64`) in
  `src/Emulators/vice/ViceInterface/`
- `CDebugInterfaceAtari` in `src/Emulators/atari800/AtariInterface/`
- `CDebugInterfaceNes` in `src/Emulators/nestopiaue/NestopiaInterface/`

Emulators are enabled/disabled via `#define` flags in
`src/Emulators/EmulatorsConfig.h` (`RUN_COMMODORE64`, `RUN_ATARI`, `RUN_NES`).

Embedded upstream sources under `src/Emulators/vice/arch/` and
`src/Emulators/atari800/sdl/` are mostly **reference copies** carrying a
`[C64D-REFERENCE-ONLY]` header — they must not grow live SDL calls
(`tests/test-reference-files.sh` enforces it; `src/Emulators/REFERENCE_FILES.md`
explains the measurement).

### Data Adapter Pattern
`CDebugDataAdapter` provides uniform memory access across different address
spaces (C64 RAM, cartridge, REU, 1541 drive RAM, NES PPU/OAM, Atari regions).
All generic memory views (hex dump, data map, watches) work through this
abstraction.

### View System
Views inherit from `CGuiView` (MTEngineSDL base class) and render via
`RenderImGui()`. Key views are in `src/Views/` with platform-specific
subfolders (`C64/`, `Atari800/`, `Nes/`).

### Central Coordinator
`CViewC64` (`src/Screens/CViewC64.cpp`) is the main application view: manages
emulator instances, layout switching, and coordinates multi-threaded
emulation. Despite the name, it manages all emulated platforms.

### App Lifecycle
Entry point is `src/RetroDebuggerAppInit.cpp`. MTEngineSDL calls
`MT_PreInit()` -> `MT_PostInit()` (creates `CViewC64`). Settings folder:
"RetroDebugger".

### Plugin System
Plugins extend `CDebuggerEmulatorPlugin` and hook into frame rendering and
input. Registered in `src/Plugins/C64D_InitPlugins.cpp`. Examples:
GoatTracker (GT2 music tracking), CRT maker.

### Symbol & Breakpoint System
`CDebugSymbols` manages labels, breakpoints, and watches organized by
`CDebugSymbolsSegment`. Supports VICE, KickAss, and other symbol file formats.
Serialized as HJSON.

### Task System
`CDebugInterfaceTask` handles deferred/thread-safe emulator state changes,
including VSync-synchronized operations. Machine-running mutations of
cartridge/USBServer state (IDE64, CRT attach/detach) must run on the
emulation thread through this queue — see the detach/attach saga records in
`AGENTS.md`.

### Remote Debugging
WebSocket-based server (`src/Remote/`) with a JSON command protocol, plus an
MCP tools endpoint (`src/Remote/MCP/`) with a startup-readiness gate: tools
are only served once the debugger server was published
(`SetDebuggerServer()`) or the call is the shutdown tool. Test client in
`tools/websockets-debugger-test/`.

### Menu Bar
`CMainMenuBar` (`src/Views/CMainMenuBar.cpp`) is the largest single file
(~4700 lines) containing all menu definitions and settings UI.

## Code Conventions

- Class names use `C` prefix (e.g. `CViewC64`, `CDebugInterface`)
- Logging via the MTEngineSDL macros above (`LOGD`/`LOGM` need
  `MT_DEBUG_LOGS`; `LOGError`/`LOGFatal` are always on)
- Configuration uses HJSON format (`CConfigStorageHjson`)
- Version string in `src/C64D_Version.h`
- Command-line parsing in `src/Tools/C64CommandLine.cpp`
- Settings storage in `src/Tools/C64SettingsStorage.cpp`

## GT2 Renoise Shortcuts

When adding or moving a functional key shortcut for the GoatTracker 2 Renoise
layout, update these places together or the UI starts lying about which key
does what: (1) the shortcut dispatcher (`CGT2RenoiseInput` / the focused GT2
view's `KeyDown`); (2) the automated GT2 shortcut tests; (3) the GoatTracker
plugin menu (`C64DebuggerPluginGoatTracker::RenderMainMenuImGui()`) — item
hints and the "Renoise Shortcuts" reference submenu; (4) the GT2 toolbar
tooltips (`CViewGT2Toolbar::GetControlTooltip()`); (5) the GT2 Song Settings
tooltips (`CViewGT2SongSettings::GetControlTooltip()`), which gate on
`keypreset` since the native SHIFT+F5..F8 bindings only arrive when the
preset is NOT `KEY_RENOISE`. Never hardcode the command-modifier word;
`GT2_CmdKey()` gives "Cmd" on macOS and "Ctrl" elsewhere. Do not add
GoatTracker-specific shortcut logic to `CMainMenuBar`; the main menu should
keep routing through generic plugin hooks.

## Writing Conventions

- **Never use `§` as a section marker.** Use `#` with the section number, e.g.
  `#11a.4`, `#15.4.1` — plain ASCII renders consistently across terminals,
  grep and diffs; the repo's spec style already uses `#N` cross-references.
- Keep commit subjects imperative and repo-scoped: `fix(vice): ...`,
  `ci(macos): ...`, `docs: ...`.

## Testing

Dual-framework test system in `src/Tests/` (ported from the engine's test
infrastructure):

1. **CTest/CTestSuite** — async integration tests (emulator state, memory ops,
   breakpoints). `src/Tests/CTestSuiteRetroDebugger.cpp` registers them;
   engine-side support: `MTEngineSDL/src/Engine/Tests/`.
2. **imgui_test_engine** — UI automation tests (menu verification, view
   interaction), registered in `src/Tests/CImGuiTests.cpp`.

Both run headlessly from the CLI. Engine-side reference:
[docs/testing.md](https://github.com/slajerek/MTEngineSDL/blob/devel/docs/testing.md).

### Running Tests

```bash
tests/run_test.sh                    # Release binary + the CTest suite
tests/run_test.sh EmulatorStartup    # one test by name
tests/run_test.sh --skip-build EmulatorStartup
tests/run_test.sh --imgui            # the ImGui UI suite (alias --imgui-tests)
tests/run_test.sh --imgui-test <filter>
tests/run_test.sh --timeout N        # guard against slow full suites
```

The runner always builds first unless `--skip-build`; it prints `Using
binary:` and runs from the git root (assets resolve via the CWD, never the
executable's location).

Known-failing ImGui tests are measured and recorded in
`tests/SKIPPED_TESTS.md`; deliberately disabled tests carry a
`// [SKIPPED-TEST: <name>] disabled <date>.` marker in the source and a row
in that file.

### Adding Tests

1. Create `src/Tests/CTestMyFeature.h/.cpp` inheriting from `CTest`
2. Register in `CTestSuite::RegisterTests()`
   (`src/Tests/CTestSuiteRetroDebugger.cpp`)
3. For UI tests, add to `RegisterRetroDebuggerTests()` in
   `src/Tests/CImGuiTests.cpp`

**Important:** emulators (C64, Atari, NES) can be enabled/disabled via the
File menu; not all may be running at test time. Tests that need a specific
emulator must check `di->isRunning`, call
`viewC64->StartEmulationThread(di)` + `SYS_Sleep(2000)` if not running, and
restore the original state with `viewC64->StopEmulationThread(di)` afterward.
See `CTestOpenAllViews` and `CTestStackAnnotation` for examples.

### Visual / VIC output validation

For tests that exercise on-screen rendering paths, RAM-level parity is
necessary but NOT sufficient: the same generated bytes can render to
different pixels depending on VIC config, screen/color RAM, sprite pointers
and multiplexer state. Such tests run via the PROD path (the Generate button's
code), hook a CPU breakpoint, frame-step via the VICE frame-step API, capture
`api->GetScreenImageWithoutBorders()`, and fail on C64-palette vs framebuffer
mismatches, always writing `/tmp/<testname>-screen.png` and
`/tmp/<testname>-state.txt`. 

### CI

Three workflows (`.github/workflows/build-{linux,macos,windows}.yml`), each
with two matrix legs (verbose logs on/off). A change counts as done only when
all three platforms are green on BOTH legs — runners differ in ways a local
machine cannot reproduce. See `AGENTS.md` for the current known state and the
macOS crash-report debugging procedure.

## MCP Skill

When the retrodebugger MCP server is connected (any `mcp__retrodebugger*`
tool is available), read `docs/mcp/retrodebugger-mcp-skill.md` before using
any MCP tools — tool usage guidelines, safe debugging defaults, and which
tool to use for which task (e.g. `retro_memory_search` for game-state
variables, not manual memory reads).
