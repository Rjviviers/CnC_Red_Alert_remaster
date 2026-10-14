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
  - [x] S07-B: `Buffer_Frame_To_Page` (`CODE/2KEYFBUF.CPP` + `CODE/SHAPEDRW.CPP`); `Set_Shape_Buffer` and the shape-buffer globals (`WIN32LIB/SHAPE/SHAPE.CPP`).
- [ ] Write a unit test for each routine, using inputs and expected outputs worked out from the assembly (compression round-trips, known blit results).
  - [x] S07-B: `tests/s07b_shape_renderer_test.cpp` (47 golden tests: every `BF_*` and `Single_Line_*` variant, clipping at every edge) and `tests/s07b_shape_buffer_test.cpp`.
- [ ] Routines used by the simulation (e.g. `COORDA.ASM`) must be bit-exact. Flag them in the log.
  - [x] S07-B: none; rendering only (see the S07-B log).
- [ ] Update the inventory: mark each file *ported*, with a link to its replacement and test.
  - [x] S07-B: rows 1 and 46 and the S07-B package row.

## Done when
The build no longer needs an assembler, and every ported routine has a passing test in CI.
