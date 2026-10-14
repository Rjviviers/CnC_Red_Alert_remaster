# Surface primitives (`WIN32LIB/DRAWBUFF`, `Clip_Rect`)

Written in session S07-A (2026-10-14), when these routines were ported from assembly to C++.
Inventory rows 10-12, 14, 16-21, 23, 24 and 44 in [assembly-inventory.md](assembly-inventory.md).

## What they are

The low-level drawing operations on 8-bit paletted surfaces. The game never calls them directly: it calls
`GraphicViewPortClass` methods (`Clear`, `Blit`, `Fill_Rect`, `Draw_Line`, `Remap`, `Scale`, `Draw_Stamp`,
`Get_Pixel`, `Put_Pixel`, `To_Buffer`), which are inline wrappers in `WIN32LIB/INCLUDE/GBUFFER.H`. Each wrapper
locks the surface and calls an `extern "C"` function, passing `this` as a `void *`. `Clip_Rect`/`Confine_Rect`
(`WIN32LIB/MISC`) are free functions used for the tactical view and the radar.

| Entry point | C++ file | Wrapper / caller | What it does |
|-------------|----------|------------------|--------------|
| `Buffer_Clear` | [`CLEAR.CPP`](../../WIN32LIB/DRAWBUFF/CLEAR.CPP) | `Clear` | Fill the whole viewport |
| `Buffer_Get_Pixel`, `Buffer_Put_Pixel` | [`GETPIX.CPP`](../../WIN32LIB/DRAWBUFF/GETPIX.CPP), [`PUTPIX.CPP`](../../WIN32LIB/DRAWBUFF/PUTPIX.CPP) | `Get_Pixel`, `Put_Pixel` | One pixel, bounds-checked |
| `Buffer_Fill_Rect` | [`FILLRECT.CPP`](../../WIN32LIB/DRAWBUFF/FILLRECT.CPP) | `Fill_Rect` | Clipped rectangle, inclusive corners |
| `Buffer_Draw_Line` | [`DRAWLINE.CPP`](../../WIN32LIB/DRAWBUFF/DRAWLINE.CPP) | `Draw_Line` | Clipped Bresenham line |
| `Buffer_Remap` | [`REMAP.CPP`](../../WIN32LIB/DRAWBUFF/REMAP.CPP) | `Remap` | Pass a rectangle through a 256-byte table |
| `Linear_Blit_To_Linear` | [`BITBLIT.CPP`](../../WIN32LIB/DRAWBUFF/BITBLIT.CPP) | `Blit` (non-DirectDraw path) | Viewport to viewport, optional colour-0 transparency |
| `Linear_Scale_To_Linear` | [`SCALE.CPP`](../../WIN32LIB/DRAWBUFF/SCALE.CPP) | `Scale` | Scaled copy, optional transparency and remap |
| `Buffer_To_Buffer` | [`TOBUFF.CPP`](../../WIN32LIB/DRAWBUFF/TOBUFF.CPP) | `To_Buffer` | Viewport rectangle to a linear buffer |
| `Buffer_To_Page` | [`TOPAGE.CPP`](../../WIN32LIB/DRAWBUFF/TOPAGE.CPP) | `Buffer_To_Page` (GBUFFER.H free function), `CODE/MSGBOX.CPP` | Linear buffer to a viewport rectangle |
| `Buffer_Draw_Stamp`, `Buffer_Draw_Stamp_Clip`, `Init_Stamps` | [`STAMP.CPP`](../../WIN32LIB/DRAWBUFF/STAMP.CPP) | `Draw_Stamp` | One terrain icon from an icon set |
| `Is_Icon_Cached`, `Cache_Copy_Icon` | [`STMPCACH.CPP`](../../WIN32LIB/DRAWBUFF/STMPCACH.CPP) | `Draw_Stamp`, `ICONCACH.CPP` | Removed video-memory icon cache; stand-ins only ([ADR-0016](../decisions/0016-drop-video-memory-icon-cache.md)) |
| `Clip_Rect`, `Confine_Rect` | [`CLIPRECT.CPP`](../../WIN32LIB/MISC/CLIPRECT.CPP) | `CODE/DISPLAY.CPP`, `CODE/RADAR.CPP` | Clip / confine a rectangle to a window |

Tests: `tests/s07a_*_test.cpp`, one per file (`ctest -R s07a`).

## How the C++ reads the viewport: `GVPORT.H`

The asm read the first eight fields of `GraphicViewPortClass` through `GBUFFER.INC`
(`Offset, Width, Height, XAdd, XPos, YPos, Pitch, GraphicBuff`, 4 bytes each). The ports read them through
[`WIN32LIB/DRAWBUFF/GVPORT.H`](../../WIN32LIB/DRAWBUFF/GVPORT.H), whose `GVPLayout` mirrors **all** data members
of the class with the same names, types and order, and `static_assert`s the `GBUFFER.INC` offsets on 32-bit builds.

