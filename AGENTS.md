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

## Known state (2026-09-18, late)

All three platforms green at devel tip `ac1c305`, both matrix legs each:

- Linux: both legs (`35358146083`).
- Windows: both legs (`35358151886`).
- macOS: both legs, `ALL TESTS PASSED (72/72)` (`35358149322`) -- first fully
  green macOS batch of the detach/attach saga.

What got fixed since `35308230558` (in merge order):

- `5808d40` lldb wrapper removed from build-macos.yml; run_test.sh treats a
  complete passing results file as authoritative over the wrapper exit.
- `e566787` attach while PAUSED now runs `cartridge_attach_image` +
  synchronous `maincpu_reset()` (a queued IK_RESET is never drained while
  the CPU loop is stopped).
- `69ba177` attach while RUNNING went through `cartridge_attach_image` on
  the main thread (raced mem_powerup's zeroed RAM -> BRK auto-pause ->
  cart never mapped); now runs on the CPU thread via
  `CDebugInterfaceViceTaskAttachCartridge` (task queue, template
  `CDebugInterfaceViceTaskReset`).
- `fba993d` run_test.sh polls ~30s for the macOS crash report instead of
  one instant check.
- `552d1a1` IDE64 attach/detach + usbserver resource setters also run on
  the emulation thread when the machine is running: `usbserver_activate()`
  creates/destroys `usb_alarm` on `maincpu_alarm_context` (a queue the CPU
  loop walks every clock) and the teardown cancels the listener sockets --
  the Ide64UsbListener sigsegv source ("no crash report found", 2 of 4
  arm64 runs, always ~0.9s after the test client closed, right at Step 4
  `SetIde64UsbServerEnabled(false)` -> `DetachIde64Cartridge()`).
- `ac1c305` adds `WaitCpuDebugInterruptTasksApplied()`: queueing a task
  breaks the settings chain's synchronous set-then-read contract (first
  attempt failed Ide64Settings 71/72, both macOS legs, reading the stale
  resource before the CPU thread drained). The wait is bounded at 2s and
  bails when the debugger leaves RUNNING.

Not yet fully closed: `VicePlatformAbstraction` frame pacing (36 vs 31
frames/700ms) has not recurred since; revisit it separately if it returns.
