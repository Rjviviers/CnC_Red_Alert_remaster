# S07 — Assembly → C++

**Status:** Not started · **Wave:** 3 · **Depends on:** S02, S03 · **Parallel with:** S06, S08, S09
**Owns:** the assembly files listed in `docs/codebase/assembly-inventory.md`; new C++ files beside them

## Goal
Replace every assembly routine the build needs with portable C++ that behaves identically.

## Suggested split for parallel agents
One agent per area from the inventory, for example: blitting/draw buffers, shape drawing, LCW/compression,
text printing, keyboard buffer, misc (CPUID, coordinates).

## Inputs from S02 ([assembly-inventory.md](../codebase/assembly-inventory.md), [ADR-0010](../decisions/0010-asm-porting-policy.md))
- Only **25** asm files need porting (~8,150 lines). Use the five packages from the inventory as the parallel split:
  **S07-A** surface primitives, **S07-B** shape renderer (`2KEYFBUF.ASM`, the big one; needs golden-image tests),
  **S07-C** text/palette/cursor, **S07-D** data codecs (adopt `CODE/LCW.CPP`), **S07-E** audio decoders (adopt `CODE/ADPCM.CPP`;
  may move to S09). Movie-only asm (S07-F) is skipped because movies are stubbed (ADR-0005).
- No linked `.ASM` file is on the simulation path. The real simulation risk is the **inline** `#pragma aux` asm in
  `CODE/JSHELL.H` (`Fixed_To_Cardinal`, `Cardinal_To_Fixed`) and `CODE/COORD.CPP` (`calcx`/`calcy`).
- Never port the asm `Desired_Facing*`, `Random` or `Calculate_CRC`; the C++ versions are the real ones.
- The asm reads `GraphicViewPortClass` and `WWMouseClass` at hard-coded offsets (`GBUFFER.INC`). Coordinate with S06/S08.

## Inputs from S01
- `calcx`/`calcy` and `Fixed_To_Cardinal` (Watcom inline asm) feed unit movement. They must be bit-exact.
  The simulation itself has no floating point.

## Tasks
- [ ] For each routine, write the C++ version with the same signature the callers expect.
  - [x] S07-D: `CODE/LCWCOMP.CPP`, `WIN32LIB/WSA/XORDELTA.CPP`, `WIN32LIB/MEM/MEM_COPY.CPP`, `WIN32LIB/MISC/REVERSE.CPP`, `WIN32LIB/MISC/RANDOM.CPP` (`RandNumb` only); `LCW_Uncompress` adopted into `CODE/LCW.CPP` ([log](../logs/2026-10-14-S07D-data-codecs.md))
- [ ] Write a unit test for each routine, using inputs and expected outputs worked out from the assembly (compression round-trips, known blit results).
  - [x] S07-D: `tests/s07d_lcw_test.cpp`, `tests/s07d_xordelta_test.cpp`, `tests/s07d_misc_test.cpp`; on 32-bit GCC also compared with the original instructions (`tests/s07d_asm_oracle.cpp`, ADR-0019)
- [ ] Routines used by the simulation (e.g. `COORDA.ASM`) must be bit-exact. Flag them in the log.
  - [x] S07-D: none of its routines is simulation code; map data is decoded by `LCW_Uncomp`, whose output is unchanged (log, "Determinism impact")
- [ ] Update the inventory: mark each file *ported*, with a link to its replacement and test.
  - [x] S07-D: rows 3, 29, 32, 41, 42, 47

## Done when
The build no longer needs an assembler, and every ported routine has a passing test in CI.
