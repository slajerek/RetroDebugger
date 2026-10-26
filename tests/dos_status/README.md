# Portable DOS-status checks

These tests exercise the production feature headers without starting VICE or a native emulator.  Small queue, mutex, and backend doubles provide deterministic control over copies, contention, and task retirement.  They do not validate SDL locking, native attachment, CPU-thread placement, or paused-site availability.

From the repository root, run `bash tests/dos_status/run.sh`.  It uses the sibling MTEngineSDL SHA-256 implementation and the repository's existing `assets.roms/dos1541` and `assets.roms/dos1541II` references, with no downloads.  Override `MTENGINE_ROOT`, `CXX`, `DOS_STATUS_TEST_OUT`, or `DOS_STATUS_TEST_FLAGS` if needed.  Requires Python 3, a C++17 compiler, and pthreads.  Assertions must remain enabled.

Coverage:
- Known buffered OK, not-ready, startup, and read-error text; missing/bad bounds, binary text, and incomplete syntax.
- Both stock 1541 and 1541-II fixtures run the parser/task suite, including non-consuming copies and unknown-ROM refusals.  Disagreeing drive-type fields and the other model's ROM are rejected.
- Exact ROM identity, stock lower-half mirroring in both current and execution ROM, independent lower-half mutations, execution-ROM mismatch, the upper-only VICE idle-trap exception, unsupported type/mapping, disabled/absent devices, and no true-drive fallback.
- Virtual mode/code/memory-read/bounds refusals, repeated non-consuming copies, unchanged pointers/RAM, and returned-data ownership.
- Queue/interface/IO contention; one-slot retention across timeout and repeated busy calls; no backend access on expired late drain.
- Executing backend stalled past the deadline, requester completion without backend locks, late-result discard, identity-safe retirement, and completion/expiry races.
- Allocation failure after the enqueue method's inner recursive acquisition, unavailable result, task destruction, slot reuse, and foreign-thread mutex reacquisition.  `extract_enqueue.py` takes the existing production method verbatim; the added test substitutes a throwing list allocator and a small interface adapter, not a reimplemented enqueue body.
- Fake run/pause remains unchanged.  This is deliberately not native run/pause evidence.

Native MCPProtocol separately tests routing, input bounds, structured refusals, and transport errors.  MCPBridge checks the tool name and total 52 both connected and reconnected, with the disconnected roster still 3.  Those application suites and native feature behavior require a real build/run; compiling these fixtures is not a substitute.
