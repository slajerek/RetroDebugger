# Compiler x machine matrix (generated)

Written by `tests/source-debug/scripts/build_fixtures.py` from actual builds and
the manifests they committed (a toolchain-less `--matrix-only` run reads only those);
do not edit by hand. `ok` = built and committed under `tests/data/source-debug/`;
`n/a` = the toolchain or fixture has no source for that machine;
`not built` = no committed build yet; `stale` / `skip` / `fail` carry the reason.

| Fixture | Toolchain | Version | c64 | atari | nes |
|---|---|---|---|---|---|
| basic-flow | 64tass | 1.59.3120 | ok | n/a | n/a |
| basic-flow | cc65 | 2.19 | ok | ok | ok |
| basic-flow | kickass | 5.24-65ce02.e | ok | n/a | n/a |
| basic-flow | kickc | 0.8.6 | ok | ok | ok |
| basic-flow | oscar64 | 1.32.273 | ok | ok | ok |
| branches-loops | 64tass | 1.59.3120 | n/a | n/a | n/a |
| branches-loops | cc65 | 2.19 | ok | ok | ok |
| branches-loops | kickass | 5.24-65ce02.e | n/a | n/a | n/a |
| branches-loops | kickc | 0.8.6 | not built | not built | not built |
| branches-loops | oscar64 | 1.32.273 | ok | ok | ok |
| interrupts | 64tass | 1.59.3120 | n/a | n/a | n/a |
| interrupts | cc65 | 2.19 | ok | ok | ok |
| interrupts | kickass | 5.24-65ce02.e | n/a | n/a | n/a |
| interrupts | kickc | 0.8.6 | not built | not built | not built |
| interrupts | oscar64 | 1.32.273 | ok | ok | ok |
| nested-calls | 64tass | 1.59.3120 | n/a | n/a | n/a |
| nested-calls | cc65 | 2.19 | ok | ok | ok |
| nested-calls | kickass | 5.24-65ce02.e | n/a | n/a | n/a |
| nested-calls | kickc | 0.8.6 | not built | not built | not built |
| nested-calls | oscar64 | 1.32.273 | ok | ok | ok |
| same-line | 64tass | 1.59.3120 | n/a | n/a | n/a |
| same-line | cc65 | 2.19 | ok | ok | ok |
| same-line | kickass | 5.24-65ce02.e | n/a | n/a | n/a |
| same-line | kickc | 0.8.6 | ok | ok | ok |
| same-line | oscar64 | 1.32.273 | ok | ok | ok |
