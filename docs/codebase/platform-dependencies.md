# Platform Dependencies of the Win32 Build (`CODE/`)

Made in session S01. One row per source file that the original **Win32** build (`ra95.exe`) compiles,
with its subsystem and the platform APIs it uses in code that is **actually compiled** in that build.
For a narrative map of the subsystems, the boot sequence and the feature switches, see
[game-code.md](game-code.md).

> **Update from S05 (2026-10-14).** This table describes the build before S05. S05 deleted the DOS, native IPX,
> modem/Greenleaf, Westwood Online, WChat/DDE, statistics and TEN/MPath files listed here, collapsed every
> `WIN32`/DOS, `WINSOCK_IPX` and `WOLAPI_INTEGRATION` branch with `unifdef`, and removed `#include <modem.h>`,
> `<fast.h>` and `"phone.h"` from `FUNCTION.H` (so S06 no longer needs a `phone.h`). What went, and the last
> commit that still has it: [removed-code.md](removed-code.md).

## Summary

- **The build has 250 object files**, all listed in `CODE/MAKEFILE`. They come from 242 `.CPP` files, 6 `.ASM` files and
  **2 sources that are missing from the release** (`PALETTE.CPP`, `PASSEDIT.CPP`). Two more files are compiled
  because another source `#include`s them: `MAPEDSEL.CPP` and `WOLAPI/WOLAPI_I.C`. That makes **252 rows** below.
- **DirectX is used directly in only a few places.** Four files call DirectDraw directly (`CONQUER`, `STARTUP`, `STATS`, `WOLAPIOB`),
  and only `INIT.CPP` touches DirectSound (it passes the WIN32LIB DirectSound objects on to the movie player).
  Everything else draws and plays sound through WIN32LIB, so S08/S09 mostly replace WIN32LIB internals, not game code.
  34 files use WIN32LIB's DirectDraw-specific features (`AllSurfaces.SurfacesRestored`, `GBC_VIDEOMEM`, `Set_Video_Mode`, …).
- **Greenleaf is not linked into the Win32 build.** `win95.lnk` (`CODE/MAKEFILE:544-570`) has no Greenleaf library.
  The few `HM*`/`PortOpenGreenleafFast` calls that are still compiled resolve to empty stubs in `WINASM.ASM:253-298`.
  Modem play on Win32 uses WIN32LIB's `WinModemClass` instead. However, `FUNCTION.H:296-297` includes the
  Greenleaf headers `<modem.h>` and `<fast.h>` **in every file**, and they are not in the repository, so every
  file fails to compile until that include is removed or stubbed. Note: the overview's "~66 files" for `gcl` was a
  case-insensitive substring artefact (`BuildingClass` contains `gCl`). Only `NULLMGR.CPP` really calls Greenleaf API.
- **Watcom `#pragma aux` / inline asm in Win32-compiled code:** `COORD.CPP` (`calcx`/`calcy`, used by `Move_Point`),
  `MONOC.CPP` (port output), and the headers `JSHELL.H` (`Set_Bit`, `Get_Bit`, `First_True_Bit`, `First_False_Bit`,
  `Fixed_To_Cardinal`, `Cardinal_To_Fixed`, `outportb`), `RGB.H` (`outportb`, `outrgb`) and `MPU.H` (`Get_CPU_Clock`, rdtsc).
  `JSHELL.H` is included everywhere through `FUNCTION.H`, and `Fixed_To_Cardinal` is used by simulation code, so these
  must be rewritten in C++ **bit-exactly** (S06/S07). `EGOS.CPP`'s `Vsync` pragma and `DPMI.CPP` are DOS-only.
- **IPX:** in the shipped build (`WINSOCK_IPX` defined) the LAN game creates `IPXInterfaceClass`
  (`INIT.CPP:1255`, `WSPIPX.CPP`, Winsock `AF_IPX` sockets), and only the WOL internet game creates `UDPInterfaceClass`
  (`INIT.CPP:1202`). The real-mode/THIPX32 IPX code (`IPX.CPP`, `IPX95.CPP`, `IPXPROT.ASM`) is linked but never reached:
  its only callers are compiled out (`IPXMGR.CPP:149`, `:1138`, `:1864`). The game's whole network layer
  (`IPXManagerClass`, `IPXAddressClass`, the global `Ipx`) is named "IPX" but carries any `PacketTransport`.
  **S05 cannot delete the `IPX*` classes**, only the IPX transports.
- **DOS-only code inside Win32 files:** 1,735 non-blank lines in 37 files sit in `#ifndef WIN32` / `#else // WIN32`
  branches and are already compiled out. 12 files still call or include DOS/Watcom C runtime functions in compiled code
  (`_dos_getdrive`, `_dos_findfirst`, `<dos.h>`, `<mem.h>`, `<i86.h>`, …) and need shims (S06).
- **WOL:** 13 files are entirely inside `#ifdef WOLAPI_INTEGRATION`, and 1,376 lines (mostly the older WChat/internet
  path) are compiled out by `#ifndef WOLAPI_INTEGRATION`.

## Method (re-runnable)

1. **Build list.** The scanner parses `CODE/MAKEFILE`, evaluating its `!ifdef` blocks with `WIN32` defined and `GERMAN`/`FRENCH`
   undefined. It collects `OBJECTS` (linked into `ra95.exe`), the `!ifdef WIN32` `OBJECTS +=` block, `LIBFILES`
   (`jshell.lib`) and `TECHFILES` (`tech.lib`). All of these are linked by `win95.lnk`. Each `X.OBJ` is mapped to `X.CPP`, `X.ASM` or `X.C`
   in `CODE/`. Files pulled in with `#include "x.cpp"` from a build file are added.
2. **Preprocessor evaluation.** Each source is comment-stripped and run through a small **tri-state preprocessor**
   (each line is *compiled*, *dead* or *unknown*). It uses the Win32 release configuration:
   - from the makefile: `WIN32=1`, `_WIN32`, `WOLAPI_INTEGRATION`, `WINSOCK_IPX` and `ENGLISH=1`;
   - from `CODE/DEFINES.H`: `RELEASE_VERSION` plus every `FIXIT_*`, `DVD`, `MPEGMOVIE`, `MOVIE640` and `PARTIAL` switch it defines, with `TEN=0` and `MPATH=0`;
   - as undefined: `SCENARIO_EDITOR`, `CHEAT_KEYS`, `MCIMPEG`, `NEVER`, `OBSOLETE`, `DOS4G`, `__BORLANDC__`, …
   Headers are not expanded. A macro the scanner cannot resolve makes the block *unknown*, which counts as compiled.
   A file's own `#define`s are honoured.
3. **Pattern search.** Regular expressions for each dependency run over the compiled and unknown lines, with string literals blanked.
   The cells hold the **number of matching lines**. Dead lines are counted separately, by why they are dead.
4. **Hand classification.** The subsystem and notes columns were assigned by hand. For uncertain files the call sites
   were checked (for example, `TOOLTIP`, `ICONLIST` and `DIB*` are only used by WOL screens).
