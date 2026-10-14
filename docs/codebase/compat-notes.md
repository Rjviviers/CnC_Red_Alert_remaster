# Compiler compatibility notes

Every workaround that lets the 1996 Watcom C++ 10.6 code compile with MSVC and GCC, and why. Started in session S06a
([log](../logs/2026-10-14-S06a-shared-headers.md)); S06b–e add their per-file fixes here. The design decisions are in
[ADR-0014](../decisions/0014-compiler-compat-layer-and-packing.md) (compat layer, Win32 on Linux, packing, `WIN32` defines)
and [ADR-0015](../decisions/0015-no-mingw-ci-job.md) (no MinGW CI job).

## The compatibility layer (`CODE/compat/`)

Wired up in `cmake/RABuildConfig.cmake` (`ra_apply_build_settings`), so it applies to the game, `win32lib`, `vqa32wp`,
`vqm32wp` and the unit tests. Not to the S08 platform layer, which is deliberately built without the game's settings.

| Path | Used on | How | Contents |
|------|---------|-----|----------|
| `ra_compat.h` | every host | force-included (`-include` / `/FI`) | see the next table |
| `ra_crt_compat.h` | non-Windows | included by `ra_compat.h` | Microsoft/Watcom C runtime extras glibc lacks |
| `watcom_aux.h` | every host | included by `JSHELL.H` | C++ for the `#pragma aux` asm of `JSHELL.H` |
| `dos/` | every host | normal include path | `dos.h`, `mem.h`, `bios.h`, `i86.h`, `alloc.h`, `pharlap.h`, `pldos32.h`, `iostream.h` |
| `posix/` | non-Windows | `-idirafter` (a real header wins) | `io.h`, `direct.h`, `conio.h`, `share.h`, `new.h`, `process.h` |
| `win32/` | non-Windows | `-idirafter` | `windows.h`, `windowsx.h`, `mmsystem.h`, `ddraw.h`, `dsound.h`, `winsock.h`, `nspapi.h`, `svcguid.h` |

### `ra_compat.h`

| Item | Why |
|------|-----|
| `#define TRUE_FALSE_DEFINED` | 24 headers (22 in `CODE/` including `WATCOM.H`, plus `WIN32LIB/INCLUDE/WWSTD.H` and its copy `WIN32LIB/MISC/WWSTD.H`) carry `enum {false=0,true=1}; typedef int bool;` inside `#ifndef TRUE_FALSE_DEFINED`. Illegal in standard C++; defining the guard switches all of them off without touching the files. The built-in `bool` is used. **Size change: 4 → 1 byte** (see the S06a log, determinism section). |
| `cdecl` (and on non-Windows `__cdecl`, `_cdecl`, `__stdcall`, `_stdcall`, `__declspec(x)`) | Watcom accepted `cdecl` without underscores (`FUNCTION.H`, `WWALLOC.H`, `CDFILE.CPP`). On 32-bit Linux cdecl is the default convention, and `__stdcall` only appears on Win32 API declarations that the stand-in headers declare but never define. |
| Runtime headers pre-included under `#pragma pack(push, 8)` (GCC/Clang only) | The game is built with `-fpack-struct=1` (Watcom `/zp1`, ADR-0014). GCC would pack glibc/libstdc++ structs too, giving them a different layout from the compiled library. Pre-including them with natural packing makes every later `#include` of them a no-op. MSVC's headers push their own packing. |
| SDL3 pre-included the same way when `RA_HAVE_SDL3` is defined | SDL3's headers `static_assert` their natural layout (`SDL_alignment_test`) and fail under `-fpack-struct=1`. Without this `WIN32LIB/AUDIO/audio_sdl3.cpp` (S09) did not compile on Linux. |
| `#define random ra_glibc_random` around `<stdlib.h>` (non-MinGW) | glibc declares `long random(void)`, `WIN32LIB/INCLUDE/MISC.H` declares `extern "C" unsigned long random(unsigned long)` (DETPROC.ASM). Two C functions cannot share a name; the game never calls glibc's. |
| `#undef BIG_ENDIAN`, `LITTLE_ENDIAN`, `PDP_ENDIAN`, `BYTE_ORDER` (non-Windows) | glibc's `<endian.h>` defines `BIG_ENDIAN` as the constant 4321 **on every machine**. `DEFINES.H` (cell/coordinate unions), `FIXED.H` (the simulation's fixed-point type!) and `BASE64.CPP` use `#ifdef BIG_ENDIAN` to mean "big-endian machine": without the `#undef` Linux silently got the big-endian struct layouts. `wire_layout_check.cpp` has an `#error` if it comes back. |
| `_READ` = 1, `_WRITE` = 2 | Watcom's `<stdio.h>` FILE flag bits; `WIN32LIB/INCLUDE/WWFILE.H` uses them for `READ`/`WRITE` (same values as `COMPAT.H`). |

