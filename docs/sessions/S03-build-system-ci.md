# S03 — CMake + CI skeleton

**Status:** Done · **Wave:** 1 · **Depends on:** S00 · **Parallel with:** S01, S02, S04
**Owns:** `CMakeLists.txt` (root), `cmake/`, `.github/workflows/`, `docs/building.md`

## Goal
A modern build system and continuous integration that will eventually build the game.
At this stage it only needs to *try* to compile, and report how far it gets.

## Tasks
- [x] Root `CMakeLists.txt` with targets for the game, its support library and VQ, using the toolchain from ADR-0003 (32-bit x86 first).
- [ ] Platform dependency (SDL3, per ADR-0004) fetched via CMake `FetchContent` or vcpkg — record the choice in an ADR.
  *Deferred to S08: ADR-0004 is still Proposed, so no SDL dependency is added yet (Wave 1 instruction). A commented placeholder is in `CMakeLists.txt`.*
- [x] A "compile progress" mode: try to compile each source file on its own and write a report (files compiling / total). This is the scoreboard for S06.
- [x] GitHub Actions: Windows (MSVC, Win32) and Linux (GCC `-m32`) jobs. Upload the progress report as an artifact.
- [x] `docs/building.md`: how to build locally on Windows and Linux, step by step.

## Deliverables
- Build files and CI workflows
- `docs/building.md`
- Session log, including the first progress report numbers

## Done when
CI runs on every push and publishes a progress report. It may still show 0 files compiling.
