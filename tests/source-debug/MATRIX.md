# Compiler x machine matrix (generated)

Written by `tests/source-debug/scripts/build_fixtures.py` from actual builds and
the manifests they committed (a toolchain-less `--matrix-only` run reads only those);
do not edit by hand. `ok` = built and committed under `tests/data/source-debug/`;
`n/a` = the toolchain or fixture has no source for that machine;
`not built` = no committed build yet; `stale` / `skip` / `fail` carry the reason.

| Fixture | Toolchain | Version | c64 | atari | nes | c64-magicdesk | atari-xegs | nes-uxrom | nes-mmc1 | nes-mmc3 | drive1541 | c64-O2 |
|---|---|---|---|---|---|---|---|---|---|---|---|---|
| banked | 64tass | 1.59.3120 | n/a | n/a | n/a | n/a | n/a | n/a | n/a | n/a | n/a | n/a |
| banked | cc65 | 2.19 | n/a | n/a | n/a | ok | ok | ok | ok | ok | n/a | n/a |
| banked | kickass | 5.24-65ce02.e | n/a | n/a | n/a | n/a | n/a | n/a | n/a | n/a | n/a | n/a |
| banked | kickc | 0.8.6 | n/a | n/a | n/a | n/a | n/a | n/a | n/a | n/a | n/a | n/a |
| banked | oscar64 | 1.32.273 | n/a | n/a | n/a | n/a | n/a | n/a | n/a | n/a | n/a | n/a |
| banked-kickass | 64tass | 1.59.3120 | n/a | n/a | n/a | n/a | n/a | n/a | n/a | n/a | n/a | n/a |
| banked-kickass | cc65 | 2.19 | n/a | n/a | n/a | n/a | n/a | n/a | n/a | n/a | n/a | n/a |
| banked-kickass | kickass | 5.24-65ce02.e | n/a | n/a | n/a | ok | n/a | n/a | n/a | n/a | n/a | n/a |
| banked-kickass | kickc | 0.8.6 | n/a | n/a | n/a | n/a | n/a | n/a | n/a | n/a | n/a | n/a |
| banked-kickass | oscar64 | 1.32.273 | n/a | n/a | n/a | n/a | n/a | n/a | n/a | n/a | n/a | n/a |
| basic-flow | 64tass | 1.59.3120 | ok | n/a | n/a | n/a | n/a | n/a | n/a | n/a | n/a | n/a |
| basic-flow | cc65 | 2.19 | ok | ok | ok | n/a | n/a | n/a | n/a | n/a | n/a | n/a |
| basic-flow | kickass | 5.24-65ce02.e | ok | n/a | n/a | n/a | n/a | n/a | n/a | n/a | n/a | n/a |
| basic-flow | kickc | 0.8.6 | ok | ok | ok | n/a | n/a | n/a | n/a | n/a | n/a | n/a |
| basic-flow | oscar64 | 1.32.273 | ok | ok | ok | n/a | n/a | n/a | n/a | n/a | n/a | n/a |
| branches-loops | 64tass | 1.59.3120 | n/a | n/a | n/a | n/a | n/a | n/a | n/a | n/a | n/a | n/a |
| branches-loops | cc65 | 2.19 | ok | ok | ok | n/a | n/a | n/a | n/a | n/a | n/a | n/a |
| branches-loops | kickass | 5.24-65ce02.e | n/a | n/a | n/a | n/a | n/a | n/a | n/a | n/a | n/a | n/a |
| branches-loops | kickc | 0.8.6 | ok | ok | ok | n/a | n/a | n/a | n/a | n/a | n/a | n/a |
| branches-loops | oscar64 | 1.32.273 | ok | ok | ok | n/a | n/a | n/a | n/a | n/a | n/a | n/a |
| drive1541 | 64tass | 1.59.3120 | n/a | n/a | n/a | n/a | n/a | n/a | n/a | n/a | n/a | n/a |
| drive1541 | cc65 | 2.19 | n/a | n/a | n/a | n/a | n/a | n/a | n/a | n/a | ok | n/a |
| drive1541 | kickass | 5.24-65ce02.e | n/a | n/a | n/a | n/a | n/a | n/a | n/a | n/a | n/a | n/a |
| drive1541 | kickc | 0.8.6 | n/a | n/a | n/a | n/a | n/a | n/a | n/a | n/a | n/a | n/a |
| drive1541 | oscar64 | 1.32.273 | n/a | n/a | n/a | n/a | n/a | n/a | n/a | n/a | n/a | n/a |
| indirect-calls | 64tass | 1.59.3120 | n/a | n/a | n/a | n/a | n/a | n/a | n/a | n/a | n/a | n/a |
| indirect-calls | cc65 | 2.19 | ok | ok | ok | n/a | n/a | n/a | n/a | n/a | n/a | n/a |
| indirect-calls | kickass | 5.24-65ce02.e | n/a | n/a | n/a | n/a | n/a | n/a | n/a | n/a | n/a | n/a |
| indirect-calls | kickc | 0.8.6 | fail [1] | fail [2] | fail [3] | n/a | n/a | n/a | n/a | n/a | n/a | n/a |
| indirect-calls | oscar64 | 1.32.273 | ok | ok | ok | n/a | n/a | n/a | n/a | n/a | n/a | n/a |
| inline | 64tass | 1.59.3120 | n/a | n/a | n/a | n/a | n/a | n/a | n/a | n/a | n/a | n/a |
| inline | cc65 | 2.19 | n/a | n/a | n/a | n/a | n/a | n/a | n/a | n/a | n/a | n/a |
| inline | kickass | 5.24-65ce02.e | n/a | n/a | n/a | n/a | n/a | n/a | n/a | n/a | n/a | n/a |
| inline | kickc | 0.8.6 | n/a | n/a | n/a | n/a | n/a | n/a | n/a | n/a | n/a | n/a |
| inline | oscar64 | 1.32.273 | n/a | n/a | n/a | n/a | n/a | n/a | n/a | n/a | n/a | ok |
| interrupts | 64tass | 1.59.3120 | n/a | n/a | n/a | n/a | n/a | n/a | n/a | n/a | n/a | n/a |
| interrupts | cc65 | 2.19 | ok | ok | ok | n/a | n/a | n/a | n/a | n/a | n/a | n/a |
| interrupts | kickass | 5.24-65ce02.e | n/a | n/a | n/a | n/a | n/a | n/a | n/a | n/a | n/a | n/a |
| interrupts | kickc | 0.8.6 | ok | ok | ok | n/a | n/a | n/a | n/a | n/a | n/a | n/a |
| interrupts | oscar64 | 1.32.273 | ok | ok | ok | n/a | n/a | n/a | n/a | n/a | n/a | n/a |
| nested-calls | 64tass | 1.59.3120 | n/a | n/a | n/a | n/a | n/a | n/a | n/a | n/a | n/a | n/a |
| nested-calls | cc65 | 2.19 | ok | ok | ok | n/a | n/a | n/a | n/a | n/a | n/a | n/a |
| nested-calls | kickass | 5.24-65ce02.e | n/a | n/a | n/a | n/a | n/a | n/a | n/a | n/a | n/a | n/a |
| nested-calls | kickc | 0.8.6 | ok | ok | ok | n/a | n/a | n/a | n/a | n/a | n/a | n/a |
| nested-calls | oscar64 | 1.32.273 | ok | ok | ok | n/a | n/a | n/a | n/a | n/a | n/a | n/a |
| optimized | 64tass | 1.59.3120 | n/a | n/a | n/a | n/a | n/a | n/a | n/a | n/a | n/a | n/a |
| optimized | cc65 | 2.19 | n/a | n/a | n/a | n/a | n/a | n/a | n/a | n/a | n/a | n/a |
| optimized | kickass | 5.24-65ce02.e | n/a | n/a | n/a | n/a | n/a | n/a | n/a | n/a | n/a | n/a |
| optimized | kickc | 0.8.6 | n/a | n/a | n/a | n/a | n/a | n/a | n/a | n/a | n/a | n/a |
| optimized | oscar64 | 1.32.273 | n/a | n/a | n/a | n/a | n/a | n/a | n/a | n/a | n/a | ok |
| recursion | 64tass | 1.59.3120 | n/a | n/a | n/a | n/a | n/a | n/a | n/a | n/a | n/a | n/a |
| recursion | cc65 | 2.19 | ok | ok | ok | n/a | n/a | n/a | n/a | n/a | n/a | n/a |
| recursion | kickass | 5.24-65ce02.e | n/a | n/a | n/a | n/a | n/a | n/a | n/a | n/a | n/a | n/a |
| recursion | kickc | 0.8.6 | fail [4] | fail [5] | fail [6] | n/a | n/a | n/a | n/a | n/a | n/a | n/a |
| recursion | oscar64 | 1.32.273 | ok | ok | ok | n/a | n/a | n/a | n/a | n/a | n/a | n/a |
| same-line | 64tass | 1.59.3120 | n/a | n/a | n/a | n/a | n/a | n/a | n/a | n/a | n/a | n/a |
| same-line | cc65 | 2.19 | ok | ok | ok | n/a | n/a | n/a | n/a | n/a | n/a | n/a |
| same-line | kickass | 5.24-65ce02.e | n/a | n/a | n/a | n/a | n/a | n/a | n/a | n/a | n/a | n/a |
| same-line | kickc | 0.8.6 | ok | ok | ok | n/a | n/a | n/a | n/a | n/a | n/a | n/a |
| same-line | oscar64 | 1.32.273 | ok | ok | ok | n/a | n/a | n/a | n/a | n/a | n/a | n/a |

