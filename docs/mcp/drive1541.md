# Practical drive8 control

Drive-specific MCP operations let clients work with the existing 1541 debugger without stepping merely to request a pause.  They do not repair or replace the debugger's lifecycle and concurrency behavior.

## Observe a pause

Call `retro_drive1541_pause`:

```json
{"platform":"c64","device":8,"timeoutMs":1000}
```

This invokes `c64/drive1541/pause` with `{"device":8,"timeoutMs":1000}`.  Only the enabled 1541/1541-II context for device8 is supported.  Device defaults to8; timeout defaults to1000 and must be an integer0..5000.  Extra fields, other devices/platforms, strings, booleans, fractions, and overflowing integers are rejected before mutation.  The raw endpoint accepts an empty object for defaults; MCP requires `platform:"c64"`.

The request pauses the **whole emulator**, not only the drive.  It reuses existing pause signaling, never steps, and never resumes automatically.  An already observed paused/parked machine requires no new pause request.

Result fields are `device`, `outcome`, `pauseRequested`, `debugMode`, `parked`, and `timeoutMs`.  Inspect the outcome rather than just HTTP/MCP success:

- `paused_observed`: the mode and existing parked observation both indicated pause during this call.
- `timeout`: the observation budget expired without that combination.  This is not successful suspension.  A pause request already issued is not withdrawn and may take effect later.
- `busy`: another new drive-control request owns admission; this call made no new pause request.
- `unavailable`: the emulation thread or supported enabled drive context was unavailable; this call made no new pause request.

A zero budget still permits a pause request and one immediate observation.  The finite budget bounds the polling wait, not forced interruption of core work, real-time scheduling, bridge transport, or existing pause-signaling internals.

## Intended sequence and limitations

1. Request pause and check for `paused_observed` and `parked:true`.
2. Perform the intended existing debugger inspection.
3. Resume explicitly only when desired.

A sampled pause is **not a lifecycle lease**.  GUI actions, reset, shutdown, and legacy APIs can still change state after the response.  The nonblocking admission guard serializes only these new drive-control operations.  It does not change global debugger ownership or claim authoritative stop attribution.  No background polling, new worker, snapshot, tracing, or networking is introduced.

Connected and reconnected MCP bridge rosters include the new tool and its schema; disconnected bridge-local tools are unchanged.  Focused cases extend the existing MCP protocol/bridge and VICE drive tests.  Native behavior and cross-platform coverage must be validated by those builds; static inspection is not a substitute.
