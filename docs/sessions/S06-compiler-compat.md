# S06 — Compiler compatibility: game code compiles

**Status:** Not started · **Wave:** 3 · **Depends on:** S05 · **Parallel with:** S07, S08, S09
**Owns:** `CODE/*.CPP`/`*.H` (split by file group between agents; see below)

## Goal
Every kept source file compiles with MSVC and GCC (32-bit). It doesn't need to link yet.

## Suggested split for parallel agents
- **S06a:** headers and the shared core (`FUNCTION.H`, `DEFINES.H`, `EXTERNS.H`, `TYPE.H`, …). Do this first, alone.
- **S06b–S06e:** after S06a merges, split the remaining `.CPP` files into four alphabetical groups.

## Tasks
- [ ] Replace Watcom-specific syntax: `#pragma aux`, `_far`/`__near`, Watcom intrinsics, `bool` workarounds, old-style `for` scoping.
- [ ] Fix headers: `<mem.h>`, `<dos.h>`, `<i86.h>` → standard equivalents or our own shims in `CODE/compat/`.
- [ ] Keep struct layout identical where it is saved or sent over the network (`#pragma pack(1)` where the original used `/zp1`). Add `static_assert(sizeof(...))` checks for network structs.
- [ ] Watch the compile-progress score from S03 rise to 100%.

## Done when
CI reports 100% of game files compiling on both compilers. Every workaround is documented in `docs/codebase/compat-notes.md`.