5. **Spot checks.** Matches were reviewed by hand in `CONQUER`, `INIT`, `STARTUP`, `NULLMGR`, `IPXMGR`, `IPX`, `COORD`, `EGOS`,
   `MONOC`, `SCORE`, `CONST` (a false positive inside a string led to string blanking) and `SESSION`. Directive lines inside dead
   blocks (for example `EGOS.CPP`'s DOS `#pragma aux Vsync`) are excluded.

To re-run, save the two scripts from the [appendix](#appendix-scripts) and run:

```sh
python3 -I scan_deps.py <repo-root> deps.tsv              # one TSV row per build file
python3 -I gen_table.py deps.tsv > table.md               # the table below
python3 -I scan_deps.py <repo-root> --show NULLMGR.CPP Greenleaf   # print the matching compiled lines of one file
```

**Limits.** The tool uses regexes, not a compiler, so expect a few false positives and negatives per column.
Headers are scanned separately (see below). The "Win32 API" column catches common calls and types, but not every Win32 identifier.
Line counts are physical lines. "live" means non-blank, non-comment lines that are compiled or unknown.

## Dependency overview

| Dependency | Files (compiled code) | Which | Plan |
|------------|----------------------|-------|------|
| DirectDraw (direct API) | 4 | `CONQUER` (MPEG movie mode), `STARTUP` (`InitDDraw`), `STATS` (caps for stats), `WOLAPIOB` (palette read) | S08 |
| DirectDraw via WIN32LIB features | 34 | Mostly dialogs checking `AllSurfaces.SurfacesRestored`; `STARTUP`/`INIT` set up surfaces | S08 (WIN32LIB) |
| DirectSound | 1 | `INIT.CPP:1683` hands `SoundObject`/`PrimaryBufferPtr` to the VQA player; everything else uses WIN32LIB audio | S09 |
| Win32 API | 38 | Boot/window (`STARTUP`, `WINSTUB`, `KEY`), files (`RAWFILE`, `SESSION`), registry/DDE, all WOL and Winsock files | S08, S12 |
| Winsock | 21 (mostly through `PacketTransport`) | `WSPROTO`/`WSPUDP`/`WSPIPX`/`_WSPROTO` transports, `TCPIP` (pre-WOL), stats (`PACKET`, `FIELD`, `UTRACKER`, `STATS`), WOL | S12 |
| IPX (names/classes) | 20 | Network layer is named after IPX; real IPX only in `WSPIPX` (Winsock), `IPX`, `IPX95`, `IPXPROT` | S04 / S05 |
| Greenleaf | 1 (+ headers in all) | `NULLMGR.CPP` (`HMWaitForOK`, `HMSetUpEchoRoutine`, `ASSUCCESS`, `PORT *`) | S05 |
| Serial (WIN32LIB `WinModemClass`) | 3 | `NULLMGR`, `NULLDLG`, `NULLCONN` | S05 |
| Watcom `#pragma aux` / inline asm | 2 `.CPP` + 3 headers | `COORD.CPP`, `MONOC.CPP`; `JSHELL.H`, `RGB.H`, `MPU.H` (`MEMCHECK.H` is not included by any file) | S06 / S07 |
| DOS / Watcom runtime calls | 12 | `_dos_*`, `<dos.h>`, `<i86.h>`, `<conio.h>`, `<mem.h>` (`CONQUER`, `INIT`, `STARTUP`, `SESSION`, `MIXFILE`, `CDFILE`, `HEAP`, `IPX*`, `LOADDLG`, `NETDLG`, `MONOC`) | S06 shims |
| WOL API | 17 files reference it; 13 files entirely WOL | see table | S05 removes |
| Movies (VQA / MPEG) | 9 | `CONQUER` (`Play_Movie`, `PlayMpegMovie`), `INIT`, `INTRO`, `SCENARIO`, `TACTION`, `GOPTIONS`, `MPGSET`, `WINSTUB`, `GLOBALS` | stub (ADR-0005) |

Non-standard C headers included by compiled code: `mem.h` (13 files), `dos.h` (7), `io.h` (6), `direct.h` (4),
`conio.h` (3), `share.h` (3), `i86.h` (2), `process.h` (1). `FUNCTION.H` itself includes `<windows.h>`, `<mem.h>`, `<dos.h>`,
`<direct.h>`, `<process.h>` and `<new.h>`, so in practice every file depends on them.

### Headers with platform content

| Header | What | Notes |
|--------|------|-------|
| `FUNCTION.H` | Master include for every game file | Includes `<windows.h>`, `<wwlib32.h>` (WIN32LIB), VQA headers, Greenleaf `<modem.h>`/`<fast.h>` (missing), `"phone.h"` (missing), `"pcx.h"` (missing, Win32 only), `"palette.h"` |
| `WATCOM.H` | Watcom warning pragmas; `typedef int bool;` and `enum {false,true}` (`WATCOM.H:82-87`) | `bool` was **4 bytes** in the original build. Affects the layout of saved structs |
| `JSHELL.H` | `#pragma aux` helpers (`JSHELL.H:211-330`) | Included everywhere. `Fixed_To_Cardinal` is used by simulation (`FLY`, `INFANTRY`, `TECHNO`, `UNIT`, `VESSEL`) |
| `RGB.H`, `MPU.H` | `#pragma aux` port I/O and rdtsc | Port I/O only matters for DOS palette code |
| `EXTERNS.H` | `extern LPDIRECTSOUND SoundObject; …` (`EXTERNS.H:130-131`) | DirectSound types in a shared header |
| `MOVIE.H` | `MpgPlay(..., IDirectDraw*, ...)` import from `mpgdll` | DLL and import library not in repo |
| `EVENT.H` | Network event: bit-fields `Frame:26`, `ID:5`, `IsExecuted:1` (`EVENT.H:113-123`) | Bit-field layout is compiler-specific. S04/S06 must pin the wire format |
| `MEMCHECK.H` | MemCheck debugging library: inline asm, `#pragma pack` | Not included by any file, so ignore |
| `IPX95.H`, `TCPIP.H`, `WSPROTO.H`, `CCDDE.H`, `WOLAPIOB.H`, `RAWOLAPI.H` | Win32 / Winsock / COM types | Go with their `.CPP` files |

### External libraries and missing files

| Item | Needed by | In repo? |
|------|-----------|----------|
| `PALETTE.CPP` + the `PaletteClass` `palette.h` | `tech.lib`; used by `CONQUER`, `JSHELL`, `OPTIONS`, `RGB`, `VORTEX`, … | **No.** (`WIN32LIB/INCLUDE/PALETTE.H` is a different, C-function API) |
| `PASSEDIT.CPP` + `PassEdit.h` | WOL login (`WOL_LOGN.CPP:28`) | **No** (goes away with WOL) |
| `phone.h` (`PhoneEntryClass`) | `FUNCTION.H:392`, `SESSION.H` | **No** (modem phone book) |
| `pcx.h` (`PCX_HEADER`) | `FUNCTION.H:399`, `WRITEPCX.CPP` | **No** |
| Greenleaf `modem.h`, `fast.h`, `commlib.h` | `FUNCTION.H`, `NULLMGR.H`, `NULLCONN.H` | **No** (proprietary) |
| `mpgdll.lib` / MPEG DLL (`MpgPlay`) | DVD movie mode | No |
| `ipx\wwipx32.lib`, `..\dxmedia\lib\*.lib`, DirectX 5 SDK libs | linker (`MAKEFILE:561-568`) | No |
| `win32lib.lib`, `keyboard.lib`, `vqa32wp.lib`, `vqm32wp.lib` | linker | Source in `WIN32LIB/`, `WINVQ/` (S02) |

## Column legend

- **Built into:** `ra95.exe` = `OBJECTS`. "(Win32-only)" = the `!ifdef WIN32` block. `jshell.lib` / `tech.lib` = static libraries
  built from `LIBFILES` / `TECHFILES` and linked into `ra95.exe`.
- **Lines (live):** physical lines, and in brackets the non-blank, non-comment lines that the Win32 build compiles.
  **0 live** means the whole file is compiled out.
- **DirectDraw:** `D` = direct DirectDraw API, `L` = WIN32LIB DirectDraw-specific features. **Greenleaf / serial:** `G` = Greenleaf API,
  `S` = WIN32LIB `WinModemClass`.
- **DOS-only:** `Nd` = N lines compiled out because they are in a DOS branch of a `WIN32` test; `Ni` = N compiled lines that
  still use DOS/Watcom runtime calls or headers.
- **WOL:** `whole file` = the entire file is inside `#ifdef WOLAPI_INTEGRATION`; `A` = lines using WOL API identifiers;
  `g` = compiled lines inside `WOLAPI_INTEGRATION` blocks.
- **Campaign-only:** used only by the single-player campaign (subsystem *Campaign* or *Movies*).
- Other numbers = count of compiled lines that match that dependency.

Subsystem codes are explained in [game-code.md](game-code.md#subsystem-map).

## Table: every file in the Win32 build

| # | File | Built into | Subsystem | Lines (live) | DirectDraw | DirectSound | Win32 API | Winsock | IPX | Greenleaf / serial | Watcom asm | DOS-only | WOL | Movie | Campaign-only | Notes |
|--:|------|-----------|-----------|-------------:|:--:|:--:|:--:|:--:|:--:|:--:|:--:|:--:|:--:|:--:|:--:|-------|
| 1 | `AADATA.CPP` | ra95.exe | Sim: type data | 656 (286) |  |  |  |  |  |  |  |  |  |  |  |  |
| 2 | `ABSTRACT.CPP` | ra95.exe | Simulation | 217 (35) |  |  |  |  |  |  |  |  |  |  |  |  |
| 3 | `ADATA.CPP` | ra95.exe | Sim: type data | 2362 (2102) |  |  |  |  |  |  |  |  |  |  |  |  |
| 4 | `AIRCRAFT.CPP` | ra95.exe | Simulation | 4376 (2124) |  |  |  |  |  |  |  |  |  |  |  |  |
| 5 | `ANIM.CPP` | ra95.exe | Simulation | 1109 (425) |  |  |  |  |  |  |  |  |  |  |  |  |
| 6 | `AUDIO.CPP` | ra95.exe | Audio | 764 (442) |  |  |  |  |  |  |  |  |  |  |  |  |
| 7 | `BAR.CPP` | ra95.exe | UI | 240 (71) |  |  |  |  |  |  |  |  |  |  |  |  |
| 8 | `BASE.CPP` | ra95.exe | Simulation | 549 (158) |  |  |  |  |  |  |  |  |  |  |  |  |
| 9 | `BBDATA.CPP` | ra95.exe | Sim: type data | 301 (110) |  |  |  |  |  |  |  |  |  |  |  |  |
| 10 | `BDATA.CPP` | ra95.exe | Sim: type data | 3835 (3088) |  |  |  |  |  |  |  |  |  |  |  |  |
| 11 | `BUILDING.CPP` | ra95.exe | Simulation | 5713 (2803) |  |  |  |  |  |  |  |  |  |  |  |  |
| 12 | `BULLET.CPP` | ra95.exe | Simulation | 1071 (414) |  |  |  |  |  |  |  |  |  |  |  |  |
| 13 | `CARGO.CPP` | ra95.exe | Simulation | 184 (39) |  |  |  |  |  |  |  |  |  |  |  |  |
| 14 | `CARRY.CPP` | ra95.exe | Campaign | 154 (64) |  |  |  |  |  |  |  |  |  |  | yes |  |
| 15 | `CCFILE.CPP` | ra95.exe | File I/O | 694 (214) |  |  |  |  |  |  |  |  |  |  |  |  |
| 16 | `CCINI.CPP` | ra95.exe | File I/O | 1488 (378) |  |  |  |  |  |  |  |  |  |  |  |  |
| 17 | `CCPTR.CPP` | ra95.exe | Core/boot | 77 (12) |  |  |  |  |  |  |  |  |  |  |  |  |
| 18 | `CDATA.CPP` | ra95.exe | Sim: type data | 3325 (2945) |  |  |  |  |  |  |  |  |  |  |  |  |
| 19 | `CELL.CPP` | ra95.exe | Simulation | 3015 (1106) |  |  |  |  |  |  |  |  |  |  |  |  |
| 20 | `CHECKBOX.CPP` | ra95.exe | UI | 103 (25) |  |  |  |  |  |  |  |  |  |  |  |  |
| 21 | `CHEKLIST.CPP` | ra95.exe | UI | 366 (110) |  |  |  |  |  |  |  |  |  |  |  |  |
| 22 | `COLRLIST.CPP` | ra95.exe | UI | 279 (77) |  |  |  |  |  |  |  |  |  |  |  |  |
| 23 | `COMBAT.CPP` | ra95.exe | Simulation | 426 (178) |  |  |  |  |  |  |  |  |  |  |  |  |
| 24 | `COMBUF.CPP` | ra95.exe | Network core | 1165 (507) |  |  |  |  |  |  |  |  |  |  |  | hdrs: mem.h |
| 25 | `CONNECT.CPP` | ra95.exe | Network core | 839 (295) |  |  |  |  |  |  |  |  |  |  |  | hdrs: mem.h |
| 26 | `CONQUER.CPP` | ra95.exe | Core/boot | 5559 (2255) | D13 L3 |  | 17 |  | 20 |  |  | 70d 3i | A22 g67 | 25 |  | Main_Game/Main_Loop; DirectDraw used directly for MPEG movie mode; `_dos_getdrive`; hdrs: direct.h dos.h io.h share.h |
| 27 | `CONST.CPP` | ra95.exe | Sim: type data | 821 (563) |  |  |  |  |  |  |  |  |  |  |  |  |
| 28 | `CONTROL.CPP` | ra95.exe | UI | 227 (41) |  |  |  |  |  |  |  |  |  |  |  |  |
| 29 | `COORD.CPP` | ra95.exe | Simulation | 653 (284) |  |  |  |  |  |  | 2 |  |  |  |  | `calcx`/`calcy` `#pragma aux` used by `Move_Point` (simulation!) |
| 30 | `CRATE.CPP` | ra95.exe | Simulation | 186 (61) |  |  |  |  |  |  |  |  |  |  |  |  |
| 31 | `CREDITS.CPP` | ra95.exe | UI | 246 (92) |  |  |  |  |  |  |  | 6d |  |  |  |  |
| 32 | `CREW.CPP` | ra95.exe | Simulation | 40 (0) |  |  |  |  |  |  |  |  |  |  |  |  |
| 33 | `DEBUG.CPP` | ra95.exe | Editor/debug | 561 (0) |  |  |  |  |  |  |  |  |  |  |  | Fully compiled out (`CHEAT_KEYS` undefined) |
| 34 | `DIAL8.CPP` | ra95.exe | UI | 320 (99) |  |  |  |  |  |  |  |  |  |  |  |  |
| 35 | `DIALOG.CPP` | ra95.exe | UI | 999 (386) |  |  |  |  |  |  |  | 15d | g2 |  |  |  |
| 36 | `DISPLAY.CPP` | ra95.exe | Rendering | 4510 (2052) |  |  |  |  |  |  |  | 43d |  |  |  |  |
| 37 | `DOOR.CPP` | ra95.exe | Simulation | 203 (63) |  |  |  |  |  |  |  |  |  |  |  |  |
| 38 | `DRIVE.CPP` | ra95.exe | Simulation | 2334 (1272) |  |  |  |  |  |  |  |  |  |  |  |  |
| 39 | `DROP.CPP` | ra95.exe | UI | 220 (151) |  |  |  |  |  |  |  |  | g6 |  |  |  |
| 40 | `DYNAVEC.CPP` | ra95.exe | Core/boot | 300 (75) |  |  |  |  |  |  |  |  |  |  |  | hdrs: mem.h |
| 41 | `EDIT.CPP` | ra95.exe | UI | 475 (156) |  |  |  |  |  |  |  | 5d |  |  |  |  |
| 42 | `EGOS.CPP` | ra95.exe | UI | 989 (371) | L1 |  |  |  |  |  |  | 9d |  |  |  | Credits screen; `Vsync` pragma is DOS-only |
| 43 | `ENDING.CPP` | ra95.exe | Campaign | 164 (6) |  |  |  |  |  |  |  |  |  |  | yes | Almost all `#ifdef NEVER` |
| 44 | `EVENT.CPP` | ra95.exe | Network core | 1051 (499) |  |  |  |  |  |  |  |  |  |  |  | Network event struct (packed, /zp1); sent over the wire |
| 45 | `EXPAND.CPP` | ra95.exe | Campaign | 567 (236) | L2 |  |  |  |  |  |  | 4d |  |  | yes |  |
| 46 | `FACE.CPP` | ra95.exe | Simulation | 228 (67) |  |  |  |  |  |  |  |  |  |  |  |  |
| 47 | `FACING.CPP` | ra95.exe | Simulation | 184 (40) |  |  |  |  |  |  |  |  |  |  |  |  |
| 48 | `FACTORY.CPP` | ra95.exe | Simulation | 672 (217) |  |  |  |  |  |  |  |  |  |  |  |  |
| 49 | `FINDPATH.CPP` | ra95.exe | Simulation | 1295 (497) |  |  |  |  |  |  |  |  |  |  |  |  |
| 50 | `FLASHER.CPP` | ra95.exe | Simulation | 98 (12) |  |  |  |  |  |  |  |  |  |  |  |  |
| 51 | `FLY.CPP` | ra95.exe | Simulation | 135 (28) |  |  |  |  |  |  |  |  |  |  |  |  |
| 52 | `FOOT.CPP` | ra95.exe | Simulation | 2550 (934) |  |  |  |  |  |  |  |  |  |  |  |  |
| 53 | `FUSE.CPP` | ra95.exe | Simulation | 198 (46) |  |  |  |  |  |  |  |  |  |  |  |  |
| 54 | `GADGET.CPP` | ra95.exe | UI | 873 (258) |  |  |  |  |  |  |  |  |  |  |  |  |
| 55 | `GAMEDLG.CPP` | ra95.exe | UI | 521 (286) | L2 |  |  |  |  |  |  |  | A5 g54 |  |  |  |
| 56 | `GAUGE.CPP` | ra95.exe | UI | 540 (204) |  |  |  |  |  |  |  |  |  |  |  |  |
| 57 | `GLOBALS.CPP` | ra95.exe | Core/boot | 806 (259) |  |  | 1 |  | 2 |  |  | 5d |  | 3 |  | All global objects |
| 58 | `GOPTIONS.CPP` | ra95.exe | UI | 625 (358) | L2 |  |  |  |  |  |  | 7d |  | 1 |  |  |
| 59 | `GSCREEN.CPP` | ra95.exe | Rendering | 488 (114) |  |  |  |  |  |  |  | 8d |  |  |  | Page flipping base |
| 60 | `HDATA.CPP` | ra95.exe | Sim: type data | 527 (271) |  |  |  |  |  |  |  |  |  |  |  |  |
| 61 | `HEAP.CPP` | ra95.exe | Core/boot | 633 (181) |  |  |  |  |  |  |  | 1i |  |  |  | Fixed heaps of game objects; hdrs: conio.h mem.h |
| 62 | `HELP.CPP` | ra95.exe | UI | 417 (136) |  |  |  |  |  |  |  |  |  |  |  |  |
| 63 | `HOUSE.CPP` | ra95.exe | Simulation | 7766 (3807) |  |  |  |  | 3 |  |  |  |  |  |  |  |
| 64 | `IDATA.CPP` | ra95.exe | Sim: type data | 1375 (941) |  |  |  |  |  |  |  | 2d |  |  |  |  |
| 65 | `INFANTRY.CPP` | ra95.exe | Simulation | 4151 (1859) |  |  |  |  |  |  |  |  |  |  |  |  |
| 66 | `INI.CPP` | ra95.exe | File I/O | 1285 (423) |  |  |  |  |  |  |  |  |  |  |  |  |
| 67 | `INIT.CPP` | ra95.exe | Core/boot | 3618 (1293) | L1 | 2 | 7 | 14 | 3 |  |  | 55d 6i | A8 g57 | 8 |  | Init_Game/Select_Game; creates packet transports (`INIT.CPP:1202`, `:1255`); `_dos_findfirst`; hdrs: conio.h dos.h |
| 68 | `INTRO.CPP` | ra95.exe | Campaign | 304 (47) |  |  |  |  |  |  |  |  |  | 7 | yes |  |
| 69 | `IOMAP.CPP` | ra95.exe | File I/O | 501 (139) |  |  |  |  |  |  |  |  |  |  |  |  |
| 70 | `IOOBJ.CPP` | ra95.exe | File I/O | 900 (193) |  |  |  |  |  |  |  |  |  |  |  |  |
| 71 | `IPX.CPP` | ra95.exe | Network core | 1165 (9) |  |  |  |  | 5 |  |  | 423d 1i |  |  |  | Real IPX via THIPX32 DLL; entry point only reached from dead code; hdrs: i86.h mem.h |
| 72 | `IPXADDR.CPP` | ra95.exe | Network core | 509 (143) |  |  |  | 1 | 14 |  |  |  |  |  |  | hdrs: mem.h |
| 73 | `IPXCONN.CPP` | ra95.exe | Network core | 878 (106) |  |  |  | 6 | 26 |  |  | 33d |  |  |  | hdrs: mem.h |
| 74 | `IPXGCONN.CPP` | ra95.exe | Network core | 534 (169) |  |  |  |  | 20 |  |  |  |  |  |  | hdrs: mem.h |
| 75 | `IPXMGR.CPP` | ra95.exe | Network core | 2069 (592) |  |  |  | 10 | 44 |  |  | 114d 1i |  |  |  | Owns `PacketTransport` when `WINSOCK_IPX`; used by LAN **and** internet games; hdrs: i86.h mem.h |
| 76 | `IPXPROT.ASM` | ra95.exe | Network core | 113 (asm) | | | | | x |  | **asm file** | yes | | |  | DOS real-mode IPX stub (exports only used in DOS branch of IPXMGR) |
| 77 | `JSHELL.CPP` | ra95.exe | Core/boot | 661 (170) |  |  |  |  |  |  |  |  |  |  |  |  |
| 78 | `LAYER.CPP` | ra95.exe | Simulation | 164 (41) |  |  |  |  |  |  |  |  |  |  |  |  |
| 79 | `LINK.CPP` | ra95.exe | UI | 402 (97) |  |  |  |  |  |  |  |  |  |  |  |  |
| 80 | `LIST.CPP` | ra95.exe | UI | 972 (352) |  |  |  |  |  |  |  |  |  |  |  |  |
| 81 | `LOADDLG.CPP` | ra95.exe | UI | 785 (382) | L2 |  |  |  |  |  |  | 2d 2i |  |  |  | hdrs: io.h |
| 82 | `LOGIC.CPP` | ra95.exe | Simulation | 418 (117) |  |  |  |  |  |  |  |  |  |  |  |  |
| 83 | `MAP.CPP` | ra95.exe | Simulation | 2136 (901) |  |  |  |  |  |  |  |  |  |  |  |  |
| 84 | `MAPEDDLG.CPP` | ra95.exe | Editor/debug | 2709 (0) |  |  |  |  |  |  |  |  |  |  |  | Fully compiled out |
| 85 | `MAPEDIT.CPP` | ra95.exe | Editor/debug | 2174 (0) |  |  |  |  |  |  |  |  |  |  |  | Fully compiled out (`SCENARIO_EDITOR` undefined) |
| 86 | `MAPEDPLC.CPP` | ra95.exe | Editor/debug | 1825 (0) |  |  |  |  |  |  |  |  |  |  |  | Fully compiled out |
| 87 | `MAPEDTM.CPP` | ra95.exe | Editor/debug | 946 (0) |  |  |  |  |  |  |  |  |  |  |  | Fully compiled out |
| 88 | `MAPSEL.CPP` | ra95.exe | Campaign | 400 (231) | L2 |  |  | 1 |  |  |  | 13d |  |  | yes |  |
| 89 | `MENUS.CPP` | ra95.exe | UI | 1013 (433) | L2 |  | 2 |  |  |  |  | 4d |  |  |  |  |
| 90 | `MISSION.CPP` | ra95.exe | Simulation | 574 (217) |  |  |  |  |  |  |  |  |  |  |  |  |
| 91 | `MOUSE.CPP` | ra95.exe | UI | 393 (132) |  |  |  |  |  |  |  |  |  |  |  |  |
| 92 | `MPLAYER.CPP` | ra95.exe | Multiplayer UI | 1250 (449) | L2 |  |  |  | 5 |  |  |  | g24 |  |  | Multiplayer game-type menu |
| 93 | `MSGBOX.CPP` | ra95.exe | UI | 519 (240) | L2 |  |  |  |  |  |  | 8d |  |  |  |  |
| 94 | `MSGLIST.CPP` | ra95.exe | Multiplayer UI | 1400 (606) |  |  |  |  |  |  |  |  |  |  |  |  |
| 95 | `NETDLG.CPP` | ra95.exe | Multiplayer UI | 7825 (2945) | L6 |  |  | 1 | 91 |  |  | 1i | A4 g3 |  |  | LAN lobby dialogs; big WOL-dead sections; hdrs: dos.h |
| 96 | `NULLCONN.CPP` | ra95.exe | Modem/serial (+skirmish UI) | 286 (60) |  |  | 1 |  |  | S2 |  | 17d |  |  |  | Null-modem connection; hdrs: mem.h |
| 97 | `NULLDLG.CPP` | ra95.exe | Modem/serial (+skirmish UI) | 7828 (4272) | L18 |  | 2 |  |  | S25 |  | 183d | g2 |  |  | Also holds the **skirmish** setup dialog `Com_Scenario_Dialog(true)` (`NULLDLG.CPP:2904`) |
| 98 | `NULLMGR.CPP` | ra95.exe | Modem/serial (+skirmish UI) | 2621 (862) | L4 |  | 4 |  |  | G7 S55 |  | 196d |  |  |  | Serial via WIN32LIB `WinModemClass`; Greenleaf `HM*` calls still live (resolve to WINASM stubs) |
| 99 | `OBJECT.CPP` | ra95.exe | Simulation | 2240 (670) |  |  |  |  |  |  |  |  |  |  |  |  |
| 100 | `ODATA.CPP` | ra95.exe | Sim: type data | 955 (571) |  |  |  |  |  |  |  |  |  |  |  |  |
| 101 | `OPTIONS.CPP` | ra95.exe | UI | 924 (390) |  |  |  |  |  |  |  | 3d |  |  |  |  |
| 102 | `OVERLAY.CPP` | ra95.exe | Simulation | 385 (165) |  |  |  |  |  |  |  |  |  |  |  |  |
| 103 | `POWER.CPP` | ra95.exe | UI | 479 (192) |  |  |  |  |  |  |  |  |  |  |  |  |
| 104 | `PROFILE.CPP` | ra95.exe | File I/O | 871 (388) |  |  |  |  |  |  |  |  |  |  |  |  |
| 105 | `QUEUE.CPP` | ra95.exe | Network core | 4556 (1903) |  |  |  |  | 1 |  |  |  | A7 g7 |  |  | Lockstep core: Queue_AI, frame sync, CRC checks |
| 106 | `RADAR.CPP` | ra95.exe | UI | 2608 (1181) | L1 |  |  |  |  |  |  | 83d |  |  |  |  |
| 107 | `RADIO.CPP` | ra95.exe | Simulation | 294 (91) |  |  |  |  |  |  |  |  |  |  |  |  |
| 108 | `REINF.CPP` | ra95.exe | Simulation | 750 (312) |  |  |  |  |  |  |  |  |  |  |  |  |
| 109 | `RULES.CPP` | ra95.exe | Simulation | 1050 (645) |  |  |  |  |  |  |  |  |  |  |  |  |
| 110 | `SAVELOAD.CPP` | ra95.exe | File I/O | 1564 (655) |  |  |  |  |  |  |  |  |  |  |  |  |
| 111 | `SCENARIO.CPP` | ra95.exe | Simulation | 3489 (1723) | L4 |  |  |  |  |  |  | 8d | g1 | 7 |  |  |
| 112 | `SCORE.CPP` | ra95.exe | Campaign | 1936 (1034) | L2 |  |  | 1 |  |  |  | 76d |  |  | yes | Campaign score screen **and** `Multi_Score_Presentation` |
| 113 | `SCROLL.CPP` | ra95.exe | UI | 270 (104) |  |  |  |  |  |  |  |  |  |  |  |  |
| 114 | `SDATA.CPP` | ra95.exe | Sim: type data | 572 (234) |  |  |  |  |  |  |  |  |  |  |  |  |
| 115 | `SESSION.CPP` | ra95.exe | Network core | 1871 (817) |  |  | 7 |  | 1 |  |  | 56d 2i |  |  |  | SessionClass (game type, players); `FindFirstFile` for recordings; hdrs: dos.h |
| 116 | `SHAPEBTN.CPP` | ra95.exe | UI | 183 (47) |  |  |  |  |  |  |  |  |  |  |  |  |
| 117 | `SIDEBAR.CPP` | ra95.exe | UI | 2433 (1037) |  |  |  |  |  |  |  | 24d |  |  |  |  |
| 118 | `SLIDER.CPP` | ra95.exe | UI | 417 (150) |  |  |  |  |  |  |  |  |  |  |  |  |
| 119 | `SMUDGE.CPP` | ra95.exe | Simulation | 365 (140) |  |  |  |  |  |  |  |  |  |  |  |  |
| 120 | `SOUNDDLG.CPP` | ra95.exe | UI | 459 (199) | L2 |  |  |  |  |  |  | 1d |  |  |  |  |
| 121 | `SPECIAL.CPP` | ra95.exe | UI | 593 (292) | L4 |  |  |  |  |  |  |  |  |  |  |  |
| 122 | `STARTUP.CPP` | ra95.exe | Win32 platform | 1070 (344) | D3 L10 |  | 34 |  | 3 |  |  | 57d 3i | g21 |  |  | WinMain, InitDDraw; registry; `_dos_getdrive`/`_dos_setdrive`; hdrs: conio.h io.h |
| 123 | `STATBTN.CPP` | ra95.exe | UI | 272 (84) |  |  |  |  |  |  |  |  |  |  |  |  |
| 124 | `SUPER.CPP` | ra95.exe | Simulation | 392 (128) |  |  |  |  |  |  |  |  |  |  |  |  |
| 125 | `TAB.CPP` | ra95.exe | UI | 319 (107) |  |  |  |  |  |  |  | 8d |  |  |  |  |
| 126 | `TACTION.CPP` | ra95.exe | Simulation | 893 (439) |  |  |  |  |  |  |  |  |  | 1 |  |  |
| 127 | `TARGET.CPP` | ra95.exe | Simulation | 872 (304) |  |  |  |  |  |  |  |  |  |  |  |  |
| 128 | `TDATA.CPP` | ra95.exe | Sim: type data | 920 (550) |  |  |  |  |  |  |  |  |  |  |  |  |
| 129 | `TEAM.CPP` | ra95.exe | Simulation | 3077 (1466) |  |  |  |  |  |  |  |  |  |  |  |  |
| 130 | `TEAMTYPE.CPP` | ra95.exe | Simulation | 1880 (409) |  |  |  |  |  |  |  |  |  |  |  |  |
| 131 | `TECHNO.CPP` | ra95.exe | Simulation | 6350 (2631) |  |  |  |  |  |  |  |  |  |  |  |  |
| 132 | `TEMPLATE.CPP` | ra95.exe | Simulation | 242 (78) |  |  |  |  |  |  |  |  |  |  |  |  |
| 133 | `TERRAIN.CPP` | ra95.exe | Simulation | 793 (253) |  |  |  |  |  |  |  |  |  |  |  |  |
| 134 | `TEVENT.CPP` | ra95.exe | Simulation | 767 (389) |  |  |  |  |  |  |  |  |  |  |  |  |
| 135 | `TEXTBTN.CPP` | ra95.exe | UI | 349 (105) |  |  |  |  |  |  |  |  | g1 |  |  |  |
| 136 | `THEME.CPP` | ra95.exe | Audio | 618 (199) |  |  |  |  |  |  |  | 1d |  |  |  |  |
| 137 | `TOGGLE.CPP` | ra95.exe | UI | 202 (55) |  |  |  |  |  |  |  |  |  |  |  |  |
| 138 | `TRACKER.CPP` | ra95.exe | Simulation | 135 (55) |  |  |  |  |  |  |  |  |  |  |  |  |
| 139 | `TRIGGER.CPP` | ra95.exe | Simulation | 498 (165) |  |  |  |  |  |  |  |  |  |  |  |  |
| 140 | `TRIGTYPE.CPP` | ra95.exe | Simulation | 2130 (207) |  |  |  |  |  |  |  |  |  |  |  | Most of file is editor-only |
| 141 | `TXTLABEL.CPP` | ra95.exe | UI | 102 (22) |  |  |  |  |  |  |  |  |  |  |  |  |
| 142 | `UDATA.CPP` | ra95.exe | Sim: type data | 1378 (885) |  |  |  |  |  |  |  |  |  |  |  |  |
| 143 | `UNIT.CPP` | ra95.exe | Simulation | 5065 (2329) |  |  |  |  |  |  |  |  |  |  |  |  |
| 144 | `UTRACKER.CPP` | ra95.exe | Internet (pre-WOL) | 237 (45) |  |  |  | 2 |  |  |  |  |  |  |  |  |
| 145 | `VDATA.CPP` | ra95.exe | Sim: type data | 713 (264) |  |  |  |  |  |  |  |  |  |  |  |  |
| 146 | `VECTOR.CPP` | ra95.exe | Core/boot | 676 (195) |  |  |  |  |  |  |  |  |  |  |  | BooleanVectorClass uses `Set_Bit`/`Get_Bit` (`#pragma aux` in JSHELL.H); hdrs: mem.h |
| 147 | `VERSION.CPP` | ra95.exe | Core/boot | 853 (141) |  |  |  |  |  |  |  |  | A1 |  |  |  |
| 148 | `VESSEL.CPP` | ra95.exe | Simulation | 2357 (1008) |  |  |  |  |  |  |  |  |  |  |  |  |
| 149 | `VISUDLG.CPP` | ra95.exe | UI | 417 (248) | L2 |  |  |  |  |  |  |  |  |  |  |  |
| 150 | `VORTEX.CPP` | ra95.exe | Simulation | 1259 (514) |  |  |  |  |  |  |  | 3d |  |  |  |  |
| 151 | `WARHEAD.CPP` | ra95.exe | Simulation | 193 (53) |  |  |  |  |  |  |  |  |  |  |  |  |
| 152 | `WEAPON.CPP` | ra95.exe | Simulation | 337 (109) |  |  |  |  |  |  |  |  |  |  |  |  |
| 153 | `MCIMOVIE.CPP` | ra95.exe | Movies | 353 (0) |  |  |  |  |  |  |  |  |  |  | yes | Compiled empty (`MCIMPEG` undefined) |
| 154 | `MCI.CPP` | ra95.exe | Movies | 366 (0) |  |  |  |  |  |  |  |  |  |  | yes | Compiled empty (`MCIMPEG` undefined) |
| 155 | `MPGSET.CPP` | ra95.exe | Movies | 610 (255) | L2 |  |  |  |  |  |  |  |  | 7 | yes | DVD MPEG movie settings; MpgPlay lives in missing `mpgdll.lib` |
| 156 | `ICONLIST.CPP` | ra95.exe | WOL | 911 (600) |  |  | 4 |  |  |  |  |  | whole file |  |  | Only used by WOL screens |
| 157 | `WOL_MAIN.CPP` | ra95.exe | WOL | 288 (181) |  |  | 22 |  |  |  |  |  | whole file |  |  |  |
| 158 | `WOL_CHAT.CPP` | ra95.exe | WOL | 1582 (1140) | L2 |  | 14 |  | 3 |  |  |  | whole file |  |  |  |
| 159 | `RAWOLAPI.CPP` | ra95.exe | WOL | 2103 (1519) |  |  | 57 |  |  |  |  |  | whole file |  |  | `#include`s WOLAPI/WOLAPI_I.C (COM GUIDs) |
| 160 | `WOLAPIOB.CPP` | ra95.exe | WOL | 3436 (2362) | D4 L1 |  | 143 | 4 |  |  |  |  | whole file |  |  |  |
| 161 | `W95TRACE.CPP` | ra95.exe | Win32 platform | 130 (3) |  |  | 2 |  |  |  |  |  |  |  |  | Debug trace stubs; hdrs: process.h |
| 162 | `COMINIT.CPP` | ra95.exe | WOL | 52 (9) |  |  | 2 |  |  |  |  |  |  |  |  | COM init (for WOL) |
| 163 | `WOL_LOGN.CPP` | ra95.exe | WOL | 666 (352) | L2 |  | 7 |  |  |  |  |  | whole file |  |  |  |
| 164 | `WOLEDIT.CPP` | ra95.exe | WOL | 168 (63) |  |  |  |  |  |  |  | 5d | whole file |  |  |  |
| 165 | `PASSEDIT.CPP` | ra95.exe | WOL | missing | | | | | | | | | | |  | **Source missing** from the release |
| 166 | `SEDITDLG.CPP` | ra95.exe | WOL | 387 (231) | L2 |  | 2 |  |  |  |  |  | whole file |  |  |  |
| 167 | `DIBFILE.CPP` | ra95.exe | WOL | 704 (250) |  |  | 21 |  |  |  |  |  |  |  |  | GDI DIB loader, only used by WOL code; hdrs: direct.h io.h |
| 168 | `DIBUTIL.CPP` | ra95.exe | WOL | 1317 (533) |  |  | 70 |  |  |  |  |  |  |  |  | GDI DIB helpers, only used by WOL code |
| 169 | `WOL_GSUP.CPP` | ra95.exe | WOL | 3501 (2170) | L2 |  | 22 | 4 | 4 |  |  |  | A87 g2149 |  |  |  |
| 170 | `UDPADDR.CPP` | ra95.exe | Network core | 232 (117) | L2 |  |  | 1 |  |  |  |  |  |  |  |  |
| 171 | `WSPROTO.CPP` | ra95.exe | Network core | 595 (156) |  |  | 15 | 36 | 1 |  |  |  |  |  |  | Winsock transport base class; `WSAAsyncSelect`-driven |
| 172 | `WSPUDP.CPP` | ra95.exe | Network core | 432 (136) |  |  | 4 | 31 |  |  |  |  |  |  |  | Winsock UDP transport: used for `GAME_INTERNET` (WOL) only |
| 173 | `WSPIPX.CPP` | ra95.exe | Network core | 450 (172) |  |  | 12 | 52 | 19 |  |  |  |  |  |  | Winsock IPX transport (`AF_IPX`): **the LAN transport in the shipped build** |
| 174 | `_WSPROTO.CPP` | ra95.exe | Network core | 41 (1) |  |  |  | 1 |  |  |  |  |  |  |  |  |
| 175 | `TOOLTIP.CPP` | ra95.exe | WOL | 297 (201) |  |  |  |  |  |  |  |  |  |  |  | Only used by WOL screens |
| 176 | `WOL_OPT.CPP` | ra95.exe | WOL | 258 (135) | L2 |  | 4 |  |  |  |  |  | whole file |  |  |  |
| 177 | `WOL_CGAM.CPP` | ra95.exe | WOL | 390 (243) | L2 |  | 3 |  |  |  |  |  | whole file |  |  |  |
| 178 | `BIGCHECK.CPP` | ra95.exe | WOL | 85 (32) |  |  |  |  |  |  |  |  | whole file |  |  |  |
| 179 | `WOL_DNLD.CPP` | ra95.exe | WOL | 264 (149) | L2 |  | 3 |  |  |  |  |  | whole file |  |  |  |
| 180 | `WOLSTRNG.CPP` | ra95.exe | WOL | 808 (187) |  |  |  |  |  |  |  |  | whole file |  |  |  |
| 181 | `ROTBMP.CPP` | jshell.lib | Rendering | 416 (243) |  |  |  |  |  |  |  |  |  |  |  |  |
| 182 | `SPRITE.CPP` | jshell.lib | Rendering | 329 (186) |  |  |  |  |  |  |  | 17d |  |  |  |  |
| 183 | `LZO1X_C.CPP` | tech.lib | Compression/crypto | 350 (207) |  |  |  |  |  |  |  |  |  |  |  |  |
| 184 | `LZO1X_D.CPP` | tech.lib | Compression/crypto | 207 (111) |  |  |  |  |  |  |  |  |  |  |  |  |
| 185 | `LZOPIPE.CPP` | tech.lib | Compression/crypto | 322 (123) |  |  |  |  |  |  |  |  |  |  |  |  |
| 186 | `LZOSTRAW.CPP` | tech.lib | Compression/crypto | 185 (65) |  |  |  |  |  |  |  |  |  |  |  |  |
| 187 | `LZW.CPP` | tech.lib | Compression/crypto | 540 (127) |  |  |  |  |  |  |  |  |  |  |  |  |
| 188 | `LZWPIPE.CPP` | tech.lib | Compression/crypto | 316 (113) |  |  |  |  |  |  |  |  |  |  |  |  |
| 189 | `LZWSTRAW.CPP` | tech.lib | Compression/crypto | 181 (59) |  |  |  |  |  |  |  |  |  |  |  |  |
| 190 | `BUFF.CPP` | tech.lib | Core/boot | 221 (59) |  |  |  |  |  |  |  |  |  |  |  |  |
| 191 | `BENCH.CPP` | tech.lib | Core/boot | 168 (36) |  |  |  |  |  |  |  |  |  |  |  |  |
| 192 | `RECT.CPP` | tech.lib | Core/boot | 201 (67) |  |  |  |  |  |  |  |  |  |  |  |  |
| 193 | `MPU.CPP` | tech.lib | Core/boot | 49 (0) |  |  |  |  |  |  |  |  |  |  |  | Compiled empty: only Borland code; real code is `#pragma aux` in MPU.H (rdtsc) |
| 194 | `READLINE.CPP` | tech.lib | File I/O | 90 (43) |  |  |  |  |  |  |  |  |  |  |  |  |
| 195 | `PKPIPE.CPP` | tech.lib | Compression/crypto | 288 (84) |  |  |  |  |  |  |  |  |  |  |  |  |
| 196 | `PKSTRAW.CPP` | tech.lib | Compression/crypto | 305 (81) |  |  |  |  |  |  |  |  |  |  |  |  |
| 197 | `XSTRAW.CPP` | tech.lib | Compression/crypto | 152 (37) |  |  |  |  |  |  |  |  |  |  |  |  |
| 198 | `XPIPE.CPP` | tech.lib | Compression/crypto | 167 (45) |  |  |  |  |  |  |  |  |  |  |  |  |
| 199 | `RNDSTRAW.CPP` | tech.lib | Compression/crypto | 321 (83) |  |  |  |  |  |  |  |  |  |  |  |  |
| 200 | `PK.CPP` | tech.lib | Compression/crypto | 359 (85) |  |  |  |  |  |  |  |  |  |  |  |  |
| 201 | `BLWSTRAW.CPP` | tech.lib | Compression/crypto | 162 (44) |  |  |  |  |  |  |  |  |  |  |  |  |
| 202 | `STRAW.CPP` | tech.lib | Compression/crypto | 142 (34) |  |  |  |  |  |  |  |  |  |  |  |  |
| 203 | `B64STRAW.CPP` | tech.lib | Compression/crypto | 118 (38) |  |  |  |  |  |  |  |  |  |  |  |  |
| 204 | `LCWSTRAW.CPP` | tech.lib | Compression/crypto | 180 (59) |  |  |  |  |  |  |  |  |  |  |  |  |
| 205 | `CRCSTRAW.CPP` | tech.lib | Compression/crypto | 95 (13) |  |  |  |  |  |  |  |  |  |  |  |  |
| 206 | `SHASTRAW.CPP` | tech.lib | Compression/crypto | 97 (13) |  |  |  |  |  |  |  |  |  |  |  |  |
| 207 | `RAMFILE.CPP` | tech.lib | File I/O | 476 (154) |  |  |  |  |  |  |  |  |  |  |  |  |
| 208 | `LCWPIPE.CPP` | tech.lib | Compression/crypto | 314 (113) |  |  |  |  |  |  |  |  |  |  |  |  |
| 209 | `LCWCOMP.ASM` | tech.lib | Compression/crypto | 283 (asm) | | | | |  |  | **asm file** |  | | |  | LCW compressor (S02) |
| 210 | `LCW.CPP` | tech.lib | Compression/crypto | 163 (55) |  |  |  |  |  |  |  |  |  |  |  |  |
| 211 | `CRCPIPE.CPP` | tech.lib | Compression/crypto | 91 (9) |  |  |  |  |  |  |  |  |  |  |  |  |
| 212 | `SHAPIPE.CPP` | tech.lib | Compression/crypto | 91 (9) |  |  |  |  |  |  |  |  |  |  |  |  |
| 213 | `PIPE.CPP` | tech.lib | Compression/crypto | 167 (42) |  |  |  |  |  |  |  |  |  |  |  |  |
| 214 | `BLOWPIPE.CPP` | tech.lib | Compression/crypto | 203 (61) |  |  |  |  |  |  |  |  |  |  |  |  |
| 215 | `B64PIPE.CPP` | tech.lib | Compression/crypto | 166 (71) |  |  |  |  |  |  |  |  |  |  |  |  |
| 216 | `BASE64.CPP` | tech.lib | Compression/crypto | 437 (136) |  |  |  |  |  |  |  |  |  |  |  |  |
| 217 | `FIXED.CPP` | tech.lib | Core/boot | 249 (80) |  |  |  |  |  |  |  |  |  |  |  | 8.8 fixed-point used by rules/simulation |
| 218 | `MIXFILE.CPP` | tech.lib | File I/O | 581 (206) |  |  |  |  |  |  |  | 1i |  |  |  | MIX archive reader; hdrs: direct.h dos.h io.h share.h |
| 219 | `CDFILE.CPP` | tech.lib | File I/O | 721 (143) |  |  |  |  |  |  |  | 2d 1i |  |  |  | CD path search |
| 220 | `BFIOFILE.CPP` | tech.lib | File I/O | 988 (506) |  |  |  |  |  |  |  |  |  |  |  |  |
| 221 | `RAWFILE.CPP` | tech.lib | File I/O | 1349 (300) |  |  | 17 |  |  |  |  | 169d |  |  |  | Win32 `CreateFile`/`ReadFile` file backend; hdrs: direct.h share.h |
| 222 | `MP.CPP` | tech.lib | Compression/crypto | 2733 (1139) |  |  |  |  |  |  |  |  |  |  |  |  |
| 223 | `INT.CPP` | tech.lib | Compression/crypto | 47 (4) |  |  |  |  |  |  |  |  |  |  |  |  |
| 224 | `MONOC.CPP` | tech.lib | Core/boot | 1133 (430) |  |  |  |  |  |  | 1 | 1i |  |  |  | Monochrome debug screen; `#pragma aux` port output; hdrs: dos.h mem.h |
| 225 | `RANDOM.CPP` | tech.lib | Core/boot | 184 (29) |  |  |  |  |  |  |  |  |  |  |  | Deterministic LCG `RandomClass` (Scen.RandomNumber) |
| 226 | `RGB.CPP` | tech.lib | Rendering | 250 (74) |  |  |  |  |  |  |  |  |  |  |  |  |
| 227 | `HSV.CPP` | tech.lib | Rendering | 212 (55) |  |  |  |  |  |  |  |  |  |  |  |  |
| 228 | `PALETTE.CPP` | tech.lib | Rendering | missing | | | | | | | | | | |  | **Source missing** (PaletteClass) |
| 229 | `BLOWFISH.CPP` | tech.lib | Compression/crypto | 599 (290) |  |  |  |  |  |  |  |  |  |  |  |  |
| 230 | `SHA.CPP` | tech.lib | Compression/crypto | 318 (94) |  |  |  |  |  |  |  |  |  |  |  |  |
| 231 | `CRC.CPP` | tech.lib | Compression/crypto | 137 (34) |  |  |  |  |  |  |  |  |  |  |  |  |
| 232 | `SENDFILE.CPP` | tech.lib | Network core | 880 (420) | L4 |  | 6 |  | 14 |  |  |  | A11 g16 |  |  | Scenario download from host (LAN + WOL) |
| 233 | `2KEYFBUF.ASM` | ra95.exe (Win32-only) | Rendering | 4848 (asm) | | | | |  |  | **asm file** |  | | |  | Shape drawing incl. MMX (S02) |
| 234 | `CPUID.ASM` | ra95.exe (Win32-only) | Win32 platform | 185 (asm) | | | | |  |  | **asm file** |  | | |  | MMX detection |
| 235 | `GETCPU.CPP` | ra95.exe (Win32-only) | Win32 platform | 105 (12) |  |  |  |  |  |  |  |  |  |  |  | CPU/MMX detection wrapper |
| 236 | `INTERPAL.CPP` | ra95.exe (Win32-only) | Rendering | 465 (187) | L1 |  |  |  |  |  |  |  |  |  |  | 2x palette interpolation (asm in WINASM.ASM) |
| 237 | `WINASM.ASM` | ra95.exe (Win32-only) | Win32 platform | 890 (asm) | | | | |  | stubs | **asm file** |  | | |  | Greenleaf **stubs**, ModeX blit, interpolation asm |
| 238 | `WINSTUB.CPP` | ra95.exe (Win32-only) | Win32 platform | 963 (343) | L4 |  | 36 | 3 |  |  |  |  |  | 4 |  | Window class + WndProc, focus loss, memory error handler |
| 239 | `2TXTPRNT.ASM` | ra95.exe (Win32-only) | Rendering | 507 (asm) | | | | |  |  | **asm file** |  | | |  | Font blitter (S02) |
| 240 | `WRITEPCX.CPP` | ra95.exe (Win32-only) | Rendering | 229 (94) |  |  |  |  |  |  |  |  |  |  |  |  |
| 241 | `IPX95.CPP` | ra95.exe (Win32-only) | Network core | 202 (83) |  |  | 4 |  | 24 |  |  |  |  |  |  | THIPX32.DLL loader; unused when `WINSOCK_IPX` (callers compiled out) |
| 242 | `2KEYFRAM.CPP` | ra95.exe (Win32-only) | Rendering | 584 (270) |  |  | 1 |  |  |  |  |  |  |  |  |  |
| 243 | `TCPIP.CPP` | ra95.exe (Win32-only) | Internet (pre-WOL) | 910 (381) |  |  | 14 | 86 |  |  |  |  |  |  |  | Pre-WOL WChat TCP manager; no live callers outside itself |
| 244 | `INTERNET.CPP` | ra95.exe (Win32-only) | Internet (pre-WOL) | 963 (69) |  |  | 10 |  |  |  |  |  |  |  |  | Pre-WOL WChat glue; 356 lines compiled out by `WOLAPI_INTEGRATION` |
| 245 | `DDE.CPP` | ra95.exe (Win32-only) | Internet (pre-WOL) | 453 (175) |  |  | 3 |  |  |  |  |  |  |  |  |  |
| 246 | `CCDDE.CPP` | ra95.exe (Win32-only) | Internet (pre-WOL) | 432 (131) |  |  | 1 | 5 |  |  |  |  |  |  |  | DDE server; global ctor sets `RA95AlreadyRunning` (single-instance check) |
| 247 | `STATS.CPP` | ra95.exe (Win32-only) | Internet (pre-WOL) | 913 (394) | D2 |  | 5 | 6 |  |  |  |  | A15 g41 |  |  | Internet game statistics to Westwood server |
| 248 | `PACKET.CPP` | ra95.exe (Win32-only) | Internet (pre-WOL) | 464 (147) |  |  |  | 9 |  |  |  |  |  |  |  | hdrs: mem.h |
| 249 | `KEY.CPP` | ra95.exe (Win32-only) | Win32 platform | 759 (258) |  |  | 26 |  |  |  |  |  |  |  |  | WWKeyboardClass: Win32 message-driven keyboard/mouse buffer |
| 250 | `FIELD.CPP` | ra95.exe (Win32-only) | Internet (pre-WOL) | 215 (116) |  |  |  | 8 |  |  |  |  |  |  |  |  |
| 251 | `MAPEDSEL.CPP` | via `#include` in MAPEDIT.CPP | Editor/debug | 589 (0) |  |  |  |  |  |  |  |  |  |  |  | Fully compiled out; `#include`d by MAPEDIT.CPP |
| 252 | `WOLAPI/WOLAPI_I.C` | via `#include` in RAWOLAPI.CPP | WOL | 108 (26) |  |  |  |  |  |  |  |  |  |  |  | COM interface IDs, pulled in by RAWOLAPI.CPP |

<!-- subsystem counts: Simulation=52, UI=37, Compression/crypto=33, WOL=20, Network core=18, Core/boot=16, Sim: type data=13, File I/O=13, Rendering=12, Internet (pre-WOL)=8, Win32 platform=7, Campaign=6, Editor/debug=6, Multiplayer UI=3, Modem/serial (+skirmish UI)=3, Movies=3, Audio=2; total=252 -->

## Files in `CODE/` that are **not** in the Win32 build

These 41 sources are in `CODE/` but the Win32 makefile never compiles them (34 `.CPP`, 6 `.ASM`, 1 `.C`). Most are the DOS build,
abandoned experiments or the TEN/MPath online services. S05 can delete them without affecting the Win32 build.

| Group | Files |
|-------|-------|
| DOS-build alternatives of Win32 files (`!else` branch, `MAKEFILE:421-424`) | `KEYFBUFF.ASM`, `TXTPRNT.ASM`, `KEYFRAME.CPP` |
| DOS-only helpers | `DPMI.CPP` (`#pragma aux`, inline asm), `IPXREAL.ASM` (real-mode IPX, built by a special rule `MAKEFILE:716-726`), `CWSTUB.C` (DOS extender stub), `SUPPORT.ASM`, `2SUPPORT.ASM`, `COORDA.ASM`, `KEYBOARD.CPP` |
| TEN / MPath online services (`TEN=0`, `MPATH=0`) | `CCTEN.CPP`, `TENMGR.CPP`, `CCMPATH.CPP`, `MPLIB.CPP`, `MPLPC.CPP`, `MPMGRD.CPP`, `MPMGRW.CPP` |
| Old or alternative connection code | `SEQCONN.CPP`, `NOSEQCON.CPP`, `COMQUEUE.CPP` |
| Not referenced / experiments | `ADPCM.CPP` (+ `ITABLE.CPP`, `DTABLE.CPP` that it `#include`s), `ALLOC.CPP`, `BMP8.CPP`, `CLASS.CPP`, `CONFDLG.CPP`, `CSTRAW.CPP`, `DESCDLG.CPP`, `FILE.CPP`, `INIBIN.CPP`, `INICODE.CPP`, `LCWUNCMP.CPP`, `LZWOTRAW.CPP`, `NUMBER.CPP`, `RAND.CPP`, `STUB.CPP`, `SURFACE.CPP`, `TARCOM.CPP`, `TEMP.CPP`, `TURRET.CPP` |

`MAPEDSEL.CPP` is not in the makefile, but it is compiled through `#include` from `MAPEDIT.CPP:2172` (and is then compiled out by
`SCENARIO_EDITOR`). The "not in build" status was derived mechanically. The grouping is by hand, from file headers and
names. Confirming that *every* one of them is unreferenced is S05's job.

## Appendix: scripts

The scripts live here, in the page, because S01 only owns documentation files. S03 may move them to a `tools/` directory.
They need only Python 3 and its standard library.

<details>
<summary><code>scan_deps.py</code>: build list, tri-state preprocessor, dependency patterns</summary>

```python
#!/usr/bin/env python3
"""
S01 dependency scanner for the Red Alert Win32 build.

Usage:  python3 -I scan_deps.py <repo root> <out.tsv>

1. Parses CODE/MAKEFILE: OBJECTS + the '!ifdef WIN32' OBJECTS += block + LIBFILES (jshell.lib)
   + TECHFILES (tech.lib) evaluated with WIN32 defined and GERMAN/FRENCH undefined.
2. Adds sources pulled in with #include "x.cpp"/"x.c" from a build file.
3. For every source, runs a small tri-state preprocessor (True / False / unknown) using the
   Win32 release configuration (makefile /D flags + CODE/DEFINES.H), so code in `#ifndef WIN32`
   or `#else // WIN32` branches is recognised as dead in the Win32 build.
4. Greps comment-stripped, not-dead lines for each dependency pattern.
"""
import os, re, sys

ROOT = sys.argv[1] if len(sys.argv) > 1 else '.'
CODE = os.path.join(ROOT, 'CODE')
OUT = sys.argv[2] if len(sys.argv) > 2 else None

# ---------------------------------------------------------------- build list
def makefile_objects():
    lines = open(os.path.join(CODE, 'MAKEFILE'), encoding='latin-1').read().replace('\r', '').split('\n')
    groups, cur, cond = [], None, []
    def active():
        for name, br in cond:
            v = (name == 'WIN32')
            if br == 'else':
                v = not v
            if not v:
                return False
        return True
    for ln in lines:
        s = ln.strip()
        m = re.match(r'!ifdef\s+(\w+)', s)
        if m: cond.append([m.group(1), 'if']); continue
        if s.startswith('!else'): cond[-1][1] = 'else'; continue
        if s.startswith('!endif'): cond.pop(); continue
        m = re.match(r'(OBJECTS|LIBFILES|TECHFILES)\s*(\+?=)\s*(.*)', s)
        if m:
            cur = {'OBJECTS': 'ra95.exe', 'LIBFILES': 'jshell.lib', 'TECHFILES': 'tech.lib'}[m.group(1)]
            if m.group(2) == '+=':
                cur += ' (WIN32 block)'
            if not active():
                cur = None
            rest = m.group(3)
        elif cur is not None:
            rest = s
        else:
            continue
        if cur:
            for tok in re.findall(r'(\w+)\.OBJ', rest, re.I):
                groups.append((cur, tok.upper()))
        if not rest.endswith('&'):
            cur = None
    return groups

# ---------------------------------------------------------------- configuration
# Win32 release build: makefile CC_CFG + CODE/DEFINES.H (RELEASE_VERSION) + Watcom /bt=NT predefines.
DEFINED = {
    'WIN32': 1, '_WIN32': 1, 'WOLAPI_INTEGRATION': 1, 'WINSOCK_IPX': 1, 'ENGLISH': 1,
    '__WATCOMC__': 1100, '__386__': 1, '__NT__': 1, '__FLAT__': 1, '__cplusplus': 1,
    'RELEASE_VERSION': 1, 'MOVIE640': 1, 'NETWORK': 1, 'TIMING_FIX': 1, 'MPATH': 0, 'TEN': 0,
    'FIXIT_SCORE_CRASH': 1, 'FIXIT_MULTI_SAVE': 1, 'FIXIT_NO_COMP_ALLY': 1, 'FIXIT_DESTNET': 1,
    'FIXIT_RANDOM_GAME': 1, 'FIXIT_FORCE_CD': 1, 'FIXIT_IP_CRASH': 1, 'FIXIT_IP_STATS': 1,
    'FIXIT_NAME_OVERRIDE': 1, 'FIXIT_RADAR_JAMMED': 1, 'FIXIT_CAPTURE_BIB': 1, 'FIXIT_BASE_ANNOUNCE': 1,
    'FIXIT_APTIVA_MODEM': 1, 'FIXIT_FLAG_CHECK': 1, 'FIXIT_ANTS': 1, 'FIXIT_CSII': 1, 'FIXIT_CARRIER': 1,
    'FIXIT_PHASETRANSPORT': 1, 'FIXIT_ENGINEER': 1, 'FIXIT_PATCH_108': 1, 'STEVES_LOAD_OVERRIDE': 1,
    'FIXIT_DIFFICULTY': 1, 'FIXIT_VERSION': 1, 'FIXIT_MODEM_LOAD_CRASH': 1, 'FIXIT_PHONELIST_CRASH': 1,
    'FIXIT_VERSION_3': 1, 'DVD': 1, 'MPEGMOVIE': 1, 'PARTIAL': 1, 'RESFACTOR': 2, 'NDEBUG': 1,
}
UNDEFINED = {
    'GERMAN', 'FRENCH', 'SPAIN', 'DOS4G', 'GF_WATCOM_S', 'INTERNAL_VERSION', 'PLAYTEST_VERSION',
    'SCENARIO_EDITOR', 'CHEAT_KEYS', 'VIRGIN_CHEAT_KEYS', 'MIRROR_QUEUE', 'DEV_VERSION', 'INTERNET_OFF',
    'MCIMPEG', 'SORTDRAW', 'DONGLE', 'BOGUSCD', 'FIXIT_FAST_LOAD', 'FIXIT_ENGINEER_CAPTURE',
    'FIXIT_HELI_LANDING', 'FIXIT_MINE_PASSABLE', 'NEVER', 'OBSOLETE', 'TOFIX', '__BORLANDC__', '_MSC_VER',
    '__GNUC__', '__TURBOC__', 'BIG_ENDIAN', 'RANDOM_COUNT', 'DENZIL_MIXEXTRACT', 'DEV_VER_NAME',
    'MSDOS', '__MSDOS__', '__DPMI16__', '__DPMI32__', 'FOREIGN_VERSION', 'OLDWAY', 'OLD',
}
PLATFORM = {'WIN32', '_WIN32', 'DOS4G', '__NT__', '_Windows', 'WINDOWS', '_WINDOWS', 'MSDOS', '__MSDOS__',
            '__DPMI32__', '__DPMI16__', 'WWLIB32_H'}

# ---------------------------------------------------------------- tri-state #if evaluator
TOK = re.compile(r'\s*(defined|0[xX][0-9a-fA-F]+[uUlL]*|\d+[uUlL]*|[A-Za-z_]\w*|&&|\|\||==|!=|<=|>=|<<|>>|[!()<>+\-*/%&|^~?:])')

class Expr:
    def __init__(self, text, macros):
        self.toks = []
        pos = 0
        text = text.strip()
        while pos < len(text):
            m = TOK.match(text, pos)
            if not m:
                self.toks = None
                return
            self.toks.append(m.group(1)); pos = m.end()
            if not text[pos:].strip():
                break
        self.i = 0; self.macros = macros; self.names = set()
    def peek(self): return self.toks[self.i] if self.i < len(self.toks) else None
    def take(self): t = self.peek(); self.i += 1; return t
    def value(self):
        if self.toks is None: return None
        try:
            v = self.orx()
            return v
        except Exception:
            return None
    # precedence: || , && , ==/!= , relational , additive , unary/primary  (enough for this code base)
    def orx(self):
        v = self.andx()
        while self.peek() == '||':
            self.take(); w = self.andx()
            v = True if (v is True or w is True) else (False if (v is False and w is False) else None)
            v = None if v is None else int(v)
        return v
    def andx(self):
        v = self.eqx()
        while self.peek() == '&&':
            self.take(); w = self.eqx()
            if v == 0 or w == 0: v = 0
            elif v is None or w is None: v = None
            else: v = 1
        return v
    def eqx(self):
        v = self.relx()
        while self.peek() in ('==', '!='):
            op = self.take(); w = self.relx()
            v = None if (v is None or w is None) else int((v == w) if op == '==' else (v != w))
        return v
    def relx(self):
        v = self.addx()
        while self.peek() in ('<', '>', '<=', '>='):
            op = self.take(); w = self.addx()
            v = None if (v is None or w is None) else int(eval(f'{v}{op}{w}'))
        return v
    def addx(self):
        v = self.unary()
        while self.peek() in ('+', '-', '|', '&', '*'):
            op = self.take(); w = self.unary()
            v = None if (v is None or w is None) else eval(f'{v}{op}{w}')
        return v
    def unary(self):
        t = self.take()
        if t == '!':
            v = self.unary(); return None if v is None else int(not v)
        if t == '(':
            v = self.orx(); self.take(); return v
        if t == 'defined':
            par = self.peek() == '('
            if par: self.take()
            n = self.take()
            if par: self.take()
            self.names.add(n)
            if n in self.macros: return 1
            if n in UNDEFINED: return 0
            return None
        if re.match(r'\d|0[xX]', t):
            return int(re.sub(r'[uUlL]+$', '', t), 0)
        self.names.add(t)
        if t in self.macros:
            v = self.macros[t]
            return v if isinstance(v, int) else None
        if t in UNDEFINED: return 0
        return None

def strip_comments(src):
    """Remove /* */ and // comments, keeping line structure."""
    out, i, n, in_block = [], 0, len(src), False
    res = []
    while i < n:
        if in_block:
            j = src.find('*/', i)
            if j < 0:
                res.append(re.sub(r'[^\n]', '', src[i:])); break
            res.append(re.sub(r'[^\n]', '', src[i:j])); i = j + 2; in_block = False; continue
        c = src[i]
        if c == '"' or c == "'":
            j = i + 1
            while j < n and src[j] != c and src[j] != '\n':
                j += 2 if src[j] == '\\' else 1
            res.append(src[i:j + 1]); i = j + 1; continue
        if src.startswith('/*', i): in_block = True; i += 2; continue
        if src.startswith('//', i):
            j = src.find('\n', i); j = n if j < 0 else j; i = j; continue
        res.append(c); i += 1
    return ''.join(res)

def analyse_lines(text):
    """Return list of (line, state, reason_names, active_names) per line.
    state: True = compiled in Win32 build, False = dead, None = depends on something unknown."""
    macros = dict(DEFINED)
    saved_undefined = set(UNDEFINED)   # restored at the end: #define in one file must not leak into the next
    stack = []   # frames: dict(val, taken, names)
    result = []
    def cur_state():
        st = True
        for f in stack:
            if f['val'] is False or f['val'] == 0: return False
            if f['val'] is None: st = None
        return st
    def dead_reason():
        for f in stack:
            if f['val'] is False or f['val'] == 0: return f['names']
        return set()
    def active_names():
        s = set()
        for f in stack:
            if f['val'] not in (False, 0, None): s |= f['names']
        return s
    lines = text.split('\n')
    k = 0
    while k < len(lines):
        ln = lines[k]
        # join continuation lines for directives
        full = ln
        joined = 0
        while full.rstrip().endswith('\\') and k + 1 < len(lines):
            k += 1; joined += 1; full = full.rstrip()[:-1] + ' ' + lines[k]
        s = full.strip()
        m = re.match(r'#\s*(\w+)\s*(.*)', s)
        if m:
            d, arg = m.group(1), m.group(2).strip()
            parent_live = cur_state() is not False
            if d in ('ifdef', 'ifndef'):
                n = arg.split()[0] if arg else ''
                v = 1 if n in macros else (0 if n in UNDEFINED else None)
                if d == 'ifndef' and v is not None: v = int(not v)
                stack.append({'val': v, 'taken': v, 'names': {n}})
            elif d == 'if':
                e = Expr(arg, macros); v = e.value()
                stack.append({'val': v, 'taken': v, 'names': e.names if e.toks else set()})
            elif d == 'elif' and stack:
                f = stack[-1]
                e = Expr(arg, macros); v = e.value()
                if f['taken'] not in (0, False, None): nv = 0
                elif f['taken'] is None: nv = None if v != 0 else 0
                else: nv = v
                f['names'] = f['names'] | (e.names if e.toks else set())
                f['val'] = nv
                if nv not in (0, False, None): f['taken'] = nv
                elif nv is None and f['taken'] in (0, False): f['taken'] = None
            elif d == 'else' and stack:
                f = stack[-1]
                t = f['taken']
                f['val'] = None if t is None else int(not t)
            elif d == 'endif' and stack:
                stack.pop()
            elif d == 'define' and cur_state() is True:
                mm = re.match(r'(\w+)(\(.*?\))?\s*(.*)', arg)
                if mm and not mm.group(2):
                    val = mm.group(3).strip()
                    try: macros[mm.group(1)] = int(val.split()[0].strip('()'), 0) if val else 1
                    except Exception: macros[mm.group(1)] = 1
                    UNDEFINED.discard(mm.group(1))
            elif d == 'undef' and cur_state() is True:
                n = arg.split()[0] if arg else ''
                macros.pop(n, None)
            result.append((full, 'directive' if (parent_live and cur_state() is not False) or d in ('if', 'ifdef', 'ifndef', 'elif', 'else', 'endif') else 'deaddirective', set(), set()))
            result.extend([('', 'directive', set(), set())] * joined)   # keep line numbers aligned
        else:
            result.append((ln, cur_state(), dead_reason(), active_names()))
        k += 1
    UNDEFINED.clear(); UNDEFINED.update(saved_undefined)
    return result

# ---------------------------------------------------------------- dependency patterns
P = lambda s: re.compile(s)
PATTERNS = {
    'DirectDraw': P(r'\b(LPDIRECTDRAW\w*|IDirectDraw\w*|DirectDraw\w*|DDSURFACEDESC|DDSCAPS\w*|DDERR_\w+|DD_OK|DDLOCK_\w+|DDSCL_\w+|DDBLT\w*|PaletteSurface)\b|ddraw\.h'),
    'DD via WIN32LIB': P(r'\b(GBC_VIDEOMEM|GBC_VISIBLE|Get_DD_Surface|Set_Video_Mode|Reset_Video_Mode|Get_IsDirectDisplay|Set_DD_Palette|Allow_Hardware_Blits|AllSurfaces|Prep_Direct_Draw|Wait_Blit)\b'),
    'DirectSound': P(r'\b(LPDIRECTSOUND\w*|IDirectSound\w*|DirectSound\w*|DSBUFFERDESC|DSBCAPS_\w+|DSSCL_\w+|DS_OK|DSERR_\w+|SoundObject|PrimaryBufferPtr)\b|dsound\.h'),
    'Win32 API': P(r'\b(HWND|HINSTANCE|HANDLE|HDC|HKEY|HRESULT|LRESULT|WPARAM|LPARAM|MSG|WM_[A-Z_]+|PeekMessage|GetMessage|DispatchMessage|TranslateMessage|SendMessage|PostMessage|MessageBox|CreateWindow\w*|ShowWindow|UpdateWindow|SetForegroundWindow|SetFocus|GetTickCount|timeGetTime|timeSetEvent|timeKillEvent|timeBeginPeriod|QueryPerformance\w+|Sleep|CreateFile\w*|ReadFile|WriteFile|CloseHandle|FindFirstFile\w*|FindNextFile\w*|GetDriveType\w*|GetLogicalDrives|GetVolumeInformation\w*|GetDiskFreeSpace\w*|LoadLibrary\w*|GetProcAddress|FreeLibrary|Reg[A-Z]\w+|FindWindow\w*|FindClose|GetSystemTime|GetLocalTime|SYSTEMTIME|WIN32_FIND_DATA|INVALID_HANDLE_VALUE|CreateMutex\w*|CreateEvent\w*|SetEvent|WaitForSingleObject|CreateThread|_beginthread\w*|GetModuleFileName\w*|ShellExecute\w*|WinExec|CreateProcess\w*|GlobalAlloc|GlobalMemoryStatus|VirtualAlloc|VirtualLock|GetDC|ReleaseDC|SetCursor|ShowCursor|ClipCursor|GetCursorPos|SetCursorPos|GetAsyncKeyState|GetKeyState|MainWindow|ProgramInstance|OutputDebugString\w*|GetLastError|CoInitialize|CoCreateInstance|CoUninitialize|IUnknown|DdeInitialize\w*|GetSystemMetrics|SetWindowPos|GetWindowRect|SetTimer|KillTimer|DefWindowProc|CallWindowProc|GetFileAttributes\w*|SetCurrentDirectory\w*|GetCurrentDirectory\w*|CreateDirectory\w*|DeleteFile\w*|MoveFile\w*|CopyFile\w*|GetPrivateProfile\w*|WritePrivateProfile\w*|GetVersionEx\w*|lstrcpy\w*|wsprintf\w*|SystemParametersInfo\w*|InvalidateRect|BeginPaint|EndPaint|GetClientRect|BitBlt|StretchDIBits|SetDIBitsToDevice|CreateDIBSection|SelectObject|DeleteObject|LoadImage\w*|LoadIcon\w*|LoadCursor\w*|GetStockObject|CreateFont\w*|TextOut\w*|SetTextColor|SetBkMode|GetTextExtent\w*|CreateCompatibleDC|DeleteDC|DIB\w*|BITMAPINFO\w*)\b|<windows\.h>|<windowsx\.h>|<mmsystem\.h>|<objbase\.h>'),
    'Winsock': P(r'\b(WSA[A-Z]\w*|SOCKET|INVALID_SOCKET|SOCKET_ERROR|sockaddr\w*|SOCKADDR\w*|gethostbyname|gethostbyaddr|gethostname|inet_addr|inet_ntoa|htons|htonl|ntohs|ntohl|closesocket|recvfrom|sendto|ioctlsocket|setsockopt|getsockopt|INADDR_\w+|AF_INET|AF_IPX|NSPROTO_IPX|hostent|in_addr|FD_READ|FD_WRITE|PacketTransport|WinsockInterfaceClass|UDPInterfaceClass|IPXInterfaceClass)\b|winsock\.h|wsipx\.h'),
    'IPX': P(r'\b(IPXManagerClass|IPXConnClass|IPXGlobalConnClass|IPXAddressClass|IPXInterfaceClass|Ipx|IPX_[A-Z]\w*|IPXHeaderType|ECBType|IPXProtocol\w*|Get_IPX\w*|AF_IPX|NSPROTO_IPX|SOCKADDR_IPX|Load_IPX\w*|Unload_IPX\w*|_IPX_\w*)\b|ipx95\.h|wsipx\.h|"ipx\.h"'),
    'Greenleaf': P(r'\b(PortOpenGreenleaf\w*|HM[A-Z][a-zA-Z]+|PortClose|PortKillTime|ReadBuffer|WriteBuffer|SpaceUsedInTXBuffer|FreeSpaceInTXBuffer|ClearTXBuffer|ClearRXBuffer|UseRtsCts|UseDtrDsr|UseXonXoff|SetDtr|SetRts|GetModemStatus|GetLineStatus|ClearLineStatus|ReadChar|WriteChar|ASSUCCESS|ASBUF\w+|ASGENERALERROR|PORT|IdleCallback|Change\w*Parameters|SetFossil\w*)\b|<modem\.h>|<fast\.h>|commlib\.h|gcl\.h'),
    'Serial (WinModem)': P(r'\b(WinModemClass|SerialPort|Serial_Port_\w+|Dial_Modem|Get_Modem_\w+|Set_Modem_\w+|Send_Command_To_Modem|wincomm\.h|modemreg\.h|ModemRegistryEntryClass)\b|wincomm\.h|modemreg\.h'),
    'Watcom pragma/asm': P(r'#\s*pragma\s+aux\b|\b_asm\b|\b__asm\b|^\s*asm\s*\{'),
    'DOS-isms': P(r'\b(int386\w*|intdos\w*|_dos_\w+|union\s+REGS|struct\s+SREGS|REGS|SREGS|segread|Real_Mode\w*|DPMI\w*|_disable|_enable|outp\w?|inp\w?|_bios_\w+|bios\w+|_harderr|Set_Video_Mode\s*\(\s*0x|VESA\w*|_far|__far|_near|__near|Watcom\w*|MK_FP|FP_SEG|FP_OFF|kbhit|getch|_heapmin|_dpmi\w*|0xB0000|0xA0000)\b|<dos\.h>|<i86\.h>|<conio\.h>|<bios\.h>'),
    'WOL': P(r'\b(pWolapi|WolapiObject|Wolapi\w*|WOL_\w+|WOL::\w+|IChat\w*|INetUtil\w*|IDownload\w*|IRTPatcher\w*|IGroup\w*|C\w+EventSink|RAWOLAPI|WOLAPI\w*|WOLGameInfo|wolapi\.h|wolapi\\\\?\w+)\b|wolapi'),
    'Movie (VQA/MPEG)': P(r'\b(VQA_\w+|VQAHandle|VQAConfig|Play_Movie|MpgPlay\w*|MPG\w+|MCIMovie\w*|MciMovie\w*|MPGSettings\w*|amstream|IAMMultiMediaStream|IMultiMediaStream|Play_MPEG\w*|MCIMPEG)\b|mcimovie\.h|movie\.h|mpgset\.h'),
    'Non-std headers': P(r'#\s*include\s*<(dos|i86|mem|conio|bios|direct|io|malloc|process|share|dir|env)\.h>'),
}
STR = re.compile(r'"(?:[^"\\]|\\.)*"')
NONSTD = re.compile(r'#\s*include\s*<(dos|i86|mem|conio|bios|direct|io|malloc|process|share|dir|env)\.h>', re.I)

def scan(path):
    raw = open(path, encoding='latin-1').read().replace('\r', '')
    text = strip_comments(raw)
    per = analyse_lines(text)
    hits = {k: 0 for k in PATTERNS}
    first = {k: None for k in PATTERNS}
    dos_dead = wol_dead = feat_dead = other_dead = live = wol_live = unk = 0
    headers = set()
    for idx, (ln, st, reason, act) in enumerate(per, 1):
        if not ln.strip():
            continue
        if st == 'deaddirective':
            continue
        if st == 'directive':
            if re.match(r'\s*#\s*(if|ifdef|ifndef|elif|else|endif)\b', ln):
                continue
            if NONSTD.search(ln):
                headers.add(NONSTD.search(ln).group(1).lower() + '.h')
            # count directive-level patterns (#pragma aux, #include <...>)
            for k, rx in PATTERNS.items():
                if k in ('Watcom pragma/asm', 'DirectDraw', 'DirectSound', 'Win32 API', 'Winsock', 'IPX', 'Greenleaf', 'Serial (WinModem)', 'WOL', 'Movie (VQA/MPEG)', 'DOS-isms') and rx.search(ln):
                    hits[k] += 1; first[k] = first[k] or idx
            continue
        if st is False:
            if reason & PLATFORM: dos_dead += 1
            elif 'WOLAPI_INTEGRATION' in reason: wol_dead += 1
            elif reason & {'SCENARIO_EDITOR', 'CHEAT_KEYS', 'VIRGIN_CHEAT_KEYS', 'MIRROR_QUEUE'}: feat_dead += 1
            else: other_dead += 1
            continue
        if st is None: unk += 1
        live += 1
        ln = STR.sub('""', ln)
        if 'WOLAPI_INTEGRATION' in act: wol_live += 1
        for k, rx in PATTERNS.items():
            if k == 'Non-std headers': continue
            if rx.search(ln):
                hits[k] += 1; first[k] = first[k] or idx
    return dict(hits=hits, first=first, dos_dead=dos_dead, wol_dead=wol_dead, feat_dead=feat_dead,
                other_dead=other_dead, live=live, wol_live=wol_live, unknown=unk, headers=sorted(headers),
                total=raw.count('\n') + 1)

def main():
    files = {f.upper(): f for f in os.listdir(CODE)}
    rows, seen = [], set()
    for grp, obj in makefile_objects():
        if obj in seen:
            continue
        seen.add(obj)
        src = next((files[obj + e] for e in ('.CPP', '.ASM', '.C') if obj + e in files), None)
        rows.append((grp, obj, src))
    # sources #included as .cpp/.c from build files
    extra = []
    for grp, obj, src in rows:
        if src and src.upper().endswith('.CPP'):
            for inc in re.findall(r'#\s*include\s*"([^"]+\.(?:cpp|c))"', open(os.path.join(CODE, src), encoding='latin-1').read(), re.I):
                extra.append(('#included by ' + src, inc.replace('\\', '/'), inc.replace('\\', '/')))
    with open(OUT, 'w') as o:
        cols = list(PATTERNS)
        o.write('\t'.join(['group', 'object', 'source', 'lines', 'live', 'dos_dead', 'wol_dead', 'feat_dead', 'other_dead', 'wol_live', 'unknown'] + cols + ['first_lines', 'headers']) + '\n')
        for grp, obj, src in rows + extra:
            path = None
            if src:
                for cand in (os.path.join(CODE, src), os.path.join(CODE, src.upper()), os.path.join(CODE, *src.upper().split('/'))):
                    if os.path.exists(cand): path = cand; break
            if not path:
                o.write(f'{grp}\t{obj}\tMISSING\n'); continue
            rel = os.path.relpath(path, CODE)
            if rel.upper().endswith('.ASM'):
                o.write(f'{grp}\t{obj}\t{rel}\t{open(path, encoding="latin-1").read().count(chr(10))}\tASM\n'); continue
            r = scan(path)
            o.write('\t'.join([grp, obj, rel, str(r['total']), str(r['live']), str(r['dos_dead']), str(r['wol_dead']), str(r['feat_dead']), str(r['other_dead']), str(r['wol_live']), str(r['unknown'])]
                              + [str(r['hits'][k]) for k in cols]
                              + [';'.join(f'{k}@{r["first"][k]}' for k in cols if r['first'][k]), ','.join(r['headers'])]) + '\n')

def show(fname, cat):
    """Debug: print live lines in CODE/<fname> matching category <cat>."""
    text = strip_comments(open(os.path.join(CODE, fname), encoding='latin-1').read().replace('\r', ''))
    for idx, (ln, st, reason, act) in enumerate(analyse_lines(text), 1):
        if st == 'deaddirective' or (st == 'directive' and re.match(r'\s*#\s*(if|ifdef|ifndef|elif|else|endif)\b', ln)):
            continue
        if st is not False and st != 'directive':
            ln = STR.sub('""', ln)
        if st is not False and PATTERNS[cat].search(ln):
            print(f'{fname}:{idx}: [{st}] {ln.strip()[:120]}')

if __name__ == '__main__':
    if OUT == '--show':
        show(sys.argv[3], sys.argv[4])
    else:
        main()
```

</details>

<details>
<summary><code>gen_table.py</code>: subsystem map and markdown table</summary>

```python
#!/usr/bin/env python3
"""Turn deps.tsv (from scan_deps.py) + the hand-made subsystem map below into the markdown table
used in docs/codebase/platform-dependencies.md.  Usage: gen_table.py deps.tsv > table.md"""
import csv, sys

# ---- hand classification (S01). Subsystem codes are explained in the doc.
SUB = {}
def put(code, names):
    for n in names.split():
        SUB[n] = code
put('SIM', 'ABSTRACT AIRCRAFT ANIM BASE BUILDING BULLET CARGO CELL COMBAT COORD CRATE CREW DOOR DRIVE FACE FACING '
           'FACTORY FINDPATH FLASHER FLY FOOT FUSE HOUSE INFANTRY LAYER LOGIC MAP MISSION OBJECT OVERLAY RADIO REINF '
           'SMUDGE SUPER TARGET TEAM TECHNO TEMPLATE TERRAIN UNIT VESSEL VORTEX WARHEAD WEAPON TRIGGER TRIGTYPE '
           'TEVENT TACTION TEAMTYPE SCENARIO RULES TRACKER')
put('DATA', 'AADATA ADATA BBDATA BDATA CDATA HDATA IDATA ODATA SDATA TDATA UDATA VDATA CONST')
put('NET', 'QUEUE EVENT SESSION CONNECT COMBUF IPXMGR IPXCONN IPXGCONN IPXADDR UDPADDR WSPROTO WSPUDP WSPIPX '
           '_WSPROTO SENDFILE IPX IPX95 IPXPROT')
put('MPUI', 'NETDLG MPLAYER MSGLIST')
put('MODEM', 'NULLCONN NULLMGR NULLDLG')
put('INET', 'TCPIP INTERNET CCDDE DDE STATS PACKET FIELD UTRACKER')
put('WOL', 'WOL_MAIN WOL_CHAT RAWOLAPI WOLAPIOB WOL_LOGN WOLEDIT PASSEDIT SEDITDLG WOL_GSUP WOL_OPT WOL_CGAM '
           'BIGCHECK WOL_DNLD WOLSTRNG ICONLIST TOOLTIP DIBFILE DIBUTIL COMINIT WOLAPI_I')
put('UI', 'BAR CHECKBOX CHEKLIST COLRLIST CONTROL CREDITS DIAL8 DIALOG DROP EDIT GADGET GAMEDLG GAUGE GOPTIONS HELP '
          'LINK LIST LOADDLG MENUS MSGBOX OPTIONS SCROLL SHAPEBTN SIDEBAR SLIDER SOUNDDLG SPECIAL STATBTN TAB TEXTBTN '
          'TOGGLE TXTLABEL VISUDLG POWER EGOS RADAR MOUSE')
put('REND', 'DISPLAY GSCREEN ROTBMP SPRITE INTERPAL 2KEYFBUF 2TXTPRNT 2KEYFRAM WRITEPCX RGB HSV PALETTE')
put('AUD', 'AUDIO THEME')
put('FILE', 'CCFILE CCINI INI IOMAP IOOBJ SAVELOAD PROFILE MIXFILE CDFILE BFIOFILE RAWFILE RAMFILE READLINE')
put('COMP', 'LZO1X_C LZO1X_D LZOPIPE LZOSTRAW LZW LZWPIPE LZWSTRAW LCWSTRAW LCWPIPE LCWCOMP LCW PKPIPE PKSTRAW PK '
            'XSTRAW XPIPE RNDSTRAW BLWSTRAW STRAW B64STRAW CRCSTRAW SHASTRAW CRCPIPE SHAPIPE PIPE BLOWPIPE B64PIPE '
            'BASE64 BLOWFISH SHA CRC MP INT')
put('CORE', 'CONQUER INIT GLOBALS HEAP JSHELL DYNAVEC VECTOR CCPTR FIXED RANDOM VERSION BUFF RECT MONOC BENCH MPU')
put('CAMP', 'CARRY EXPAND MAPSEL ENDING INTRO SCORE')
put('MOVIE', 'MCIMOVIE MCI MPGSET')
put('EDIT', 'DEBUG MAPEDDLG MAPEDIT MAPEDPLC MAPEDTM MAPEDSEL')
put('PLAT', 'STARTUP WINSTUB KEY CPUID GETCPU WINASM W95TRACE')

NOTE = {
    'CONQUER': 'Main_Game/Main_Loop; DirectDraw used directly for MPEG movie mode; `_dos_getdrive`',
    'INIT': 'Init_Game/Select_Game; creates packet transports (`INIT.CPP:1202`, `:1255`); `_dos_findfirst`',
    'STARTUP': 'WinMain, InitDDraw; registry; `_dos_getdrive`/`_dos_setdrive`',
    'WINSTUB': 'Window class + WndProc, focus loss, memory error handler',
    'KEY': 'WWKeyboardClass: Win32 message-driven keyboard/mouse buffer',
    'QUEUE': 'Lockstep core: Queue_AI, frame sync, CRC checks',
    'EVENT': 'Network event struct (packed, /zp1); sent over the wire',
    'NULLDLG': 'Also holds the **skirmish** setup dialog `Com_Scenario_Dialog(true)` (`NULLDLG.CPP:2904`)',
    'NULLMGR': 'Serial via WIN32LIB `WinModemClass`; Greenleaf `HM*` calls still live (resolve to WINASM stubs)',
    'NULLCONN': 'Null-modem connection',
    'IPXMGR': 'Owns `PacketTransport` when `WINSOCK_IPX`; used by LAN **and** internet games',
    'IPX': 'Real IPX via THIPX32 DLL; entry point only reached from dead code',
    'IPX95': 'THIPX32.DLL loader; unused when `WINSOCK_IPX` (callers compiled out)',
    'IPXPROT': 'DOS real-mode IPX stub (exports only used in DOS branch of IPXMGR)',
    'WSPIPX': 'Winsock IPX transport (`AF_IPX`): **the LAN transport in the shipped build**',
    'WSPUDP': 'Winsock UDP transport: used for `GAME_INTERNET` (WOL) only',
    'WSPROTO': 'Winsock transport base class; `WSAAsyncSelect`-driven',
    'TCPIP': 'Pre-WOL WChat TCP manager; no live callers outside itself',
    'INTERNET': 'Pre-WOL WChat glue; 356 lines compiled out by `WOLAPI_INTEGRATION`',
    'CCDDE': 'DDE server; global ctor sets `RA95AlreadyRunning` (single-instance check)',
    'STATS': 'Internet game statistics to Westwood server',
    'MCIMOVIE': 'Compiled empty (`MCIMPEG` undefined)',
    'MCI': 'Compiled empty (`MCIMPEG` undefined)',
    'MPGSET': 'DVD MPEG movie settings; MpgPlay lives in missing `mpgdll.lib`',
    'MPU': 'Compiled empty: only Borland code; real code is `#pragma aux` in MPU.H (rdtsc)',
    'COORD': '`calcx`/`calcy` `#pragma aux` used by `Move_Point` (simulation!)',
    'EGOS': 'Credits screen; `Vsync` pragma is DOS-only',
    'MONOC': 'Monochrome debug screen; `#pragma aux` port output',
    'DEBUG': 'Fully compiled out (`CHEAT_KEYS` undefined)',
    'MAPEDIT': 'Fully compiled out (`SCENARIO_EDITOR` undefined)',
    'MAPEDDLG': 'Fully compiled out', 'MAPEDPLC': 'Fully compiled out', 'MAPEDTM': 'Fully compiled out',
    'MAPEDSEL': 'Fully compiled out; `#include`d by MAPEDIT.CPP',
    'TRIGTYPE': 'Most of file is editor-only',
    'ENDING': 'Almost all `#ifdef NEVER`',
    'SCORE': 'Campaign score screen **and** `Multi_Score_Presentation`',
    'RANDOM': 'Deterministic LCG `RandomClass` (Scen.RandomNumber)',
    'FIXED': '8.8 fixed-point used by rules/simulation',
    'VECTOR': 'BooleanVectorClass uses `Set_Bit`/`Get_Bit` (`#pragma aux` in JSHELL.H)',
    'PASSEDIT': '**Source missing** from the release', 'PALETTE': '**Source missing** (PaletteClass)',
    'RAWOLAPI': '`#include`s WOLAPI/WOLAPI_I.C (COM GUIDs)',
    'WOLAPI_I': 'COM interface IDs, pulled in by RAWOLAPI.CPP',
    'DIBUTIL': 'GDI DIB helpers, only used by WOL code', 'DIBFILE': 'GDI DIB loader, only used by WOL code',
    'TOOLTIP': 'Only used by WOL screens', 'ICONLIST': 'Only used by WOL screens',
    'SENDFILE': 'Scenario download from host (LAN + WOL)',
    'RAWFILE': 'Win32 `CreateFile`/`ReadFile` file backend',
    'MIXFILE': 'MIX archive reader', 'CDFILE': 'CD path search',
    'INTERPAL': '2x palette interpolation (asm in WINASM.ASM)',
    'WINASM': 'Greenleaf **stubs**, ModeX blit, interpolation asm',
    '2KEYFBUF': 'Shape drawing incl. MMX (S02)', '2TXTPRNT': 'Font blitter (S02)', 'CPUID': 'MMX detection',
    'LCWCOMP': 'LCW compressor (S02)', 'GETCPU': 'CPU/MMX detection wrapper',
    'SESSION': 'SessionClass (game type, players); `FindFirstFile` for recordings',
    'MPLAYER': 'Multiplayer game-type menu', 'NETDLG': 'LAN lobby dialogs; big WOL-dead sections',
    'W95TRACE': 'Debug trace stubs', 'COMINIT': 'COM init (for WOL)',
    'HEAP': 'Fixed heaps of game objects', 'GLOBALS': 'All global objects', 'GSCREEN': 'Page flipping base',
}
SUBNAME = {
    'SIM': 'Simulation', 'DATA': 'Sim: type data', 'NET': 'Network core', 'MPUI': 'Multiplayer UI',
    'MODEM': 'Modem/serial (+skirmish UI)', 'INET': 'Internet (pre-WOL)', 'WOL': 'WOL', 'UI': 'UI', 'REND': 'Rendering',
    'AUD': 'Audio', 'FILE': 'File I/O', 'COMP': 'Compression/crypto', 'CORE': 'Core/boot', 'CAMP': 'Campaign',
    'MOVIE': 'Movies', 'EDIT': 'Editor/debug', 'PLAT': 'Win32 platform',
}

def camp(sub):
    return 'yes' if sub in ('CAMP', 'MOVIE') else ''

def main():
    rows = list(csv.reader(open(sys.argv[1]), delimiter='\t'))
    h = rows[0]
    ix = {c: i for i, c in enumerate(h)}
    out = []
    out.append('| # | File | Built into | Subsystem | Lines (live) | DirectDraw | DirectSound | Win32 API | Winsock | IPX | Greenleaf / serial | Watcom asm | DOS-only | WOL | Movie | Campaign-only | Notes |')
    out.append('|--:|------|-----------|-----------|-------------:|:--:|:--:|:--:|:--:|:--:|:--:|:--:|:--:|:--:|:--:|:--:|-------|')
    n = 0
    counts = {}
    for r in rows[1:]:
        grp, obj, src = r[0], r[1], r[2]
        name = src.split('/')[-1].rsplit('.', 1)[0].upper() if src != 'MISSING' else obj
        built = grp.replace(' (WIN32 block)', ' (Win32-only)').replace('#included by ', 'via `#include` in ')
        sub = SUB.get(name, '??')
        counts[sub] = counts.get(sub, 0) + 1
        n += 1
        if src == 'MISSING':
            out.append(f'| {n} | `{obj}.CPP` | {built} | {SUBNAME[sub]} | missing | | | | | | | | | | | {camp(sub)} | {NOTE.get(name, "")} |')
            continue
        if len(r) < 6 or r[4] == 'ASM':
            asm = 'asm file'
            gl = 'stubs' if name == 'WINASM' else ''
            dos = 'yes' if name == 'IPXPROT' else ''
            out.append(f'| {n} | `{src}` | {built} | {SUBNAME[sub]} | {r[3]} (asm) | | | | | {"x" if name == "IPXPROT" else ""} | {gl} | **{asm}** | {dos} | | | {camp(sub)} | {NOTE.get(name, "")} |')
            continue
        g = lambda c: int(r[ix[c]])
        def cell(v, tag=''):
            return f'{tag}{v}' if v else ''
        dd = ' '.join(x for x in (cell(g('DirectDraw'), 'D'), cell(g('DD via WIN32LIB'), 'L')) if x)
        gl = ' '.join(x for x in (cell(g('Greenleaf'), 'G'), cell(g('Serial (WinModem)'), 'S')) if x)
        dos_parts = []
        if g('dos_dead'): dos_parts.append(f'{g("dos_dead")}d')
        if g('DOS-isms'): dos_parts.append(f'{g("DOS-isms")}i')
        if g('wol_live') and g('wol_live') >= g('live') - 2:
            wol = 'whole file'
        else:
            wol = ' '.join(x for x in (cell(g('WOL'), 'A'), cell(g('wol_live'), 'g')) if x)
        hdr = r[ix['headers']]
        note = NOTE.get(name, '')
        if hdr:
            note = (note + '; ' if note else '') + 'hdrs: ' + hdr.replace(',', ' ')
        out.append(f'| {n} | `{src}` | {built} | {SUBNAME[sub]} | {r[3]} ({g("live")}) | {dd} | {cell(g("DirectSound"))} | {cell(g("Win32 API"))} | '
                   f'{cell(g("Winsock"))} | {cell(g("IPX"))} | {gl} | {cell(g("Watcom pragma/asm"))} | {" ".join(dos_parts)} | {wol} | '
                   f'{cell(g("Movie (VQA/MPEG)"))} | {camp(sub)} | {note} |')
    print('\n'.join(out))
    print()
    print('<!-- subsystem counts: ' + ', '.join(f'{SUBNAME[k]}={v}' for k, v in sorted(counts.items(), key=lambda kv: -kv[1])) + f'; total={n} -->')
    missing = [k for k in counts if k == '??']
    if missing:
        sys.stderr.write('UNCLASSIFIED present\n')

main()
```

</details>
