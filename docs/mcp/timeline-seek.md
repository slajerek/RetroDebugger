# Timeline seek

`retro_timeline_seek` provides strict backward restoration for **C64, both PAL and NTSC**.  It uses retained native checkpoints and replay, not debugger-side PC polling or reverse execution of individual opcodes.

This is separate from `retro_timeline`, whose `c64/timeline/status` and `c64/timeline/frame` routes still expose existing frame navigation with queued acceptance only.  Their `completionKnown: false` contract does not become a precise-seek completion report.  The two request paths share an admission gate: a pending precision request makes coarse navigation busy, and a queued coarse restore prevents precision mutation until the CPU can safely admit it.  Capability/status reads remain nonmutating; `busy` includes either restore path.

The native route is `<platform>/timeline/seek`.  `atari800` and `nes` accept read-only capability status, but **all mutating modes return `unsupported_platform`** without pausing, allocating a request sequence, or dispatching a restore.  This restriction applies to native, MCP, and direct manager calls; it does not disable other Atari/NES debugger tools or legacy GUI features.  Those platforms require separate implementation and validation before timeline seeking is enabled.

## Requests

Every MCP call requires `platform` and exactly one `mode`.  The native endpoint omits `platform`, which is already part of its path.

| Mode | Additional fields | Meaning |
|---|---|---|
| `status` | None | Read coverage, counter units, recording/replay flags, and the latest request result without changing run state. |
| `last_read` | `address` | Restore to the recorded reader's instruction boundary. |
| `last_write` | `address` | Restore to the recorded writer's instruction boundary. |
| `cycle` | `cycle` | Restore to an exact backend recorded-counter coordinate. |
| `frame` | `frame` | Restore to a backend video-frame boundary. |
| `back` | `unit`, optional `count` | Resolve a backward distance from the admitted origin. |

`address` is an integer from 0 to 65535.  `cycle` and `frame` must be exact nonnegative integers; native representability and retained coverage are checked separately.  Do not pass large counters through floating-point conversions.

For `back`, `unit` is `instructions`, `video_frames`, or `emulated_seconds`.  `count` defaults to 1 and must be an integer from 1 to 1,000,000.  Mutating modes accept `timeoutMs` from 1 to 60,000, defaulting to 5,000.  Conflicting fields, unknown fields, fractional values, and implicit units are rejected rather than given an undocumented precedence.

```json
{"platform":"c64","mode":"status"}
{"platform":"c64","mode":"last_write","address":1024}
{"platform":"c64","mode":"back","unit":"instructions","count":3}
{"platform":"c64","mode":"back","unit":"video_frames"}
```

## Pause and completion

Mutating C64 requests require an **already paused** emulator with a running emulation thread.  They do not silently pause a running machine or advance an instruction merely to make admission possible.  Successful restoration ends paused.

The native emulation thread admits the request, resolves its destination, imports the checkpoint, and replays where necessary.  Success requires an observed destination, imported CPU registers, and a settled CPU pause.  No extra video frame is executed solely to refresh the display.  Paused GUI images and widgets may remain stale; immediate visible-coordinate or pixel equality is not promised.  Controls must remain responsive, and ordinary execution must restore normal display updates.

A well-formed refusal is structured result data, not necessarily an MCP transport error.  Check all of:

- `outcome`: `restored` is the successful seek outcome; `status` is a status response.
- `success`: true only for completed restoration.
- `completed`: false means the requested work has not reached its final acknowledged state.
- `mutationStarted`: distinguishes admission refusal from an interrupted or failed restore.
- `actualStateIsFinal`: whether `actual` is the final CPU-published observation for this request.  A result is an observation, not a lease preventing another client from subsequently changing the machine.

`origin` and `actual` contain counter, frame, instruction-boundary phase, pause state, and CPU registers.  Unavailable observations are `null`; zero is a valid counter, PC, or frame.  The result also identifies the resolved destination, requested parameters, source access when applicable, and observed counter/frame differences.  `resolvedRelativeDistance` describes the retained-index or timebase calculation, not independent proof that replay reproduced every historical effect.

