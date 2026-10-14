#!/usr/bin/env python3
"""Generate cmake/RASources.cmake from the original Watcom makefiles.

The original build (Watcom wmake + TASM) lists every object file in makefiles.
This script reads those lists, resolves each object name to a real source file
(case-insensitively, because the makefiles mix upper and lower case), and writes
CMake variables that the root CMakeLists.txt uses.

Sources it reads (Win32 configuration only, i.e. as if `wmake -DWIN32` was run):
  CODE/MAKEFILE                OBJECTS (+ the !ifdef WIN32 additions), LIBFILES, TECHFILES
  WIN32LIB/MAKEFILE            LIBS  -> one sub-makefile per component library
  WIN32LIB/<LIB>/MAKEFILE      OBJECTS
  WINVQ/VQA32/MAKEFILE         OBJECTS (vqa32wp.lib)
  WINVQ/VQM32/MAKEFILE         OBJECTS (vqm32wp.lib)

Usage:
  python3 scripts/gen_cmake_sources.py           # (re)write cmake/RASources.cmake
  python3 scripts/gen_cmake_sources.py --check   # exit 1 if the committed file is stale

Never edit cmake/RASources.cmake by hand; change this script and regenerate.
"""

import argparse
import os
import re
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
OUTPUT = os.path.join(ROOT, "cmake", "RASources.cmake")

# Symbols treated as defined when evaluating `!ifdef` in the makefiles.
# WIN32 selects the Windows 95 build in CODE/MAKEFILE. GERMAN/FRENCH are not
# defined, so LANGUAGE=ENGLISH (only affects defines, not the source list).
MAKE_DEFINES = {"WIN32"}

# Preferred source extension when an object has more than one candidate.
SOURCE_EXTS = [".CPP", ".C", ".ASM"]


def parse_wmake(path, defines=MAKE_DEFINES):
    """Return {VARIABLE: [values...]} for simple `VAR = a & b &` assignments.

    Supports the subset of Watcom wmake syntax used by these makefiles:
    `#` comments, `&` line continuation, `=` and `+=`, and
    `!ifdef/!ifndef/!else/!endif` (environment checks like `%WIN32LIB` are
    treated as defined, so the sub-makefiles' `!error` lines are skipped).
    """
    with open(path, "r", encoding="latin-1") as f:
        raw_lines = f.read().splitlines()

    # Join continuation lines (a trailing `&`, possibly followed by spaces).
    lines = []
    buf = ""
    for line in raw_lines:
        line = line.split("#", 1)[0].rstrip()
        if line.endswith("&"):
            buf += line[:-1] + " "
            continue
        buf += line
        lines.append(buf)
        buf = ""
    if buf:
        lines.append(buf)

    variables = {}
    stack = []  # list of booleans: is the current branch active?
    for line in lines:
        stripped = line.strip()
        m = re.match(r"!(ifdef|ifndef)\s+(\S+)", stripped, re.I)
        if m:
            sym = m.group(2)
            is_def = sym.startswith("%") or sym.upper() in defines
            stack.append(is_def if m.group(1).lower() == "ifdef" else not is_def)
            continue
        if re.match(r"!else\b", stripped, re.I):
            stack[-1] = not stack[-1]
            continue
        if re.match(r"!endif\b", stripped, re.I):
            stack.pop()
            continue
        if not all(stack):
            continue
        m = re.match(r"([A-Za-z_][A-Za-z0-9_]*)\s*(\+?=)\s*(.*)$", stripped)
        if m and not line[0].isspace():
            name, op, value = m.group(1).upper(), m.group(2), m.group(3)
            items = value.split()
            if op == "+=":
                variables.setdefault(name, []).extend(items)
            else:
                variables[name] = items
    return variables


def dir_index(directory):
    """Map upper-case file name -> real file name for one directory."""
    return {name.upper(): name for name in os.listdir(directory)}


def resolve_objects(directory, objects):
    """Resolve `FOO.OBJ` names to source files in `directory`.

    Returns (cxx, asm, missing) as lists of paths relative to ROOT, with
    duplicates removed (first occurrence wins).
    """
    index = dir_index(os.path.join(ROOT, directory))
    cxx, asm, missing = [], [], []
    seen = set()
    for obj in objects:
        stem = os.path.splitext(obj)[0].upper()
        if stem in seen:
            continue
        seen.add(stem)
        for ext in SOURCE_EXTS:
            real = index.get(stem + ext)
            if real:
                rel = "/".join([directory, real])
                (asm if ext == ".ASM" else cxx).append(rel)
                break
        else:
            missing.append("/".join([directory, obj]))
    return cxx, asm, missing


def cmake_list(name, items, comment):
    out = ["# " + comment, "set(" + name]
    out += ["    " + item for item in items]
    out.append(")")
    return "\n".join(out) + "\n"


