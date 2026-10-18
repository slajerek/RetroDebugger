# Compiler x machine matrix (generated)

Written by `tests/source-debug/scripts/build_fixtures.py` from actual builds and
the manifests they committed (a toolchain-less `--matrix-only` run reads only those);
do not edit by hand. `ok` = built and committed under `tests/data/source-debug/`;
`n/a` = the toolchain or fixture has no source for that machine;
`not built` = no committed build yet; `stale` / `skip` / `fail` carry the reason.

| Fixture | Toolchain | Version | c64 | atari | nes | nes-uxrom |
|---|---|---|---|---|---|---|
| banked | 64tass | 1.59.3120 | n/a | n/a | n/a | n/a |
| banked | cc65 | 2.19 | n/a | n/a | n/a | ok |
| banked | kickass | 5.24-65ce02.e | n/a | n/a | n/a | n/a |
| banked | kickc | 0.8.6 | n/a | n/a | n/a | n/a |
| banked | oscar64 | 1.32.273 | n/a | n/a | n/a | n/a |
| basic-flow | 64tass | 1.59.3120 | ok | n/a | n/a | n/a |
| basic-flow | cc65 | 2.19 | ok | ok | ok | n/a |
| basic-flow | kickass | 5.24-65ce02.e | ok | n/a | n/a | n/a |
| basic-flow | kickc | 0.8.6 | ok | ok | ok | n/a |
| basic-flow | oscar64 | 1.32.273 | ok | ok | ok | n/a |
| branches-loops | 64tass | 1.59.3120 | n/a | n/a | n/a | n/a |
| branches-loops | cc65 | 2.19 | ok | ok | ok | n/a |
| branches-loops | kickass | 5.24-65ce02.e | n/a | n/a | n/a | n/a |
| branches-loops | kickc | 0.8.6 | not built | not built | not built | n/a |
| branches-loops | oscar64 | 1.32.273 | ok | ok | ok | n/a |
| indirect-calls | 64tass | 1.59.3120 | n/a | n/a | n/a | n/a |
| indirect-calls | cc65 | 2.19 | ok | ok | ok | n/a |
| indirect-calls | kickass | 5.24-65ce02.e | n/a | n/a | n/a | n/a |
| indirect-calls | kickc | 0.8.6 | not built | not built | not built | n/a |
| indirect-calls | oscar64 | 1.32.273 | ok | ok | ok | n/a |
| interrupts | 64tass | 1.59.3120 | n/a | n/a | n/a | n/a |
| interrupts | cc65 | 2.19 | ok | ok | ok | n/a |
| interrupts | kickass | 5.24-65ce02.e | n/a | n/a | n/a | n/a |
| interrupts | kickc | 0.8.6 | not built | not built | not built | n/a |
| interrupts | oscar64 | 1.32.273 | ok | ok | ok | n/a |
| nested-calls | 64tass | 1.59.3120 | n/a | n/a | n/a | n/a |
| nested-calls | cc65 | 2.19 | ok | ok | ok | n/a |
| nested-calls | kickass | 5.24-65ce02.e | n/a | n/a | n/a | n/a |
| nested-calls | kickc | 0.8.6 | not built | not built | not built | n/a |
| nested-calls | oscar64 | 1.32.273 | ok | ok | ok | n/a |
| recursion | 64tass | 1.59.3120 | n/a | n/a | n/a | n/a |
| recursion | cc65 | 2.19 | ok | ok | ok | n/a |
| recursion | kickass | 5.24-65ce02.e | n/a | n/a | n/a | n/a |
| recursion | kickc | 0.8.6 | not built | not built | not built | n/a |
| recursion | oscar64 | 1.32.273 | ok | ok | ok | n/a |
| same-line | 64tass | 1.59.3120 | n/a | n/a | n/a | n/a |
| same-line | cc65 | 2.19 | ok | ok | ok | n/a |
| same-line | kickass | 5.24-65ce02.e | n/a | n/a | n/a | n/a |
| same-line | kickc | 0.8.6 | ok | ok | ok | n/a |
| same-line | oscar64 | 1.32.273 | ok | ok | ok | n/a |
