# Session log: S07-A — Assembly → C++: surface primitives

- **Date:** 2026-10-14
- **Session brief:** [S07](../sessions/S07-asm-to-cpp.md) (package S07-A)
- **Branch / PR:** `worktree-agent-aaf69e5d20d863004` (based on `claude/cool-bohr-9vg7ac` at `66de4c5`) / not pushed
- **Status at end:** Done

## Goal
Port the `WIN32LIB/DRAWBUFF` surface primitives (`BITBLIT`, `CLEAR`, `DRAWLINE`, `FILLRECT`, `GETPIX`, `PUTPIX`,
`REMAP`, `SCALE`, `STAMP`, `TOBUFF`, `TOPAGE`) and `WIN32LIB/MISC/CLIPRECT` to portable C++ with the same
signatures and behaviour, remove the video-memory icon cache (`STMPCACH`), and unit-test every routine.

## What was done
1. Fast-forwarded to `claude/cool-bohr-9vg7ac` (`66de4c5`, test harness present).
2. Checked the package against the inventory: the S07-A row lists exactly the 12 files above; `STMPCACH` (row 21)
   is marked "remove the cache with S08". `FILLQUAD`, `GETCLIP`, `SZREGION`, `TXTPRNT` are not S07-A (not pulled /
   shadowed).
3. Read every asm file line by line and wrote down the clipping rules, loop shapes, flag-based compares
   (signed vs unsigned), and what the asm leaves in `EAX`. These are in the comment block of each `.CPP` and in
   [surface-primitives.md](../codebase/surface-primitives.md).
4. `GBUFFER.H` cannot compile on GCC yet (it includes `<ddraw.h>`, and `WWSTD.H` includes `dos.h`/`bios.h` and
   `typedef int bool`). Rather than patch shared headers, the ports read the viewport through a new header,
   `WIN32LIB/DRAWBUFF/GVPORT.H`: `GVPLayout` mirrors all data members of `GraphicViewPortClass` (names, types, order)
   and `static_assert`s the `GBUFFER.INC` offsets on 32-bit builds. The `GBUFFER.H` wrappers are unchanged and still
   pass `this`. Same approach for `CLIPRECT.CPP` (`MISC.H` needs `<windows.h>`): it declares its own prototypes.
5. Wrote the 13 C++ files next to the asm, deleted the 13 `.ASM` files, updated `cmake/RASources.cmake`
   (win32lib C++ list + asm list).
6. Replaced `STMPCACH.ASM` with a stand-in instead of a port: `Is_Icon_Cached` always returns -1,
   `Cache_Copy_Icon` is a plain copy (ADR-0016). All drawing still happens: `Draw_Stamp` falls back to
   `Buffer_Draw_Stamp_Clip`, as the original did for every uncached icon.
7. Tests: one executable per asm file, with small buffers, ASCII-picture expected images (`tests/s07a_surface.h`)
   worked out by hand from the asm, viewports with non-zero `XAdd` and `Pitch`, clipping on every side, rejection,
   and the quirks listed below. All tests passed on the first full run; to check that the pictures really detect
   errors, the Bresenham accumulator was mutated (`>> 1` → `>> 2`) and 5 drawline cases failed as expected
   (mutation reverted).
8. Documentation: inventory rows, new codebase page, ADR-0016, brief checkboxes (S07-A sub-items).

## Decisions
- [ADR-0016](../decisions/0016-drop-video-memory-icon-cache.md) (Proposed): drop the DirectDraw video-memory icon
  cache; `STMPCACH.CPP` is a link stand-in until S08 deletes `ICONCACH`.
- `GVPORT.H` layout mirror instead of including `GBUFFER.H` (see step 4). No header in `WIN32LIB/INCLUDE` or
  `CODE/` was edited.
- Return values the asm left undefined (`Linear_Blit_To_Linear`, `Linear_Scale_To_Linear`, `Buffer_To_Buffer`,
  `Buffer_To_Page`) return 0. Grep shows no caller uses them. `Buffer_Get_Pixel` keeps the asm's out-of-range value
  (the rejected coordinate), because it was well defined.
- Three deliberate differences, where the asm would hang or fault: `Buffer_Clear` with `Height <= 0`, the stamp
  fast path for icons less than 4 rows high (both looped ~4 billion times), and `Linear_Scale_To_Linear` divide
  faults (it returns without drawing).
- Overlapping blits: direction chosen with an unsigned pointer compare; the asm's compare was signed, which only
  differs for a buffer straddling `0x80000000`.
- Ambiguous / quirky behaviour preserved on purpose (all tested): source clipping in `Linear_Blit_To_Linear` does
  not move the destination; `Buffer_Fill_Rect` takes 16-bit coordinates; scaling's row accumulator starts at
  `-dst_height` (2x upscale repeats row 0 three times); stamp map lookup replaces only the low byte; stamp remap
  tests transparency after the remap but scale tests it before; `Buffer_To_Buffer`'s size check ignores the
  left/top skip; `Confine_Rect` reports "shifted" for a position of exactly 0.
- The brief's checkboxes cover all S07 packages, so they stay unticked; S07-A added ticked sub-items. Status left
  as is.

