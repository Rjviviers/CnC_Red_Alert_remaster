# S02 — Audit: support libraries & assembly

**Status:** Not started · **Wave:** 1 · **Depends on:** S00 · **Parallel with:** S01, S03, S04
**Type:** read-only investigation; produces documentation only
**Owns:** `docs/codebase/libraries.md`, `docs/codebase/assembly-inventory.md`

## Goal
Work out which parts of `WIN32LIB/` and `WINVQ/` the game actually uses, and which of the
roughly 340 assembly files really need porting.

## Tasks
- [ ] Document what each `WIN32LIB` subfolder does (AUDIO, DRAWBUFF, KEYBOARD, SHAPE, TIMER, WW_WIN, …) and which game files call into it.
- [ ] Produce `win32lib.lib` and the `vqa32wp.lib` member lists from their makefiles.
- [ ] Inventory **every** `.ASM` file the Win32 build links (from `CODE/` and the libraries). For each, record its purpose, callers, calling convention (Watcom register-based `/5r`, `#pragma aux`), and whether a straightforward C++ equivalent exists.
- [ ] Confirm that `WWFLAT32/`, `VQ/`, `IPX/` and `LAUNCH/` are DOS-only or unused by the Win32 build (input for S05).
- [ ] Note where HMI SOS and DirectSound are used (input for S09).
- [ ] Note what `LAUNCHER/` does and whether we need it.

## Deliverables
- `docs/codebase/libraries.md`
- `docs/codebase/assembly-inventory.md`: one row per assembly file, with a port priority
- Session log

## Done when
S05, S07 and S09 can be planned in detail from these pages alone.
