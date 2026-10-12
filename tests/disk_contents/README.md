# 1541 logical disk contents regression

The `Disk contents regression` GitHub Actions workflow runs this test automatically on pull requests targeting `master`, `main`, or `devel`, and on pushes to those branches.  It can also be started with `workflow_dispatch`.  This is a separate Linux CI job, not part of the application's built-in test suite; it requires no GUI SDK or running emulator.  The job uploads the test logs even after a test failure.

To run it locally from the repository root on Linux with GCC, Bash, and GNU `timeout`:

```sh
tests/disk_contents/run.sh /tmp/rd-disk-contents
DISK_CONTENTS_SANITIZE=1 tests/disk_contents/run.sh /tmp/rd-disk-contents-asan
```

The output directory retains the executable, object files, authored images, and logs.  Each test process has a 30-second timeout.  Existing application tests are unchanged.

The test compiles the complete production `CDataAdapterViceDrive1541DiskContents.cpp` and `CDiskImageD64.cpp`, together with VICE's GCR codec, disk-image dispatch, filesystem dispatch, D64/G64 readers, and sector checks.  `support/` supplies small host/SDK doubles for logging, locks, byte buffers, GUI types, attachment pointers, allocation, and file positioning.  GUI rendering, the address editor, the full debug interface, emulator threads, and native attachment are not compiled or exercised.  Image type and geometry are supplied as attachment metadata; the G64 metadata matches VICE's `header[9] / 2` track calculation.  No independent GCR decoder is implemented.

## Coverage

- An authored 35-track D64 and equivalent 84-slot G64, with BAM, disk name/ID, two linked directory sectors, two PRGs, and a multi-sector file.  All 683 ordinary sectors, directory entries, BAM free count, file bytes, and hex-byte availability are checked.
- Bad checksums, missing sector headers, missing raw tracks, stale-cache invalidation, truncated D64 reads, empty and populated directory cycles, file cycles, invalid chain links, invalid final-sector lengths, and invalid entry indexes.
- All 119 additional sectors on tracks 36--42, unsupported geometry, direct/cached read and write bounds, a raw-GCR fallback with an odd half-track, half-track-only media, and detach.
- Zero write calls during successful or failed viewing, and unchanged loaded GCR bytes.  A separate write-status test checks supplied sector bytes, raw-GCR success/failure codes, refusal to edit unavailable/read-only sectors, and backing-image write failure.  It does not validate successful persisted disk edits.

For the preserved baseline comparison, set `DISK_CONTENTS_BASE_SOURCE` to a directory containing the unchanged adapter and parser `.cpp` and `.h` files.  That mode runs the shared ordinary-image and bad-checksum regressions against the old void-returning interface.  The additional status-returning tests require the repaired interface.

This is a focused portable test, not an application build or native GUI pass.  Windows/macOS, logging configurations, actual drive attachment, concurrent refresh, UI presentation, the existing application suites, and PR #106 integration remain separate gates.  The vendored `fsimage.c` has a pre-existing `archdep_stat` pointer-type compile warning in an unexercised function; it is not changed or suppressed here.
