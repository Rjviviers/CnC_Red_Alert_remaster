#!/usr/bin/env python3
"""Fail if a WIN32LIB component folder holds a header that shadows a different copy in WIN32LIB/INCLUDE.

Why: a .CPP's own folder is searched first for #include "x.h". On case-insensitive file systems (Windows)
a stale local copy (e.g. WIN32LIB/MISC/WWSTD.H) then silently replaces the shared header, while Linux,
which is case-sensitive, still uses WIN32LIB/INCLUDE. Both platforms must compile the same headers.
Identical copies are allowed (they cannot change behaviour). See docs/building.md.
"""
import os
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
LIB = os.path.join(ROOT, "WIN32LIB")
SHARED = os.path.join(LIB, "INCLUDE")


def main():
    shared = {f.lower(): os.path.join(SHARED, f) for f in os.listdir(SHARED) if f.lower().endswith(".h")}
    bad = []
    for entry in sorted(os.listdir(LIB)):
        folder = os.path.join(LIB, entry)
        if entry == "INCLUDE" or not os.path.isdir(folder):
            continue
        for f in sorted(os.listdir(folder)):
            key = f.lower()
            if key in shared:
                local = os.path.join(folder, f)
                with open(local, "rb") as a, open(shared[key], "rb") as b:
                    if a.read() != b.read():
                        bad.append((os.path.relpath(local, ROOT), os.path.relpath(shared[key], ROOT)))
    for local, other in bad:
        print("shadowing header differs: %s vs %s" % (local, other))
    if bad:
        print("Delete the stale copy (or make it identical); WIN32LIB/INCLUDE is the source of truth.")
        return 1
    print("check_shadow_headers: OK")
    return 0


if __name__ == "__main__":
    sys.exit(main())
