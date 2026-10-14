# ADR-0012: Hand-maintained CMake source lists

- **Status:** Proposed
- **Date:** 2026-10-14
- **Session:** S05

## Context
In S03 the CMake source lists (`cmake/RASources.cmake`) were generated from the original Watcom
makefiles by `scripts/gen_cmake_sources.py`, and CI failed if the committed file was stale
(`gen_cmake_sources.py --check`). That kept the CMake build faithful to the original while nothing had
changed. S05 is the first session that deletes source files (DOS, IPX, modem, Westwood Online), and
later sessions will add new ones (S05's `SKIRMDLG.CPP`, the platform layer in S08, sockets in S12).
The original makefiles describe a Watcom/TASM build that no longer exists and cannot be run.

## Options considered
1. **Keep generating, and edit the original makefiles too.** Every deletion or new file means editing a
   `wmake` makefile nobody can run, only so a script can read it back. The makefiles stop describing the
   original build, so they lose their value as a historical reference, and the generator needs to grow
   (new directories, new targets) for a format nobody uses.
2. **Keep generating, plus a hand-written "overrides" list.** Two sources of truth; confusing.
3. **Freeze the generated file and maintain it by hand.** One file to edit, in CMake's own syntax.
   The makefiles stay untouched as a record of the original build.

## Decision
Option 3. `cmake/RASources.cmake` is frozen as generated at commit `7a69f38` and edited by hand from S05
on. The generator `scripts/gen_cmake_sources.py` is deleted (it stays in the repository history), and the
CI step that ran `--check` is removed from `.github/workflows/build.yml`. The original makefiles are kept
unchanged for reference; they are no longer the source of truth and still name deleted files.

## Consequences
- Anyone who adds, deletes or renames a source file edits `cmake/RASources.cmake` in the same commit.
  If they forget, CMake configure fails (missing file) or the file is silently not built (new file).
- The variable names (`RA_GAME_CXX_SOURCES`, `RA_*_ASM_SOURCES`, `RA_*_MISSING_OBJECTS`) stay the same,
  so `CMakeLists.txt` and `scripts/compile_progress.py` (which counts the ASM lists) need no change.
- `docs/building.md` documents the new rule.
- The CMake files may later be split per directory (for example when S08 adds the platform layer); that
  is a separate decision.
