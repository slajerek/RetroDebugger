# Inspect a mounted disk without opening the browser

`retro_disk_directory` lets clients inspect the native C64's mounted standard DOS directory without loading a program, opening a GUI view, flushing a disk, or changing run/pause.  It uses the existing drive sector adapter and `CDiskImageD64` parser, with task-local adapter state instead of the browser cache.  It is not available for C64 Ultimate, Atari, or NES.

## Reliable use

1. Connect to the native debugger.  Inspect its current media, or explicitly attach a known-valid image if changing media is intended; directory inspection itself never attaches anything.
2. Call `retro_disk_directory` with `{"platform":"c64","device":8}`.  Device is an integer from 8 through 11; omission selects 8.  No path, drive code, or directory GUI is needed.
3. Check `status` and `complete` before using directory contents.  Only `ok` and `empty` have `complete:true`.  Do not interpret an unavailable result as an empty disk, or retry in an unbounded loop.

For example, this MCP JSON-RPC request lists device 9:

```json
{"jsonrpc":"2.0","id":1,"method":"tools/call","params":{"name":"retro_disk_directory","arguments":{"platform":"c64","device":9}}}
```

The native WebSocket endpoint is `c64/disk/directory`:

```json
{"fn":"c64/disk/directory","token":"directory-1","params":{"device":9}}
```

The native response retains the existing transport envelope.  Invalid parameters return HTTP-style status 406; operational outcomes below use transport status 200 with a structured `result`.  MCP returns that result as JSON text content.  Transport/disconnection failures remain MCP errors; an operational `timeout`, `busy`, or `partial` result is not a successful directory, even though transport succeeded.

## Result contract

Complete results contain:

- `device`, `source:"mounted_gcr"`, `imageType:"D64"|"G64"`, and copied `imagePath` provenance.  The path is not used to reopen the image.
- `diskName` (16 bytes) and `diskId` (the parser's seven-byte DOS header field, including padding and DOS type).  Each is `{display, petscii}`; `petscii` is an array of original byte values.  Display is deliberately lossy ASCII: trailing space/shift-space is trimmed, and non-ASCII bytes become `?`.  Use raw bytes for exact names, not display text.
- `freeBlocks` from the standard BAM, with `freeBlocksScope:"standard_35_tracks_excluding_directory_track"`.  This is neither a filesystem consistency audit nor a free-space estimate for custom extended formats.
- `entries`, each with `name`, `type` (`DEL`, `SEQ`, `PRG`, `USR`, or `REL`), `typeCode`, `blocks`, `closed`, `locked`, `startTrack`, and `startSector`.  Sizes are DOS blocks, not byte lengths; tracks are 1-based, and sectors are 0-based.  This call does not read or validate file chains.

| Status | Meaning |
|---|---|
| `ok` | Complete, nonempty standard DOS directory |
| `empty` | Complete, readable standard DOS directory with no entries |
| `busy` | Feature already has an outstanding task, or task-queue lock was contended |
| `unavailable` | No mounted disk, mounted GCR unavailable, backend lock contention, backend exception, or task enqueue failure (`task_enqueue_failed`); see `reason` |
| `timeout` | The finite CPU-task deadline expired; no directory payload |
| `unsupported` | Image type, geometry, or directory layout is outside the supported standard DOS view |
| `malformed` | Invalid directory link, invalid BAM count, or too many entries |
| `cyclic` | Directory chain revisits a sector |
| `unreadable` | A required sector cannot be read or decoded |
| `partial` | Failure after reading at least one directory sector; `reason` retains `cyclic`, `malformed`, `unreadable`, or `unsupported` |

Parser failures include `errorTrack`, `errorSector`, and `directorySectorsRead`.  Incomplete results deliberately omit entries, header names, and free-block data rather than present a complete-looking partial list.  `unavailable` is also used when an absent drive has no mounted image.  Standard 35-track DOS layouts on supported 35--42-track D64/G64 images are inspected; custom filesystems are not decoded.  Mounted true-drive GCR must be present: there is no persisted-host-image fallback.

## Availability and ownership limits

Admission tries the existing recursive task mutex without waiting.  There is one outstanding directory task for the process, including across endpoint re-registration.  The CPU callback tries the interface and I/O locks without waiting, and never accesses shared browser cache state.

The requester has a one-second monotonic deadline.  It releases the queue lock before waiting, and timeout never takes backend locks.  An expired callback that has not started performs no backend work.  An executing read may finish later into owned storage, but its late reply is discarded.  Task retirement, not timeout, frees the admission slot.

Some paused sites do not service the existing queue.  The call preserves that pause and can time out, leaving the feature busy until a later normal queue drain.  One bounded task may remain until process exit.  There is no automatic resume, step, queue drain, cancellation framework, or promise of prompt reclamation or universal paused availability.  Retry only after a meaningful state change, and never resume solely because a directory read timed out unless that state change is independently intended.
