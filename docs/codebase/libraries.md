# Support libraries (`WIN32LIB/`, `WINVQ/` and the other non-game directories)

Written in session S02 (2026-10-14). Read-only audit; no source was changed.
Companion page: [assembly-inventory.md](assembly-inventory.md) (one row per assembly file).

> **Update from S05 (2026-10-14).** `WWFLAT32/`, `VQ/`, `IPX/`, `LAUNCH/`, `LAUNCHER/`, the `TOOLS/` binaries,
> `WIN32LIB/SRCDEBUG/`, `EXAMPLE/`, `MOVIE/`, `WINCOMM/`, the `OLD/`/`TEST/` folders, `WINVQ/VPLAY32/`, `WINVQ/VQAVIEW/`
> and the prebuilt HMI SOS libraries were deleted in S05. `PROFILE`, `MONO` and `PLAYCD` were kept for now
> (outside the S05 brief; `PROFILE` and `PLAYCD` are still referenced by compiled game code). See [removed-code.md](removed-code.md).

**Terms used on this page**

- **Member**: an object file that the library makefile puts into a `.lib`.
- **Pulled**: the linker takes a library member into `ra95.exe` only when compiled game code
  references a symbol it defines. A member can be in the library and still never be pulled.
- **Shadowed**: the same symbol is also defined by a game object file (or by an earlier library), so
  the library copy is never pulled.
- **Dead at runtime**: pulled because compiled code references it, but that code path is never
  taken in the Win32 build.

