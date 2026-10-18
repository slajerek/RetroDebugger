#!/usr/bin/env python3
"""Build the source-level debugging fixtures with pinned toolchains (RD-002).

Usage (from the repository root; run Python isolated):
    python3 -I tests/source-debug/scripts/build_fixtures.py [--tool-dir DIR]
        [--toolchain NAME ...] [--fixture NAME ...] [--check | --matrix-only]

For every toolchain in tests/source-debug/toolchains/*.yaml, every machine it
lists, and every fixture directory under tests/source-debug/fixtures/ that has
that machine's source file, this builds the fixture and copies the declared
outputs to tests/data/source-debug/<toolchain>/<machine>/<fixture>/ together
with a manifest.json (tool version, argv, build directory, SHA-256 of the
inputs and outputs). These are the Tier 1 golden artifacts the PR suite reads.
It then rewrites tests/source-debug/MATRIX.md, the compiler x machine matrix
as found by actually building, not as documented.

Executables are looked up, in order: the toolchain's environment variable
(e.g. RD_OSCAR64), each `candidates` entry under --tool-dir (or $RD_TOOL_DIR),
then each candidate on PATH. A toolchain whose executable is missing or whose
version does not match its pin is reported as skipped, never built with
whatever happens to be installed. `version_path_match` accepts a build whose
version text is wrong when its resolved path names the version (Homebrew's
cc65 2.19 prints "V2.18 - N/A" from .../Cellar/cc65/2.19/bin).

Reproducibility: each cell builds in the fixed directory
/tmp/rd-source-debug/<toolchain>/<machine>/<fixture> with every input's mtime
pinned to FIXED_MTIME, because compilers write both into their debug files
(Oscar64 and KickAssembler store absolute source paths, cc65 stores the
mtimes of main.c and of its own intermediate main.s, 64tass dates its listing
with the source mtime); mtimes are pinned again before every step. The artifacts are kept
byte-exact as the tools wrote them; the manifest records the build directory
and the executables' directories so a loader test can remap the paths.

--check rebuilds into the fixed directories and compares with the committed
artifacts instead of overwriting them; it exits 1 on any difference.

A cell this run did not build (filtered out, or its toolchain skipped) is
taken from its committed manifest: `ok` when the manifest's version is the
pin's and its input hashes are the fixture's current files, `stale` when
not, `not built` without one. --matrix-only builds nothing and rewrites
MATRIX.md from the manifests alone, for a machine without the toolchains.
"""

import argparse
import hashlib
import json
import os
import re
import shutil
import subprocess
import sys

try:
    import yaml
except ImportError:
    sys.exit("build_fixtures.py needs PyYAML (python3 -m pip install pyyaml)")

ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", "..", ".."))
SOURCE_DEBUG = os.path.join(ROOT, "tests", "source-debug")
FIXTURES = os.path.join(SOURCE_DEBUG, "fixtures")
TOOLCHAINS = os.path.join(SOURCE_DEBUG, "toolchains")
DATA = os.path.join(ROOT, "tests", "data", "source-debug")
MATRIX = os.path.join(SOURCE_DEBUG, "MATRIX.md")
BUILD_ROOT = "/tmp/rd-source-debug"
FIXED_MTIME = 1767225600  # 2026-01-01T00:00:00Z
STEP_TIMEOUT = 300
MACHINES = ["c64", "atari", "nes", "c64-magicdesk", "nes-uxrom", "nes-mmc1"]	# c64-magicdesk, nes-uxrom, nes-mmc1: banked cartridge builds (RD-026, RD-030)


def sha256(path):
    h = hashlib.sha256()
    with open(path, "rb") as f:
        for chunk in iter(lambda: f.read(65536), b""):
            h.update(chunk)
    return h.hexdigest()