`GBUFFER.H` is not included because it drags in `<ddraw.h>`, `<windows.h>` and DOS headers (via `WWSTD.H`), which
do not compile on GCC yet. `GVPORT.H` also declares the entry points with plain C types (`BOOL` → `int`,
`VOID` → `void`, `LONG` → `long`), compatible with the `DRAWBUFF.H` declarations.

Rules for the layout:
- The row stride every routine uses is `Width + XAdd + Pitch`. `Pitch` is the DirectDraw surface's extra pitch;
  S08 may keep it 0 for SDL surfaces, but the routines still add it.
- `Offset` is a `long` holding the address of the viewport's top-left pixel. That only works where `long` can
  hold a pointer (32-bit builds, ADR-0003; MSVC x64 would need a change in the class and in `GVPLayout`).
- If anyone changes the data members of `GraphicViewPortClass`, `GVPLayout` must change with them. Once
  `GBUFFER.H` compiles, add `static_assert(sizeof(GraphicViewPortClass) == sizeof(GVPLayout), "")` in a file
  that sees both (for example `WIN32LIB/DRAWBUFF/GBUFFER.CPP`).

## Behaviour kept from the asm (non-obvious)

All of these have unit tests.

- **Clipping** in `BITBLIT`, `REMAP`, `SCALE`, `TOBUFF`, `TOPAGE` and `Clip_Rect` uses Cohen-Sutherland
  outcodes where the right/bottom test is `x > width` (so `x == width` is "inside"); `GVP_Outcode` in `GVPORT.H`.
- **`Linear_Blit_To_Linear`**: clipping the *source* rectangle does not move the destination (with `x_pixel = -1`
  the image lands one pixel left of where an unclipped copy would put it). Clipping the destination keeps both
  aligned. Overlapping blits in one buffer: same address → nothing; source before destination → copied backwards
  (bottom row first, right to left), otherwise forwards. Without transparency this is memmove; with transparency
  the direction is visible in the result. Only bit 0 of `trans` counts.
- **`Buffer_Fill_Rect`**: coordinates are truncated to 16 bits (the asm declared them `WORD`); corners are
  inclusive and may come in any order.
- **`Buffer_Draw_Line`**: iterative clipping with truncating `imul`/`idiv`; lines are always drawn from the upper
  end; the error accumulator starts at major/2.
- **`Linear_Scale_To_Linear`**: 16.16 fixed-point x step with the fraction reset to 0 on every row; the row
  accumulator starts at `-dst_height`, so a 2x vertical upscale repeats the first source row three times and the
  last once. With transparency the zero test is on the source pixel *before* the remap.
- **`Buffer_Draw_Stamp`**: the icon number goes through the set's map, replacing only its low byte. With a remap
  table, transparency is tested *after* the remap and the set's per-icon transparency flag is ignored. The
  opaque fast path copies `Width/4` DWORDs per row and `Height/4` groups of 4 rows (exact for RA's 24x24 icons).
  `Buffer_Draw_Stamp_Clip` clips to the given window only, not to the viewport.
- **`Buffer_To_Buffer`**: the buffer row length is the requested `w`; pixels clipped off the left/top leave their
  buffer bytes untouched; the size check is `w * clipped_height > size` and ignores the left/top skip.
- **`Buffer_Get_Pixel`**: out of range returns the rejected coordinate (whatever the asm left in `EAX`).
- **`Confine_Rect`**: a position of exactly 0 reports "shifted" (1) although it does not move.
- **Return values** the asm left undefined (`Linear_Blit_To_Linear`, `Linear_Scale_To_Linear`, `Buffer_To_Buffer`,
  `Buffer_To_Page`) are 0 now. No caller reads them.

Deliberate differences: `Buffer_Clear` with `Height <= 0` and the stamp fast path with an icon height below 4 draw
nothing (the asm looped about 4 billion times), and `Linear_Scale_To_Linear` returns without drawing where the asm
would have raised a divide fault. Overlap direction is decided with an unsigned address compare (the asm's was
signed; it only differs for a buffer that straddles address `0x80000000`).

## For S08 (platform layer)

- The video-memory icon cache is gone ([ADR-0016](../decisions/0016-drop-video-memory-icon-cache.md)).
  `Is_Icon_Cached` always returns -1, so `GraphicViewPortClass::Draw_Stamp` (clip-window version) always takes
  the `Buffer_Draw_Stamp_Clip` path. S08 can delete `WIN32LIB/DRAWBUFF/ICONCACH.CPP`/`.H` (and its copy in
  `INCLUDE`), the cache branch of `Draw_Stamp` in `GBUFFER.H`, `IconCacheAllowed` (`BUFFGLBL.CPP`), the
  `Register_Icon_Set` call in `CODE/CDATA.CPP` and the `CachedIcons*` counters, then `STMPCACH.CPP`.
- The `Blit` and `Fill_Rect` wrappers still have DirectDraw-only branches (`DD_Linear_Blit_To_Linear`, hardware
  colour fill). With SDL surfaces `IsDirectDraw` should be false, which routes everything to these ports.
- `Lock()`/`Unlock()` in the wrappers can become no-ops for memory surfaces; the ports only need `Offset` to point
  at the pixels.
