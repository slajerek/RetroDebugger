#!/usr/bin/env python3
"""Build portable seams from current product bodies and unchanged upstream support.

No product/test source is rewritten.  Overrides select preserved pre-fix bodies
or deliberate negative controls in a caller-owned output directory.
"""
import argparse
from pathlib import Path
import shutil


def function(source, signature):
    text = source.read_text()
    assert text.count(signature) == 1, (source, signature)
    start = text.index(signature)
    opening = text.index('{', start)
    depth = 1
    end = opening + 1
    while depth:
        depth += (text[end] == '{') - (text[end] == '}')
        end += 1
    return text[start:end] + '\n'


def replace_once(text, old, new):
    assert text.count(old) == 1, old
    return text.replace(old, new)


parser = argparse.ArgumentParser()
parser.add_argument('output', type=Path)
parser.add_argument('--queue-source', type=Path)
parser.add_argument('--reader-source', type=Path)
args = parser.parse_args()
root = Path(__file__).resolve().parents[2]
out = args.output.resolve()
out.mkdir(parents=True, exist_ok=True)
queue = args.queue_source or root / 'src/DebugInterface/CDebugInterface.cpp'
reader = args.reader_source or root / 'src/Emulators/vice/ViceInterface/CDebuggerServerApiVice.cpp'
(out / 'queue.inc').write_text(function(queue, 'void CDebugInterface::AddCpuDebugInterruptTask(CDebugInterfaceTask *task)'))
(out / 'reader.inc').write_text(function(reader, 'static json ReadMountedDiskDirectory(CDebugInterfaceVice *vice, int device)'))
# Preserve upstream tests/support on disk.  The old no-op SDK cannot observe
# exclusion, so this new fixture gives those same calls real recursive locks.
support = out / 'backend-support'
support.mkdir(exist_ok=True)
for source in (root / 'tests/disk_contents/support').glob('*.h'):
    if source.name != 'drive.h':  # driveimage.c and this fixture use real VICE structs.
        shutil.copyfile(source, support / source.name)
sdk = (support / 'TestSdk.h').read_text()
sdk = replace_once(sdk, '#include <filesystem>', '#include <filesystem>\n#include "backend_mutex.h"\nclass CDebugInterfaceTask;')
sdk = replace_once(sdk, ' void LockMutex() {}\n void UnlockMutex() {}\n void LockIoMutex() {}\n void UnlockIoMutex() {}', ''' CSlrMutex interfaceMutex, mediaMutex, queueMutex;
 CSlrMutex *breakpointsMutex = &interfaceMutex, *ioMutex = &mediaMutex, *tasksMutex = &queueMutex;
 int reversedLockOrder = 0;
 void LockMutex() { if (mediaMutex.Depth() && !interfaceMutex.Depth()) ++reversedLockOrder; interfaceMutex.Lock(); }
 void UnlockMutex() { interfaceMutex.Unlock(); }
 void LockIoMutex() { mediaMutex.Lock(); }
 void UnlockIoMutex() { mediaMutex.Unlock(); }
 void AddCpuDebugInterruptTask(CDebugInterfaceTask *);''')
sdk = replace_once(sdk, 'class CDebugInterfaceVice : public CDebugInterface {', 'class CDebugSymbols;\nclass CDebugInterfaceVice : public CDebugInterface {')
sdk = replace_once(sdk, ' CDataAdapterViceDrive1541DiskContents *dataAdapterDrive1541DiskContents = nullptr;', ' CDataAdapterViceDrive1541DiskContents *dataAdapterDrive1541DiskContents = nullptr;\n CDebugSymbols *symbolsDrive1541DiskContents = nullptr;')
# The original UI string stub has no ownership semantics.  Use scratch storage
# rather than claiming to validate the legacy GUI string allocation lifecycle.
sdk = replace_once(sdk, ' CSlrString(char *) {}', ' static void *operator new(size_t) { alignas(CSlrString) static unsigned char storage[64]; return storage; }\n static void operator delete(void *) {}\n CSlrString(char *) {}')
(support / 'TestSdk.h').write_text(sdk)
(support / 'CDebugInterface.h').write_text('#pragma once\n#include "TestSdk.h"\n')
(support / 'SYS_Threading.h').write_text('#pragma once\n#include "backend_mutex.h"\n')
print('Extracted product bodies:', queue, reader)
print('Generated SDK adaptation:', support)
