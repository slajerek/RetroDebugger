# Agent notes

## CI: always check all three platforms

Every build-relevant change must be verified on ALL THREE platforms before
the work counts as done:

- Linux   -- `.github/workflows/build-linux.yml`
- macOS   -- `.github/workflows/build-macos.yml`
- Windows -- `.github/workflows/build-windows.yml`

Green on one or two platforms is not green. The runners differ in ways a
local machine cannot reproduce (arm64 macOS off-logs leg, ClangCL+MSBuild on
Windows, Music scheduler timing), and failures in the past landed only on
the platform nobody checked:

- 2026-09-17/18: the 1.0.1 culling deleted the Windows vcxproj include list
  and the `md5/MD5.h` qualifier; Linux looked fine the whole time because
  CMake carried that state nowhere else.
- 2026-09-18: the Ide64UsbListener teardown segfault reproduced only on the
  GitHub arm64 macOS runners (USB-server teardown vs the emulation thread).
- `DetachCartridgePaused` / `VicePlatformAbstraction` flake only under CI
  scheduling pressure (step polls hardened; frame pacing still sensitive).

After pushing, watch `gh run list` until every workflow's BOTH matrix legs
(logs on/off) finish; a job that only produced the Linux leg is not evidence
for the other two.

## How to debug the macOS CI failures

- `tests/run_test.sh` prints the newest macOS crash report
  (`~/Library/Logs/DiagnosticReports/Retro Debugger-*.ips`) whenever the app
  dies abnormally, so a silent sigsegv in CI still logs the faulting frame.
- `MT_TEST_LLDB=1 ./tests/run_test.sh --skip-build` launches the app under
  `lldb -b -o run -o "bt all" -o quit` -- use it when the crash reporter has
  nothing. Note: lldb slows scheduling and can HIDE an otherwise
  deterministic race (us: the segfault disappeared under it).
- macOS workflows pin the engine via `MTENGINE_REF` (devel: `origin/devel`),
  cloned fresh every run; engine-side fixes go to `slajerek/MTEngineSDL`
  branch `devel` FIRST, then the app push (the macOS/macOS-bundle fix lives
  in the engine driver: `-derivedDataPath $APP_DIR/build-macos`).

## Known state (2026-09-18)

- Linux: green (`35301512684`).
- Windows: green, both legs (`35304298951`).
- macOS: no build/staging failures, no segfaults; two remaining CI flakes:
  `DetachCartridgePaused` step 5 (re-attach after a running detach
  intermittently never maps on the off-logs standalone leg) and
  `VicePlatformAbstraction` frame pacing (36 vs 31 frames/700ms). Both
  documented with evidence under runs `35308230558`, `35306224267`.
