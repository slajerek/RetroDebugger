# RetroDebugger WebSocket regression suite

A pytest suite that exercises RetroDebugger's JSON-over-WebSocket debugger API
(port 3563). It captures how the **current VICE 3.1 build behaves** so the
in-progress VICE 3.1 → 3.10 upgrade can be validated against a stable baseline.
Tests assert *observable behavior* (register values, memory contents, cycle
counts) rather than internal emulator state, so they remain meaningful across
the version jump.

See `../docs/superpowers/specs/2026-05-27-vice-3.10-upgrade-design.md` for the
upgrade design and `../docs/vice-hook-surface.md` for the VICE modification
catalog.

## Prerequisites

1. **RetroDebugger must be running** with the WebSocket server enabled on
   `ws://127.0.0.1:3563/stream`.

   - Enable the server once by setting `RunDebuggerServerWebSockets: true` in
     `~/Library/RetroDebugger/settings.hjson`.
   - Launch:
     ```sh
     "/Applications/Retro Debugger.app/Contents/MacOS/Retro Debugger" -c64 -unpause &
     ```
   - Give it ~6 seconds to bind the port. Verify: `nc -z 127.0.0.1 3563 && echo READY`.

2. **True Drive Emulation must be on** (the drive-memory tests need it):
   Settings → Emulator → C64 → Drive emulation → True drive.

3. **Python deps** are installed in a local virtualenv at `tests/.venv`
   (created automatically the first time; see Setup below).

## Setup (first time only)

```sh
cd tests
python3 -m venv .venv
source .venv/bin/activate
pip install -r requirements.txt
```

The `.venv/` directory is gitignored.

## Building RetroDebugger from source (macOS)

To verify source changes (e.g. the VICE upgrade work), you must run a build of
*this repo*, not the pre-installed release app. Verified working procedure on
macOS (Xcode 26.5, Apple Silicon):

**Prerequisites (already satisfied on this machine — no downloads needed):**
- Xcode installed (`xcodebuild -version`).
- The sibling library **MTEngineSDL** must resolve at `../MTEngineSDL` relative
  to this repo, i.e. `~/Projects/MTEngineSDL`. It actually lives at
  `~/develop/MTEngineSDL`, so create a symlink once:
  ```sh
  ln -s ~/develop/MTEngineSDL ~/Projects/MTEngineSDL
  ```
- SDL2 is bundled as a prebuilt static lib inside MTEngineSDL
  (`~/develop/MTEngineSDL/platform/MacOS/libs/libSDL2.a`) — nothing to install.
- MTEngineSDL builds via its own `platform/MacOS/build.sh`; the RD Xcode project
  links it automatically.

**Build (use Release — see warning below):**
```sh
cd platform/MacOS
xcodebuild -project c64d.xcodeproj -scheme "Retro Debugger" \
  -configuration Release -arch arm64 \
  CODE_SIGN_IDENTITY="" CODE_SIGNING_REQUIRED=NO CODE_SIGNING_ALLOWED=NO build
```
- `CODE_SIGNING_*=NO` is required: the project is configured for the original
  author's Apple Developer team, which you don't have. Unsigned is fine for a
  locally-run build.
- The built app lands at:
  `~/Library/Developer/Xcode/DerivedData/c64d-*/Build/Products/Release/Retro Debugger.app`

**⚠️ Use the Release configuration, not Debug.** The Debug config enables the
ImGui test engine (`-DENABLE_IMGUI_TEST_ENGINE`), which crashes under the
scripted WebSocket workload (`ImGuiTestEngine_CrashHandler() Crashed`). Release
is stable and matches the conditions the suite baseline was captured against.

**Run the from-source build and point the suite at it:**
```sh
pkill -f "Retro Debugger"; sleep 2
APP=~/Library/Developer/Xcode/DerivedData/c64d-*/Build/Products/Release/"Retro Debugger.app/Contents/MacOS/Retro Debugger"
"$APP" -c64 -unpause &
until nc -z 127.0.0.1 3563; do sleep 1; done   # wait for the WebSocket port
./tests/run-ws-tests.sh
```
The from-source Release build reproduces the baseline (`28 passed, 1 skipped,
1 xfailed`), so it is the build to verify all source changes against.

## Running the tests

From the repository root, use the runner script (it activates the venv for you):

```sh
./tests/run-ws-tests.sh
```

A clean run looks like:

```
28 passed, 1 skipped, 1 xfailed in ~21s
```

The `1 skipped` and `1 xfailed` are **expected** — they track two real,
known bugs in the current 3.1 build (see "Known non-passing tests" below).
A run is healthy as long as it reports `28 passed` with **0 failed**.

### Variations

```sh
./tests/run-ws-tests.sh -v                      # one line per test
./tests/run-ws-tests.sh websocket/test_cpu.py   # a single file
./tests/run-ws-tests.sh -k warp                 # tests matching a name
./tests/run-ws-tests.sh --durations=10          # show the 10 slowest tests
```

### Running pytest directly

```sh
cd tests
source .venv/bin/activate
pytest websocket/ -v
```

## What the suite covers

| File | Endpoint family |
|---|---|
| `websocket/test_lifecycle.py` | reset, pause/continue, warp |
| `websocket/test_cpu.py` | cpu/status, counters, makejmp, step/instruction, step/cycle |
| `websocket/test_memory.py` | main memory + drive 1541 read/write, RAM-vs-I/O views |
| `websocket/test_breakpoints.py` | CPU PC breakpoints, memory-access breakpoints |
| `websocket/test_chips.py` | VIC, CIA, SID register read/write |
| `websocket/test_input.py` | keyboard, joystick |
| `websocket/test_load.py` | top-level `load` (PRG) |

Shared fixtures live in `conftest.py`:

- `rd` — one session-scoped WebSocket connection
- `fresh_cpu` — hard-resets the machine before each test (and forces warp off)
- `loaded_fixture` — loads `fixtures/known_state.prg`, runs it to the `park`
  loop at `$0837`, and leaves the CPU paused there with known sentinels in
  memory (`$C000-$C003 = AA BB CC DD`, screen cleared)

`retrodebugger.py` is the WebSocket client. Its `_call()` sends a unique
`token` per request and matches it in the response, skipping asynchronous event
frames — preserve that correlation if you edit the client, or event frames will
race the responses.

## Test fixture

`fixtures/known_state.prg` is a tiny 6502 program (source:
`fixtures/known_state.asm`) that establishes a known machine state. Rebuild it
with KickAssembler after editing the source:

```sh
./tests/fixtures/build.sh
```

`park` is at `$0837`; tests that reference it use that address.

## Known non-passing tests (current 3.1 bugs — Phase 4 fix targets)

- **`test_load.py::test_load_nonexistent_file_does_not_crash`** *(skipped)* —
  loading a nonexistent path crashes the WebSocket connection (no close frame)
  instead of returning an error. 3.10 should return a 4xx and keep the
  connection alive.
- **`test_input.py::test_key_down_accepted_by_api`** *(xfail)* — `key/down`
  calls `alarm_set()` without holding the emulator mutex from the WebSocket
  thread, so the KERNAL keyboard buffer isn't reliably populated. The fix is to
  wrap `key/down`/`key/up` in `LockMutex` in `CDebuggerServerApiVice.cpp`.

These are documented in full in `../docs/superpowers/notes/2026-05-27-phase-0-baseline.md`.
