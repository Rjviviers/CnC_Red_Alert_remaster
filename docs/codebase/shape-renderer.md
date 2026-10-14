# Shape renderer (`Buffer_Frame_To_Page`)

Every unit, building, infantry, overlay, animation and bullet on the map is drawn by one function,
`Buffer_Frame_To_Page`. It was `CODE/2KEYFBUF.ASM` (2,575 lines of TASM, the largest asm file). Session S07-B
ported it to C++ (removed from the tree; read it with `git show 66de4c5:CODE/2KEYFBUF.ASM`). Line numbers
in this page and in the code comments refer to that file.

| File | What |
|------|------|
| [`CODE/2KEYFBUF.CPP`](../../CODE/2KEYFBUF.CPP) | `Buffer_Frame_To_Page` with the original signature. Reads the viewport fields and forwards to `Buffer_Frame_To_View` |
| [`CODE/SHAPEDRW.CPP`](../../CODE/SHAPEDRW.CPP), [`CODE/SHAPEDRW.H`](../../CODE/SHAPEDRW.H) | The renderer itself. No game headers, so it is unit tested on its own |
| [`WIN32LIB/SHAPE/SHAPE.CPP`](../../WIN32LIB/SHAPE/SHAPE.CPP) | `_ShapeBuffer`, `_ShapeBufferSize`, `Set_Shape_Buffer` (was `SHAPE.ASM`) |
| [`tests/s07b_shape_renderer_test.cpp`](../../tests/s07b_shape_renderer_test.cpp) | Golden tests, expectations worked out from the asm |

## Call path

`CC_Draw_Shape` (`CODE/CONQUER.CPP`) is the only caller:

1. `Build_Frame` (`CODE/2KEYFRAM.CPP`) decompresses frame *n* of a keyframe shape file (LCW + XOR deltas) and
   returns a pointer to it. With the **big shape buffer** on (`UseBigShapeBuffer`, the normal case) it returns a
   pointer to a *frame header* in a cache; otherwise to the raw pixels in `_ShapeBuffer`.
2. Rotated or scaled shapes are drawn into a scratch bitmap first; `UseOldShapeDraw = true` then tells the
   renderer the pointer is raw pixels.
3. `SHAPE_FADING|SHAPE_PREDATOR` together (aircraft and bullet shadows) is turned into `SHAPE_GHOST` with
   `DisplayClass::SpecialGhost`.
4. `SHAPE_TRANS` is always added, the fade count is always 1, and the predator argument is the game frame
   number (negated when `x > window width * 4`, which is always clipped away in practice).

```c
long __cdecl Buffer_Frame_To_Page(int x, int y, int w, int h, void *Buffer,
                                  GraphicViewPortClass &view, int flags, ...);
```

The optional arguments follow `flags`, each present only if its flag is set, **in this order**:

| Flag | Value | Arguments |
|------|-------|-----------|
| `SHAPE_CENTER` | 0x0020 | none: `x -= w>>1`, `y -= h>>1` (arithmetic shift) |
| `SHAPE_TRANS` | 0x0040 | none: colour 0 is transparent. Not in `SHAPE.H`'s enum; only the asm defined it |
| `SHAPE_GHOST` | 0x1000 | `void *ghost_table` |
| `SHAPE_FADING` | 0x0100 | `void *fade_table, int count` (`count & 63`; 0 means 2^32, see below) |
| `SHAPE_PREDATOR` | 0x0200 | `int offset` |
| `SHAPE_PARTIAL` | 0x4000 | `int partial` (`& 255`) |

Other `ShapeFlags_Type` bits (flip, scale, priority, colour remap) are ignored by this function.

## Shape data

**Raw frame:** `w * h` bytes, one palette index per pixel, row after row, no padding.

**Big-shape-buffer frame** (`CODE/2KEYFRAM.CPP`, cached forever after the first decompression):

```
ShapeFrameHeaderType          12 bytes with 32-bit pointers
  unsigned draw_flags         0xFFFFFFFF until the first draw, then flags & (TRANS|FADING|PREDATOR|GHOST)
  char    *shape_data         offset of the pixels from the buffer start
  int      shape_buffer       1 = TheaterShapeBufferStart, 0 = BigShapeBufferStart
unsigned char line[h]         one line header per row, built on the first draw
unsigned char pixels[w*h]
```

The struct is declared twice, in `2KEYFRAM.CPP` (`ShapeHeaderType`) and in `SHAPEDRW.H` (`ShapeFrameHeaderType`);
they must stay identical.

**Line header** (built by `Setup_Shape_Header`, asm 941-1082) for the flags of the first draw:

