# Session log: S07-C — Assembly → C++: text, palette, cursor, upscaler

- **Date:** 2026-10-14
- **Session brief:** [S07](../sessions/S07-asm-to-cpp.md), package S07-C from [assembly-inventory.md](../codebase/assembly-inventory.md)
- **Branch / PR:** `worktree-agent-af76c51ad7838b320` (worktree of `claude/cool-bohr-9vg7ac`) / not pushed
- **Status at end:** Done

## Goal
Replace the S07-C assembly (inventory rows 2, 4, 26, 31, 39, 45) with portable C++ that has the same symbols,
signatures and output, with unit tests whose expected values are worked out from the asm.

## What was done
1. Fast-forwarded to `claude/cool-bohr-9vg7ac` (`66de4c5`, test harness present).
2. Read each asm file line by line and wrote the C++ next to it, then deleted the `.ASM`:

   | Asm (deleted) | C++ | Exports |
   |---------------|-----|---------|
   | `CODE/2TXTPRNT.ASM` | `CODE/2TXTPRNT.CPP` | `Buffer_Print`, `Get_Font_Palette_Ptr`, `ColorXlat` |
   | `WIN32LIB/FONT/SETFPAL.ASM` | `WIN32LIB/FONT/SETFPAL.CPP` | `Set_Font_Palette_Range` |
   | `WIN32LIB/PALETTE/PAL.ASM` | `WIN32LIB/PALETTE/PAL.CPP` | `Set_Palette_Range`, `Bump_Color`, `CurrentPalette` |
   | `WIN32LIB/MISC/FADING.ASM` | `WIN32LIB/MISC/FADING.CPP` | `Build_Fading_Table` |
   | `WIN32LIB/KEYBOARD/WWMOUSE.ASM` | `WIN32LIB/KEYBOARD/WWMOUSE.CPP` | `Mouse_Shadow_Buffer`, `Draw_Mouse`, `ASM_Set_Mouse_Cursor` |
   | `CODE/WINASM.ASM` | `CODE/WINASM.CPP` | `Asm_Interpolate`, `Asm_Interpolate_Line_Double`, `Asm_Interpolate_Line_Interpolate`, `MouseQX`, `MouseQY`, `Stop_Execution`, `Int3`; no-op stubs `ModeX_Blit`, `Set_Palette_Register` |

3. **`WWMOUSE` was ported** (not left for S08): it is ~400 lines of straightforward clipping, copying and RLE, so the
   port was not disproportionate. S08 can still switch to an OS cursor later and drop it.
4. The new files do **not** include the Westwood headers: every one of them (`FONT.H`, `PALETTE.H`, `MISC.H`,
   `DRAWBUFF.H`, `SHAPE.H`, `IFF.H`, `MOUSE.H`) still pulls in `windows.h`/`ddraw.h`/`dos.h` and fails on Linux (S06a
   is fixing that). Each file declares the prototypes it defines/uses with `extern "C"`, copied from those headers,
   and says so in a comment. `__cdecl` is left out because cdecl is the default on both 32-bit targets.
   No change to `WIN32LIB/INCLUDE` was needed.
5. Struct access: the asm read `GraphicViewPortClass` and `WWMouseClass` at fixed offsets (`GBUFFER.INC`,
   `MOUSE.INC`). The C++ reads them through small mirror structs that declare the same members in the same order,
   so they follow the real classes under any packing. `static_assert`s check the mirrors against the asm offsets
   when the build is 32-bit with 1-byte packing (the original configuration). The layout of both classes was not
   changed.
6. Unit tests (four executables, registered under `# --- S07-C ---` in `tests/tests.cmake`), all with synthetic data:
   - `s07c_text_test`: a hand-built font with three glyphs (normal, zero-height, odd width with junk high nibbles);
     transparent and opaque background, `\n` vs `\r`, forced wrap at the right edge, bottom overflow, NULL font,
     XAdd/Pitch, the initial `ColorXlat` contents, `Set_Font_Palette_Range` masking.
   - `s07c_palette_test`: `CurrentPalette` initial value, `Set_Palette_Range` copy + display hand-off,
     `Bump_Color` stepping and WORD truncation, and five `Build_Fading_Table` cases on two synthetic palettes
     (toward black, toward white with the 8-bit signed arithmetic, tie-breaking and early exact match, fraction
     clamping including negative values, NULL arguments). Every expected index was computed by hand (comments in the test).
   - `s07c_mouse_test`: drawing with transparency, clipping on each side, fully off-screen, pitch, shadow buffer
     store/restore (clipped and unclipped), and `ASM_Set_Mouse_Cursor` for uncompressed, 16-colour and compressed
     shapes (with stand-ins for the shape library and LCW), plus the "too big" path.
   - `s07c_interp_test`: the three upscalers on a 4x2 image (expected bytes written out), a 320-pixel line against
     the formula, and the leftover symbols.
