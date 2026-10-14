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

## Inputs from S04
- S06 owns the **wire layout** of network structs. Packets are raw structs, and their layout depends on Watcom's enum
  sizes, bitfield layout and `/zp1` packing. With modern defaults, `EventClass` grows from 20 to 32 bytes, and GCC
  and MSVC disagree on other structs. Pin enum underlying types, packing and bitfields to the original Watcom layout,
  and add `static_assert`s (see [lan-multiplayer.md § Keeping network structs layout-stable](../design/lan-multiplayer.md#keeping-network-structs-layout-stable)).
  This is required for Windows and Linux builds to play together.

## Inputs from S01
- The release is missing `PALETTE.CPP`, `PASSEDIT.CPP`, `phone.h`, `pcx.h`, and Greenleaf's `modem.h`/`fast.h`.
  Recreate them or remove the references first (S05 drops the Greenleaf ones). Nothing builds until this is done.
  **Owner decision (2026-10-14):** there's no other copy of these files. S06 rewrites them from how the rest
  of the code uses them, and documents each rewritten file in `docs/codebase/compat-notes.md`.
- Network events depend on byte packing, compiler-specific bitfields and `unsigned long` fields, and the original
  `bool` was 4 bytes. See [game-code.md](../codebase/game-code.md) (determinism section).

## Tasks
- [ ] Replace Watcom-specific syntax: `#pragma aux`, `_far`/`__near`, Watcom intrinsics, `bool` workarounds, old-style `for` scoping.
- [ ] Fix headers: `<mem.h>`, `<dos.h>`, `<i86.h>` → standard equivalents or our own shims in `CODE/compat/`.
- [ ] Keep struct layout identical where it is saved or sent over the network (`#pragma pack(1)` where the original used `/zp1`). Add `static_assert(sizeof(...))` checks for network structs.
- [ ] Watch the compile-progress score from S03 rise to 100%.

## Done when
CI reports 100% of game files compiling on both compilers. Every workaround is documented in `docs/codebase/compat-notes.md`.
