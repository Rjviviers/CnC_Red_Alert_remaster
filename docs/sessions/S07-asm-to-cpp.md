# S07 — Assembly → C++

**Status:** Not started · **Wave:** 3 · **Depends on:** S02, S03 · **Parallel with:** S06, S08, S09
**Owns:** the assembly files listed in `docs/codebase/assembly-inventory.md`; new C++ files beside them

## Goal
Replace every assembly routine the build needs with portable C++ that behaves identically.

## Suggested split for parallel agents
One agent per area from the inventory, for example: blitting/draw buffers, shape drawing, LCW/compression,
text printing, keyboard buffer, misc (CPUID, coordinates).

## Inputs from S01
- `calcx`/`calcy` and `Fixed_To_Cardinal` (Watcom inline asm) feed unit movement. They must be bit-exact.
  The simulation itself has no floating point.

## Tasks
- [ ] For each routine, write the C++ version with the same signature the callers expect.
- [ ] Write a unit test for each routine, using inputs and expected outputs worked out from the assembly (compression round-trips, known blit results).
- [ ] Routines used by the simulation (e.g. `COORDA.ASM`) must be bit-exact. Flag them in the log.
- [ ] Update the inventory: mark each file *ported*, with a link to its replacement and test.

## Done when
The build no longer needs an assembler, and every ported routine has a passing test in CI.
