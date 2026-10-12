#!/usr/bin/env python3
"""Compile the Code menu's label-selection block with debug-interface doubles.

Run with Python 3 and a C++ compiler, without building or launching the GUI.
An optional source path allows checking the same cases against an older revision.
"""

import argparse
import itertools
from pathlib import Path
import re
import subprocess
import tempfile


PROJECT_DIR = Path(__file__).resolve().parents[1]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("source", nargs="?", type=Path,
                        default=PROJECT_DIR / "src/Views/CMainMenuBar.cpp")
    args = parser.parse_args()
    source = args.source.read_text()
    start = 'const char *t = "Step Instruction";'
    end = 'if (ImGui::MenuItem(t, kbsStepOverInstruction->cstr))'
    if source.count(start) != 1 or source.count(end) != 1:
        raise RuntimeError("Cannot uniquely locate the Code menu label-selection block")
    block = source.split(start, 1)[1].split(end, 1)[0]
    definitions = (PROJECT_DIR / "src/DebugInterface/DebuggerDefs.h").read_text()
    modes = "\n".join(re.findall(
        r"^#define DEBUGGER_MODE_(?:RUNNING|PAUSED)\s+[^\n]+", definitions, re.MULTILINE))

    checks = []

    def add(name, interfaces, expected):
        checks.append(f'check("{name}", {{{", ".join(interfaces)}}}, "{expected}");')

    # Active paused C64, inactive Atari, and inactive NES, in every order.
    for index, order in enumerate(itertools.permutations(["&c64", "&atari", "&nes"])):
        add(f"paused C64 with inactive running-mode interfaces, order {index}",
            order, "Step Instruction")
    # Any active running emulator must select Pause, irrespective of its position.
    for index, order in enumerate(itertools.permutations(["&c64", "&activeRunning", "&nes"])):
        add(f"active running interface, order {index}", order, "Pause")
    add("active running C64", ["&runningC64", "&atari", "&nes"], "Pause")
    add("only active paused C64", ["&c64"], "Step Instruction")
    add("all active interfaces paused", ["&c64", "&activePaused"], "Step Instruction")
    add("only inactive running-mode interfaces", ["&atari", "&nes"], "Step Instruction")
    add("no interfaces", [], "Step Instruction")

    program = r'''
#include <cstring>
#include <iostream>
#include <vector>
''' + modes + r'''
struct CDebugInterface
{
    bool isRunning;
    int debugMode;
    int GetDebugMode() const { return debugMode; }
};
struct View { std::vector<CDebugInterface *> debugInterfaces; };
int failures = 0;
void check(const char *name, std::vector<CDebugInterface *> interfaces, const char *expected)
{
    View view{interfaces};
    View *viewC64 = &view;
    ''' + start + block + r'''
    if (std::strcmp(t, expected) != 0 || isPaused != (std::strcmp(expected, "Step Instruction") == 0))
    {
        std::cerr << "FAIL: " << name << ": expected " << expected << ", got " << t
                  << " (isPaused=" << isPaused << ")\n";
        failures++;
    }
}
int main()
{
    CDebugInterface c64{true, DEBUGGER_MODE_PAUSED};
    CDebugInterface atari{false, DEBUGGER_MODE_RUNNING};
    CDebugInterface nes{false, DEBUGGER_MODE_RUNNING};
    CDebugInterface activeRunning{true, DEBUGGER_MODE_RUNNING};
    CDebugInterface runningC64{true, DEBUGGER_MODE_RUNNING};
    CDebugInterface activePaused{true, DEBUGGER_MODE_PAUSED};
''' + "\n".join(checks) + f'''
    if (!failures) std::cout << "PASS: {len(checks)} Code menu label cases (compiled source block, interface doubles)\\n";
    return failures ? 1 : 0;
}}
'''
    with tempfile.TemporaryDirectory(prefix="step-menu-label-") as directory:
        cpp = Path(directory) / "test.cpp"
        binary = Path(directory) / "test"
        cpp.write_text(program)
        subprocess.run(["c++", "-std=c++11", "-Wall", "-Wextra", "-Werror",
                        str(cpp), "-o", str(binary)], check=True)
        return subprocess.run([str(binary)]).returncode


if __name__ == "__main__":
    raise SystemExit(main())
