# ADR-0014: Compiler compatibility layer, Win32 stand-in headers for Linux, and struct packing

- **Status:** Proposed
- **Date:** 2026-10-14
- **Session:** S06a

## Context
The game was written for Watcom C++ 10.6 on DOS/Win32. To compile with MSVC and GCC (32-bit, [ADR-0003](0003-toolchain.md))
S06 has to answer four questions before S06b–e can fix individual `.CPP` files in parallel:

1. Where do the things Watcom supplied implicitly go: the pre-standard `typedef int bool`, `cdecl`, `strcmpi`,
   `_MAX_PATH`, and headers no modern compiler has (`dos.h`, `mem.h`, `bios.h`, `pharlap.h`, `iostream.h`, …)?
2. On Linux, 201 files stopped at `#include <windows.h>`. S08 replaces the Win32 code with SDL3
   ([ADR-0004](0004-platform-library.md)), but S06 cannot wait for that.
3. Keep Watcom's global 1-byte packing (`/zp1`), or pack only saved and network structs?
4. What do the `WIN32`/`_WIN32` defines from the original makefile mean on a non-Windows host?

## Options considered
**Compatibility code**
1. *Edit every file.* Hundreds of changes in files S06b–e, S07, S08 and S09 are also editing.
2. *One force-included header plus shim directories (chosen).* `CODE/compat/ra_compat.h` is included in front of every
   translation unit (`-include`/`/FI`) and holds the few things that would otherwise be repeated everywhere; missing headers
   become small shim files on the include path. The original files stay untouched.

**Win32 on Linux**
1. *Wait for S08.* The Linux scoreboard would stay near 0% until the end of Wave 3 and hide every other error.
2. *Use MinGW-w64's Windows headers on Linux.* They conflict with glibc and assume a Windows ABI.
3. *Wine's headers (winelib).* A large external dependency for a temporary need.
4. *Our own minimal stand-in headers (chosen).* `CODE/compat/win32/` holds `windows.h`, `mmsystem.h`, `ddraw.h`,
   `dsound.h`, `winsock.h`, `windowsx.h` (plus empty `nspapi.h`/`svcguid.h`) with exactly the types, constants and
   function declarations the code names, with the SDK's sizes and values. Functions are declared, never defined.

**Packing**
1. *Pack only wire and save structs (`#pragma pack(1)` per struct).* Clean ABI, but every struct read from a file
   (MIX, SHP, PCX, CPS, VQA, AUD, fonts, icon sets, …) or written raw (save games) must be found and wrapped first, the
   asm ports read `GraphicViewPortClass`/`WWMouseClass` at Watcom offsets, and a miss shows up only at run time.
2. *Keep global packing (chosen), and shield the headers that must not be packed.* Same layout as Watcom for all game
   and library structs. MSVC: `/Zp1` with `WINDOWS_IGNORE_PACKING_MISMATCH`; the Windows SDK and SDL push their own
   packing. GCC: `-fpack-struct=1` would also pack glibc, libstdc++ and SDL3 structs (SDL3's headers even refuse to
   compile), so `ra_compat.h` includes the C/C++ runtime headers (and SDL3 when enabled) first under
   `#pragma pack(push, 8)`, and the shim headers do the same around system includes.

## Decision
1. **Force-included `CODE/compat/ra_compat.h` + shim directories**, all wired in `cmake/RABuildConfig.cmake`:
   - `CODE/compat/ra_compat.h` (every target): native `bool` (defines `TRUE_FALSE_DEFINED`, which turns off the 24 copies of
     `enum {false,true}; typedef int bool;`), `cdecl`, runtime headers under natural packing, removes glibc's `BIG_ENDIAN`,
     renames glibc's `random()`, Watcom's `_READ`/`_WRITE`; on non-Windows hosts it includes `ra_crt_compat.h`
     (`strcmpi`, `stricmp`, `strupr`, `itoa`, `_MAX_PATH`, `_splitpath`, `_makepath`, …).
   - `CODE/compat/dos/` (every host, normal include path): headers no current compiler has.
   - `CODE/compat/posix/` (non-Windows hosts, searched after the system headers with `-idirafter`): Microsoft runtime headers
     glibc lacks (`io.h`, `direct.h`, `conio.h`, `share.h`, `new.h`, `process.h`). A real header always wins.
   - `CODE/compat/win32/` (non-Windows hosts, `-idirafter`): the Win32 stand-ins.
   - `CODE/compat/watcom_aux.h`: bit-exact C++ for the Watcom `#pragma aux` assembly in `JSHELL.H`.
2. **Linux gets Win32 declarations from `CODE/compat/win32/`** until S08/S09/S12 remove the Win32, DirectX and WinSock
   code. Anything still calling a declared-only function fails at link time (S10), which is where it must be replaced.
   Handles are declared like the SDK (`struct HWND__ *`), which S09's `audio_compat.h` relies on. Values match the SDK.
3. **Global 1-byte packing stays** (`RA_PACK_STRUCTS=ON`), with system and SDL headers shielded as described. The layout
   of `GraphicViewPortClass` and `WWMouseClass` is unchanged. Per [ADR-0013](0013-lan-scope-port-crossplay-naming.md) each
   compiler's network layout only has to be stable: `CODE/wire_layout_check.cpp` (compiled into the game) pins the sizes and
   offsets of the network structs per compiler, with the original Watcom size in a comment, plus `GraphicViewPortClass`
   against `GBUFFER.INC` and S07-A's mirror `GVPLayout`.
4. **`WIN32`/`_WIN32` are only defined on Windows hosts.** No source tests them any more (S05); on Linux they made SDL3
   take its Windows path. `WINSOCK_IPX`/`WOLAPI_INTEGRATION` stay undefined (S05).

## Consequences
- Linux goes from 10/284 to 197/286 compiling files before the Wave 3 merges (236/319 after merging S07–S09). The
  remaining errors are per-file problems for S06b–e (narrowing in tables, old `for` scope, template syntax).
- Global packing remains an ABI hazard for any *new* third-party header: include it through the same shield (add it to
  `ra_compat.h`'s natural-packing block) or compile the code outside `ra_apply_build_settings()` like the S08 platform
  layer. A later ADR can move to per-struct packing once the asm is gone and the file-format structs are known
  (`docs/codebase/file-formats.md`); `wire_layout_check.cpp` will catch wire-layout changes when that happens.
- Native `bool` is 1 byte, not Watcom's 4, and enums are 4 bytes, not 1: the wire structs are larger than the original
  (`EventClass` 32 instead of 20 bytes; 16 instead of 26 events per packet). Allowed by ADR-0009/0013; recorded in the S06a log.
- The stand-in headers grow on demand. They must never be used with a real Windows SDK (`#error` guard), and they shrink
  as S08/S09/S12 remove Win32 code. When nothing includes `windows.h` on Linux any more, delete `CODE/compat/win32/`.
- [ADR-0015](0015-no-mingw-ci-job.md) decides the related CI question (no MinGW job).
