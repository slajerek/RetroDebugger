#!/bin/sh
# Rebuild known_state.prg from known_state.asm using KickAssembler.
# Also emits a .sym file capturing label addresses (used by tests
# that assert PC values relative to the `park` label).
set -eu
cd "$(dirname "$0")"
kickass known_state.asm -o known_state.prg -symbolfile
echo "Built: $(pwd)/known_state.prg ($(wc -c < known_state.prg) bytes)"
if [ -f known_state.sym ]; then
  echo "Symbols at $(pwd)/known_state.sym"
fi