| Bit | Name | Set when |
|-----|------|----------|
| 1 | `BLIT_TRANSPARENT` | `SHAPE_TRANS` and the row has a colour-0 pixel |
| 2 | `BLIT_GHOST` | `SHAPE_GHOST` and the row has a non-zero pixel with `is_translucent[c] != 0xFF` |
| 4 | `BLIT_FADING` | `SHAPE_FADING` and the row has a non-transparent pixel |
| 8 | `BLIT_PREDATOR` | `SHAPE_PREDATOR` and the row has a non-transparent pixel |
| 16 | `BLIT_SKIP` | the whole row is transparent (the byte is exactly 16) |

## Tables

* **Ghost table:** `is_translucent[256]` followed by `translucent[n][256]`. A source colour `c` with
  `g = is_translucent[c] != 0xFF` is drawn as `translucent[g][dest]`; otherwise as `c`.
* **Fade table:** 256 bytes. The colour is looked up `count` times (`v = fade[v]`).
* **Predator table:** `BFPredTable = 1,3,2,5,2,3,4,1` (bytes to the right of the pixel being drawn). For a negative
  predator argument the offsets are `{-1,-3,-2,-5,-2,-4,-3,-1} + Width + XAdd + Pitch` as 16-bit values, i.e.
  pixels on the next row, computed again on every predator call.

## Two drawing systems

`Buffer_Frame_To_Page` picks one of two systems:

* **Line-header system** (the fast path): big shape buffer on, `UseOldShapeDraw` false, `draw_flags` equal to the
  call's flags (or 0xFFFFFFFF, in which case the headers are built now) **and the shape is not clipped**. Each row
  is drawn by the `Single_Line_*` routine that the jump table gives for the row's header byte. The jump table is
  16 rows (the call's TRANS/GHOST/FADING/PREDATOR bits; row 15, `AllFlagsJumpTable`, right after building the
  headers) of 32 entries (header byte `& 31`). It is transcribed literally in `SHAPEDRW.CPP`.
* **Old system:** everything else (raw pixels, clipped shapes, flags that differ from `draw_flags`). One
  `BF_*` routine, chosen by the call's TRANS/GHOST/FADING/PREDATOR bits, draws the whole clipped rectangle.

Note: the headers are built before clipping is checked, so a frame first drawn partly off screen gets its headers
anyway.

### Clipping

Cohen-Sutherland outcodes built from sign bits (`shld`): for `(x, y)` and `(x+w, y+h)`, left = `< 0`,
right = `> Width`, top = `< 0`, bottom = `> Height`. If both corners share an outside bit the shape is skipped.
Otherwise the rectangle is cut to `[0, Width) x [0, Height)` and drawn with the old system; an empty result
draws nothing. The source keeps its stride (`w`), the destination row is `Width + XAdd + Pitch` bytes.

## Per-pixel behaviour of every routine

`s` = source pixel, `d` = destination pixel, `fade()` = `count` lookups, `ghost(v)` = ghost-table rule above,
`pred` = the predator step below. "skip" = destination untouched. A routine with `_Trans` in its name skips
`s == 0` first (and then does not advance the predator).

| Routine (asm line) | Letter | Pixel result |
|--------------------|--------|--------------|
| `BF_Copy` (1567), `Short/Long_Single_Line_Copy` (1732, 1697) | C | `s` |
| `Single_Line_Skip` (1750) | K | whole row skipped |
| `BF_Trans` (2664), `Single_Line_Trans` (1619) | T | `s` |
| `BF_Ghost(_Trans)` (2715, 2783), `Single_Line_Ghost(_Trans)` (1765, 1798) | G g | `ghost(s)` |
| `BF_Fading(_Trans)` (2857, 2924), `Single_Line_Fading(_Trans)` (1912, 1947) | F f | `fade(s)` |
| `Single_Line_Single_Fade(_Trans)` (1989, 2014) | S s | `fade_table[s]` once (only used when count is 1) |
| `BF_Ghost_Fading(_Trans)` (2997, 3082), `Single_Line_Ghost_Fading(_Trans)` (2063, 2107) | H h | `fade(ghost(s))` |
| `BF_Predator` (3173), `Single_Line_Predator(_Trans)` (2204, 2249) | P p | predator pixel: `d[off]`; other pixels: **skip** |
| `BF_Predator_Trans` (3255) | | predator pixel: `d[off]`; other pixels: **`s`** |
| `BF_Predator_Ghost(_Trans)` (3336, 3426) | | `ghost(pred ? d[off] : s)` |
| `Single_Line_Predator_Ghost(_Trans)` (2294, 2351) | Q q | same, but see "wide ghost index" |
| `BF_Predator_Fading(_Trans)` (3522, 3611) | | `fade(pred ? d[off] : s)` |
| `Single_Line_Predator_Fading(_Trans)` (2411, 2464) | R r | same, but the offset is advanced with AND (see below) |
| `BF_Predator_Ghost_Fading(_Trans)` (3706, 3813) | | `fade(ghost(pred ? d[off] : s))` |
| `Single_Line_Predator_Ghost_Fading(_Trans)` (2520, 2589) | U u | **`fade(0)`**: EAX is cleared before the fade loop, so every drawn pixel gets the same colour |