def generate():
    sections = []

    # --- Game (CODE/) -----------------------------------------------------
    code = parse_wmake(os.path.join(ROOT, "CODE", "MAKEFILE"))
    # The Win32 exe links OBJECTS plus jshell.lib (LIBFILES) and tech.lib
    # (TECHFILES). We fold all three into the one game target.
    groups = [("OBJECTS", "main object list incl. !ifdef WIN32 additions"),
              ("LIBFILES", "jshell.lib"),
              ("TECHFILES", "tech.lib")]
    game_objs = []
    for var, _ in groups:
        game_objs += code.get(var, [])
    cxx, asm, missing = resolve_objects("CODE", game_objs)
    sections.append(cmake_list(
        "RA_GAME_CXX_SOURCES", cxx,
        "Game: CODE/MAKEFILE OBJECTS + LIBFILES (jshell.lib) + TECHFILES (tech.lib), Win32"))
    sections.append(cmake_list(
        "RA_GAME_ASM_SOURCES", asm,
        "Game: TASM sources (listed, not compiled yet; see session S07)"))
    sections.append(cmake_list(
        "RA_GAME_MISSING_OBJECTS", missing,
        "Game: objects named in the makefile with no source file in the repo"))

    # --- Win32 support library (WIN32LIB/) --------------------------------
    top = parse_wmake(os.path.join(ROOT, "WIN32LIB", "MAKEFILE"))
    w_cxx, w_asm, w_missing = [], [], []
    w_index = dir_index(os.path.join(ROOT, "WIN32LIB"))
    for lib in top.get("LIBS", []):
        sub = w_index[os.path.splitext(lib)[0].upper()]
        subdir = "WIN32LIB/" + sub
        mk = os.path.join(ROOT, subdir, dir_index(os.path.join(ROOT, subdir))["MAKEFILE"])
        objs = parse_wmake(mk).get("OBJECTS", [])
        c, a, m = resolve_objects(subdir, objs)
        w_cxx += c
        w_asm += a
        w_missing += m
    sections.append(cmake_list(
        "RA_WIN32LIB_CXX_SOURCES", w_cxx,
        "win32lib.lib: WIN32LIB/MAKEFILE LIBS -> each WIN32LIB/<LIB>/MAKEFILE OBJECTS"))
    sections.append(cmake_list(
        "RA_WIN32LIB_ASM_SOURCES", w_asm,
        "win32lib.lib: TASM sources (listed, not compiled yet; see session S07)"))
    sections.append(cmake_list(
        "RA_WIN32LIB_MISSING_OBJECTS", w_missing,
        "win32lib.lib: objects with no source file in the repo"))

    # --- VQ player (WINVQ/) -----------------------------------------------
    for lib, subdir in (("VQA32", "WINVQ/VQA32"), ("VQM32", "WINVQ/VQM32")):
        objs = parse_wmake(os.path.join(ROOT, subdir, "MAKEFILE")).get("OBJECTS", [])
        c, a, m = resolve_objects(subdir, objs)
        name = lib.lower() + "wp.lib"
        sections.append(cmake_list("RA_%s_CXX_SOURCES" % lib, c,
                                   "%s: %s/MAKEFILE OBJECTS" % (name, subdir)))
        sections.append(cmake_list("RA_%s_ASM_SOURCES" % lib, a,
                                   "%s: TASM sources (listed, not compiled yet; see session S07)" % name))
        sections.append(cmake_list("RA_%s_MISSING_OBJECTS" % lib, m,
                                   "%s: objects with no source file in the repo" % name))

    header = (
        "# GENERATED FILE - DO NOT EDIT.\n"
        "# Source lists derived from the original Watcom makefiles (Win32 configuration).\n"
        "# Regenerate with:  python3 scripts/gen_cmake_sources.py\n"
        "# Paths are relative to the repository root.\n\n"
    )
    return header + "\n".join(sections)


def main():
    parser = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    parser.add_argument("--check", action="store_true",
                        help="do not write; exit 1 if cmake/RASources.cmake is out of date")
    args = parser.parse_args()

    content = generate()
    if args.check:
        try:
            with open(OUTPUT, "r", encoding="utf-8") as f:  # universal newlines: CRLF checkouts are fine
                current = f.read()
        except FileNotFoundError:
            current = None
        if current != content:
            print("cmake/RASources.cmake is out of date; run: python3 scripts/gen_cmake_sources.py")
            return 1
        print("cmake/RASources.cmake is up to date")
        return 0

    os.makedirs(os.path.dirname(OUTPUT), exist_ok=True)
    with open(OUTPUT, "w", encoding="utf-8", newline="\n") as f:
        f.write(content)
    print("wrote", os.path.relpath(OUTPUT, ROOT))
    return 0


if __name__ == "__main__":
    sys.exit(main())