def find_executable(spec, tool_dir):
    env = spec.get("env")
    if env and os.environ.get(env):
        path = os.environ[env]
        return path if os.access(path, os.X_OK) else None
    for candidate in spec.get("candidates", []):
        if tool_dir:
            path = os.path.join(tool_dir, candidate)
            if os.path.isfile(path) and os.access(path, os.X_OK):
                return path
    for candidate in spec.get("candidates", []):
        found = shutil.which(candidate)
        if found:
            return found
    return None


def expand(argv, exes, fixture):
    out = []
    for arg in argv:
        for name, path in exes.items():
            arg = arg.replace("{" + name + "}", path)
        # {toolchains}: tests/source-debug/toolchains, for per-toolchain
        # companion sources (kickc-nes-vectors.c)
        arg = arg.replace("{toolchains}", os.path.join(ROOT, "tests", "source-debug", "toolchains"))
        out.append(arg.replace("{fixture}", fixture))
    return out


def portable_argv(argv, exes):
    """argv with executable paths replaced by their names, for the manifest."""
    by_path = {path: name for name, path in exes.items()}
    toolchains = os.path.join(ROOT, "tests", "source-debug", "toolchains")
    return [by_path.get(arg, arg).replace(toolchains, "{toolchains}") for arg in argv]


def run(argv, cwd):
    env = dict(os.environ, SOURCE_DATE_EPOCH=str(FIXED_MTIME))
    proc = subprocess.run(argv, cwd=cwd, env=env, stdout=subprocess.PIPE,
                          stderr=subprocess.STDOUT, timeout=STEP_TIMEOUT)
    text = proc.stdout.decode("utf-8", "replace")
    # The JVM announces proxy settings on every start; that is not tool output.
    text = "\n".join(l for l in text.splitlines() if not l.startswith("Picked up JAVA_TOOL_OPTIONS"))
    return proc.returncode, text


def resolve_toolchain(tc, tool_dir):
    """Returns ({name: path}, {name: version line}, None) or (None, None, reason)."""
    exes, versions = {}, {}
    for name, spec in tc["executables"].items():
        path = find_executable(spec, tool_dir)
        if path is None:
            return None, None, "%s not found" % name
        exes[name] = path
    for name, spec in tc["executables"].items():
        try:
            _, text = run(expand(spec["version_argv"], exes, ""), cwd=ROOT)
        except (OSError, subprocess.TimeoutExpired) as e:
            return None, None, "%s version check failed: %s" % (name, e)
        line = next((l.strip() for l in text.splitlines() if spec["version_match"] in l), None)
        if line is None and spec.get("version_path_match"):
            # a build that reports a wrong number (upstream cc65 2.19 prints
            # V2.18) is accepted from an install directory naming the version
            real = os.path.realpath(exes[name])
            if spec["version_path_match"] in real:
                first = next((l.strip() for l in text.splitlines() if l.strip()), "")
                line = "%s (%s)" % (first, os.path.dirname(real))
        if line is None:
            return None, None, "%s is not version %s" % (name, spec["version_match"])
        versions[name] = line
    return exes, versions, None