### `ra_crt_compat.h` (non-Windows)

`_MAX_PATH`/`_MAX_DRIVE`/`_MAX_DIR`/`_MAX_FNAME`/`_MAX_EXT` (Microsoft values), `stricmp`, `_stricmp`, `strcmpi`,
`strnicmp`, `_strnicmp`, `strncmpi`, `memicmp`, `strupr`, `strlwr`, `strrev`, `strset`, `strnset`, `itoa`/`ltoa`/`ultoa`
(and underscore forms; Microsoft semantics: a minus sign only in base 10), `_lrotl`/`_lrotr` (32-bit),
`_splitpath`/`_makepath` (both `/` and `\` separate directories; a drive letter is only parsed, never produced),
`_fullpath` (`realpath`). Inline functions, not macros, so names like `index` in the code are not disturbed.

### `dos/` (every host)

| Header | What it provides |
|--------|------------------|
| `dos.h` | `struct find_t`, `_dos_findfirst`, `_dos_findnext` (**real implementations**: `INIT.CPP` scans `SC*.MIX`/`SS*.MIX`, `LOADDLG.CPP` lists save games, `CDFILE.CPP` checks directories). On POSIX: `opendir` + `fnmatch(FNM_CASEFOLD)` (DOS names are case-insensitive); on Windows: `_findfirst`. The search state lives behind a pointer in `find_t::reserved` and is freed when the search ends; a search abandoned early leaks one small block, as on DOS. `_dos_getdrive`/`_dos_setdrive`, `_dos_getdiskfree` (**real**: `Disk_Space_Available` in `CONQUER.CPP`; capped below 2 GB so the game's 32-bit product cannot overflow; POSIX reports drive C:). `union REGS`/`struct SREGS`/`segread`/`FP_OFF`/`FP_SEG` for DOS-only code that never runs. On MSVC/MinGW it uses the runtime's `struct _diskfree_t`. |
| `mem.h` | `<string.h>` plus Borland `movmem` (source first!) and `setmem`. |
| `bios.h` | Empty: `WWSTD.H` includes it, nothing calls a BIOS function. |
| `i86.h` | Same as `dos.h`. |
| `alloc.h` | `<stdlib.h>`, `<malloc.h>`. |
| `pharlap.h`, `pldos32.h` | `REALPTR` (4 bytes) and friends, for `WINVQ/INCLUDE/VQM32/VESAVID.H`'s structures. Nothing calls the DOS extender. |
| `iostream.h` | `<iostream>` + `<iomanip>` with the common names pulled out of `std` (pre-standard iostreams). |

`delay()` is deliberately **not** provided: the code uses `delay` as a variable name in many places.

### `posix/` (non-Windows; searched after the system headers)

`io.h` (`O_BINARY`=0, `tell`, `filelength`, `eof`, `chsize`, underscore forms), `share.h` (`SH_*` constants; `sopen` ignores
sharing, POSIX has no mandatory locks), `direct.h` (one-argument `mkdir`, `_getdcwd`, drive functions report C:),
`conio.h` (`kbhit`/`getch` report no key; `inp`/`outp` do nothing because user-mode port I/O is impossible on every
current OS, and would have crashed the Win32 build too), `new.h` (`<new>`), `process.h` (`<unistd.h>`, `_getpid`).

### `win32/` (non-Windows)

Minimal Win32 (`windows.h`, `windowsx.h`), winmm (`mmsystem.h`), DirectDraw (`ddraw.h`, DirectX 5 interfaces), DirectSound
(`dsound.h`) and WinSock 1.1 (`winsock.h`, on top of the real BSD socket headers) declarations. Rules:
- Types, sizes and constant values are the 32-bit Windows SDK's (`DWORD` is `unsigned long`, which is 4 bytes on 32-bit
  Linux too). Handles are `struct NAME__ *` like the SDK's `STRICT` mode; S09's `WIN32LIB/AUDIO/audio_compat.h` relies on
  `HWND` being `struct HWND__ *`.
- Functions are **declared only**. A call that survives to S10's link step is a call S08/S09/S12 must replace.
- Structs are wrapped in `#pragma pack(push, 8)` (`WAVEFORMATEX` in `pack(1)`, as in the SDK) so the game's
  `-fpack-struct=1` does not change them.
- `windows.h` defines the `min`/`max` macros like the SDK (unless `NOMINMAX`); `LIST.H` and about 14 `.CPP` files use them.
- `typedef int BOOL` is skipped if `WWSTD.H` already did `#define BOOL int`.
- An `#error` stops the headers from ever being used with a real Windows SDK.
- Add what a file needs, in the same style. Delete the directory when nothing on Linux includes `windows.h` any more.

## Build settings

| Setting | Why |
|---------|-----|
| `WIN32`/`_WIN32` only on Windows hosts | No source tests them (S05). They tell glibc/SDL3 the host is Windows (SDL3 then wanted `<process.h>`). Some WIN32LIB/WINVQ component files `#define WIN32` themselves (`TIMER.H`, `RAWFILE.H`, `VQAPLAY.H`, …); include SDL before them if that ever matters. |
| Global 1-byte packing kept (`RA_PACK_STRUCTS`) | ADR-0014. Matches Watcom `/zp1` for every game/library struct (file formats, save games, the asm-read `GraphicViewPortClass`/`WWMouseClass`). |
| `-fsigned-char` | Watcom `/j`. Unchanged from S03. |
| `-fpermissive` (GCC, from S03) | Kept for now. It only downgrades a handful of diagnostics (e.g. "extra qualification" in `GBUFFER.H`) and does **not** hide the errors S06b–e have to fix. S06e should try removing it once everything compiles. |
| No `-Wno-narrowing` | 565 of the remaining errors are narrowing conversions in brace initialisers (508 of them in one table in `COORD.CPP`). They are legal C++98 but errors in C++11+. Fix them with explicit casts in the tables, which also documents the intent, rather than turning the check off. |

## Header fixes (S06a)

### `CODE/` headers

| File | Change | Why |
|------|--------|-----|
| `JSHELL.H` | `#pragma aux` asm (`Set_Bit`, `Get_Bit`, `First_True_Bit`, `First_False_Bit`, `Fixed_To_Cardinal`, `Cardinal_To_Fixed`, `outportb`, `outport`) kept under `#ifdef __WATCOMC__`; otherwise `CODE/compat/watcom_aux.h` | GCC/MSVC ignore `#pragma aux`, leaving undefined functions. The C++ is bit-exact, including the 32-bit wrap of the product in `Fixed_To_Cardinal` and `Cardinal_To_Fixed(0, x) == x`; `tests/jshell_asm_test.cpp` runs the original instruction sequences as GCC inline asm and compares 200 000 random inputs. **Simulation code** (FLY, INFANTRY, TECHNO, UNIT, VESSEL). |
| `RGB.H` | `outportb`/`outport`/`outrgb` asm under `__WATCOMC__`; otherwise no-ops | VGA port I/O; impossible in user mode. Palettes go through `PaletteClass::Set`. |
| `MPU.H` | `Get_CPU_Clock` (RDTSC) via `__rdtsc()` (MSVC) / `__builtin_ia32_rdtsc()` (GCC) | Benchmarks only (`BENCH.H`). Not `<x86intrin.h>`: it `#define`s `_rotl`, which breaks the `_rotl` template in `JSHELL.H`. |
| `WATCOM.H` | `#pragma warning` lines only for Watcom | Watcom warning numbers; noise or errors elsewhere. |
| `FTIMER.H` | `using BasicTimerClass<T>::Timer; using ...::Started;` in `TTimerClass` and `CDTimerClass` | Two-phase lookup: members of a dependent base must be named explicitly. No layout change. |
| `INT.H` | `typename T::RemainderTable` | Two-phase lookup. |
| `SEARCH.H` | `typename IndexClass<T>::NodeElement` | Two-phase lookup. |
| `MIXFILE.H` | `Node<MixFileClass<T> >` | Template name used as a type in its own base clause. |
| `LIST.H` | `extern GraphicViewPortClass SeenBuff;` before the templates | Non-dependent name used in a template before `EXTERNS.H` declares it. |
| `VECTOR.H` | Own placement `operator new` only for Watcom; otherwise `<new>` | Redefinition of the standard placement new. |
| `FUNCTION.H` | `extern CCPtr<AircraftClass> y1;` renamed `y1_` | Clashes with the POSIX Bessel function `y1()` from `<math.h>`. The `yN` declarations are never defined or used. |
| `EXTERNS.H` | `extern "C" int MouseQX, MouseQY;` added | Used by `ENDING.CPP`/`INTRO.CPP`, defined by S07-C's `CODE/WINASM.CPP`; no header declared them. |
| `EXTERNS.H` (+ `INIT.CPP:1202-1203`) | `SoundObject`, `PrimaryBufferPtr` removed | DirectSound is gone (S09); their only use was handing them to the VQA player. |
| `PALETTE.H`, `PCX.H` | New (reconstructed) | See [Rewritten files](#rewritten-files). |

S07-C's new `.CPP` files declare their own prototypes because the Westwood headers did not compile on GCC then. They do
now; those local declarations can be replaced by the headers (S07-C's call; S06a did not touch them).

### Shared headers in `WIN32LIB/` and `WINVQ/` (minimal compile fixes)

| File | Change |
|------|--------|
| `WIN32LIB/INCLUDE/` `DESCMGMT.H`, `PLAYCD.H`, `SHAPE.H`, `SOSRES.H`, `WWLIB32.H`, `WWMEM.H`, `WW_WIN.H`; `WINVQ/INCLUDE/VQA32/` `CAPTION.H`, `SOSDATA.H`, `SOSDEFS.H`, `SOSRES.H`; component copies `WIN32LIB/MEM/WWMEM.H`, `WIN32LIB/MISC/WWLIB32.H`, `WIN32LIB/PLAYCD/PLAYCD.H`, `WINVQ/VQA32/{CAPTION,SOSDATA,SOSDEFS,SOSRES}.H`; `WIN32LIB/DIPTHONG/_DIPTABL.CPP` | Removed the trailing DOS end-of-file byte (Ctrl-Z, 0x1A): "stray '\32' in program". |
| `WIN32LIB/INCLUDE/PALETTE.H` | `extern "C" extern unsigned char CurrentPalette[]` → `extern "C" unsigned char CurrentPalette[]` | Two storage-class specifiers; same meaning. |
| `WIN32LIB/INCLUDE/WWSTD.H` | `#define VOID void` → `#include <windows.h>` | The Windows headers (real and stand-in) only typedef `CHAR`/`SHORT`/`LONG` when `VOID` is not yet defined; every file including `WWSTD.H` before `windows.h` lost those types. |
| `WIN32LIB/INCLUDE/SHAPE.H` | `SHAPE_TRANS = 0x0040` added to `ShapeFlags_Type` | Used by `CONQUER.CPP` (which also `#define`s it locally); requested by S07-B. |

Still left in component directories owned by other sessions (same Ctrl-Z fix needed): `WIN32LIB/AUDIO/SOSRES.H`,
`WIN32LIB/SHAPE/SHAPE.H`, `WIN32LIB/WW_WIN/WW_WIN.H`, and the `.CPP` files `WIN32LIB/IFF/WRITELBM.CPP`,
`WIN32LIB/MISC/{DELAY,FINDARGV,IRANDOM}.CPP`, `WIN32LIB/PALETTE/{MORPHPAL,PALETTE}.CPP`,
`WIN32LIB/WW_WIN/{WINDOWS,WINHIDE}.CPP`, `WINVQ/VQA32/CAPTION.CPP`. `WIN32LIB/PALETTE/PALETTE.H` (component copy) has the
same `extern "C" extern`.

## Rewritten files

The owner confirmed (2026-10-14) there is no other copy of these files. They were rebuilt from every use in the code.
None of them is simulation code.

### `CODE/PALETTE.H` — `PaletteClass`

| Member | Inferred from |
|--------|---------------|
| Class is exactly `RGBClass Palette[256]` (768 bytes, no vtable); `static_assert` added | `CCFileClass("EGOPAL.PAL").Read(&CCPalette, sizeof(CCPalette))` (EGOS), `(PaletteClass *)MFCD::Retrieve(...)` (DISPLAY), `memmove(palcopy, palette, sizeof(PaletteClass))` into a 768-byte buffer (WRITEPCX), `memmove(&GamePalette[0], ..., 768L)` (INIT) |
| `enum {COLOR_COUNT=256}` | `PaletteClass::COLOR_COUNT` (JSHELL, VORTEX, OPTIONS) |
| `PaletteClass()`, `PaletteClass(RGBClass const &)` (fill) | `BlackPalette(RGBClass(0,0,0))`, `WhitePalette(RGBClass(255,255,255))` (GLOBALS) |
| `RGBClass & operator[](int)` and `const` version | `palette[color]` on const references (JSHELL, VORTEX), `ScorePalette[i] = ...` (SCORE), `pal[254].Adjust(...)` (MAPSEL) |
| `operator unsigned char *()` and `operator unsigned char const *() const` | `memcpy(CCPalette, ...)`, `(unsigned char*)CCPalette` (EGOS), `Load_Title_Screen(..., CCPalette)` and `Increase_Palette_Luminance(ScorePalette, ...)` taking `unsigned char *` |
| `Adjust(int ratio)` — toward black | `BlackPalette.Adjust(0x08, WhitePalette); ...; BlackPalette.Adjust(0xFF);` (CONQUER, GOPTIONS: undo a white flash), and `RGB.H`'s commented-out `Adjust(int ratio, RGBClass const & rgb = BlackColor)` |
| `Adjust(int ratio, PaletteClass const &)` | the same calls; per entry `RGBClass::Adjust(ratio, palette[i])` |
| `Partial_Adjust(int, char * lut)`, `Partial_Adjust(int, PaletteClass const &, char * lut)` | EGOS slide show: `PaletteLUT` is 1 for picture colors, 0 for font colors; "The text colors never fade" |
| `Set(int time = 0, void (*callback)(void) = 0) const` | `Set()`, `Set(FADE_PALETTE_FAST, Call_Back)`, `Set(TIMER_SECOND*2, Call_Back)`. Implemented with WIN32LIB's `Fade_Palette_To` (fades over `time` ticks, immediate for 0), then records `CurrentPalette` |
| `int Closest_Color(RGBClass const &) const` | `palette.Closest_Color(trycolor)` (JSHELL `Make_Fading_Table`); same search as the hand-written loops in JSHELL/VORTEX (smallest `Difference`, first index wins ties) |
| `static PaletteClass CurrentPalette` | `PaletteClass::CurrentPalette[color]` (RGB.CPP `RGBClass::Set`, CONQUER). RGB.CPP casts the element to `RGBClass &`, which suggests the original may have been `const`; non-const works for every use. |
| `operator==`/`!=` | Not used by the code; added for completeness. |
| Include guard `PALETTECLASS_H` | `WIN32LIB/INCLUDE/PALETTE.H` (C palette functions, reached through `<wwlib32.h>`) already uses `PALETTE_H` and both are included by `FUNCTION.H`. |

Not reconstructed: an `Adjust(PaletteClass const &, int)` overload (`INTRO.CPP:285` calls it, but inside `#ifdef OBSOLETE`).

### `CODE/PALETTE.CPP`

Implements the members above (`Adjust` uses `RGBClass::Adjust`, which already clamps the ratio to 0..255). Added to
`RA_GAME_CXX_SOURCES` (it was the last entry in `RA_GAME_MISSING_OBJECTS`, from `TECHFILES`).

### `CODE/PCX.H`

| Item | Inferred from |
|------|---------------|
| `RGB` and `PCX_HEADER` (128 bytes), identical to `CODE/FILEPCX.H`/`WIN32LIB/INCLUDE/FILEPCX.H`, explicitly `pack(1)` and size-asserted | `WRITEPCX.CPP` fills `PCX_HEADER` field by field (id, version, encoding, pixelsize, x, y, width, height, xres, yres, ega_palette[16], nothing, color_planes, byte_per_line, palette_type, filler) and casts the palette to `RGB *` |
| `int Write_PCX_File(FileClass &, GraphicBufferClass &, PaletteClass *)` | definition in `WRITEPCX.CPP`; callers in DEBUG, CONQUER, GADGET pass `&GamePalette` |
| Include guard `PCX_H` (the same as `FILEPCX.H`) | `WINSTUB.CPP` includes `FUNCTION.H` (so `pcx.h`) and later `"filepcx.h"`; a shared guard skips the second, otherwise identical, anonymous-struct typedefs, which would clash in C++ |

## Layout checks (`CODE/wire_layout_check.cpp`)

Compiled into the game. Pins, per compiler, the size of `CommHeaderType` (7), `GlobalHeaderType` (9), `IPXAddressClass`
(10), `EventClass` (32; Watcom 20), `EventClass::EventType` and the other wire enums (4; Watcom 1), `SpecialClass`
(GCC 2 / MS 4; Watcom 4), `GlobalPacketType` (144; Watcom 139) with its version and scenario offsets,
`RemoteFileTransferType` (200; Watcom 198), `SerialPacketType`, and `MAX_IPX_PACKET_SIZE` (512; Watcom 520). Also the
`GraphicViewPortClass` offsets from `GBUFFER.INC` and S07-A's mirror `GVPLayout` (`WIN32LIB/DRAWBUFF/GVPORT.H`) against the
real class. The `_WIN32` numbers were measured with MinGW-w64 (MSVC bit-field layout); confirm on the first MSVC CI run.

`WWMouseClass` (read by the mouse asm via `MOUSE.INC`) is not asserted: its members are private. Its layout depends on
1-byte packing (`MCFlags`/`MCCount` are two `char`s followed by pointers), which ADR-0014 keeps.

## Known follow-ups

- **64-bit:** `GraphicViewPortClass::Get_Offset()` (and the `Offset` member) is `long`, which truncates pointers on LP64
  and LLP64. Do not change it while the S07 ports and the asm offsets depend on it; it belongs to the 64-bit ADR.
- **For-loop scope:** Watcom let `for (int i ...)` variables live after the loop. Many remaining errors are uses of such
  variables after the loop. Watch out on Linux: an out-of-scope `index` silently becomes the POSIX function `index()`
  from `<strings.h>`, giving odd errors such as "no post-increment operator for type" or
  "unresolved overloaded function type". Declare the variable before the loop.
- **MinGW-only:** 14 files fail only with MinGW, because its `windows.h` does not define `min`/`max` in C++ (MSVC's does).