Everything here is grounded in the source; the method is at the end of
[assembly-inventory.md](assembly-inventory.md#method-re-runnable). Anything not checked is marked **unverified**.

## 1. How the Win32 game is linked

`CODE/MAKEFILE` with `WIN32` defined builds `ra95.exe` through the generated `win95.lnk`
(`CODE/MAKEFILE` lines 544-570). In order, the linker gets:

| # | Input | Built from | In repo? | Notes |
|---|-------|-----------|----------|-------|
| 1 | All `$(OBJECTS)` | `CODE/*.CPP`, `CODE/*.ASM` | yes | Object files are always linked. Asm objects: `2KEYFBUF`, `CPUID`, `WINASM`, `2TXTPRNT`, `LCWCOMP`, `IPXPROT` (see the inventory) |
| 2 | `jshell.lib` | `$(LIBFILES)` (empty list) | n/a | Empty library |
| 3 | `tech.lib` | `SENDFILE.OBJ` | yes | |
| 4 | `win32lib.lib` | `WIN32LIB/MAKEFILE` (`$(WWFLAT)` is set to `..\win32lib` for Win32, line 56) | source only | See section 2 |
| 5 | `vqa32wp.lib` | `WINVQ/VQA32/MAKEFILE` | source only | VQA movie player |
| 6 | `vqm32wp.lib` | `WINVQ/VQM32/MAKEFILE` | source only | VQA "misc" helpers |
| 7 | `ipx\wwipx32.lib` | `IPX/MAKETH32.BAT` (`wlib -c -n wwipx32.lib +thipx32.dll`). The link path is relative to `CODE/`, so the built library was presumably copied to `CODE\ipx\` (**unverified**) | source only | Import library for the `THIPX32.DLL` Win95 thunk DLL. `CODE/IPX95.CPP:107` also loads that DLL with `LoadLibrary`/`GetProcAddress` |
| 8 | `dxguid.lib`, `ddraw.lib`, `dsound.lib` | DirectX 5 SDK | no | |
| 9 | `keyboard.lib` | `WIN32LIB/KEYBOARD` | source only | Also a member of `win32lib.lib` (`WIN32LIB/MAKEFILE` `LIBS`); listed twice, harmless |
| 10 | `mpgdll.lib` | **unverified**: not in the repo, and no makefile here builds it (`WIN32LIB/MOVIE/MAKEFILE:112` builds `mpeg.lib`, not `mpgdll.lib`) | no | MPEG movie support |
| 11 | `amstrmid.lib`, `strmbasd.lib` | DirectX Media SDK (DirectShow) | no | MPEG movies |
| 12 | `uuid.lib` | Watcom/Windows SDK | no | |

The link uses `option redefsok` (duplicate definitions allowed) and `option eliminate` (unreferenced
code segments dropped). Symbol resolution relies on the usual Watcom `wlink` behaviour: object files
win over libraries, and libraries are searched in the order listed, so `win32lib.lib` wins over
`vqm32wp.lib` for duplicate names such as `LCW_Uncompress` (**unverified**: no Watcom toolchain was
run; this is the documented `wlink` behaviour).

The Win32 compiler flags matter for every assembly interface: `/5r` (Watcom register calling
convention by default, so asm entry points declared `__cdecl` must say so explicitly), `/zp1`
(byte-packed structs, and the asm reads C++ objects at hard-coded offsets, see
[assembly-inventory.md](assembly-inventory.md#3-calling-conventions-and-layout-traps)), `/j` (signed `char`).
`WIN32LIB` itself is compiled with `/4 /j` and no `/zp` option in `WIN32LIB/PROJECT.CFG` (**unverified**
whether `wpp386`'s default packing differs from the game's `/zp1` for shared structs).

## 2. `WIN32LIB/` – Westwood's Win32 support library

`WIN32LIB/MAKEFILE` builds `win32lib.lib` out of 18 sub-libraries (the `LIBS` macro):
`audio dipthong drawbuff font iff keyboard mem misc mono palette shape tile timer ww_win wsa profile playcd wincomm`.
`RAWFILE`, `MOVIE`, `EXAMPLE` and `SRCDEBUG` are **not** part of it.

### 2.1 Member list of `win32lib.lib`

Generated from each `WIN32LIB/<SUB>/MAKEFILE` `OBJECTS` macro (90 members: 48 C++, 42 asm).
**Bold** = asm.

| Sub-library | Members (source file) |
|-------------|----------------------|
| AUDIO | SOUNDIO.CPP, SOUNDINT.CPP, SOUNDLCK.CPP, **SOSCODEC.ASM**, **OLSOSDEC.ASM**, **AUDUNCMP.ASM** |
| DIPTHONG | DIPTHONG.CPP, _DIPTABL.CPP |
| DRAWBUFF | **BITBLIT.ASM**, BUFFER.CPP, BUFFGLBL.CPP, **CLEAR.ASM**, **DRAWLINE.ASM**, DRAWRECT.CPP, **FILLQUAD.ASM**, **FILLRECT.ASM**, GBUFFER.CPP, **GETCLIP.ASM**, **GETPIX.ASM**, **PUTPIX.ASM**, REGIONSZ.CPP, **REMAP.ASM**, **SCALE.ASM**, **STAMP.ASM**, **SZREGION.ASM**, **TOBUFF.ASM**, **TOPAGE.ASM**, **TXTPRNT.ASM**, ICONCACH.CPP, **STMPCACH.ASM** |
| FONT | FONT.CPP, LOADFONT.CPP, SET_FONT.CPP, **SETFPAL.ASM**, **TEXTPRNT.ASM** |
| IFF | IFF.CPP, LOAD.CPP, LOADPICT.CPP, WRITELBM.CPP, **LCWCOMP.ASM**, **LCWUNCMP.ASM**, LOADPCX.CPP, WRITEPCX.CPP, **PACK2PLN.ASM** |
| KEYBOARD | KEYBOARD.CPP, MOUSE.CPP, **WWMOUSE.ASM** |
| MEM | ALLOC.CPP, MEM.CPP, NEWDEL.CPP, **MEM_COPY.ASM**, **VMPAGEIN.ASM** |
| MISC | **CRC.ASM**, DDRAW.CPP, DELAY.CPP, **DETPROC.ASM**, **FACING8.ASM**, **FACING16.ASM**, **FACINGFF.ASM**, **FADING.ASM**, FINDARGV.CPP, IRANDOM.CPP, LIB.CPP, **OPSYS.ASM**, **RANDOM.ASM**, **REVERSE.ASM**, **SHAKESCR.ASM**, **CLIPRECT.ASM**, EXIT.CPP, VERSION.CPP |
| MONO | MONO.CPP |
| PALETTE | LOADPAL.CPP, MORPHPAL.CPP, PALETTE.CPP, **PAL.ASM** |
| SHAPE | GETSHAPE.CPP, **SHAPE.ASM** |
| TILE | ICONSET.CPP |
| TIMER | TIMER.CPP, TIMERDWN.CPP, TIMERINI.CPP |
| WW_WIN | WINDOWS.CPP, WINHIDE.CPP |
| WSA | WSA.CPP, **XORDELTA.ASM** |
| PROFILE | WPROFILE.CPP, **APROFILE.ASM** |
| PLAYCD | GETCD.CPP |
| WINCOMM | WINCOMM.CPP, MODEMREG.CPP |

Not built into the library even though the folder holds them: `DRAWBUFF/FTPUTPIX.ASM`, `DRAWBUFF/SHADOW.ASM`,
`PLAYCD/PLAYCD.ASM`, the 15 shape-drawing files `SHAPE/DRAWSHP.ASM`, `SHAPE/DS_*.ASM` (13), `SHAPE/SETSHAPE.ASM`
(the game draws shapes with `CODE/2KEYFBUF.ASM` instead), everything under `*/OLD/`, `*/TEST/`,
`MEM/MSVC/`, `MEM/OLDMEM/`, and the whole `SRCDEBUG/` folder (93 asm files: 60 are byte-identical
copies of library sources, checked with `cmp`, and 33 are copies of older DOS library files that
have no counterpart left, e.g. `VESA.ASM`, `KEYIREAL.ASM`, `TIMERA.ASM`).

### 2.2 What each sub-library does and who calls it

"Game caller" is one example of compiled game code that calls into the sub-library.

| Sub-library | Purpose | Platform API | Example game caller | Verdict for the port |
|-------------|---------|-------------|--------------------|---------------------|
| AUDIO | Sound effects and streamed music: `Audio_Init`, `Play_Sample`, `File_Stream_Sample_Vol`; mixes into DirectSound buffers; decompresses Westwood ADPCM (`AUDUNCMP.ASM`) and HMI SOS IMA-ADPCM (`SOSCODEC.ASM`, `OLSOSDEC.ASM`) | DirectSound (`SOUNDIO.CPP:1017` `DirectSoundCreate`) | `CODE/AUDIO.CPP:466` `Play_Sample`; `CODE/THEME.CPP:341` | Replace backend with SDL audio (S09); keep the decoders (ported to C++) |
| DIPTHONG | String table lookup (`Extract_String`) and foreign-language digraph compression | none | `CODE/INLINE.H:932` | Keep (pure C++) |
| DRAWBUFF | `GraphicBufferClass` / `GraphicViewPortClass` (`WIN32LIB/INCLUDE/GBUFFER.H`): 8-bit software surfaces optionally backed by DirectDraw surfaces; all primitives (clear, blit, scale, line, rect, pixel, remap, text, stamps) are asm called through inline wrappers in `GBUFFER.H`; video-memory icon cache (`ICONCACH.CPP`, `STMPCACH.ASM`) | DirectDraw (`GBUFFER.CPP`, `ICONCACH.CPP`) | `CODE/CONQUER.CPP:432` `VisiblePage.Clear()`; `CODE/BAR.CPP:216` `Fill_Rect`; `CODE/CELL.CPP:1027` `Draw_Stamp` | Keep the class API, port the asm (S07), re-back surfaces with SDL (S08) |
| FONT | Font loading/metrics (`Set_Font`, `String_Pixel_Width`, `Char_Pixel_Width`); font colour translation (`SETFPAL.ASM`) | none | `CODE/CONQUER.CPP:4598` `Set_Font`; `CODE/DIALOG.CPP:613` `Set_Font_Palette` | Keep; port `SETFPAL.ASM` |
| IFF | IFF/LBM/PCX loading and LCW (de)compression | none | `CODE/2KEYFRAM.CPP:330` `LCW_Uncompress`; reached indirectly through `TILE/ICONSET.CPP` | Keep `LCW_Uncompress` (C++ version exists, see inventory); most of IFF is shadowed by `CODE/JSHELL.CPP:193,270` and `CODE/CCFILE.CPP:616-626` |
| KEYBOARD | `WWKeyboardClass` (Windows message keyboard queue) and `WWMouseClass` (software mouse cursor drawn into the surface, `WWMOUSE.ASM`) | Win32 messages, `GetCursorPos`-style APIs (**unverified** which) | `CODE/GSCREEN.CPP:475` `WWMouse->Draw_Mouse` | The game has its own `WWKeyboardClass` in `CODE/KEY.CPP` (object wins over library). Mouse: port or replace by an OS cursor (S08) |
| MEM | `Alloc`/`Free`, `Mem_Copy` (memmove), `Force_VM_Page_In` (Win95 paging hint) | none | `CODE/2KEYFRAM.CPP:262` `Alloc`; `CODE/MAP.CPP:1292` `Mem_Copy` | Keep allocators, replace asm with `memmove`/no-op |
| MISC | DirectDraw setup and palette (`DDRAW.CPP`: `Set_Video_Mode`, `Set_DD_Palette`), `Delay`, CRC, facing helpers, fading tables, clip helpers, random, byte swap, screen shake, CPU/OS detection | DirectDraw | `CODE/INIT.CPP:2083` `Set_Video_Mode`; `CODE/DISPLAY.CPP:408` `Build_Fading_Table` | Replace `DDRAW.CPP` with SDL (S08); port the few asm members that are pulled |
| MONO | Monochrome debug display (MDA at `0xB0000`) | port I/O (`MONO/MONO.CPP:61` `#pragma aux output`) | `CODE/MONOC.CPP` is the game's own copy | Delete (debug hardware) |
| PALETTE | `Set_Palette`, palette fades/morphs, `CurrentPalette` (`PAL.ASM`) | via `Set_DD_Palette` | `CODE/EGOS.CPP:328` `Set_Palette` | Keep; port `PAL.ASM` (it is a 768-byte copy and a call) |
| SHAPE | Shape buffer globals (`SHAPE.ASM`) and header helpers (`GETSHAPE.CPP`); the actual shape drawer is in `CODE/2KEYFBUF.ASM` | none | `CODE/INIT.CPP:3184` `Set_Shape_Buffer` | Keep; `SHAPE.ASM` becomes plain C++ globals |
| TILE | Icon (terrain template) sets: `Get_Icon_Set_Map`, `Load_Icon_Set` | none | `CODE/MAP.CPP:1292`, `CODE/TEMPLATE.CPP:96` | Keep (pure C++) |
| TIMER | `TimerClass`, `CountDownTimerClass`, `WinTimerClass` (the 60 Hz tick behind `TickCount`) | `timeSetEvent` (`TIMER/TIMERINI.CPP`) | 51 game files, e.g. `CODE/CCDDE.CPP:73` | Replace the backend with SDL timers (S08); frame pacing only, not simulation state |
| WW_WIN | Text windows: `WindowList[]`, `Change_Window`, `Window_Print` | none | `CODE/CDATA.CPP:3181` `WindowList` | Keep |
| WSA | WSA animations (`Open_Animation`, `Animate_Frame`) with XOR-delta frames (`XORDELTA.ASM`) | none | `CODE/MAPSEL.CPP:158`, `CODE/SCORE.CPP:427` | Keep; port `XORDELTA.ASM` |
| PROFILE | Sampling profiler (`Start_Profiler`/`Stop_Profiler`) using a timer callback and `APROFILE.ASM` hooks | `timeSetEvent`, thread context | `CODE/WINSTUB.CPP:236` `Stop_Profiler` | Delete (developer tool) |
| PLAYCD | CD drive detection (`GetCDClass`), Red Book audio (`RedBookClass`) | `GetLogicalDrives`/MCI (**unverified** which) | `CODE/CDFILE.CPP:281` `GetCDClass temp;` | Delete together with CD checks (S05 decides) |
| WINCOMM | `WinModemClass`, `WinNullModemClass`: serial/modem play | Win32 comm API | `CODE/NULLDLG.CPP:1157` `new WinModemClass` | Delete (ADR 0005 drops modem play) |

Not linked: `RAWFILE` (the game has its own `CODE/RAWFILE.CPP`), `MOVIE` (DirectShow MPEG player that
builds `mpeg.lib`, `WIN32LIB/MOVIE/MAKEFILE:112`), `EXAMPLE` (sample program), `SRCDEBUG` (debug copies).

### 2.3 Where the game overrides the library

These game definitions replace library code. Porting the library copy would be wasted work.

| Library symbol | Game replacement | Effect |
|----------------|------------------|--------|
| `Desired_Facing8/16/256` (`MISC/FACING*.ASM`) | `CODE/FACE.CPP:65,150` (C++), library prototypes disabled with `#if (0)` in `WIN32LIB/INCLUDE/MISC.H` | Asm never used. The **C++ version is the simulation authority** |
| `Calculate_CRC` (`MISC/CRC.ASM`, `VQM32/CRC.ASM`) | `CODE/INIT.CPP:2376` `extern "C"` wrapper around `CRCEngine` | Asm never pulled |
| `Buffer_Print`, `Get_Font_Palette_Ptr` (`DRAWBUFF/TXTPRNT.ASM`) | `CODE/2TXTPRNT.ASM` (C++ `CODE/2TXTPRNT.CPP` since S07-C) | Library member never pulled |
| `Size_Of_Region` → `Buffer_Size_Of_Region` (`DRAWBUFF/SZREGION.ASM`) | `#define Size_Of_Region(a, b) ((a)*(b))` in `CODE/COMPAT.H:51` | Asm never pulled |
| `SetPalette` (`VQM32/PALETTE.ASM`, VGA DAC writes) | `CODE/WINSTUB.CPP:695` `extern "C"` | Asm never pulled |
| `Load_Uncompress`, `Load_Picture` (`IFF/LOAD.CPP`, `IFF/LOADPICT.CPP`) | `CODE/JSHELL.CPP:193,270` | Library versions not used by the game |
| `Mono_*` (`VQM32/MONO.ASM`) | `CODE/MONOC.CPP`, `WIN32LIB/MONO/MONO.CPP` (C++) | Asm never pulled |
| `__PRO` (`PROFILE/APROFILE.ASM`) | `CODE/INIT.CPP:3528` (empty) | Duplicate definition, allowed by `option redefsok` |
| `WWKeyboardClass` (`KEYBOARD/KEYBOARD.CPP`) | `CODE/KEY.CPP` | Game version wins |
| `Random()`, `Get_Random_Mask`, `IRandom` (`MISC/RANDOM.ASM`, `MISC/IRANDOM.CPP`) | `CODE/RANDOM.CPP` (`RandomClass`), `Sim_Random_Pick` in `CODE/INLINE.H:1006` | The library RNG is never called; only its seed variable `RandNumb` is written (`CODE/INIT.CPP:2451-2491`). The **simulation RNG is C++** |

### 2.4 Text, palette, fading and cursor internals (learned in S07-C)

These routines were assembly until S07-C; the C++ ports keep their exact behaviour. Details a caller can trip over:

- **Font format** (`Buffer_Print`, `CODE/2TXTPRNT.CPP`). A font starts with a 14-byte header of little-endian
  offsets: info block (+4), per-character offset table (+6, 256 words), width table (+8, 256 bytes), data (+10),
  height table (+12, 256 words: low byte = blank rows above the glyph, high byte = rows of data). The info block's
  byte 4 is the line height (max character height). Glyph pixels are 4-bit, two per byte, **low nibble first**;
  every row starts on a new byte.
- **`ColorXlat`** (defined in `CODE/2TXTPRNT.CPP`) translates font pixel values to palette colours. The low nibble
  indexes `ColorXlat[n]`, the high nibble is used **unshifted**, so it indexes `ColorXlat[n * 16]`.
  `Set_Font_Palette_Range` (`WIN32LIB/FONT/SETFPAL.CPP`) fills both entries; each `Buffer_Print` call overwrites
  entry 0 with the background and entries 1 and 16 with the foreground. A translated colour of 0 is transparent.
- **Line breaks.** `'\n'` returns to x = 0 of the viewport, `'\r'` to the starting x; a character that would cross
  the right edge forces a `'\r'`-style break. The return value is the address of the next character, or 0 when the
  text runs off the bottom (callers treat 0 as "did not fit").
- **`CurrentPalette`** (`WIN32LIB/PALETTE/PAL.CPP`) is a copy of the last palette passed to `Set_Palette_Range`,
  which then calls `Set_DD_Palette` (the display layer; S08 replaces it). It starts as 768 bytes of 255.
- **`Build_Fading_Table`** (`WIN32LIB/MISC/FADING.CPP`) maps colours 1-255 to the closest palette colour to
  `orig - (orig - target) * frac / 256`, computed in 8/16-bit signed arithmetic. Colour 0 always maps to 0 and is
  never chosen; a colour never maps to itself; ties go to the higher index. The game's own
  `Conquer_Build_Fading_Table` (`CODE/JSHELL.CPP:467`) is a separate C++ function and is used for most tables.
- **Software cursor** (`WIN32LIB/KEYBOARD/WWMOUSE.CPP`). `ASM_Set_Mouse_Cursor` un-RLEs a shape (0 followed by a
  count = run of transparent pixels; compact shapes remap through their 16-byte table) into `MouseCursor`,
  staging LCW-compressed shapes in `_ShapeBuffer` first. It returns the old `PrevCursor` and stores the new shape
  there even when the shape is too big to use. `Mouse_Shadow_Buffer` saves/restores the screen under the cursor
  with the current `CursorWidth` as buffer stride. These routines read `WWMouseClass` and `GraphicViewPortClass`
  members through mirror structs, so **the member order of those two classes must not change**.

## 3. `WINVQ/` – Win32 VQA movie player

Two libraries, both Watcom `-5r -zp1` with TASM32 (`WINVQ/VQA32/MAKEFILE`, `WINVQ/VQM32/MAKEFILE`).

**`vqa32wp.lib`** (11 members): CONFIG.CPP, TASK.CPP, LOADER.CPP, DRAWER.CPP, AUDIO.CPP, MONODISP.CPP,
DSTREAM.CPP, **UNVQBUFF.ASM**, **UNVQVESA.ASM**, VERTAG.CPP, CAPTION.CPP (`unvqxmde.obj` is commented out).

**`vqm32wp.lib`** (32 members): IFF.CPP, PROFILE.CPP, AUDZAP.CPP, **AUDUNZAP.ASM**, TARGA.CPP, VERTAG.CPP,
SORTPAL.CPP, **PALETTE.ASM**, **MONO.ASM**, **LCWUNCMP.ASM**, **LCWCOMP.ASM**, TESTVB.CPP, **VB.ASM**,
**MCGABUF.ASM**, VIDEO.CPP, **XMODE.ASM**, XMODEPG.CPP, **VESABUF.ASM**, VESABLIT.CPP, VESAVID.CPP,
**SOSCODEC.ASM**, CHRWIDTH.CPP, MIXFILE.CPP, **CRC.ASM**, **FILLRECT.ASM**, CAPTOKEN.CPP, HUFFCMP.CPP,
**HUFFDCMP.ASM**, MEM.CPP, **DRAWCHAR.ASM**, **TEXTPRNT.ASM**, FONT.CPP.

Compile-time switches for the Watcom build (`WINVQ/VQA32/VQAPLAY.H:52-66`): `VQADIRECT_SOUND 1`,
`VQAAUDIO_ON 1`, `VQAMCGA_ON 1`, `VQABLOCK_4X2 1`, everything else (`XMODE`, `VESA`, `MONO`, `VIDEO`,
other block sizes, woofer) `0`. `VQACAPTIONS_ON` is `0` (`WINVQ/VQA32/CAPTION.H:45`).
The game plays movies into a RAM buffer: `CODE/INIT.CPP:1660` sets `VQACFGF_BUFFER`.

Result: only 4 of the 17 asm members are pulled (`UNVQBUFF` for `UnVQ_4x2`, `AUDUNZAP`, VQM32 `SOSCODEC`,
`MCGABUF`), and `MCGABUF` is dead at runtime (it writes to `0xA0000`, `WINVQ/VQA32/DRAWER.CPP:1680-1689`,
which only runs when `VQACFGF_BUFFER` is not set). Details per file in the inventory.

Callers in the game: `CODE/CONQUER.CPP:2941` and `CODE/INTRO.CPP:253` (`VQA_Play`).

ADR 0005 (proposed) drops movies. If it is accepted, both WINVQ libraries can go, together with
`CODE/INTERPAL.CPP`'s 2x interpolation and the asm in `CODE/WINASM.ASM` that backs it (but note the
interpolation is also used by `CODE/MAPSEL.CPP` and `CODE/SCORE.CPP`, see the inventory).