Notes:

[1] indirect-calls / kickc / c64: fail kickc exited 1: //-------------------------------------------------- | Compiling main.c  | /private/tmp/rd-source-debug/kickc/c64/indirect-calls/main.c:17: error: Initializer is not a constant value.
[2] indirect-calls / kickc / atari: fail kickc exited 1: //-------------------------------------------------- | Compiling main.c  | /private/tmp/rd-source-debug/kickc/atari/indirect-calls/main.c:17: error: Initializer is not a constant value.
[3] indirect-calls / kickc / nes: fail kickc exited 1: //-------------------------------------------------- | Compiling main.c /Users/mars/develop/github/RetroDebugger/tests/source-debug/toolchains/kickc-nes-vectors.c  | /private/tmp/rd-source-debug/kickc/nes/indirect-calls/main.c:17: error: Initializer is not a constant value.
[4] recursion / kickc / c64: fail kickc exited 1: //-------------------------------------------------- | Compiling main.c  | ERROR! Recursion not allowed! Occurs in sum
[5] recursion / kickc / atari: fail kickc exited 1: //-------------------------------------------------- | Compiling main.c  | ERROR! Recursion not allowed! Occurs in sum
[6] recursion / kickc / nes: fail kickc exited 1: //-------------------------------------------------- | Compiling main.c /Users/mars/develop/github/RetroDebugger/tests/source-debug/toolchains/kickc-nes-vectors.c  | ERROR! Recursion not allowed! Occurs in sum
