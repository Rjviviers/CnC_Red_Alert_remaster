# Session log: S07-B — Assembly → C++: shape renderer

- **Date:** 2026-10-14
- **Session brief:** [S07](../sessions/S07-asm-to-cpp.md) (package S07-B)
- **Branch / PR:** `worktree-agent-abaeecbb16ac12ae2` (not pushed) / none yet
- **Status at end:** Done

## Goal
Port `CODE/2KEYFBUF.ASM` (`Buffer_Frame_To_Page`, the renderer for every unit and building) and
`WIN32LIB/SHAPE/SHAPE.ASM` to portable C++. The output must match the original pixel for pixel, for every flag
combination. Add golden tests whose expected values come from reading the asm.

## What was done
1. Fast-forwarded to `claude/cool-bohr-9vg7ac` (`66de4c5`, has `tests/ra_test.h`).
2. Read the asm (lines 1-3929; 3930-4848 are the dead DOS copy after `END`) and the only caller,
   `CC_Draw_Shape` (`CODE/CONQUER.CPP`), plus `CODE/2KEYFRAM.CPP` (the frame cache and its header struct).
3. Extracted the 512-entry `NewShapeJumpTable` with a script (16 rows × 32 entries, one letter per routine) and
   checked that, for every header byte that is a subset of the call's flags, each row matches `AllFlagsJumpTable`
   except entries 4 and 5, which are patched at run time. The table is transcribed literally in `SHAPEDRW.CPP`.
4. Wrote the renderer as one row loop, `Draw_Row<F>`, where `F` is a compile-time set of pixel operations
   (transparent, ghost, fade, predator) plus flags for each asm quirk that changes pixels. The 16 `BF_*` and
   19 `Single_Line_*` routines are each one instantiation. The predator state and the patched jump table stay
   file-level statics, as in the asm.
5. Split the code in two files so that it can be tested: `CODE/SHAPEDRW.CPP/.H` (no game headers, compiles today) and
   `CODE/2KEYFBUF.CPP` (the original `extern "C" __cdecl` signature, reads `GraphicViewPortClass` through its
   `Get_*` accessors, forwards a `va_list`).
6. `SHAPE.ASM` became `WIN32LIB/SHAPE/SHAPE.CPP` (`_ShapeBuffer`, `_ShapeBufferSize`, `Set_Shape_Buffer`).
7. Wrote 47 + 2 golden tests and worked out every expected pixel by hand from the asm (line references are in the
   test comments). All passed on the first run. To check that the tests catch errors, I made 10 mutations
   (removing each kept bug, the fade-2^32 logic, the jump-table patch, XAdd/Pitch in the predator table,
   off-by-one clipping, the predator skip, and `SHAPE_PARTIAL`). Every mutation made at least one test fail.
8. Deleted both `.ASM` files and updated `cmake/RASources.cmake`, `tests/tests.cmake`, the inventory, the brief, and
   the new page [shape-renderer.md](../codebase/shape-renderer.md).

## Decisions
- No ADR (0017 not used). Small decisions:
  - **Kept the asm bugs that change pixels.** These are: the `and` instead of `add` in the predator-fading line
    routines, fading from colour 0 in the predator+ghost+fading line routines, the wide ghost index, the fade count
    0 = 2^32 lookups, and `BF_Predator_Trans` drawing the source colour. The project rule is exactness. None of them
    are reachable with today's `CC_Draw_Shape` arguments.
  - **Diverged only where the asm crashes, hangs, or uses garbage.** Width or height ≤ 0 on the line-header path,
    a jump-table address in EAX at the start of a row, the stray `pop ecx`, the return value, and the
    `ShapeBuffer` aliases. All are listed in shape-renderer.md.
  - **Did not add `SHAPE_TRANS` to `WIN32LIB/INCLUDE/SHAPE.H`.** My code doesn't need it (it uses its own `BF_SHAPE_*`
    constants), so I made no header edits. See "Problems".

## Files changed
| File / folder | Change |
|---------------|--------|
| `CODE/2KEYFBUF.ASM` | deleted |
| `CODE/2KEYFBUF.CPP` | new: `Buffer_Frame_To_Page` glue |
| `CODE/SHAPEDRW.CPP`, `CODE/SHAPEDRW.H` | new: the renderer |
| `WIN32LIB/SHAPE/SHAPE.ASM` | deleted |
| `WIN32LIB/SHAPE/SHAPE.CPP` | new |
| `cmake/RASources.cmake` | removed the two `.ASM`; added `CODE/2KEYFBUF.CPP`, `CODE/SHAPEDRW.CPP` (after `2KEYFRAM.CPP`), `WIN32LIB/SHAPE/SHAPE.CPP` (after `GETSHAPE.CPP`) |
| `tests/tests.cmake` | two tests under `# --- S07-B ---` |
| `tests/s07b_shape_renderer_test.cpp`, `tests/s07b_shape_buffer_test.cpp` | new |
| `docs/codebase/shape-renderer.md` | new |
| `docs/codebase/assembly-inventory.md` | rows 1, 46 and the S07-B package row marked ported |
| `docs/sessions/S07-asm-to-cpp.md` | S07-B sub-items ticked |

## Verification
```sh
cmake -S . -B build                      # arch=x86 (pointer size 4), pack-structs=ON
cmake --build build --target ra_tests    # no warnings
ctest --test-dir build                   # 100% tests passed, 0 tests failed out of 3
```
`s07b_shape_renderer_test`: 47 tests, 0 failed. `s07b_shape_buffer_test`: 2 tests, 0 failed.
I also built natively for 64-bit with `g++ -std=c++14 -Wall -Wextra -O2` (47/47 pass). `CODE/SHAPEDRW.CPP` and
`WIN32LIB/SHAPE/SHAPE.CPP` pass `-fsyntax-only` with the project flags. `CODE/2KEYFBUF.CPP` stops at
`function.h` → `windows.h`, like every other game file until S06/S08.

## Determinism impact
None. This is rendering only. The renderer reads and writes pixel buffers and its own statics; it never touches
simulation state, random numbers or network events. `ShapeFrameHeaderType` is a cache in memory and is never
saved or sent over the network.

## Problems & open questions
- `SHAPE_TRANS` (0x40) is used by `CODE/CONQUER.CPP` but is defined in no header (only the asm had it).
  S06 must add `SHAPE_TRANS = 0x0040` to `ShapeFlags_Type` in `WIN32LIB/INCLUDE/SHAPE.H`.
- `GraphicViewPortClass::Get_Offset()` returns `long`, which truncates pointers on 64-bit builds (S06/S08).
- `WIN32LIB/KEYBOARD/WWMOUSE.ASM` uses the `ShapeBuffer` alias. Its port (S07-C) must use `_ShapeBuffer`.
- `CC_Draw_Shape` with `SHAPE_GHOST|SHAPE_PREDATOR` (and no fading) passes the predator offset where the ghost
  table is expected. This is a latent caller bug and is unchanged.

## Next steps
- S10: link `2KEYFBUF.CPP`, `SHAPEDRW.CPP` and `SHAPE.CPP`. Check visually against the original game (stealth tank
  = predator, aircraft shadows = ghost, fog/shroud = fading).
- If profiling ever shows the renderer is hot, specialise `Draw_Row` for long runs. It is a plain per-pixel loop
  now, which is plenty for 640×400 8-bit.