Not linked: `WINVQ/VPLAY32/` (stand-alone player), `WINVQ/VQAVIEW/` (Windows viewer tool, includes a
prebuilt `VQAVIEW.EXE`), `WINVQ/VQA32/OLD/`, `WINVQ/VQA32/UNVQXMDE.ASM`, `WINVQ/VQM32/PORTIO.ASM`.
`WINVQ/LIB/` holds six prebuilt **HMI SOS** libraries (`SOSDBLC.LIB`, `SOSDW1CR.LIB`, `SOSDW1CS.LIB`,
`SOSDW1PR.LIB`, `SOSDW1PS.LIB`, `SOSMBLC.LIB`) that the Win32 link never references.

## 4. The other top-level directories

| Directory | What it is | Win32 build uses it? | Evidence | Recommendation (input for S05) |
|-----------|-----------|---------------------|----------|-------------------------------|
| `WWFLAT32/` | DOS version of the support library (118 asm, VESA/MCGA/real-mode keyboard and timer, HMI SOS audio) | **No** | `CODE/MAKEFILE:62` sets `WWFLAT=..\wwflat32` only in the non-`WIN32` branch; `wwflat32.lib` only in the DOS `PROJ_LIBS` (line 433) and `conquer.lnk` | Delete |
| `VQ/` | DOS VQA player (19 asm, HMI SOS) | **No** | Only `..\vq\include` (DOS `CC_CFG`, line 104) and `..\vq\lib` (DOS `conquer.lnk`, line 537) | Delete |
| `IPX/` | Not purely DOS: it builds the Win95 IPX thunk DLLs `THIPX32.DLL`/`THIPX16.DLL` and the import library `wwipx32.lib` (`IPX/MAKETH32.BAT`, `IPX/MAKEFILE`) | **Yes, linked**: `win95.lnk` adds `ipx\wwipx32.lib` (`CODE/MAKEFILE:561`); at runtime `CODE/IPX95.CPP:107` `Load_IPX_Dll` loads `THIPX32.DLL` | 17 asm files, built with MASM (`ml`) and the 16/32-bit `thunk` compiler; none of them is linked into `ra95.exe` itself | Delete with native IPX (ADR 0005). Remove the `wwipx32.lib` line and `CODE/IPX95.CPP` together |
| `LAUNCH/` | `LAUNCH.ASM`: DOS real-mode launcher stub (MASM 6) that sets DOS/4GW environment and deletes swap files (header, `LAUNCH/LAUNCH.ASM:22-43`) | **No** | Not referenced by any makefile | Delete |
| `LAUNCHER/` | Separate Windows program (MSVC 5 project `LAUNCHER.DSP`) by Neal Kettler, 1998: reads a `.lcf` config, applies downloaded patches (`.exe` patches or RTPatch via `patchw32.dll`, `LAUNCHER/PATCH.CPP:197`), then starts the game (`LAUNCHER/MAIN.CPP:24-40`). Part of the WOL auto-update flow | **No** (separate exe) | Built only by its own `.DSP` | Delete. The port will have its own update story; RTPatch is proprietary |
| `TOOLS/` | Prebuilt asset-pipeline executables (`MIXFILE.EXE`, `KEYFRAME.EXE`, `ICONMAP.EXE`, `FONTMAKE.EXE`, `WWCOMP.EXE`, `AUDIOMAK.EXE`, …) plus sources for two of them: `TOOLS/MIX/MIXFILE.CPP` and `TOOLS/AUDIOMAK/` (`AUDIOMAK.CPP`, `SCODE.ASM`) | **No** | Not referenced by `CODE/MAKEFILE` (which expects `utils\tasm`, `utils\ebn`, `linker\nwlink` that are **not** in the repo) | Delete the binaries. Keep `MIXFILE.CPP` and `AUDIOMAK.CPP` only as format references, or move them to an `archive/` folder |