`unsupported_platform` is a completed refusal with `success: false`, `mutationStarted: false`, `sequence: 0`, and null origin/actual/destination.  It leaves the current run state unchanged, even when the unsupported emulator is running or stopped.  Malformed requests still receive ordinary validation errors.

C64 admission can refuse with outcomes such as `pause_required`, `no_history`, `history_unavailable`, `access_unavailable`, `not_at_instruction_boundary`, `instruction_history_unavailable`, `unsupported_timebase`, `native_target_overflow`, or `busy`.  No restore is initiated for those refusals.  After admission, failures such as `target_boundary_missed`, `access_pc_mismatch`, load failure, interruption, or cancellation report a changed-state outcome instead of claiming a harmless no-op.  There is no silent earliest-frame clamp.

A timeout before mutation cancels the request.  `timeout_cancellation_pending` means mutation already began and the CPU still needs to acknowledge cancellation.  Do not immediately issue another mutation; inspect `status` and the matching request `sequence` until the request settles.  The returned `actual` during pending cancellation is only the last CPU-published observation, not a sample of the still-running CPU.

## Units and coverage

| Platform | `counterDomain` in status | Seeking |
|---|---|---|
| C64 PAL/NTSC | `cpu_clock` | Supported; seconds additionally require retained coverage with a verified constant native clock rate. |
| Atari | `instruction_debug_clock` | Unsupported; capability status only. |
| NES | `instruction_index` | Unsupported; capability status only. |

Status includes `seekSupported`, `supportedPlatforms`, and `supportedModes`.  Unsupported platforms report `seekSupported: false`, only `status` in `supportedModes`, `unsupportedReason: "unsupported_platform"`, no seek coverage, and no supported relative units.  Their counter-domain labels describe existing debugger counters, not enabled restoration.

Instruction backtracking uses a bounded contiguous index of main-CPU dispatch boundaries, not sparse per-address execution markers.  A pre-dispatch origin is required.  A memory breakpoint after an opcode but before retirement is not interchangeable with that boundary.  The index retains up to 262,144 boundaries (about 2 MiB of counter storage) and is cleared on gaps or unrelated counter regression, so checkpoint coverage alone does not guarantee instruction-count coverage.  It is maintained continuously for C64, including when snapshot recording is off; disabling recording does not remove its per-instruction work.  Device-CPU instructions are not counted.

`video_frames` means emulated video frames, not animation updates or GUI refreshes.  `emulated_seconds` uses the native timebase rather than host wall time or warp speed.  Status distinguishes captured checkpoint range from finite instruction coverage.  Successful capture receipts are required by this operation; an old timeline loaded from a file is not automatically treated as newly verified capture evidence.

## Historical-state limits

- Last-access records identify a CPU address and recorded instruction boundary, not necessarily a physical RAM bank or the precise bus-access cycle.  The final PC must agree with the recorded source PC.  Checkpoint restoration invalidates equal/newer access records; a single latest-access record cannot recover the previous writer that it replaced.
- Native checkpoint restoration and subsequent execution are not independent proof of historical equivalence.  Input recording and input replay are separate flags and can be disabled.  Enabling either now cannot reconstruct previously unrecorded input.  Check `inputRecording`, `inputReplay`, and `replayEquivalenceVerified`.
- C64 restoration uses linked chip and disk checkpoints.  This operation does not promise rollback of external disk files, mutable media, changed cartridges, or host-side effects; `externalMediaRollbackGuaranteed` is false.
- Avoid concurrent debugger edits, resets, media changes, or other restoration requests.  Detected history changes and interrupted restores fail explicitly, but timestamps alone are not complete branch or value provenance.

This operation does not configure recording, save or load timeline files, clear history, or silently enable missing historical input replay.
