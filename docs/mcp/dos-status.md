# Non-consuming native C64 DOS status

This observation helps inspect a drive's buffered reply without consuming channel 15 or disturbing the running program.  It is **not** a fresh IEC query, a media-health check, or proof that stock DOS is currently executing.

## Calls and results

Call MCP `retro_drive_dos_status` with `{"platform":"c64","device":8}`.  Devices 8 through 11 are accepted; the platform and device are required.  C64 Ultimate, Atari, and NES are unsupported.  The corresponding debugger endpoint is `c64/drive/dosStatus`, with `{"device":8}` and no binary input.

Check `outcome` and `available` before interpreting any status:

| Outcome | Meaning |
|---|---|
| `complete` | A bounded, structurally complete buffered status interpretation is available |
| `unsupported` | Firmware, mapping, binary reply, or representation is not supported |
| `transient` | Buffer bounds, pointer/state, or syntax are incomplete/inconsistent |
| `unavailable` | Device/backend is absent, disabled, contended, or failed |
| `busy` | Queue admission is contended, or the previous status task has not retired |
| `timeout` | The 250 ms monotonic request deadline expired |

Only `complete` has `available:true` and `text`, `code`, `track`, and `sector`.  Refusals are structured results, not invented DOS codes; invalid inputs and transport failures are errors.  Do not treat transport success as status availability.

For example, a **synthetic** complete result can contain:

```json
{
  "device": 8,
  "platform": "c64",
  "outcome": "complete",
  "available": true,
  "backend": "true-drive-rom-workspace",
  "observation": "buffered-state-not-fresh-IEC",
  "text": "74,DRIVE NOT READY,00,00",
  "code": 74,
  "track": 0,
  "sector": 0,
  "mediaPresent": false
}
```

The actual response also includes `reason`, `interpretation`, `rawBytes`, and provenance fields described below.  `mediaPresent` is only the captured image-pointer presence (or null when unknown), not disk validity.  An absent image does not manufacture code 74: an old buffered reply can still say OK.

A reliable sequence is: request once, check availability and provenance, and then display the qualified buffered value or the refusal.  Repeating a successful observation does not advance a pointer or clear a reply.  If busy/timeout occurs, do not resume, step, reset, or flood retries just to obtain status.  Retry only after independently intended execution has allowed the pending task to retire.

## Supported representations and limits

**Virtual mode:** when VICE's `DriveTrueEmulation` is off, copy the selected device's command-channel buffer, `bufptr`, `length`, mode, read mode, and `last_code` together on the CPU thread.  `backend` is `virtual-command-channel`.  Length is VICE's last-byte index; `rawBytes` includes its CR terminator and is bounded to 256 bytes.  A readable command-channel reply must have bounded pointers, printable two-digit status syntax, a CR terminator, and a matching `lastCode`.  Memory-read replies (code 4) and binary/non-text data are unsupported.  The returned `end` is the last-byte index, not a byte count.

**True-drive mode:** `backend` is `true-drive-rom-workspace`; there is **never** virtual fallback, even for an absent/disabled device or an unsupported drive.  Both attached drive-type fields must agree and identify either a 1541 or a 1541-II.  The current C000-FFFF ROM must match the corresponding exact 16 KiB stock reference:

| Drive | Bundled reference | SHA-256 |
|---|---|---|
| 1541 | `assets.roms/dos1541` (325302-01 + 901229-05) | `d1d45afb46fd4e2b48d93ca367b889d75654a3b7acf73044e51f6d880c09369e` |
| 1541-II | `assets.roms/dos1541II` | `326c289c38753323d7e8167897447cf61ef35189d82eb8d75210ece949adda7c` |

The lower current-ROM and execution-ROM halves at 8000-BFFF must both mirror that stock image, as mapped by VICE.  The upper execution ROM must match, except for VICE's known enabled idle trap at EC9B; that exception does not apply to the lower mirror.  RAM expansions, Professional DOS, Supercard, StarDOS, and DS1216 mappings are refused.  No other revisions, 1571, or 1581 are claimed.

Copy the 36 bytes at 02D5-02F8, pointer A5/A6, end byte 0249, and channel state F7.  Return `rawAddress`, `rawBytes`, `pointer`, `end`, `channelState`, `driveType`, and `firmwareSha256` where captured.  The end byte is the low address of the last sector digit, **not** a CR offset or length.  The workspace has no stored CR.  Qualification requires F7=88, pointer within 02D6 through the end, a bounded end address, and complete printable `NN,MESSAGE,TT,SS` syntax.  Original bytes, including the unused tail, are preserved separately from the parsed text.

The stock formatter at E6C7 holds the pointer at 02D5 while constructing text, writes the end address at E6FC, and advances the pointer at E6FF.  These checks reject that construction phase; they do not prove the last writer's identity.  A custom RAM loader can overwrite this workspace even with an unchanged stock ROM.  A syntactically valid buffer can be stale or custom-written.  “Complete” means complete **buffer syntax**, not a fresh response, whole-drive consistency, or DOS/media-health semantics.

Reference: Commodore 1541 325302-01 + 901229-05 ROM and the bundled stock 1541-II ROM identified above, formatter E6C1-E72A, reader D414-D44C; these formatter/reader ranges match in both references.  Bundled VICE `drive/iec/memiec.c`, `drive/iec/iecrom.c`, `drive/driverom.c`, `vdrive/vdrive-command.c`, and `vdrive/vdrive-iec.c`.  The hash identifies the bytes, not provenance or licensing of arbitrary firmware.

## Execution and retention

Capture uses one feature-local owned task on the existing CPU interrupt queue.  Admission try-locks the public recursive task mutex, enqueues under it, and releases it before waiting.  The callback try-locks the interface and IO locks; it never waits for those locks.  No request mutex covers backend work.

At most one status task is outstanding, even after timeout.  Expired unstarted work does not access the backend.  Already executing work may finish into owned storage, but a late reply is discarded.  Timeout never takes queue, IO, or backend locks.

Some paused sites cannot service this queue.  They can time out **without changing run/pause** and leave the feature busy until a later ordinary drain.  One abandoned task can remain until process exit.  This is bounded retention, not prompt reclamation or universal paused availability.  No queue cancellation, new paused-loop servicing, blanket drain, IEC polling, injection, flush, initialize/reset, or full snapshot is used.