## 5. Audio: HMI SOS and DirectSound (input for S09)

- **The Win32 game does not use the HMI SOS driver.** Every SOS driver call (`sosDIGI*`, `sosTIMER*`)
  in linked code is in `WINVQ/VQA32/AUDIO.CPP` under `#if(!VQADIRECT_SOUND)`, which is `0` for Watcom
  (`WINVQ/VQA32/VQAPLAY.H:56`), e.g. `WINVQ/VQA32/AUDIO.CPP:181,408,499,797`. `WIN32LIB/AUDIO/SOSFNCT.H`
  only declares the functions. No `SOS*.LIB` is on the Win32 link line.
- **What is used from HMI is only the ADPCM codec** (HMI's IMA-ADPCM variant, shipped as asm):
  - `WIN32LIB/AUDIO/SOSCODEC.ASM` (`sosCODECInitStream`, `sosCODECDecompressData`: mono 16-bit) and
    `WIN32LIB/AUDIO/OLSOSDEC.ASM` (`General_sosCODECDecompressData`: other formats), called from
    `WIN32LIB/AUDIO/SOUNDINT.CPP:239-241` and `SOUNDIO.CPP:1704`.
  - `WINVQ/VQM32/SOSCODEC.ASM` (`VQA_sosCODEC*`) for movie audio, `WINVQ/VQA32/LOADER.CPP:502,2823`.
  - A **C++ port of the mono decoder already exists**: `CODE/ADPCM.CPP` (with tables `CODE/ITABLE.CPP`,
    `CODE/DTABLE.CPP`). It is not in `CODE/MAKEFILE`, so it was never built in this tree.
- **Westwood's own audio compression**: `WIN32LIB/AUDIO/AUDUNCMP.ASM` (`Decompress_Frame`, for
  `SCOMP_WESTWOOD`, `SOUNDINT.CPP:234`) and the movie equivalent `WINVQ/VQM32/AUDUNZAP.ASM` (`AudioUnzap`).
