# ADR-0010: Port only the assembly the game needs; prefer the existing C++ versions

- **Status:** Proposed (suggested by S02; awaiting project owner)
- **Date:** 2026-10-14
- **Session:** S02

## Context
The repository has 357 `.ASM` files, and 66 of them take part in the Win32 link. S02 found that only 25 really need
porting (~8,150 lines). The rest are never pulled into the executable, are shadowed by C++ code, are dead, or are
trivial (see [assembly-inventory.md](../codebase/assembly-inventory.md)). For some routines the repo already has
C++ versions: `CODE/LCW.CPP`, `CODE/ADPCM.CPP`, `CODE/FACE.CPP`, `CODE/RANDOM.CPP`, and `Calculate_CRC` in `CODE/INIT.CPP`.

## Decision
- Port only the 25 assembly files the inventory marks as needed (S07 packages A–E). Delete or replace the rest (S05).
- Where a C++ version already exists, adopt it instead of porting the asm.
- **Never** let the asm versions of `Desired_Facing*`, `Random`/`IRandom` or `Calculate_CRC` replace the C++ ones. The
  game's lockstep simulation depends on the C++ versions.
- Movie-only asm (old package S07-F) is not ported: movies are stubbed (ADR-0005).

## Consequences
S07 is about a fifth of the size the raw file count suggests. Some conclusions depend on how the Watcom linker picked
between duplicate symbols (unverified). If a symbol turns out to be missing at link time (S10), revisit this inventory.
