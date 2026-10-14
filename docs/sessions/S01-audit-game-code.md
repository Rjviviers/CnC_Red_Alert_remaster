# S01 — Audit: game code (`CODE/`)

**Status:** Not started · **Wave:** 1 · **Depends on:** S00 · **Parallel with:** S02, S03, S04
**Type:** read-only investigation; produces documentation only
**Owns:** `docs/codebase/game-code.md`, `docs/codebase/platform-dependencies.md`

## Goal
Know exactly which files in `CODE/` belong to the Win32 build, and what each one depends on.
Later sessions are planned from this inventory.

## Tasks
- [ ] Extract the Win32 object list from `CODE/MAKEFILE` (the `OBJECTS` list plus the `!ifdef WIN32` blocks). Produce the list of kept `.CPP`/`.ASM` files.
- [ ] Group the files by subsystem: simulation, UI/menus, rendering, audio, networking, file I/O, campaign-only, editor/debug.
- [ ] For each file, record its platform dependencies: DirectDraw, DirectSound, Win32 API, Winsock, Greenleaf (`gcl`), Watcom `#pragma aux`, inline asm, DOS-only code.
- [ ] List every preprocessor switch that selects a platform or feature (`WIN32`, `WOLAPI_INTEGRATION`, `WINSOCK_IPX`, `DOS4G`, `GERMAN`/`FRENCH`, …) and what it controls.
- [ ] Find the main loop and initialisation path (`CONQUER.CPP`, `INIT.CPP`, `STARTUP.CPP`) and document the boot sequence step by step.
- [ ] Flag code that probably affects determinism: random numbers, fixed-point math, floating point.

## Deliverables
- `docs/codebase/game-code.md`: subsystem map, boot sequence, feature switches
- `docs/codebase/platform-dependencies.md`: a table of file → dependencies
- Session log

## Done when
Every file in the Win32 build is classified and every platform dependency is listed.
