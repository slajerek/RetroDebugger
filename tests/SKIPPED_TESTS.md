# Skipped tests

Tests that are registered in the source but **commented out of the suite**.

**A skipped test is invisible.** Nothing fails, nothing warns, and the suite
goes green — which is exactly why this file exists and why every skip is
bracketed in the source with

```
// [SKIPPED-TEST: <name>] disabled <date>.
```

`grep -rn "SKIPPED-TEST" src/` finds them all in one command, and
`tests/test-skipped-tests.sh` fails if a marker has no row here, or a row has no
marker.

**The re-enable condition is per test, not global.** One of these is waiting on
*work*; one is waiting on a *fix*. Turning them all back on together would
re-break the suite for the wrong reason.

## The skips

| Test | Disabled | Why | **Re-enable when** |
|---|---|---|---|
| `GT2InstrumentOps` | 2026-08-18 | Fails **consistently**: *"migration: gt2 has key=0, global has key=0"*. Not flaky — it is describing real unfinished GT2 migration work. | **GT2 work restarts in c64d.** Turn it on at the START of that work, not the end: it is a specification of what the migration owes. |
| `XPartyStripeAnim` | 2026-08-18 | **FLAKY, which is a different problem.** Across three runs it gave *"1/2 stripe states were NOT a rotation"*, *"no samples landed in the sim main loop"*, and a pass. | **The flakiness is fixed.** NOT tied to GT2. A test with a different answer each run cannot distinguish a regression from noise. |

## Why they were skipped now

The SDL3/HDR programme needs a **stable baseline** for c64d before an SDL
major-version bump touches input, audio and windowing at once. Measured
beforehand, the suite gave
**78/82, then 80/82, then a three-name failure set** — the count itself moved,
so comparing before/after would have been worthless in both directions: a port
breaking two tests could read 80/82 and look clean.

**These three are removed to make the remaining ones trustworthy, not to make
the number look better.**

## The baseline, MEASURED

**79/79, three consecutive headless runs, 2026-08-18** (Release, macOS,
`SDL_AUDIODRIVER=dummy`). The claim this file exists to support is that the
count stopped moving — so the count was *observed* three times rather than
predicted once from 82 minus 3. Any deviation from 79/79 after an SDL3 port is
now a real signal.

```
SDL_AUDIODRIVER=dummy "build-macos/Build/Products/Release/Retro Debugger.app/Contents/MacOS/Retro Debugger" \
    --headless --run-suite --exit-after-tests
```

## Before re-enabling any of them

Run the suite several times first and confirm the *rest* is stable. A 78/82 run
implied **four** failures while only three were ever named, so at least one more
intermittent test may still be in there — and it will be much easier to find
against an otherwise-quiet baseline.

## The ImGui UI suite — currently failing, kept in the run

Measured 2026-09-17 (private and public trees identical): `--imgui` gives
**13/17 private / 11/15 public**; the two-test delta is the private-only
`instruments_browser_*` pair. Four names fail in BOTH trees the same way, so
they are a **baseline**, not an export regression:

| Test | Why it fails today |
|---|---|
| `default_workspaces_create_and_menu` | pre-existing UI fail, seen before the export work |
| `autolayout_preserve_scan_fixture` | pre-existing UI fail, seen before the export work |
| `autolayout_preserve_scan_fixture_atari` | pre-existing UI fail, seen before the export work |
| `dock_focus_keyboard` | needs a layouts fixture with GT2 docked; without one the focus path never engages cleanly |

`dock_focus_keyboard`'s body treats "GT2 plugin not docked" as a skip — the
fail above is the docked case actually misbehaving, so it stays in the run
until fixed rather than being hidden.