- **DirectSound** is used in exactly two places:
  - `WIN32LIB/AUDIO/` (`SOUNDIO.CPP`, `SOUNDINT.CPP`, `SOUNDLCK.CPP`): device creation
    (`SOUNDIO.CPP:1017` `DirectSoundCreate`), primary buffer, streaming buffers, mixing.
  - `WINVQ/VQA32/AUDIO.CPP` (movie audio), which gets the device from the game
    (`CODE/INIT.CPP:1683-1684` passes `SoundObject`, `PrimaryBufferPtr`).
  - The game itself only declares the two globals (`CODE/EXTERNS.H:130-131`) and calls the library API
    (`Audio_Init`, `Play_Sample`, `File_Stream_Sample_Vol`, …), e.g. `CODE/AUDIO.CPP:466`, `CODE/THEME.CPP:341`,
    `CODE/NULLDLG.CPP:7350`.
- For S09: replace the DirectSound layer inside `WIN32LIB/AUDIO` behind the existing API, and keep the
  three decoders (ported to C++, test with golden data).

## 6. Things S05 can delete outright

Safe because nothing in the Win32 link uses them (evidence above and in the inventory):

- `WWFLAT32/`, `VQ/`, `LAUNCH/`, `LAUNCHER/`, `TOOLS/` binaries, `WIN32LIB/SRCDEBUG/`, `WIN32LIB/EXAMPLE/`,
  `WIN32LIB/MOVIE/` (if MPEG movies are dropped), every `OLD/`, `OLDTEST/`, `TEST/`, `MSVC/`, `OLDMEM/` folder
  inside `WIN32LIB/` and `WINVQ/`, `WINVQ/VPLAY32/`, `WINVQ/VQAVIEW/`.
