# Existing C64 frame navigation

`retro_timeline` exposes the existing periodic snapshot timeline.  It does not add instruction-history tracking, enable recording, change retention, or implement precise seeking.

## Read-only status

```json
{"platform":"c64","action":"status"}
```

This calls `c64/timeline/status` with empty parameters.  The result includes:

- `navigation: "native_frame"` and `completionReporting: "acceptance_only"`;
- `running` (emulation thread available), `paused`, and `busy`;
- `recording`, `intervalFrames`, and `retention` from existing settings;
- `storedSnapshots`, `oldestFrame`, `newestFrame`, and `oldestFrameExclusive: true`.

Empty history has null bounds, not invented frame zero.  Frame maps and busy flags are observed under the snapshot-manager lock without calling the legacy state-changing `IsPerformingSnapshotRestore()` query.  Settings are copied under their existing interface lock separately.  This is not one atomic transaction across settings, CPU mode, and other UI activity.  Status does not pause, resume, restore, activate recording, or resolve a prior request's completion.

## Queue a native frame request

```json
{"platform":"c64","action":"frame","frame":120}
```

This calls `c64/timeline/frame` with `{"frame":120}`.  The emulator must already be paused and running as a registered emulation thread, with no restore/save operation active.  `frame` must be an actual JSON integer from 0 through `INT_MAX` (2147483647), strictly above the oldest retained frame, and at most the newest retained frame.  A retained upper bound beyond `INT_MAX` is refused rather than narrowed to the legacy signed-int API.  A single retained frame therefore offers status, but no navigable range.

The lower bound is deliberately exclusive: the existing machinery selects an earlier snapshot and silently adjusts requests at/below its oldest frame.  This adapter refuses that adjustment instead.  It queues through `RestoreSnapshotByFrame(frame, -1, DEBUGGER_MODE_PAUSED)` under the existing recursive manager lock, not through a server-thread chips restore.  Existing CPU-side restoration/replay may resume execution before its native frame pause.

An accepted result is:

```json
{"platform":"c64","navigation":"native_frame","status":"queued","accepted":true,"requestedFrame":120,"completionKnown":false,"reason":null}
```

**Queued is not completed restoration.**  There is no request ID, completion wait, cancellation API, instruction/cycle guarantee, or returned actual destination.  A later idle/paused status alone does not prove this request succeeded: history/UI changes, breakpoints, or other controls can interfere.  No new replay deadline or automatic recovery is introduced.  Transport timeouts are not cancellation; inspect before deciding what to do next, and do not blindly retry.

Well-formed requests that cannot be queued return HTTP 200 with `status: "refused"`, `accepted: false`, `completionKnown: false`, and a reason: `emulator_stopped`, `busy`, `pause_required`, `no_history`, `frame_range_unrepresentable`, `frame_out_of_range`, or `not_queued`.  Check `accepted`, not just the MCP error flag.  Malformed parameters and unsupported platforms are errors; a missing snapshot manager is unavailable.  Extra fields, strings, booleans, fractions, negative frames, and overflow values are rejected before narrowing or dispatch.  Raw status accepts an empty object or omitted/null parameters; MCP requires explicit `platform` and `action`.

## Scope and compatibility

- C64 only.  Atari, NES, C64U, and drive CPU navigation are not supported by this contract.
- Reuses existing recording and replay behavior.  No new per-instruction hooks, continuous index, recorder switch, or device-fidelity guarantee.
- No back-N-instructions, strict cycle/time destination, or precise last-reader/writer positioning.
- This tool's acceptance-only contract must remain distinct from any future precise-seek API.  It must not silently acquire a different meaning for `status: "queued"` or `completionKnown`.
- Existing last-access records and rendered/published CPU views are not proof of request-specific restoration, instruction identity, or complete hidden-state preservation.

The connected bridge roster gains one debugger tool; the disconnected bridge-local roster is unchanged.  Existing native/CI tests still apply to a binary containing this adapter.  Source or portable helper tests alone do not establish native restoration behavior.