That is 16 `BF_*` + 19 `Single_Line_*` (+ the long copy) routines. In C++ they are all one template,
`Draw_Row<F>`, instantiated with flags that select each operation and each quirk.

### Predator step

`BFPartialPred` is 0x100 unless `SHAPE_PARTIAL` sets it to 0-255. For each drawn pixel:
`sum = BFPartialCount + BFPartialPred`; if bits 8-15 of `sum` are zero the pixel is not a predator pixel
(`BFPartialCount = sum`); otherwise those bits are cleared, the pixel is a predator pixel, `off` is the table
entry at `BFPredOffset`, and `BFPredOffset`'s low byte becomes `(low + 2) & 0x0E`. The start offset is
`(arg * 2) & 0x0E` (negative: `0xFFFFFF00 | ((-(arg*2)) & 0x0E)`, which selects the next-row table).
Predator reads (`d[off]`) are always to the right on the same row or on the next row, so they read
pixels this draw has not written yet; they can read past the viewport (as the original did).

## Original bugs that are kept (they change pixels)

1. **R/r advance the predator offset with `and` instead of `add`** (asm 2429, 2484): the offset never moves
   from 0 or 2, so the shimmer pattern is constant. The old-system `BF_Predator_Fading` uses `add`.
2. **U/u fade colour 0** instead of the pixel (asm 2561, 2633).
3. **Predator, partial pixels:** `Single_Line_Predator` and `BF_Predator` skip non-predator pixels, but
   `BF_Predator_Trans` draws the source colour. So with `SHAPE_PARTIAL` a shape looks different clipped and unclipped.
4. **Wide ghost index in Q/q** (asm 2319, 2379): the ghost lookup uses all of EAX. After a predator pixel bits
   8-15 still hold the offset's high byte, so with the next-row table and rows of 256 bytes or more it reads
   `is_translucent[0x100*k + c]`, i.e. into the `translucent` rows. Reproduced exactly.
5. **Fade count 0 is 2^32 lookups** (`dec ecx / jnz`). Computed exactly through the colour's cycle.
6. The jump table is patched in place on every call with `SHAPE_FADING` (entries 4 and 5 become
   Single_Fade or Fading by count). Kept as a mutable table; the result only depends on the current call.

None of 1-5 can happen with the arguments `CC_Draw_Shape` passes today (count 1, no `SHAPE_PARTIAL`,
fading+predator turned into ghost, negative predator offsets only off screen). They are kept so the port is exact
for any caller. (`CC_Draw_Shape` with `SHAPE_GHOST|SHAPE_PREDATOR` would pass only the predator offset, which the
renderer then reads as the ghost table pointer: a latent bug in the caller, not in the renderer.)

## Deliberate differences from the asm

| Asm behaviour | C++ | Why |
|---------------|-----|-----|
| Width or height `<= 0` with the line-header system: endless loop / 2^32 rows | draws nothing | crash in the original |
| Q/q at the start of a row: bits 8-31 of EAX hold a jump-table address, used in the ghost lookup of a non-predator pixel before the first predator pixel | treated as 0 | the address is meaningless; only reachable with `SHAPE_PARTIAL` + ghost + predator |
| `Single_Line_Predator_Ghost_Trans` ends with a stray `pop ecx` (asm 2399) that shifts the stack, so the caller's registers are restored wrongly | nothing | no effect on pixels |
| Return value: whatever was in EAX | 0 | no caller uses it |
| `UseOldShapeDraw` (a `bool`) read as a dword | read as `bool` | the asm also read 3 unrelated bytes |
| `ShapeBuffer` / `ShapeBufferSize` (aliases of `_ShapeBuffer` / `_ShapeBufferSize` in `SHAPE.ASM`) | not provided | C++ can't alias variables; only `WWMOUSE.ASM` used them |

## Notes for later sessions

* `GraphicViewPortClass::Get_Offset()` returns `long`. On a 64-bit build that truncates the pointer
  (S06/S08). The renderer itself takes `unsigned char *`.
* `ShapeFrameHeaderType` holds a `char *` used as an offset, so it is 16 bytes on 64-bit builds. That is fine as
  long as `2KEYFRAM.CPP` and `SHAPEDRW.H` agree; it only matters if the header is ever written to disk.
* Single-threaded: the predator state and the patched jump table are file-level statics, as in the asm.