def build_cell(tc, machine, mspec, fixture, exes, versions, check):
    """Builds one fixture for one machine. Returns ("ok"|"fail"|"diff", detail)."""
    fixture_dir = os.path.join(FIXTURES, fixture)
    build_dir = os.path.join(BUILD_ROOT, tc["name"], machine, fixture)
    shutil.rmtree(build_dir, ignore_errors=True)
    shutil.copytree(fixture_dir, build_dir)

    steps = []
    for argv in mspec["steps"]:
        # Pinned before EVERY step: an intermediate (cc65's main.s) is a later
        # step's source, and its mtime lands in the debug file too.
        for name in os.listdir(build_dir):
            os.utime(os.path.join(build_dir, name), (FIXED_MTIME, FIXED_MTIME))
        full = expand(argv, exes, fixture)
        try:
            code, text = run(full, build_dir)
        except (OSError, subprocess.TimeoutExpired) as e:
            return "fail", "%s: %s" % (os.path.basename(full[0]), e)
        steps.append(portable_argv(full, exes))
        if code != 0:
            tail = " | ".join(text.strip().splitlines()[-3:])
            return "fail", "%s exited %d: %s" % (os.path.basename(full[0]), code, tail)

    missing = [o for o in mspec["outputs"] if not os.path.isfile(os.path.join(build_dir, o))]
    if missing:
        return "fail", "missing outputs: " + ", ".join(missing)

    dest = os.path.join(DATA, tc["name"], machine, fixture)
    if check:
        diffs = [o for o in mspec["outputs"]
                 if not os.path.isfile(os.path.join(dest, o))
                 or sha256(os.path.join(dest, o)) != sha256(os.path.join(build_dir, o))]
        return ("diff", "differs: " + ", ".join(diffs)) if diffs else ("ok", "")

    shutil.rmtree(dest, ignore_errors=True)
    os.makedirs(dest)
    outputs = {}
    for o in mspec["outputs"]:
        shutil.copy2(os.path.join(build_dir, o), os.path.join(dest, o))
        outputs[o] = {"sha256": sha256(os.path.join(dest, o)), "size": os.path.getsize(os.path.join(dest, o))}
    inputs = {name: sha256(os.path.join(fixture_dir, name)) for name in sorted(os.listdir(fixture_dir))}
    manifest = {
        "fixture": fixture,
        "toolchain": tc["name"],
        "machine": machine,
        "version": tc["version"],
        "commit": tc.get("commit"),
        "versionReported": versions,
        "steps": steps,
        "buildDir": build_dir,
        "toolDirs": {name: os.path.dirname(os.path.realpath(path)) for name, path in exes.items()},
        "fixedMtime": FIXED_MTIME,
        "source": mspec["source"],
        "inputs": inputs,
        "outputs": outputs,
    }
    with open(os.path.join(dest, "manifest.json"), "w") as f:
        json.dump(manifest, f, indent=2, sort_keys=True)
        f.write("\n")
    return "ok", ""


def committed_status(tc, machine, fixture):
    """The status of a cell from its committed manifest, without building it."""
    path = os.path.join(DATA, tc["name"], machine, fixture, "manifest.json")
    if not os.path.isfile(path):
        return "not built", ""
    with open(path) as f:
        manifest = json.load(f)
    fixture_dir = os.path.join(FIXTURES, fixture)
    inputs = {name: sha256(os.path.join(fixture_dir, name)) for name in sorted(os.listdir(fixture_dir))}
    if manifest.get("version") != tc["version"]:
        return "stale", "committed with %s %s, pinned %s" % (tc["name"], manifest.get("version"), tc["version"])
    if manifest.get("inputs") != inputs:
        return "stale", "the fixture changed since the committed build"
    return "ok", ""


def previous_failures():
    """The failed cells of the current MATRIX.md, from its notes: a failed
    build commits no manifest, so a run filtered to other toolchains or
    fixtures would otherwise forget them."""
    failures = {}
    if not os.path.isfile(MATRIX):
        return failures
    with open(MATRIX) as f:
        for line in f:
            m = re.match(r"^\[\d+\] (\S+) / (\S+) / (\S+): fail (.*)$", line.rstrip("\n"))
            if m:
                failures[(m.group(1), m.group(2), m.group(3))] = ("fail", m.group(4))
    return failures


def write_matrix(results, fixtures, toolchains):
    lines = [
        "# Compiler x machine matrix (generated)",
        "",
        "Written by `tests/source-debug/scripts/build_fixtures.py` from actual builds and",
        "the manifests they committed (a toolchain-less `--matrix-only` run reads only those);",
        "do not edit by hand. `ok` = built and committed under `tests/data/source-debug/`;",
        "`n/a` = the toolchain or fixture has no source for that machine;",
        "`not built` = no committed build yet; `stale` / `skip` / `fail` carry the reason.",
        "",
        "| Fixture | Toolchain | Version | " + " | ".join(MACHINES) + " |",
        "|---|---|---|" + "---|" * len(MACHINES),
    ]
    notes = []
    for fixture in fixtures:
        for tc in toolchains:
            cells = []
            for machine in MACHINES:
                status, detail = results.get((fixture, tc["name"], machine), ("n/a", ""))
                if detail:
                    notes.append("- %s / %s / %s: %s %s" % (fixture, tc["name"], machine, status, detail))
                    status += " [%d]" % len(notes)
                cells.append(status)
            lines.append("| %s | %s | %s | %s |" % (fixture, tc["name"], tc["version"], " | ".join(cells)))
    if notes:
        lines += ["", "Notes:", ""] + ["[%d] %s" % (i + 1, n[2:]) for i, n in enumerate(notes)]
    with open(MATRIX, "w") as f:
        f.write("\n".join(lines) + "\n")