- All prebuilt HMI SOS libraries: `WINVQ/LIB/SOS*.LIB`, `WWFLAT32/LIB/SOSDW1PS.LIB`,
  `WWFLAT32/AUDIO/SOSDW1PS.LIB`, `WWFLAT32/AUDIO/NEW/SOSDW1PS.LIB` (proprietary binaries; also a licensing
  question, **unverified** whether they may be redistributed).
- `IPX/` and the `wwipx32.lib` link line, `CODE/IPXPROT.ASM`, `CODE/IPXREAL.ASM` (with native IPX, ADR 0005).
- `WIN32LIB/WINCOMM/` and the modem stubs in `CODE/WINASM.ASM` (with modem play, ADR 0005).
- `WIN32LIB/PROFILE/`, `WIN32LIB/MONO/`.
- The asm files that are library members but never pulled, and the `CODE/*.ASM` files that are not
  in the Win32 object list. Full list in [assembly-inventory.md](assembly-inventory.md#2-delete-list).

## 7. Notes for other sessions

- **S03 (build system):** the Win32 makefile reuses the macro name `WWFLAT` for `..\win32lib`; don't let
  that name suggest `WWFLAT32/` is needed. The external tools the makefile expects (`utils\tasm`,
  `utils\tlink`, `utils\ebn`, `linker\nwlink`) are not in the repo.
- **S08 (platform layer):** DirectDraw lives in `WIN32LIB/MISC/DDRAW.CPP`, `WIN32LIB/DRAWBUFF/GBUFFER.CPP`
  and `ICONCACH.CPP`; timers in `WIN32LIB/TIMER/TIMERINI.CPP` (`timeSetEvent`); mouse in
  `WIN32LIB/KEYBOARD/MOUSE.CPP`. The video-memory icon cache can be removed instead of ported.
- **S01 / S03 / S10: sources missing from the repository.** `CODE/MAKEFILE` lists `PALETTE.OBJ` (line 395)
  and `PASSEDIT.OBJ` (line 322), but there is no `CODE/PALETTE.CPP`, `CODE/PASSEDIT.CPP` or game `PALETTE.H`
  anywhere in the repo. `CODE/FUNCTION.H:318` includes `"palette.h"`, which then resolves to the library's
  `WIN32LIB/INCLUDE/PALETTE.H`, and that file does not define the `PaletteClass` the game uses
  (`PaletteClass::CurrentPalette` in `CODE/CONQUER.CPP:4575`; only a forward declaration in `CODE/RGB.H:41`).
  `PaletteClass` has to be rewritten before the game can compile.
- **S01 / S07:** inline assembly through `#pragma aux` is in game headers, not asm files, but some of it
  is simulation-critical (`Fixed_To_Cardinal`/`Cardinal_To_Fixed` in `CODE/JSHELL.H:292-315`). See the
  inventory's section on inline asm.
