#!/usr/bin/env python3
"""Generate case-insensitive include shim directories (non-Windows only).

The original code was written on DOS/Windows, where file names are not case
sensitive. It does things like `#include "function.h"` when the file is called
`FUNCTION.H`, and `#include <vqm32\\all.h>` with a backslash. On Linux both fail.

Instead of renaming source files, this script (run by CMake at configure time)
creates, inside the build tree, one "shim" directory per include/source
directory. A shim directory contains symbolic links whose names are the exact
spellings the code uses and whose targets are the real files:

    build/case_shim/CODE/function.h         -> <repo>/CODE/FUNCTION.H
    build/case_shim/CODE/wolapi\\wolapi.h    -> <repo>/CODE/WOLAPI/WOLAPI.H
    build/case_shim/WINVQ__INCLUDE/vqm32\\all.h -> <repo>/WINVQ/INCLUDE/VQM32/ALL.H

(A backslash is an ordinary file-name character on Linux, so a link literally
named `vqm32\\all.h` satisfies `#include <vqm32\\all.h>`.)

Spellings that do not resolve to any project file but are not plain lower case
(`<sys\\stat.h>`, `<STDLIB.H>`) get a one-line forwarding header in
`build/case_shim/_system/`, e.g. `#include <sys/stat.h>`. CMake adds that
directory with -idirafter so it is searched last.

Only spellings that actually occur in the scanned sources are created, and only
when the exact spelling does not already exist. See ADR-0008.

Usage (CMake does this for you):
  make_case_shim.py --source-root <repo> --out <build>/case_shim \\
                    --dirs CODE WIN32LIB/INCLUDE ... --scan CODE WIN32LIB WINVQ
"""

import argparse
import os
import re
import shutil
import sys

INCLUDE_RE = re.compile(r'^\s*#\s*include\s*([<"])([^>"]+)[>"]')
SCAN_EXTS = {".C", ".CPP", ".H", ".HPP", ".I"}


def shim_name(rel_dir):
    """Directory name used for the shim of one repo-relative directory."""
    return rel_dir.replace("\\", "/").strip("/").replace("/", "__")


def collect_spellings(roots):
    spellings = set()
    for root in roots:
        for dirpath, _, files in os.walk(root):
            for name in files:
                if os.path.splitext(name)[1].upper() not in SCAN_EXTS:
                    continue
                try:
                    with open(os.path.join(dirpath, name), "r", encoding="latin-1") as f:
                        for line in f:
                            m = INCLUDE_RE.match(line)
                            if m:
                                spellings.add(m.group(2).strip())
                except OSError:
                    pass
    return spellings


_listing_cache = {}


def ci_lookup(directory, name):
    """Return the real entry in `directory` matching `name` case-insensitively."""
    if directory not in _listing_cache:
        try:
            _listing_cache[directory] = {e.upper(): e for e in os.listdir(directory)}
        except OSError:
            _listing_cache[directory] = {}
    return _listing_cache[directory].get(name.upper())


def resolve(base, spelling):
    """Resolve an include spelling relative to `base`, ignoring case and
    treating both `/` and `\\` as separators. Returns a real path or None."""
    path = base
    for part in re.split(r"[\\/]+", spelling):
        if part in ("", "."):
            continue
        if part == "..":
            path = os.path.dirname(path)
            continue
        real = ci_lookup(path, part)
        if real is None:
            return None
        path = os.path.join(path, real)
    return path if os.path.isfile(path) else None


def place(out_dir, spelling):
    """Path inside `out_dir` for a spelling: `/` makes sub-directories,
    `\\` stays part of the file name. Returns None for unsafe spellings."""
    parts = spelling.split("/")
    if spelling.startswith("/") or ".." in parts:
        return None
    return os.path.join(out_dir, *parts)


def main():
    ap = argparse.ArgumentParser(description="Generate case-insensitive include shims")
    ap.add_argument("--source-root", required=True)
    ap.add_argument("--out", required=True)
    ap.add_argument("--dirs", nargs="+", required=True,
                    help="repo-relative include/source directories to shim")
    ap.add_argument("--scan", nargs="+", required=True,
                    help="repo-relative directories to scan for #include spellings")
    args = ap.parse_args()

    root = os.path.abspath(args.source_root)
    out = os.path.abspath(args.out)
    if os.path.isdir(out):
        shutil.rmtree(out)
    os.makedirs(out)

    spellings = collect_spellings([os.path.join(root, d) for d in args.scan])
    resolved_anywhere = set()
    links = 0

    for rel in args.dirs:
        base = os.path.join(root, rel)
        shim = os.path.join(out, shim_name(rel))
        os.makedirs(shim, exist_ok=True)
        for spelling in sorted(spellings):
            target = resolve(base, spelling)
            if target is None:
                continue
            resolved_anywhere.add(spelling)
            if os.path.exists(os.path.join(base, spelling)):
                continue  # exact spelling already works
            dest = place(shim, spelling)
            if dest is None or os.path.lexists(dest):
                continue
            os.makedirs(os.path.dirname(dest), exist_ok=True)
            os.symlink(target, dest)
            links += 1

    system_dir = os.path.join(out, "_system")
    os.makedirs(system_dir, exist_ok=True)
    wrappers = 0
    for spelling in sorted(spellings - resolved_anywhere):
        normal = re.sub(r"[\\/]+", "/", spelling).lower()
        if normal == spelling or ".." in normal.split("/"):
            continue  # already portable, or a relative path we cannot forward
        dest = place(system_dir, spelling)
        if dest is None or os.path.lexists(dest):
            continue
        os.makedirs(os.path.dirname(dest), exist_ok=True)
        with open(dest, "w", encoding="ascii", errors="replace") as f:
            f.write("/* Generated by tools/make_case_shim.py: case/backslash forwarder. */\n")
            f.write("#include <%s>\n" % normal)
        wrappers += 1

    print("case shim: %d include spellings, %d links, %d system forwarders -> %s"
          % (len(spellings), links, wrappers, out))
    return 0


if __name__ == "__main__":
    sys.exit(main())