def main():
    parser = argparse.ArgumentParser(description=__doc__.split("\n\n")[0])
    parser.add_argument("--tool-dir", default=os.environ.get("RD_TOOL_DIR"))
    parser.add_argument("--toolchain", action="append", help="only these toolchains")
    parser.add_argument("--fixture", action="append", help="only these fixtures")
    parser.add_argument("--check", action="store_true", help="compare with committed artifacts, write nothing")
    parser.add_argument("--matrix-only", action="store_true", help="build nothing, rewrite MATRIX.md from the manifests")
    args = parser.parse_args()
    if args.check and args.matrix_only:
        parser.error("--check and --matrix-only exclude each other")

    toolchains = []
    for name in sorted(os.listdir(TOOLCHAINS)):
        if name.endswith(".yaml"):
            with open(os.path.join(TOOLCHAINS, name)) as f:
                toolchains.append(yaml.safe_load(f))
    if args.toolchain:
        toolchains = [tc for tc in toolchains if tc["name"] in args.toolchain]
    fixtures = sorted(d for d in os.listdir(FIXTURES) if os.path.isdir(os.path.join(FIXTURES, d)))
    if args.fixture:
        fixtures = [f for f in fixtures if f in args.fixture]

    results = {}
    failed = False
    for tc in toolchains:
        exes, versions, reason = (None, None, "--matrix-only") if args.matrix_only else resolve_toolchain(tc, args.tool_dir)
        for machine, mspec in tc["machines"].items():
            for fixture in fixtures:
                if not os.path.isfile(os.path.join(FIXTURES, fixture, mspec["source"])):
                    continue
                if args.matrix_only:
                    status, detail = committed_status(tc, machine, fixture)
                elif exes is None:
                    status, detail = "skip", reason
                else:
                    status, detail = build_cell(tc, machine, mspec, fixture, exes, versions, args.check)
                results[(fixture, tc["name"], machine)] = (status, detail)
                failed |= status in ("fail", "diff")
                print("%-9s %-8s %-6s %-14s %s" % (status, tc["name"], machine, fixture, detail))

    if not args.check:
        failures = previous_failures()
        # the whole matrix, whatever this run was filtered to: cells it did
        # not build (or whose toolchain it skipped) come from the manifests
        all_toolchains = []
        for name in sorted(os.listdir(TOOLCHAINS)):
            if name.endswith(".yaml"):
                with open(os.path.join(TOOLCHAINS, name)) as f:
                    all_toolchains.append(yaml.safe_load(f))
        all_fixtures = sorted(d for d in os.listdir(FIXTURES) if os.path.isdir(os.path.join(FIXTURES, d)))
        for tc in all_toolchains:
            for machine, mspec in tc["machines"].items():
                for fixture in all_fixtures:
                    key = (fixture, tc["name"], machine)
                    if not os.path.isfile(os.path.join(FIXTURES, fixture, mspec["source"])):
                        continue
                    if key not in results or results[key][0] == "skip":
                        committed = committed_status(tc, machine, fixture)
                        if committed[0] == "not built" and key in failures:
                            committed = failures[key]
                        if key not in results or committed[0] == "ok":
                            results[key] = committed
        write_matrix(results, all_fixtures, all_toolchains)
    return 1 if failed else 0


if __name__ == "__main__":
    sys.exit(main())
