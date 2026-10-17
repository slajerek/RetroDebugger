# Source-level debugging fixtures

Fixtures and pinned toolchains for the source-level debugging programme
(`docs/specs/source-level-debugging/`, spec #9 and RD-002).

- `fixtures/<name>/` -- one directory per scenario: `main.c` for the C
  compilers, `<toolchain>-<machine>.asm` for hand-written assembler fixtures.
- `toolchains/*.yaml` -- one pinned toolchain each: version, where it comes
  from, how to find its executables, and per machine the exact build steps and
  the outputs to keep.
- `scripts/build_fixtures.py` -- builds every fixture x toolchain x machine
  cell and commits the results as Tier 1 golden artifacts under
  `tests/data/source-debug/<toolchain>/<machine>/<fixture>/`, each with a
  `manifest.json` (tool versions, argv, build directory, SHA-256 of inputs and
  outputs). It also rewrites `MATRIX.md`.
- `MATRIX.md` -- the compiler x machine matrix as found by building.

## Regenerating

```bash
python3 -I tests/source-debug/scripts/build_fixtures.py --tool-dir <dir>
python3 -I tests/source-debug/scripts/build_fixtures.py --tool-dir <dir> --check
```

`--tool-dir` holds the toolchains the yaml `candidates` name (for example
`oscar64/bin/oscar64`, `bin/kickc`, `bin/kickass`); anything else is found on
`PATH`, and every executable can be forced with its environment variable
(`RD_OSCAR64`, `RD_CC65`, `RD_CA65`, `RD_LD65`, `RD_KICKC`, `RD_KICKASS`,
`RD_64TASS`). A missing tool or a version other than the pin is reported as
`skip`, never built with whatever is installed. The yaml `obtain` fields say
how each pin was obtained.

Builds are reproducible: `--check` rebuilds and compares byte for byte. The
compilers write absolute source paths (Oscar64, KickAssembler) and file mtimes
(cc65, 64tass) into their debug files, so every cell builds in the fixed
directory `/tmp/rd-source-debug/<toolchain>/<machine>/<fixture>` with input
mtimes pinned to 2026-01-01. Artifacts stay byte-exact as the tools wrote
them; loaders remap the recorded `buildDir` and `toolDirs` (manifest) to the
checkout.

Jars and compiler binaries are never committed; only what they produce.
