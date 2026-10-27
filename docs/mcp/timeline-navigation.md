# Existing C64 frame navigation

`retro_timeline` can wait for an existing C64 frame rewind in the same call, so inspection need not start with external completion polling.  It reuses the periodic snapshot timeline without adding instruction-history tracking, enabling recording, changing retention, or implementing precise seeking.

## Read-only status

```json
{"platform":"c64","action":"status"}
```

This calls `c64/timeline/status` with empty parameters.  The result includes:

- `navigation: "native_frame"` and `completionReporting: "acceptance_only"`;
- `running` (emulation thread available), `paused`, and `busy`;
- `recording`, `intervalFrames`, and `retention` from existing settings;
- `storedSnapshots`, `oldestFrame`, `newestFrame`, and `oldestFrameExclusive: true`.

Empty history has null bounds, not invented frame zero.  Frame maps and busy flags are observed under the snapshot-manager lock without calling the legacy state-changing `IsPerformingSnapshotRestore()` query.  Settings are copied under their existing interface lock separately.  This is not one atomic transaction across settings, CPU mode, and other UI activity.  Unqualified status does not pause, resume, restore, activate recording, or resolve a prior request's completion.  Its compatibility field `completionReporting: "acceptance_only"` describes the default queued contract; request-specific observation is opt-in below.

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

**Queued is not completed restoration.**  Omitting `waitMs` preserves this exact response, without a request ID or returned actual destination.  A later idle/paused status alone does not prove this request succeeded: history/UI changes, breakpoints, or other controls can interfere.  No new replay deadline, cancellation API, instruction/cycle guarantee, or automatic recovery is introduced.  Transport timeouts are not cancellation; inspect before deciding what to do next, and do not blindly retry.

Well-formed requests that cannot be queued return HTTP 200 with `status: "refused"`, `accepted: false`, `completionKnown: false`, and a reason: `emulator_stopped`, `busy`, `pause_required`, `no_history`, `frame_range_unrepresentable`, `frame_out_of_range`, or `not_queued`.  Check `accepted`, not just the MCP error flag.  Malformed parameters and unsupported platforms are errors; a missing snapshot manager is unavailable.  Extra fields, strings, booleans, fractions, negative frames, and overflow values are rejected before narrowing or dispatch.  Raw status accepts an empty object or omitted/null parameters; MCP requires explicit `platform` and `action`.

## Wait for this request, then inspect

Pause the C64, read status to choose a retained frame, and make one frame call with `waitMs`:

```json
{"platform":"c64","action":"frame","frame":120,"waitMs":5000}
```

The direct endpoint takes `{"frame":120,"waitMs":5000}`.  `waitMs` must be an actual JSON integer from 0 through 5000; strings, booleans, fractions, and extra fields remain invalid.  The five-second wait is below the bridge's existing thirty-second socket timeout.  Zero queues a tracked request without waiting.  Admission failures still return the existing `refused` result, with no request ID.

An admitted request returns a compact observation, for example:

```json
{"platform":"c64","navigation":"native_frame","requestId":7,"requestedFrame":120,"accepted":true,"status":"completed","completionKnown":true,"reason":null,"observedFrame":120,"observedCycle":2345678}
```

- `completed` means this request restored successfully, reached its native frame target, and arrived at the main CPU's stable paused observation point after imports.  `observedFrame` and `observedCycle` report what was observed, not a promise of exact instruction replay or full device fidelity.  Inspect CPU, memory, or the screen next; no preliminary status polling is needed.
- `pending` with `reason: "wait_timeout"` means only that the bounded wait expired.  Navigation remains active; timeout is neither cancellation nor success.  With zero wait, the pending reason is `in_progress`.
- `cancelled` means this request cannot produce a completion observation.  Reasons include `cancelled`, `history_cleared`, `snapshot_load`, `superseded`, `reset`, `restore_failed`, and `interrupted`.  Unrelated pauses cannot count as successful completion.
- `unavailable` with `reason: "request_not_retained"` means no evidence for that ID is retained.  It does not establish that the original request was refused, completed, or cancelled.

Only `completed` sets `completionKnown: true` and non-null observed fields.  Cancellation here invalidates completion evidence; it does not introduce rollback or a new stop command.  Soft and hard reset requests invalidate pending completion before their existing reset path runs; hard reset also clears history as before.  A stopped emulator or deleted history cannot turn an unknown pause into success; a stop with no existing cancellation signal can leave the request pending, but the wait is still bounded.

If a request remains pending, optional read-only follow-up uses the same tool:

```json
{"platform":"c64","action":"status","requestId":7}
```

This calls `c64/timeline/status` with `{"requestId":7}` and never waits.  The positive integer ID identifies only this manager's current process lifetime; do not reuse it across restarts.  Bookkeeping retains one tracked request, not history.  A later tracked admission replaces the retained observation, including terminal evidence; older/unknown IDs return `unavailable`, including to an older waiter.  Existing UI or untracked rewinds invalidate a pending request rather than completing it.  Refused admissions do not replace retained evidence.  Other clients can still change the emulator after an observation; this API does not reserve it across subsequent inspection calls.

## Scope and compatibility

- C64 only.  Atari, NES, C64U, and drive CPU navigation are not supported by this contract.
- Reuses existing recording and replay behavior.  No new per-instruction hooks, continuous index, recorder switch, or device-fidelity guarantee.
- No back-N-instructions, strict cycle/time destination, or precise last-reader/writer positioning.
- The default acceptance-only contract and optional native-frame completion observation remain distinct from precise seeking.  `status: "queued"` never acquires a different meaning.
- Existing last-access records and rendered/published CPU views are not proof of request-specific restoration, instruction identity, or complete hidden-state preservation.

Completion extends the existing tool and endpoints without changing either the connected or disconnected bridge tool count.  Existing native/CI tests still apply to a binary containing this adapter.  Source or portable helper tests alone do not establish native restoration behavior.
