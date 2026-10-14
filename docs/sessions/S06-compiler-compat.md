# S06 — Compiler compatibility: game code compiles

**Status:** Not started · **Wave:** 3 · **Depends on:** S05 · **Parallel with:** S07, S08, S09
**Owns:** `CODE/*.CPP`/`*.H` (split by file group between agents; see below)

## Goal
Every kept source file compiles with MSVC and GCC (32-bit). It doesn't need to link yet.

## Suggested split for parallel agents
- **S06a:** headers and the shared core (`FUNCTION.H`, `DEFINES.H`, `EXTERNS.H`, `TYPE.H`, …). Do this first, alone.
- **S06b–S06e:** after S06a merges, split the remaining `.CPP` files into four alphabetical groups.

## Inputs from S03
- Baseline: 10 of 316 files compile (GCC `-m32`). Scoreboard: `python3 scripts/compile_progress.py` (see `docs/building.md`).
- Most common first errors: `windows.h` missing on Linux (201 files), DOS/Watcom headers (`dos.h`, `io.h`, `mem.h`,
  `bios.h`, `pharlap.h`, `conio.h`), `ddraw.h` (16), the old `typedef int bool` / `enum {false,true}` (26),
  `cdecl`, `strcmpi`, `_MAX_PATH`, `iostream.h`, and a stray Ctrl-Z byte in `_DIPTABL.CPP`.
- Decide: add a MinGW-w64 i686 cross-compile job to CI? It sees real Windows headers, so it's a better
  scoreboard than plain GCC on Linux (S03 got 27/316 with it locally).
- Decide: keep global 1-byte packing (Watcom `/zp1`), or pack only the structs that are saved or sent over the network?
  Global packing needs `WINDOWS_IGNORE_PACKING_MISMATCH` on MSVC. Record the decision as an ADR.
- `CODE/PASSEDIT.CPP`, `CODE/PALETTE.CPP` and `PassEdit.h` are referenced but missing from the repo. Remove the references or stub them.

## Tasks
- [ ] Replace Watcom-specific syntax: `#pragma aux`, `_far`/`__near`, Watcom intrinsics, `bool` workarounds, old-style `for` scoping.
- [ ] Fix headers: `<mem.h>`, `<dos.h>`, `<i86.h>` → standard equivalents or our own shims in `CODE/compat/`.
- [ ] Keep struct layout identical where it is saved or sent over the network (`#pragma pack(1)` where the original used `/zp1`). Add `static_assert(sizeof(...))` checks for network structs.
- [ ] Watch the compile-progress score from S03 rise to 100%.

## Done when
CI reports 100% of game files compiling on both compilers. Every workaround is documented in `docs/codebase/compat-notes.md`.
