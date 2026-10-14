# Assembly inventory

Written in session S02 (2026-10-14). Read-only audit; no source was changed.
Companion page: [libraries.md](libraries.md) (what the libraries are and how the game is linked).

> **Update from S05 (2026-10-14).** The delete list in section 2 was carried out in S05, except
> `WIN32LIB/PROFILE/APROFILE.ASM` (row 48, still pulled by `Stop_Profiler`), `WINVQ/VQM32/MCGABUF.ASM` (row 58,
> still referenced by `DRAWER.CPP`) and the dead parts of `CODE/WINASM.ASM` other than the modem stubs
> (`ModeX_Blit`, `Set_Palette_Register`). Row numbers still refer to the table below. See
> [removed-code.md](removed-code.md).

This page answers one question for S05 and S07: **which assembly files does the Win32 game really need,
and what does it take to replace each one?**

## Summary

| Count | What |
|------:|------|
| **357** | `.ASM` files in the repository (CODE 12, IPX 17, LAUNCH 1, TOOLS 1, VQ 19, WIN32LIB 166, WINVQ 23, WWFLAT32 118) |
| **291** | never take part in the Win32 link (DOS libraries, debug copies, `OLD/` folders, tools, files a makefile doesn't list) |
| **66** | take part in the Win32 link: 6 game objects from `CODE/` (+ `IPXREAL.ASM`, assembled into a binary blob that `IPXPROT.ASM` includes), 42 `win32lib.lib` members, 17 `vqa32wp.lib`/`vqm32wp.lib` members |
| **31** | of those 66 are not needed: 27 library members that are never pulled or are shadowed by game code, plus `IPXPROT.ASM`/`IPXREAL.ASM` (dead on Win32), `APROFILE.ASM` (profiler) and `MCGABUF.ASM` (dead at runtime) → **delete** |
| **7** | are needed but replaceable by a trivial line or two (`memmove`, a global variable, a no-op) |
| **3** | are needed for movies only (`UnVQ_4x2`, movie audio). ADR 0005 (proposed) drops movies → defer or delete |
| **25** | **really need porting**: 1 large (`CODE/2KEYFBUF.ASM`), 12 medium, 12 small. For 2 of them a C++ version already exists in `CODE/` (`LCW.CPP`, `ADPCM.CPP`) |
| **0** | assembly files on the deterministic simulation path. The simulation's maths uses C++ (`CODE/FACE.CPP`, `CODE/RANDOM.CPP`, `CODE/LCW.CPP`) and **inline `#pragma aux` asm in game headers** (see [section 5](#5-inline-assembly-pragma-aux-in-game-code)), which is S01's to classify but S07's to port |

About 8,150 lines of asm code (comments and blank lines excluded) need porting in the 25 files, plus
about 1,500 if movies stay.

### Suggested split for S07 (work packages that can run in parallel)

| WP | Theme | Files | asm lines | Size | Notes |
|----|-------|-------|----------:|------|-------|
| **S07-A** | Surface primitives | `DRAWBUFF/` `BITBLIT`, `CLEAR`, `DRAWLINE`, `FILLRECT`, `GETPIX`, `PUTPIX`, `REMAP`, `SCALE`, `STAMP`, `TOBUFF`, `TOPAGE`; `MISC/CLIPRECT` | 2,273 | 12 files, mostly S | All take a `GraphicViewPortClass*` and read it through `GBUFFER.INC` offsets: port them together, behind the existing `GBUFFER.H` inline wrappers. Remove the video-memory icon cache (`STMPCACH`) at the same time (with S08). **Done in S07-A**: all 12 ported and `STMPCACH` removed, see [surface-primitives.md](surface-primitives.md) |
| **S07-B** | Shape renderer | `CODE/2KEYFBUF.ASM`, `SHAPE/SHAPE.ASM` | 2,602 | 1 L + 1 S | The biggest single file: `Buffer_Frame_To_Page` with 36 line-drawing variants (ghost, fading, predator, translucent). Needs golden-image tests |
| **S07-C** | Text, palette, cursor, upscaler | `CODE/2TXTPRNT.ASM`, `FONT/SETFPAL`, `PALETTE/PAL`, `MISC/FADING`, `KEYBOARD/WWMOUSE`, `CODE/WINASM.ASM` (interpolation + data only) | 1,621 | 6 files, S/M | `WWMOUSE` can be dropped instead if S08 uses an OS cursor |
| **S07-D** | Data codecs | `IFF/LCWUNCMP` (adopt `CODE/LCW.CPP`), `CODE/LCWCOMP.ASM`, `WSA/XORDELTA`, `MEM/MEM_COPY`, `MISC/REVERSE`, `MISC/RANDOM` (keep only `RandNumb`) | 750 | 6 files | Pure functions: unit-test with round trips and golden vectors. Lossless formats, so output must be exact |
| **S07-E** | Audio decoders | `AUDIO/SOSCODEC` (adopt `CODE/ADPCM.CPP`), `AUDIO/OLSOSDEC`, `AUDIO/AUDUNCMP` | 1,100 | 3 files | Could move to S09. Test by decoding game `.AUD` files and comparing PCM |
| (S07-F) | Movies, only if kept | `VQA32/UNVQBUFF` (`UnVQ_4x2` only), `VQM32/AUDUNZAP`, `VQM32/SOSCODEC` | 1,521 | 3 files | Defer until ADR 0005 is decided |

Stubs and deletions (`CPUID`, `VMPAGEIN`, `APROFILE`, `IPXPROT`/`IPXREAL`, `MCGABUF`, the dead parts of
`WINASM`, and every "Delete" row below) are S05's job; nothing in S07 should wait for them.

## 1. Inventory: every assembly file in the Win32 link

**Status**
- **Used**: pulled into `ra95.exe` and executed.
- **Pulled, dead**: pulled because compiled code references it, but the code path never runs on Win32.
- **Not pulled**: in the library, but no compiled code references it.
- **Shadowed**: the same symbol is defined by game code, so the library copy never wins.
- **Linked, dead**: a game object file (always linked) whose functions are only called from DOS branches.

**Calling convention (Conv)**
- **C**: TASM `PROC name C near` with `ARG`: cdecl, arguments on the stack, caller pops, `_name` symbol.
  The game is compiled with `/5r`, so the C++ prototypes must say `__cdecl` (they do, e.g. `WIN32LIB/INCLUDE/DRAWBUFF.H`).
- **R**: Watcom register convention, symbol `name_`, arguments in `EAX`, `EDX`, `EBX`, `ECX`.
  The C++ side is a plain `extern "C"` prototype with no `__cdecl`.
- **D**: data exported to C++ (`extern "C"` variables).
- **this**: the routine receives a C++ object pointer and reads fields at fixed offsets (see [section 3](#3-calling-conventions-and-layout-traps)).

**Sim?** "No" means rendering, audio, UI or I/O only. "Data" means it decodes or encodes a lossless file
format: it must be exact, but it is not timing- or float-sensitive.

**Diff** = estimated port difficulty (S < 150 lines and straightforward, M = 150-600 lines or tricky,
L = large). **Prio**: P1 = needed for the first skirmish (S10/S11), P2 = needed for complete UI,
P3 = optional / movies / replace with a stub, X = delete. "Lines" = code lines without comments or blanks.

### 1.1 Game objects (`CODE/`)

| # | File | Exports (Conv) | Purpose | Example callers | Status | Sim? | Lines | Diff | Prio |
|---|------|----------------|---------|-----------------|--------|------|------:|------|------|
| 1 | `CODE/2KEYFBUF.ASM` | `Buffer_Frame_To_Page` (C) | Draws one shape frame into a viewport with clipping and the ghost / fading / predator / translucent / remap variants (`BF_*`, `Single_Line_*` internal routines and a jump table). Lines 3930-4848 are a dead copy of the DOS `KEYFBUFF.ASM` after the first `END` (line 3929); TASM ignores them | `CODE/CONQUER.CPP:3533` (in `CC_Draw_Shape`) | Used | No | 2,575 | L | P1 |
| 2 | `CODE/2TXTPRNT.ASM` | `Buffer_Print`, `Get_Font_Palette_Ptr` (C, this); `ColorXlat` (D) | Prints text with the current font. Shadows `WIN32LIB/DRAWBUFF/TXTPRNT.ASM` | `GraphicViewPortClass::Print` (`WIN32LIB/INCLUDE/GBUFFER.H:984`), e.g. `CODE/DIALOG.CPP:633` | Used | No | 276 | M | P1 |
| 3 | `CODE/LCWCOMP.ASM` | `LCW_Comp` (C) | LCW compressor | `CODE/LCWPIPE.CPP:200`, `CODE/LCWSTRAW.CPP:172` (map/overlay packs, compressed pipes) | Used | Data | 168 | M | P2 |
| 4 | `CODE/WINASM.ASM` | (a) `Asm_Interpolate`, `Asm_Interpolate_Line_Double`, `Asm_Interpolate_Line_Interpolate` (C); (b) `ModeX_Blit` (R); (c) `Set_Palette_Register` (R); (d) `Stop_Execution` (C), `Int3` (`_Int3`); (e) 15 empty modem stubs `FastGetPortHardware` … `HMSendString` (R); (f) `MouseQX`, `MouseQY`, `Memory_Error_Exit`, `_AbortModemFunctionPtr` (D) | Grab bag. (a) 2x upscaler with a palette interpolation table; (b) 320x200 blit; (c) VGA DAC port writes; (d) `int 3` debugger breaks; (e) link stubs, no bodies; (f) globals | (a) `CODE/INTERPAL.CPP:402-410`, reached from `CONQUER.CPP`, `MAPSEL.CPP`, `SCORE.CPP`; (b) `CODE/GSCREEN.CPP:479` when the screen is 320 wide; (c) only `CODE/WINSTUB.CPP:566` inside `Colour_Debug`, which nothing calls; (d) `CODE/KEY.CPP:607` (Scroll Lock); (e) only `#else // WIN32` branches of `CODE/NULLMGR.CPP`, e.g. `:400`; (f) `CODE/ENDING.CPP:126` | Used (a, d, f); dead (c, e); (b) probably dead, **unverified** | No | 548 | M | P2 for (a)+(f); X for the rest |
| 5 | `CODE/CPUID.ASM` | `Detect_MMX_Availability` (C); `CPUType`, `VendorID` (D) | CPUID-based CPU and MMX detection (MASM syntax) | `CODE/GETCPU.CPP:86`; `CPUType` goes into WOL statistics, `CODE/STATS.CPP:576` | Used | No | 64 | S | P3 (stub, or delete with WOL stats) |
| 6 | `CODE/IPXPROT.ASM` + `CODE/IPXREAL.ASM` | `Get_RM_IPX_Address`, `Get_RM_IPX_Size` (C) | Returns the address/size of a real-mode IPX callback blob (`IPXREAL.ASM`, assembled with `tlink`/`ebn` into `obj\win32\ipxreal.ibn`, `CODE/MAKEFILE:710-729`) | Only `CODE/IPXMGR.CPP:1864`, in the `#else` branch of `#ifdef WIN32` | Linked, dead (in `OBJECTS` unconditionally, `CODE/MAKEFILE:233`) | No | 20 + 121 | n/a | X |

### 1.2 `win32lib.lib` members (`WIN32LIB/`)

| # | File | Exports (Conv) | Purpose | Example callers | Status | Sim? | Lines | Diff | Prio |
|---|------|----------------|---------|-----------------|--------|------|------:|------|------|
| 7 | `AUDIO/SOSCODEC.ASM` | `sosCODECInitStream`, `sosCODECDecompressData`, `sosCODEC_Lock`, `sosCODEC_Unlock` (C) | HMI SOS IMA-ADPCM decoder, mono 16-bit. **C++ port exists**: `CODE/ADPCM.CPP` (not built) | `WIN32LIB/AUDIO/SOUNDINT.CPP:239`, `SOUNDIO.CPP:1704`, `SOUNDLCK.CPP:116` | Used | No | 446 | S | P1 (S09) |
| 8 | `AUDIO/OLSOSDEC.ASM` | `General_sosCODECDecompressData` (C); also defines `sosCODECInitStream`, `sosCODEC_Lock`, `sosCODEC_Unlock` again | ADPCM decoder for the other channel/bit-size combinations | `WIN32LIB/AUDIO/SOUNDINT.CPP:241` | Used. Which duplicate the librarian keeps is **unverified** | No | 437 | M | P1 (S09) |
| 9 | `AUDIO/AUDUNCMP.ASM` | `Decompress_Frame` (C) | Westwood ADPCM (`SCOMP_WESTWOOD`) frame decoder | `WIN32LIB/AUDIO/SOUNDINT.CPP:234` | Used | No | 217 | M | P1 (S09) |
| 10 | `DRAWBUFF/BITBLIT.ASM` | `Linear_Blit_To_Linear` (C, this) | Viewport-to-viewport copy, optional transparency, overlap-safe | `GBUFFER.H:776` (`Blit` when either side is not a DirectDraw surface), e.g. `CODE/CONQUER.CPP:2424` | Used → **Ported (S07-A)**: [`BITBLIT.CPP`](../../WIN32LIB/DRAWBUFF/BITBLIT.CPP), test [`s07a_bitblit_test.cpp`](../../tests/s07a_bitblit_test.cpp) | No | 332 | M | P1 |
| 11 | `DRAWBUFF/CLEAR.ASM` | `Buffer_Clear` (C, this) | Fill a viewport with one colour | `GBUFFER.H:679`; `CODE/CONQUER.CPP:432` | Used → **Ported (S07-A)**: [`CLEAR.CPP`](../../WIN32LIB/DRAWBUFF/CLEAR.CPP), test [`s07a_clear_test.cpp`](../../tests/s07a_clear_test.cpp) | No | 55 | S | P1 |
| 12 | `DRAWBUFF/DRAWLINE.ASM` | `Buffer_Draw_Line` (C, this) | Clipped Bresenham line | `GBUFFER.H:1145`; `CODE/BAR.CPP:155` | Used → **Ported (S07-A)**: [`DRAWLINE.CPP`](../../WIN32LIB/DRAWBUFF/DRAWLINE.CPP), test [`s07a_drawline_test.cpp`](../../tests/s07a_drawline_test.cpp) | No | 294 | M | P1 |
| 13 | `DRAWBUFF/FILLQUAD.ASM` | `Buffer_Fill_Quad` (C) | Convex quad fill | none (no `Fill_Quad` call in `CODE/`) | Not pulled | No | 423 | n/a | X |
| 14 | `DRAWBUFF/FILLRECT.ASM` | `Buffer_Fill_Rect` (C, this) | Clipped rectangle fill | `GBUFFER.H:1207`; `CODE/BAR.CPP:216` | Used → **Ported (S07-A)**: [`FILLRECT.CPP`](../../WIN32LIB/DRAWBUFF/FILLRECT.CPP), test [`s07a_fillrect_test.cpp`](../../tests/s07a_fillrect_test.cpp) | No | 137 | S | P1 |
| 15 | `DRAWBUFF/GETCLIP.ASM` | `get_clip` (C) | Clip helper | none | Not pulled | No | 49 | n/a | X |
| 16 | `DRAWBUFF/GETPIX.ASM` | `Buffer_Get_Pixel` (C, this) | Read a pixel | `GBUFFER.H:657`; `CODE/INIT.CPP:2569` | Used → **Ported (S07-A)**: [`GETPIX.CPP`](../../WIN32LIB/DRAWBUFF/GETPIX.CPP), test [`s07a_getpix_test.cpp`](../../tests/s07a_getpix_test.cpp) | No | 33 | S | P1 |
| 17 | `DRAWBUFF/PUTPIX.ASM` | `Buffer_Put_Pixel` (C, this) | Write a pixel | `GBUFFER.H:633`; `CODE/DIALOG.CPP:232` | Used → **Ported (S07-A)**: [`PUTPIX.CPP`](../../WIN32LIB/DRAWBUFF/PUTPIX.CPP), test [`s07a_putpix_test.cpp`](../../tests/s07a_putpix_test.cpp) | No | 34 | S | P1 |
| 18 | `DRAWBUFF/REMAP.ASM` | `Buffer_Remap` (C, this) | Remap a rectangle through a 256-byte table | `GBUFFER.H:1229`; `CODE/CELL.CPP:1029` | Used → **Ported (S07-A)**: [`REMAP.CPP`](../../WIN32LIB/DRAWBUFF/REMAP.CPP), test [`s07a_remap_test.cpp`](../../tests/s07a_remap_test.cpp) | No | 107 | S | P1 |
| 19 | `DRAWBUFF/SCALE.ASM` | `Linear_Scale_To_Linear` (C, this) | Scaled blit with transparency and remap | `GBUFFER.H:885`; `CODE/RADAR.CPP:699`, `CODE/SCORE.CPP:304` | Used → **Ported (S07-A)**: [`SCALE.CPP`](../../WIN32LIB/DRAWBUFF/SCALE.CPP), test [`s07a_scale_test.cpp`](../../tests/s07a_scale_test.cpp) | No | 430 | M | P2 |
| 20 | `DRAWBUFF/STAMP.ASM` | `Buffer_Draw_Stamp`, `Buffer_Draw_Stamp_Clip` (C, this); `Init_Stamps` (C, unused: call commented out in `WIN32LIB/TILE/ICONSET.CPP:221`); `IconCount`, `IconSize`, `LastIconset`, `MapPtr`, `StampPtr` (D) | Draws one terrain icon (template tile), with remap and clipping | `GBUFFER.H:1073,1123`; `CODE/CELL.CPP:1027` | Used → **Ported (S07-A)**: [`STAMP.CPP`](../../WIN32LIB/DRAWBUFF/STAMP.CPP), test [`s07a_stamp_test.cpp`](../../tests/s07a_stamp_test.cpp) | No | 351 | M | P1 |
| 21 | `DRAWBUFF/STMPCACH.ASM` | `Is_Icon_Cached`, `Cache_Copy_Icon` (R); `Is_Stamp_Registered` (R, unused); `IconData`, `IconEntry` (D) | Lookup/copy for the DirectDraw video-memory icon cache | `GBUFFER.H:1102`; `WIN32LIB/DRAWBUFF/ICONCACH.CPP:474`; the game registers sets at `CODE/CDATA.CPP:3137` | Used → **Removed (S07-A)**: video-memory icon cache dropped ([ADR-0016](../decisions/0016-drop-video-memory-icon-cache.md)); stand-in [`STMPCACH.CPP`](../../WIN32LIB/DRAWBUFF/STMPCACH.CPP) (`Is_Icon_Cached` always -1, `Cache_Copy_Icon` ported) until S08 deletes `ICONCACH`; test [`s07a_stmpcach_test.cpp`](../../tests/s07a_stmpcach_test.cpp) | No | 125 | S | P3 (remove the cache with S08) |
| 22 | `DRAWBUFF/SZREGION.ASM` | `Buffer_Size_Of_Region` (C) | w*h | Replaced by `#define Size_Of_Region(a, b) ((a)*(b))`, `CODE/COMPAT.H:51` | Shadowed | No | 28 | n/a | X |
| 23 | `DRAWBUFF/TOBUFF.ASM` | `Buffer_To_Buffer` (C, this) | Viewport to linear buffer | `GBUFFER.H:701`; `CODE/STATBTN.CPP:224` | Used → **Ported (S07-A)**: [`TOBUFF.CPP`](../../WIN32LIB/DRAWBUFF/TOBUFF.CPP), test [`s07a_tobuff_test.cpp`](../../tests/s07a_tobuff_test.cpp) | No | 183 | S | P1 |
| 24 | `DRAWBUFF/TOPAGE.ASM` | `Buffer_To_Page` (C, this) | Linear buffer to viewport | `CODE/MSGBOX.CPP:455` (direct), `GBUFFER.H:1288` | Used → **Ported (S07-A)**: [`TOPAGE.CPP`](../../WIN32LIB/DRAWBUFF/TOPAGE.CPP), test [`s07a_topage_test.cpp`](../../tests/s07a_topage_test.cpp) | No | 179 | S | P1 |
| 25 | `DRAWBUFF/TXTPRNT.ASM` | `Buffer_Print`, `Get_Font_Palette_Ptr` (C); `ColorXlat` (D) | Older text printer | Same symbols defined by `CODE/2TXTPRNT.ASM` | Shadowed | No | 273 | n/a | X |
| 26 | `FONT/SETFPAL.ASM` | `Set_Font_Palette_Range` (C) | Copies a font colour table into `ColorXlat` | `Set_Font_Palette` macro (`CODE/COMPAT.H:56`); `CODE/DIALOG.CPP:613` | Used | No | 36 | S | P1 |
| 27 | `FONT/TEXTPRNT.ASM` | `Text_Print` (C) | Old free-function text printer | Only `WINVQ/VQA32/CAPTION.CPP:249` (compiled out: `VQACAPTIONS_ON 0`) and a comment in `WW_WIN/WINDOWS.CPP:148` | Not pulled | No | 238 | n/a | X |
| 28 | `IFF/LCWCOMP.ASM` | `LCW_Compress` (C) | LCW compressor (library copy) | none | Not pulled | Data | 168 | n/a | X |
| 29 | `IFF/LCWUNCMP.ASM` | `LCW_Uncompress` (C) | LCW decompressor. **C++ exists**: `CODE/LCW.CPP:72` (`LCW_Uncomp`, already used by the game's pipes) and `CODE/LCWUNCMP.CPP:73` (`LCW_Uncompress`, not built) | `CODE/2KEYFRAM.CPP:330`; also `WINVQ/VQA32/LOADER.CPP` | Used | Data | 162 | S | P1 |
| 30 | `IFF/PACK2PLN.ASM` | `Pack_2_Plane` (C) | Chunky to planar for LBM writing | Only `WIN32LIB/IFF/WRITELBM.CPP:255`; the game never calls `Write_LBM_File` | Not pulled | No | 36 | n/a | X |
| 31 | `KEYBOARD/WWMOUSE.ASM` | `Draw_Mouse`, `Mouse_Shadow_Buffer`, `ASM_Set_Mouse_Cursor` (C, this) | Software mouse cursor: save/restore background, draw cursor shape | `WIN32LIB/KEYBOARD/MOUSE.CPP:274,296`; game: `CODE/GSCREEN.CPP:475` `WWMouse->Draw_Mouse` | Used | No | 416 | M | P2 |
| 32 | `MEM/MEM_COPY.ASM` | `Mem_Copy`, `Largest_Mem_Block` (C) | Overlap-safe copy (memmove); largest free block | `CODE/MAP.CPP:1292`, 18 game lines in total; `Largest_Mem_Block` only from `CODE/ALLOC.CPP:212`, which is not built | Used | No (it is `memmove`) | 87 | S | P1 (→ `memmove`) |
| 33 | `MEM/VMPAGEIN.ASM` | `Force_VM_Page_In` (C) | Touches each page of a buffer (Win95 paging hint) | `CODE/EGOS.CPP:892` | Used | No | 23 | S | P3 (→ no-op) |
| 34 | `MISC/CRC.ASM` | `Calculate_CRC` (C) | Checksum | Defined in C++ by `CODE/INIT.CPP:2376` (`extern "C"`) | Shadowed | n/a | 51 | n/a | X |
| 35 | `MISC/DETPROC.ASM` | `Processor` (C) | CPU type | Only `CODE/INIT.CPP:204`, under `CHEAT_KEYS`, which `CODE/DEFINES.H:157` defines only for `INTERNAL_VERSION`/`PLAYTEST_VERSION` | Not pulled (release) | No | 44 | n/a | X |
| 36 | `MISC/FACING8.ASM` | `Desired_Facing8` (C) | 8-way facing | Prototype disabled with `#if (0)` in `WIN32LIB/INCLUDE/MISC.H`; game uses C++ `CODE/FACE.CPP:65` | Shadowed | **Do not port**: the C++ is the simulation authority | 45 | n/a | X |
| 37 | `MISC/FACING16.ASM` | `Desired_Facing16` (C) | 16-way facing | none | Not pulled | n/a | 54 | n/a | X |
| 38 | `MISC/FACINGFF.ASM` | `Desired_Facing256` (C) | 256-way facing | As row 36; C++ in `CODE/FACE.CPP:150` | Shadowed | **Do not port** | 66 | n/a | X |
| 39 | `MISC/FADING.ASM` | `Build_Fading_Table` (C) | Builds a 256-byte remap table that fades each colour toward a target (nearest-colour search) | `CODE/DISPLAY.CPP:408`, `CODE/JSHELL.CPP:374` | Used | No (tables only affect drawing) | 123 | M | P1 |
| 40 | `MISC/OPSYS.ASM` | `Operating_System` (C); `OperatingSystem` (D) | DOS/Windows detection | none | Not pulled | No | 65 | n/a | X |
| 41 | `MISC/RANDOM.ASM` | `Random`, `Get_Random_Mask` (C); `RandNumb` (D) | Old library RNG | `Random()` and `IRandom()` are never called; only the seed `RandNumb` is written, `CODE/INIT.CPP:2451,2462,2491`. The game's RNG is `CODE/RANDOM.CPP` | Used (data only) | No | 39 | S | P2 (→ a C++ `long RandNumb`, or delete the writes) |
| 42 | `MISC/REVERSE.ASM` | `Reverse_Long`, `Reverse_Short`, `Swap_Long` (C) | Byte swapping for IFF | `WIN32LIB/IFF/IFF.CPP:163`. Pulled through `CODE/MAP.CPP:1292` → `Get_Icon_Set_Map` (`TILE/ICONSET.CPP:335`, same object as `Load_Icon_Set`) → `Open_Iff_File`/`Read_Iff_Chunk` → `Reverse_Long` | Pulled; probably dead (the game never calls `Load_Icon_Set`, **unverified** at runtime) | No | 41 | S | P3 |
| 43 | `MISC/SHAKESCR.ASM` | `Shake_Screen` (C) | Screen shake via VGA registers | Only `CODE/CONQUER.CPP:5379`, in the `#else` branch of `#ifdef WIN32` | Not pulled | No | 80 | n/a | X |
| 44 | `MISC/CLIPRECT.ASM` | `Clip_Rect`, `Confine_Rect` (C) | Clip/confine a rectangle to bounds | `CODE/DISPLAY.CPP:2232`, `:573`; `CODE/RADAR.CPP:1960` | Used → **Ported (S07-A)**: [`CLIPRECT.CPP`](../../WIN32LIB/MISC/CLIPRECT.CPP), test [`s07a_cliprect_test.cpp`](../../tests/s07a_cliprect_test.cpp) | No (view position, per client) | 138 | S | P1 |
| 45 | `PALETTE/PAL.ASM` | `Set_Palette_Range`, `Bump_Color` (C); `CurrentPalette`, `PaletteTable` (D) | On Win95 (`IF 1` branch) copies 768 bytes into `CurrentPalette` and calls `Set_DD_Palette_` (R, `WIN32LIB/MISC/DDRAW.CPP`); the VGA port code is under `ifdef NOT_FOR_WIN95` | `WIN32LIB/PALETTE/PALETTE.CPP:91` (`Set_Palette`) ← `CODE/WINSTUB.CPP:705` and others | Used | No | 222 | S | P1 |
| 46 | `SHAPE/SHAPE.ASM` | `Set_Shape_Buffer` (C); `ShapeBuffer`, `ShapeBufferSize`, `_ShapeBuffer`, `_ShapeBufferSize` (D) | Stores the shape scratch buffer | `CODE/INIT.CPP:3184` | Used | No | 27 | S | P1 (→ C++ globals) |
| 47 | `WSA/XORDELTA.ASM` | `Apply_XOR_Delta`, `Apply_XOR_Delta_To_Page_Or_Viewport` (C) | XOR-delta frame decoder (keyframe shapes, WSA animations) | `CODE/2KEYFRAM.CPP:47` (`Apply_Delta` macro); `WIN32LIB/WSA/WSA.CPP` via `Animate_Frame` from `CODE/MAPSEL.CPP:166`, `CODE/SCORE.CPP:455` | Used | Data | 253 | M | P1 |
| 48 | `PROFILE/APROFILE.ASM` | `__PRO`, `__EPI`, `Profile_Init`, `Profile_End` (C/none); `New_Profiler_Callback`, `Old_Profiler_Callback` (R); 6 data | Sampling-profiler hooks | Pulled because `CODE/WINSTUB.CPP:236` calls `Stop_Profiler` (`WIN32LIB/PROFILE/WPROFILE.CPP:105-146`); `__PRO` is also defined at `CODE/INIT.CPP:3528` | Pulled (developer tool) | No | 104 | n/a | X |

### 1.3 `vqa32wp.lib` and `vqm32wp.lib` members (`WINVQ/`)

| # | File | Exports (Conv) | Purpose | Example callers | Status | Sim? | Lines | Diff | Prio |
|---|------|----------------|---------|-----------------|--------|------|------:|------|------|
| 49 | `VQA32/UNVQBUFF.ASM` | `UnVQ_4x2` (used); `UnVQ_2x2`, `UnVQ_2x3`, `UnVQ_4x4`, `UnVQ_4x2_Woofer` (compiled out by `VQAPLAY.H`) (C) | Vector-quantised block decode into a buffer | `WINVQ/VQA32/DRAWER.CPP:466` | Used (movies) | No | 549 | M | P3 |
| 50 | `VQA32/UNVQVESA.ASM` | `UnVQ_4x2_VESA320_32K` (C) | VESA direct decode | Only `DRAWER.CPP:548`, under `VQAVESA_ON` (0) | Not pulled | No | 199 | n/a | X |
| 51 | `VQM32/AUDUNZAP.ASM` | `AudioUnzap` (C) | Movie audio (Westwood ADPCM) | `WINVQ/VQA32/LOADER.CPP:2705` | Used (movies) | No | 206 | M | P3 |
| 52 | `VQM32/CRC.ASM` | `Calculate_CRC` (C) | Checksum | Shadowed by `CODE/INIT.CPP:2376` | Shadowed | n/a | 47 | n/a | X |
| 53 | `VQM32/DRAWCHAR.ASM` | `Draw_Char`, `Draw_Char_Setup`, `Set_Font_Palette_Range` (C) | Caption text | `CAPTION.CPP:238` (compiled out); `Draw_Char_Setup` call commented out in `WIN32LIB/FONT/SET_FONT.CPP:80` | Not pulled | No | 232 | n/a | X |
| 54 | `VQM32/FILLRECT.ASM` | `Fill_Rect`, `Eor_Region` (C) | Free-function rect fill | none (the game's `Fill_Rect` calls are `GraphicViewPortClass` methods) | Not pulled | No | 90 | n/a | X |
| 55 | `VQM32/HUFFDCMP.ASM` | `HuffDecompress`, `BuildHuffTree` (C) | Huffman decoder | none | Not pulled | No | 194 | n/a | X |
| 56 | `VQM32/LCWCOMP.ASM` | `LCW_Compress` (C) | LCW compressor (copy) | none | Not pulled | n/a | 164 | n/a | X |
| 57 | `VQM32/LCWUNCMP.ASM` | `LCW_Uncompress` (C) | LCW decompressor (copy) | Same symbol in `win32lib.lib`, which comes first on the link line (**unverified**: `wlink` library order) | Not pulled | n/a | 110 | n/a | X |
| 58 | `VQM32/MCGABUF.ASM` | `MCGA_Blit`, `MCGA_BufferCopy`, `ClearVRAM` (C) | Copies to VGA memory at `0xA0000` | `WINVQ/VQA32/DRAWER.CPP:1689` is compiled (`VQAMCGA_ON 1`) but only runs for direct-to-screen MCGA; the game sets `VQACFGF_BUFFER` (`CODE/INIT.CPP:1660`) | Pulled, dead | No | 65 | n/a | X |
| 59 | `VQM32/MONO.ASM` | `Mono_*` (13 symbols) (C) | Mono display | Game uses C++ `CODE/MONOC.CPP` and `WIN32LIB/MONO/MONO.CPP` | Shadowed | No | 326 | n/a | X |
| 60 | `VQM32/PALETTE.ASM` | `SetPalette`, `ReadPalette`, `SetDAC`, `TranslatePalette` (C) | VGA DAC writes | `SetPalette` defined in C++ at `CODE/WINSTUB.CPP:695`; the rest only under `VQACAPTIONS_ON`/`VQAVESA_ON` (0) | Shadowed | No | 117 | n/a | X |
| 61 | `VQM32/SOSCODEC.ASM` | `VQA_sosCODECInitStream`, `VQA_sosCODECDecompressData`, `VQA_sosCODECCompressData` (C) | Movie audio IMA-ADPCM (same algorithm as row 7) | `WINVQ/VQA32/LOADER.CPP:502,2823` | Used (movies) | No | 766 | S (reuse the row 7 port) | P3 |
| 62 | `VQM32/TEXTPRNT.ASM` | `Text_Print` (C) | Caption text | Compiled out | Not pulled | No | 82 | n/a | X |
| 63 | `VQM32/VB.ASM` | `WaitVB`, `WaitNoVB` (C) | VGA vertical retrace | Only under `VQAXMODE_ON`/`VQAVESA_ON` (0), e.g. `DRAWER.CPP:1214` | Not pulled | No | 37 | n/a | X |
| 64 | `VQM32/VESABUF.ASM` | `Buf_320x200_To_VESA_*`, `Copy_Row`, `Copy_Word_Row` (C) | VESA blits | Only under `VQAVESA_ON` (0) | Not pulled | No | 286 | n/a | X |
| 65 | `VQM32/XMODE.ASM` | `SetXMode`, `ClearXMode`, `ShowXPage`, `Xmode_Blit`, `Xmode_BufferCopy_320x200` (C) | Mode X | Only under `VQAXMODE_ON` (0), e.g. `DRAWER.CPP:994` | Not pulled | No | 411 | n/a | X |

## 2. Delete list

Input for S05. "Delete" means nothing in the Win32 game needs the file; its removal only requires
dropping it from makefiles (or no change at all, if no makefile lists it).

1. **In the Win32 link but not needed (31):** rows 6 (`IPXPROT`, `IPXREAL`), 13, 15, 22, 25, 27, 28, 30, 34-38,
   40, 43, 48, 50, 52-60, 62-65, and the dead parts of row 4 (`ModeX_Blit` **after** S08 confirms 320-wide
   mode is gone, `Set_Palette_Register`, the modem stubs, `_AbortModemFunctionPtr`).
2. **`CODE/` asm files not in the Win32 object list (5):**
   `CODE/KEYFBUFF.ASM` and `CODE/TXTPRNT.ASM` (DOS-only objects, `CODE/MAKEFILE:422-424`);
   `CODE/2SUPPORT.ASM` and `CODE/SUPPORT.ASM` (not in any object list; their `Conquer_Build_Fading_Table`
   and `strtrim` are C++ in `CODE/JSHELL.CPP:467` and `CODE/READLINE.CPP:31`);
   `CODE/COORDA.ASM` (not in any object list; `Fixed_To_Cardinal`/`Cardinal_To_Fixed` are inline asm in
   `CODE/JSHELL.H:292-315`).
3. **Whole folders and files with no Win32 role (286 asm files):** `WWFLAT32/` (118), `VQ/` (19), `IPX/` (17),
   `LAUNCH/` (1), `TOOLS/AUDIOMAK/SCODE.ASM` (1), `WIN32LIB/SRCDEBUG/` (93), the non-member asm in
   `WIN32LIB/` (`DRAWBUFF/FTPUTPIX`, `DRAWBUFF/SHADOW`, `PLAYCD/PLAYCD`, `SHAPE/DRAWSHP`, `SHAPE/DS_*` x13,
   `SHAPE/SETSHAPE`, and everything under `OLD/`, `TEST/`, `MSVC/`, `OLDMEM/`: 31 files),
   and in `WINVQ/` (`VQA32/OLD/*` x3, `VQA32/UNVQXMDE`, `VQAVIEW/INTERPAL`, `VQM32/PORTIO`: 6 files).

Check: 31 + 5 + 286 = 322 files to delete, plus 25 to port, 7 to stub and 3 movie-only = 357,
the number of asm files in the repository.

## 3. Calling conventions and layout traps

Things that will bite a straight transliteration to C++:

- **Two conventions in one program.** The game is compiled `/5r` (Watcom register: first four integer
  arguments in `EAX`, `EDX`, `EBX`, `ECX`), the asm is mostly cdecl. Every cdecl asm entry point is
  declared `__cdecl` in a library header; every register-convention one (symbol ends in `_`) is a plain
  `extern "C"` prototype. When an entry point becomes C++, delete the special prototype rather than copy it.
- **Asm that calls back into C++ with the register convention:** `PAL.ASM` calls `Set_DD_Palette_`
  (`EAX` = palette); `APROFILE.ASM` callbacks; `CODE/WINASM.ASM` modem stubs. Grep for `GLOBAL .*_:near`
  in a file before porting it.
- **Struct offsets hard-coded in asm.** `DRAWBUFF/*.ASM` read `GraphicViewPortClass` fields through
  `WIN32LIB/DRAWBUFF/GBUFFER.INC` (`GBUFFER.INC` mirrors the C++ class layout under `/zp1`); `WWMOUSE.ASM`
  reads `WWMouseClass` through `KEYBOARD/MOUSE.INC`; `2KEYFBUF.ASM` and `2TXTPRNT.ASM` read font and shape
  headers. Until a file is ported, **no one may change the layout or packing** of those classes. S06
  (compiler compatibility, packing) and S08 (surfaces) must coordinate with S07-A.
- **`this` passed explicitly.** `Buffer_Clear(void *thisptr, …)` style: the C++ wrapper in `GBUFFER.H`
  passes `this`. Ported functions can become member functions.
- **Data symbols shared both ways.** `ColorXlat`, `FontPtr`, `FontXSpacing`, `FontYSpacing`, `CurrentPalette`,
  `ShapeBuffer`, `MouseQX`/`MouseQY`, `RandNumb`: define them once in C++ with `extern "C"` linkage while
  any asm still uses them.
- **Duplicate definitions resolved by link order.** `option redefsok` hides duplicate symbols
  (`sosCODECInitStream` in two members of the same library, `__PRO` in a game object and a library).
  A modern linker will report them; delete the losing copy rather than renaming.
- **Syntax.** Almost everything is TASM IDEAL mode (`IDEAL`, `P386`, `MODEL USE32 FLAT`).
  `CODE/CPUID.ASM`, part of `CODE/WINASM.ASM` (`masm` block), `LAUNCH/LAUNCH.ASM` and the `IPX/` files are MASM.
  No assembler is planned for the port (ADR 0003, proposed), so nothing is "kept as asm".

## 4. Simulation (lockstep) relevance

- **No linked `.ASM` file is on the simulation path.** Everything listed above is drawing, text, palette,
  mouse, audio, file decoding or debug.
- The functions that *look* simulation-related in the libraries are not used by the game:
  `Desired_Facing8/16/256` (asm disabled, C++ in `CODE/FACE.CPP`), `Random`/`IRandom` (C++ `RandomClass` in
  `CODE/RANDOM.CPP` plus `Sim_Random_Pick`, `CODE/INLINE.H:1006`), `Calculate_CRC` (C++ in `CODE/INIT.CPP:2376`).
  **Porting the asm versions "for completeness" would be harmful**: they must not replace the C++ ones.
- Lossless decoders (`LCW_Uncompress`, `Apply_XOR_Delta`) feed data the game loads, so they must be exact.
  The simulation's own map data (`[MapPack]`, `[OverlayPack]`) is decoded by the C++ `LCW_Uncomp`
  (`CODE/LCW.CPP:72`, via `CODE/LCWSTRAW.CPP:167`), not by asm.
- `Mem_Copy` (row 32) is used on a map-loading path (`CODE/MAP.CPP:1292`); it is `memmove`, nothing more.

## 5. Inline assembly (`#pragma aux`) in game code

Not asm files, so not in the table above, and S01 owns `CODE/`. Listed here because S07 will port them
and **some are on the simulation path**:

| Where | Functions | Simulation? |
|-------|-----------|-------------|
| `CODE/JSHELL.H:211-315` | `Set_Bit`, `Get_Bit`, `First_True_Bit`, `First_False_Bit`, `Bound` (x2), `Fixed_To_Cardinal`, `Cardinal_To_Fixed`, `outportb`, `outport` | **Yes** for `Fixed_To_Cardinal`/`Cardinal_To_Fixed` (used in `CODE/TECHNO.CPP`, `UNIT.CPP`, `INFANTRY.CPP`, `VESSEL.CPP`, `FLY.CPP`, …). Note `Fixed_To_Cardinal` does `mul edx`, rounds with `+0x80`, **saturates** at `0xFFFFFF` before `>> 8`; `Cardinal_To_Fixed` returns the unchanged `EAX` when `base` is 0. Bit helpers: **unverified** where used |
| `CODE/COORD.CPP:406-430` | `calcx`, `calcy` (16-bit `imul` and shifts) | **Yes**: used by `Move_Point`, which moves units |
| `CODE/MPU.H:62` | `Get_CPU_Clock` (`rdtsc`) | No (timing/benchmarks) |
| `CODE/RGB.H:47-59`, `CODE/MONOC.CPP:77`, `WIN32LIB/MONO/MONO.CPP:61` | port I/O (`outportb`, `outport`, `outrgb`, `output`) | No (VGA/mono hardware; delete) |
| `CODE/EGOS.CPP:155` | `Vsync` | No |
| `CODE/DPMI.CPP` (4) | DPMI calls | No (DOS) |
| `WIN32LIB/INCLUDE/SOS.H:536`, `WINVQ/INCLUDE/VQA32/SOS.H` | `int_3` | No |

## Method (re-runnable)

All commands run from the repository root with only `python3`, `grep`, `find`, `cmp` and `sed`.
The scripts are reproduced in full in the session log
[`docs/logs/2026-10-14-S02-audit-libraries.md`](../logs/2026-10-14-S02-audit-libraries.md#scripts).

1. **Count asm files:** `find . -path ./.git -prune -o -type f -iname '*.asm' -print | cut -d/ -f2 | sort | uniq -c`
2. **Game objects in the Win32 link:** read `CODE/MAKEFILE` (`OBJECTS`, the `!ifdef WIN32` additions at
   lines 401-419, `win95.lnk` at 544-570); `grep -n -i '\bNAME\.OBJ' CODE/MAKEFILE` for each `CODE/*.ASM`.
3. **Library members:** `members.py` parses the `OBJECTS` macro of each sub-library makefile and maps each
   `.obj` to its `.CPP`/`.ASM` source (`WIN32LIB/<18 subdirs>/MAKEFILE`, `WINVQ/VQA32/MAKEFILE`,
   `WINVQ/VQM32/MAKEFILE`).
4. **Exports and first references:** `asmsyms.py` reads each asm file plus the files it `INCLUDE`s, keeps
   names declared `GLOBAL`/`PUBLIC` that the file defines (`PROC`, label or data), works out the convention
   (`C` language → cdecl; trailing `_` → Watcom register), and greps `CODE/` and the libraries for the
   C-visible name.
5. **Manual tracing** (the part a script can't do): for every symbol, check whether the reference is a
   comment, a different C++ function with the same name (e.g. `MonoClass::Text_Print`, `GraphicViewPortClass::Fill_Rect`),
   inside a disabled `#if`, or reached only through a library wrapper (`GBUFFER.H` inline methods,
   `MOUSE.CPP`, `SOUNDINT.CPP`, `WSA.CPP`). `ifctx.py FILE LINE…` prints the `#if` stack around a line.
6. **Lines of code:** `sed 's/;.*//' FILE | grep -c -v '^\s*$'` (for `2KEYFBUF.ASM`, only lines 1-3929).
7. **SRCDEBUG copies:** `cmp` each `WIN32LIB/SRCDEBUG/*.ASM` with the same name elsewhere in `WIN32LIB/`.

Limits: no Watcom/TASM toolchain was run, so "pulled" is inferred from source references and the
documented `wlink` rules, not from a link map. A link map from S03's first build of the old toolchain
(if it is ever built) or from S10 would confirm it.