7. Checked that the tests can fail: temporarily changing the fading tie-break (`<=` → `<`), the shadow-buffer store
   test (`== 1` → `!= 0`) and the high-nibble lookup (`& 0xF0` → `>> 4`) each made a test fail. The last one
   only failed after adding `high_nibble_indexes_table_unshifted`, because a consistently filled table hides it.
8. Updated `cmake/RASources.cmake` (only these six files), the inventory, `docs/codebase/libraries.md` (new
   § 2.4 with the font format, `ColorXlat`, fading and cursor details) and the S07 brief.

### Behaviour kept from the asm on purpose
- `Buffer_Print`: no left/right clipping; a character that does not fit forces a `\r`-style line feed; `\n` goes to
  x = 0, `\r` to the starting x; a glyph with 0 data rows skips the bottom-blank fill even with an opaque background;
  width/height counters are 8-bit; the high nibble indexes `ColorXlat[n << 4]`. `ColorXlat` grew from 241 to 256
  bytes; the extra bytes are never read.
- `Build_Fading_Table`: 8-bit wrapping subtraction, signed 8x8 multiply, 16-bit shift, as in the asm; colour 0 never
  chosen, no colour maps to itself, first exact match wins, otherwise ties go to the higher index; `frac` ≥ 256 (or
  negative) becomes 255.
- `Bump_Color`: only the low 16 bits of the colour numbers are used.
- `Mouse_Shadow_Buffer`: only `store == 1` saves; any other value restores. `Draw_Mouse`: visible width/height are
  counted in 8 bits. `ASM_Set_Mouse_Cursor`: returns the old `PrevCursor` and stores the new shape even when the
  shape is too big and nothing else changes; RLE decoding runs until the pixel count is exactly 0.
- `Asm_Interpolate_Line_Interpolate`: the last output row interpolates source line `lines`, **one past the end** of
  the image, and line `lines - 1` never appears on its own. The C++ keeps this; no caller uses this mode
  (`CopyType` 2 is never selected), and it is commented in the code.

### Deliberate deviations (only for inputs the game never passes)
- The upscalers' counters would underflow in the asm for widths < 4 or line counts < 1 (< 2 for
  `Line_Interpolate`) and run for ~4 billion iterations. The C++ stops instead. The game always passes 320x200.

### Dropped from the asm (nothing references them)
- `PAL.ASM`: `PaletteTable` (only used by the dead VGA branch), the private `VertBlank`, and the VGA DAC code paths.
- `WINASM.ASM`: `Asm_Create_Palette_Interpolation_Table` (its only call is commented out at
  `CODE/INTERPAL.CPP:150`; a prototype remains in `CODE/FUNCTION.H:626`, harmless), the C-linkage
  `Memory_Error_Exit` (the game uses the C++ variable in `WIN32LIB/MEM/ALLOC.CPP`; under GCC an `extern "C"` copy
  would even clash with it), and the scratch `LineBuffer`/`TopLine`/`BottomLine` became file-static.

## Decisions
- No ADR needed (the reserved number 0018 is unused): everything follows ADR-0010. Small decisions:
  - Port `WWMOUSE` rather than leave it for S08 (safe choice; S08 can still use an OS cursor).
  - Keep `ModeX_Blit` and `Set_Palette_Register` as empty stubs so `CODE/GSCREEN.CPP` and `CODE/WINSTUB.CPP` still
    link; S08 should delete the 320-wide branch of `GScreenClass::Blit_Display` and `Colour_Debug`, then the stubs.
  - Self-contained prototypes instead of the broken headers (see step 4).

## Files changed
| File / folder | Change |
|---------------|--------|
| `CODE/2TXTPRNT.CPP`, `CODE/WINASM.CPP` | New (C++ ports) |
| `WIN32LIB/FONT/SETFPAL.CPP`, `WIN32LIB/PALETTE/PAL.CPP`, `WIN32LIB/MISC/FADING.CPP`, `WIN32LIB/KEYBOARD/WWMOUSE.CPP` | New (C++ ports) |
| `CODE/2TXTPRNT.ASM`, `CODE/WINASM.ASM`, `WIN32LIB/FONT/SETFPAL.ASM`, `WIN32LIB/PALETTE/PAL.ASM`, `WIN32LIB/MISC/FADING.ASM`, `WIN32LIB/KEYBOARD/WWMOUSE.ASM` | Deleted |
| `cmake/RASources.cmake` | The six `.ASM` lines removed, the six `.CPP` files added to the game / `win32lib` lists |
| `tests/s07c_text_test.cpp`, `s07c_palette_test.cpp`, `s07c_mouse_test.cpp`, `s07c_interp_test.cpp` | New |
| `tests/tests.cmake` | Four tests under `# --- S07-C ---` |
| `docs/codebase/assembly-inventory.md` | Rows 2, 4, 26, 31, 39, 45 marked ported; S07-C note at the top |
| `docs/codebase/libraries.md` | New § 2.4 (font format, `ColorXlat`, fading, cursor); 2.3 row updated |
| `docs/sessions/S07-asm-to-cpp.md` | S07-C sub-items ticked |
| `docs/logs/2026-10-14-S07C-text-palette-cursor.md` | This log |

