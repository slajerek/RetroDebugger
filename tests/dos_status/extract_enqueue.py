#!/usr/bin/env python3
"""Extract the exact existing enqueue method for a portable allocation-failure test."""
import pathlib
import sys

source = pathlib.Path(sys.argv[1]).read_text()
signature = 'void CDebugInterface::AddCpuDebugInterruptTask(CDebugInterfaceTask *task)'
assert source.count(signature) == 1
start = source.index(signature)
opening = source.index('{', start)
depth = 1
end = opening + 1
while depth:
    assert end < len(source)
    depth += (source[end] == '{') - (source[end] == '}')
    end += 1
pathlib.Path(sys.argv[2]).write_text(source[start:end] + '\n')