## Files changed
| File / folder | Change |
|---------------|--------|
| `WIN32LIB/DRAWBUFF/GVPORT.H` | New: `GraphicViewPortClass` layout mirror (`GBUFFER.INC` in C++), helpers, prototypes |
| `WIN32LIB/DRAWBUFF/{BITBLIT,CLEAR,DRAWLINE,FILLRECT,GETPIX,PUTPIX,REMAP,SCALE,STAMP,TOBUFF,TOPAGE}.CPP` | New: ports |
| `WIN32LIB/DRAWBUFF/STMPCACH.CPP` | New: icon-cache stand-in (ADR-0016) |
| `WIN32LIB/MISC/CLIPRECT.CPP` | New: port of `Clip_Rect`, `Confine_Rect` |
| `WIN32LIB/DRAWBUFF/*.ASM` (12), `WIN32LIB/MISC/CLIPRECT.ASM` | Deleted |
| `cmake/RASources.cmake` | 13 C++ files added to `RA_WIN32LIB_CXX_SOURCES`, 13 asm files removed from `RA_WIN32LIB_ASM_SOURCES` |
| `tests/s07a_*_test.cpp` (13), `tests/s07a_surface.h` | New unit tests and helpers |
| `tests/tests.cmake` | 13 `ra_add_test` lines under `# --- S07-A ---` |
| `docs/codebase/surface-primitives.md` | New page: what the routines do, layout rules, quirks, notes for S08 |
| `docs/codebase/assembly-inventory.md` | S07-A package row and rows 10-12, 14, 16-21, 23, 24, 44 marked ported/removed |
| `docs/decisions/0016-drop-video-memory-icon-cache.md` | New ADR (not added to the ADR index: README files are edited by the coordinator) |
| `docs/sessions/S07-asm-to-cpp.md` | S07-A sub-items under each task |

## Verification
```sh
cmake -S . -B build -G Ninja                  # GCC 13, RA_TARGET_ARCH=x86 (-m32)
cmake --build build --target ra_tests
ctest --test-dir build --output-on-failure
```
Result: `100% tests passed, 0 tests failed out of 14` (smoke + 13 S07-A executables; 92 S07-A test cases:
bitblit 13, clear 4, drawline 11, fillrect 9, getpix 3, putpix 3, remap 5, scale 10, stamp 12, stmpcach 2,
tobuff 6, topage 4, cliprect 10).

Also:
- The 13 new win32lib objects build with the library's own flags (`ninja` on their `win32lib.dir/...CPP.o` targets).
- `g++ -m32 -std=gnu++14 -fpack-struct=1 -Wall -Wextra -fsyntax-only` on all 13 files: no warnings; same for
  `-m64` on `BITBLIT`/`SCALE`/`STAMP`.
- Every test rebuilt natively (64-bit) with `-fsanitize=address,undefined -fno-sanitize-recover=all`: all pass,
  no sanitizer reports (the 40-byte `IControl_Type` size check is 32-bit only).

## Determinism impact
None. Every S07-A routine only reads or writes pixel buffers (drawing), except `Clip_Rect`/`Confine_Rect`, which
compute the tactical view and radar positions: per-client view state, not simulation state, and the port is
integer-exact anyway. No struct layout, network event, random number or iteration order of the simulation changed.

## Problems & open questions
- `GVPLayout` and `GraphicViewPortClass` must stay in sync by hand until `GBUFFER.H` compiles; then add
  `static_assert(sizeof(GraphicViewPortClass) == sizeof(GVPLayout), "")` (suggested in `GBUFFER.CPP`).
- `GraphicViewPortClass::Offset` is a `long` holding a pointer: fine for 32-bit (ADR-0003), wrong on MSVC x64.
- `STAMP.INC` (both copies) is now unused; `GBUFFER.INC`/`DRAWBUFF.INC` are still included by other packages' asm.
  Delete them when the last asm goes.

## Next steps
- **S08:** delete the icon cache (`ICONCACH.CPP`/`.H`, the `Draw_Stamp` cache branch in `GBUFFER.H`,
  `IconCacheAllowed`, `Register_Icon_Set` in `CODE/CDATA.CPP:3136`), then `STMPCACH.CPP` and its test; keep
  `IsDirectDraw` false for SDL surfaces so `Blit`/`Fill_Rect` use these ports. See
  [surface-primitives.md § For S08](../codebase/surface-primitives.md#for-s08-platform-layer).
- **S06:** do not change the data members of `GraphicViewPortClass`, or update `GVPORT.H` with them.
- **S10:** the ports link as plain `extern "C"` cdecl symbols, as the `DRAWBUFF.H` prototypes expect.

## Review & merge notes (orchestrator)

- Merged into `claude/cool-bohr-9vg7ac` on 2026-10-14, last of the four S07 packages. Resolved list/inventory/brief conflicts
  with the same script as S07-B. Checked that all 13 new `DRAWBUFF`/`CLIPRECT` `.CPP` files are in the win32lib list, and that the
  win32lib asm list now holds only audio (S09), `VMPAGEIN` and `APROFILE`.
- On the merged tree (GCC `-m32`): `ctest` reports 23/23 test executables passing; compile progress is 36 of 311.
- Handoffs: S08 (`IsDirectDraw` false for SDL surfaces; delete the rest of the icon cache; `GVPORT.H` mirror) and S06a (a
  `static_assert` tying `GVPORT.H` to `GraphicViewPortClass` once `GBUFFER.H` compiles).
