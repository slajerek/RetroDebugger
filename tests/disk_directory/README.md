# Portable directory controls

These tests catch directory-reader lock, enqueue-exception, and unit-routing regressions without claiming native GUI or emulator acceptance.

From the repository root, supply the directory containing `json.hpp`:

```sh
JSON_INCLUDE=/path/to/nlohmann DISK_CONTENTS_SANITIZE=1 bash tests/disk_directory/run.sh /path/to/new-output
```

The runner preserves the existing `tests/disk_contents` fixture and tests, runs the original directory-model and request/protocol checks, and adds these controls:

- `insert`, `format`, and `perform-format`: compile the actual adapter translation unit, exercise `InsertFile` or `FormatDisk`, and probe a foreign-thread mounted-directory read at image acquisition, mutation, GCR buffer release, and reattachment completion.  `perform-format` also models the caller's existing outer interface lock.  The fixture checks exception and missing-image unlock, and verifies that the writer's I/O lock has ended before `DiskAttached` locks the interface.
- The IEC-write and format-command shims mutate the real mounted GCR through `gcr_write_sector`.  Both adapter paths then use the real `drive_image_attach` → G64 image reload/free path.  The fixture does not execute native file I/O, the VICE IEC/vdrive command implementation, GUI dialogs, or complete format behavior.
- `routing`: gives units 8–11 distinct mounted GCR disk/file names, deliberately different from host-file data, and checks those names, the echoed device, missing-unit handling, and nonmutation through the extracted product reader.  It tests both G64 images and D64 images backed by mounted GCR.
- `queue`: extracts `AddCpuDebugInterruptTask` verbatim and injects `std::bad_alloc` in the fixture container's `push_back`, after both recursive acquisitions.  It checks the unavailable response, captured-task destruction, slot release/reuse, unchanged queue, and reacquisition by another thread.

`prepare_controls.py` extracts the product reader and enqueue bodies without editing them.  SDK stubs, modeled machine/interface state, deterministic probe sites, and a fixture-only string scratch allocator are test seams, not native coverage.  The fixture does not verify GUI scheduling, SDK string ownership, native mutex implementation, mounted-image lifecycle integration, or the complete VICE CPU loop.

`run_controls.sh` can use `ADAPTER_SOURCE`, `READER_SOURCE`, and `QUEUE_SOURCE` overrides to test saved originals or isolated mutants.  It runs every case even when a case fails.  Removing the writer guards must reject at the writer-exclusion assertion; restoring the original enqueue method must reject foreign-thread reacquisition; forcing `driveId = 0` in the reader must reject the selected-unit contents assertion.  A compile error, sanitizer finding, or timeout is not a valid behavioral rejection.
