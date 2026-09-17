#!/usr/bin/env python3
"""Register a source file in MTEngineSDL's build systems at once.

c64d has THREE explicit file lists with no auto-discovery -- CMakeLists.txt
(Linux), platform/MacOS/c64d.xcodeproj/project.pbxproj, and
platform/Windows/c64d/c64d.vcxproj plus its .filters -- and the
repo's Definition of Done requires all of them to be updated in the SAME commit
whenever a file is added. Doing that by hand is four edits per file and an easy
thing to half-finish; a private sibling app keeps the same script under its
tools/ for exactly this reason and the engine did not, which is why this
exists.

Usage (from the MTEngineSDL repo root):
    python3 tools/add_source_file.py src/Engine/Core/Render/CRenderTarget.h
    python3 tools/add_source_file.py src/Engine/Tests/MT_CaptureHelpers.cpp
    python3 tools/add_source_file.py platform/MacOS/src.MacOS/Render/CMetalRenderTarget.mm

WHICH BUILD SYSTEMS GET AN ENTRY is decided from the path and extension, because
getting this wrong produces a build failure on a platform you are not currently
looking at:

  * anything under platform/MacOS/  -> Xcode ONLY. Linux and Windows never
    compile it, and adding it to CMake or the vcxproj breaks both.
  * .h anywhere else                -> Xcode + vcxproj/.filters. Headers are not
    compiled, so they get no CMake entry and no Xcode Sources phase entry.
  * .cpp/.c/.mm elsewhere           -> all three, plus the Xcode Sources phase.

Idempotent: re-running for an already-registered file is a no-op, so it is safe
in a loop. Xcode UUIDs are derived from a hash of the filename, so a re-run
produces the same identifier and never creates a duplicate object.
"""

import hashlib
import re
import os
import sys

REPO = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))

CMAKE = os.path.join(REPO, "CMakeLists.txt")
PBXPROJ = os.path.join(REPO, "platform/MacOS/c64d.xcodeproj/project.pbxproj")
VCXPROJ = os.path.join(REPO, "platform/Windows/c64d/c64d.vcxproj")
FILTERS = os.path.join(REPO, "platform/Windows/c64d/c64d.vcxproj.filters")

# Anchors: files already present in every list we need to touch. New entries go
# directly after them, which keeps the lists grouped and the diffs small.
# CRenderBackend.cpp is in all four; CRenderBackendMetal.mm is Xcode-only and so
# is the right anchor for other macOS-only sources.
ANCHOR = "CRenderShaderCRTMonitorOpenGL4.cpp"
ANCHOR_REL = "src/Tools/Shaders/CRenderShaderCRTMonitorOpenGL4.cpp"
ANCHOR_MAC = "CRenderShaderCRTMonitorOpenGL4.cpp"


def uuid_for(name, salt):
    """Deterministic 24-hex-char Xcode object identifier."""
    return hashlib.sha1((name + "|" + salt).encode()).hexdigest().upper()[:24]


def read(p):
    with open(p, "r", encoding="utf-8") as f:
        return f.read()


def write(p, s):
    with open(p, "w", encoding="utf-8") as f:
        f.write(s)


def xcode_filetype(name):
    if name.endswith(".h"):
        return "sourcecode.c.h"
    if name.endswith(".mm"):
        return "sourcecode.cpp.objcpp"
    if name.endswith(".c"):
        return "sourcecode.c.c"
    if name.endswith(".metal"):
        return "sourcecode.metal"
    return "sourcecode.cpp.cpp"


def add_cmake(rel):
    s = read(CMAKE)
    entry = '    "${CMAKE_CURRENT_SOURCE_DIR}/%s"' % rel
    if rel in s:
        return False
    anchor = [l for l in s.splitlines() if ANCHOR_REL in l]
    if not anchor:
        sys.exit("CMake anchor not found: " + ANCHOR_REL)
    return write(CMAKE, s.replace(anchor[0], anchor[0] + "\n" + entry, 1)) or True