## Verification
```sh
cmake -S . -B build -G Ninja                     # RA_TARGET_ARCH=x86, gcc -m32, pack-structs=ON
cmake --build build --target ra_tests
ctest --test-dir build --output-on-failure
#   100% tests passed, 0 tests failed out of 5  (smoke + 4 S07-C executables: 12 + 8 + 11 + 5 test cases)

# The six new sources inside the real targets (case shims, WIN32 defines, packing):
ninja -C build CMakeFiles/win32lib.dir/WIN32LIB/FONT/SETFPAL.CPP.o CMakeFiles/win32lib.dir/WIN32LIB/KEYBOARD/WWMOUSE.CPP.o \
  CMakeFiles/win32lib.dir/WIN32LIB/MISC/FADING.CPP.o CMakeFiles/win32lib.dir/WIN32LIB/PALETTE/PAL.CPP.o \
  CMakeFiles/redalert.dir/CODE/WINASM.CPP.o CMakeFiles/redalert.dir/CODE/2TXTPRNT.CPP.o     # 6/6 compile

# 64-bit, AddressSanitizer + UBSan, -Wall -Wextra:
cmake -S . -B build64 -G Ninja -DRA_TARGET_ARCH=native \
  "-DCMAKE_CXX_FLAGS=-fsanitize=address,undefined -fno-sanitize-recover=all -Wall -Wextra" \
  "-DCMAKE_EXE_LINKER_FLAGS=-fsanitize=address,undefined"
cmake --build build64 --target ra_tests && ctest --test-dir build64   # 5/5 passed, no warnings

# clang -m32 -fpack-struct=1 -Wall -Wextra -fsyntax-only on all ten new files: no warnings
```
MSVC was not available here; CI's `windows-msvc-x86` job builds and runs the same tests.

## Determinism impact
None. These routines only draw text, cursors and upscaled screens, keep the display palette, or build remap
tables. Checked the callers: `Build_Fading_Table`'s tables (`DisplayClass::FadingGreen/Yellow/Red`, translucent
tables from `CODE/JSHELL.CPP:374`) are only passed as remap tables to drawing calls (`CODE/CELL.CPP:1012`,
`POWER.CPP:183`, `RADAR.CPP:783`, `TECHNO.CPP:4276`); `Bump_Color` has no game caller; `CurrentPalette` and
`ColorXlat` are display state; `MouseQX`/`MouseQY` are only read by the single-player intro/ending screens. None of
it is read by `CODE/QUEUE.CPP`'s lockstep path, the RNG or the event structures, and no struct layout changed.

## Problems & open questions
- **For S06a:** `CODE/ENDING.CPP:126` and `CODE/INTRO.CPP:211` use the global `MouseQX`/`MouseQY`, but no header
  declares them. Add `extern "C" int MouseQX, MouseQY;` (they are defined in `CODE/WINASM.CPP`). Note they are
  never written, so those screens always see 0; the original probably meant `Keyboard->MouseQX`. Not changed here
  (single player, and `CODE/` belongs to S06a).
- **For S06a:** once the Westwood headers compile, the new `.CPP` files can include them instead of their own
  prototypes. The parameter types differ only where the header uses `GraphicViewPortClass *`, `VOID *` or `BOOL`
  (`void *`/`int` here), which does not matter for `extern "C"` linkage.
- **For S07-B / S07-D:** `WWMOUSE.CPP` uses `_ShapeBuffer` (`extern "C" char *`, as in `SHAPE.H`) and
  `LCW_Uncompress` (`extern "C"`, as in `IFF.H`). Keep those names and linkage when porting `SHAPE.ASM` and
  `LCWUNCMP.ASM`.
- **For S08:** `ModeX_Blit` and `Set_Palette_Register` are empty stubs; remove the 320-wide branch in
  `GScreenClass::Blit_Display` (`CODE/GSCREEN.CPP`) and `Colour_Debug` (`CODE/WINSTUB.CPP`), then the stubs.
  `Set_Palette_Range` calls `Set_DD_Palette(palette)` (now a normal C call): the SDL layer must provide it.
  The software cursor works on any 8-bit viewport; switching to an OS cursor is still possible.
- `ColorXlat` is defined in the game (`CODE/2TXTPRNT.CPP`) but used by the library (`SETFPAL.CPP`), exactly as
  before; link order is fine because the game object is always linked.

## Next steps
- S10: confirm at link time that no other object still needs a symbol that only the deleted asm defined.