def add_xcode(rel, is_header, mac_only):
    s = read(PBXPROJ)
    name = os.path.basename(rel)
    if "/* %s */ = {isa = PBXFileReference" % name in s:
        return False

    anchor_name = ANCHOR_MAC if mac_only else ANCHOR
    ref = uuid_for(name, "ref")
    # pbxproj paths are relative to the .xcodeproj directory (platform/MacOS/).
    xcode_rel = "../../" + rel

    a_ref = [l for l in s.splitlines()
             if "/* %s */ = {isa = PBXFileReference" % anchor_name in l]
    if not a_ref:
        sys.exit("pbxproj FileReference anchor not found: " + anchor_name)

    # ALWAYS SOURCE_ROOT, never the anchor's sourceTree.
    #
    # An earlier version mirrored the anchor, which is wrong in a subtler way
    # than hardcoding "<group>" was. sourceTree decides what `path` is relative
    # to -- SOURCE_ROOT is the .xcodeproj's directory, "<group>" is the
    # ENCLOSING GROUP's path -- and the engine's own anchor
    # (CRenderBackend.cpp) uses "<group>" with a path of just the BASENAME,
    # because its group already carries src/Engine/Core/Render. Copying that
    # sourceTree while emitting a repo-relative path produced
    # src/Engine/src/Engine/Tests/... and the file "could not be found".
    #
    # SOURCE_ROOT + a project-relative path is self-contained: it is correct no
    # matter which group the entry lands in, which also makes group placement
    # purely cosmetic. It is what the macOS anchor already does.
    src_tree = "SOURCE_ROOT"

    fileref = ('\t\t%s /* %s */ = {isa = PBXFileReference; fileEncoding = 4; '
               'lastKnownFileType = %s; name = %s; path = %s; '
               'sourceTree = %s; };'
               % (ref, name, xcode_filetype(name), name, xcode_rel, src_tree))
    s = s.replace(a_ref[0], a_ref[0] + "\n" + fileref, 1)

    # PBXGroup children -- put it beside the anchor so it lands in the same group.
    anchor_uuid = a_ref[0].strip().split()[0]
    grp = [l for l in s.splitlines()
           if l.strip().startswith(anchor_uuid) and l.strip().endswith("*/,")]
    if grp:
        s = s.replace(grp[0], grp[0] + "\n\t\t\t\t%s /* %s */," % (ref, name), 1)

    if not is_header:
        build = uuid_for(name, "build")
        bf = ('\t\t%s /* %s in Sources */ = {isa = PBXBuildFile; fileRef = '
              '%s /* %s */; };' % (build, name, ref, name))
        a_bf = [l for l in s.splitlines()
                if "/* %s in Sources */ = {isa = PBXBuildFile" % anchor_name in l]
        if not a_bf:
            sys.exit("pbxproj BuildFile anchor not found: " + anchor_name)
        s = s.replace(a_bf[0], a_bf[0] + "\n" + bf, 1)

        a_src = [l for l in s.splitlines()
                 if "/* %s in Sources */," % anchor_name in l]
        if not a_src:
            sys.exit("pbxproj Sources phase anchor not found: " + anchor_name)
        s = s.replace(a_src[0],
                      a_src[0] + "\n\t\t\t\t%s /* %s in Sources */," % (build, name), 1)

    write(PBXPROJ, s)
    return True


def add_vs(rel, is_header, group_hint):
    win = "..\\..\\..\\" + rel.replace("/", "\\")
    tag = "ClInclude" if is_header else "ClCompile"

    s = read(VCXPROJ)
    if win in s:
        return False
    a = [l for l in s.splitlines() if ANCHOR_REL.replace("/", "\\") in l]
    if not a:
        sys.exit("vcxproj anchor not found")
    s = s.replace(a[0], a[0] + '\n    <%s Include="%s" />' % (tag, win), 1)
    write(VCXPROJ, s)

    f = read(FILTERS)
    if win not in f:
        idx = f.find(ANCHOR_REL.replace("/", "\\"))
        if idx < 0:
            sys.exit("filters anchor not found")
        end = f.find("</ClCompile>", idx)
        end = f.find("\n", end) if end >= 0 else -1
        if end < 0:
            # self-closing anchor entry
            end = f.find("\n", idx)
        block = ('    <%s Include="%s">\n      <Filter>%s</Filter>\n    </%s>\n'
                 % (tag, win, group_hint, tag))
        f = f[:end + 1] + block + f[end + 1:]
        write(FILTERS, f)
    return True


def register(rel):
    rel = rel.lstrip("./")
    if not os.path.isfile(os.path.join(REPO, rel)):
        sys.exit("no such file: " + rel)

    name = os.path.basename(rel)
    is_header = name.endswith(".h")
    mac_only = rel.startswith("platform/MacOS/")
    group_hint = os.path.dirname(rel).replace("/", "\\")

    done = []
    if add_xcode(rel, is_header, mac_only):
        done.append("xcode")
    if not mac_only:
        if not is_header and add_cmake(rel):
            done.append("cmake")
        if add_vs(rel, is_header, group_hint):
            done.append("vs")

    scope = "macOS-only" if mac_only else ("header" if is_header else "all platforms")
    print("%-58s %-14s %s" % (rel, scope, ", ".join(done) if done else "already registered"))


def main():
    if len(sys.argv) < 2:
        sys.exit(__doc__)
    for arg in sys.argv[1:]:
        register(arg)


if __name__ == "__main__":
    main()
