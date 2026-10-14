# Session log: S06a — Compiler compatibility: shared headers and missing files

- **Date:** 2026-10-14
- **Session brief:** [S06](../sessions/S06-compiler-compat.md) (sub-session S06a; S06b–e follow)
- **Branch / PR:** `worktree-agent-a86d2f82c99398b54` (not pushed) / —
- **Status at end:** Done (S06 as a whole stays In progress)

## Goal
Make the shared foundation compile (all `CODE/` headers, the Watcom/DOS/Win32 environment, the missing
`PALETTE.CPP`/`PALETTE.H`/`pcx.h`), decide Win32-on-Linux, MinGW CI and packing, and pin the wire-struct layouts,
so that S06b–e can fix the remaining `.CPP` files independently.

## What was done
1. Fast-forwarded to `claude/cool-bohr-9vg7ac` (`66de4c5`). Baseline: `python3 scripts/compile_progress.py` → **10 of 284**.
2. Built the compatibility layer in `CODE/compat/` and wired it into `cmake/RABuildConfig.cmake`: a force-included
   `ra_compat.h`, `ra_crt_compat.h` (Microsoft runtime extras for Linux), DOS headers (`dos/`, every host), Microsoft
   runtime headers (`posix/`, non-Windows, searched last) and Win32/DirectX/WinSock stand-ins (`win32/`, non-Windows).
   Everything is listed with its reason in [compat-notes.md](../codebase/compat-notes.md).
3. First full run: 173/286. Then fixed what showed up in headers, by number of files affected:
   - stray Ctrl-Z bytes at the end of shared headers (and `_DIPTABL.CPP`, as the brief asked);
   - two-phase-lookup errors in template headers (`FTIMER.H`, `INT.H`, `SEARCH.H`, `MIXFILE.H`, `LIST.H`);
   - placement `new` redefined in `VECTOR.H`; `y1` clashing with `<math.h>`; glibc's `random()` vs `MISC.H`'s;
   - `typedef int BOOL` after `WWSTD.H`'s `#define BOOL int`; `WWSTD.H`'s `#define VOID void` hiding `CHAR`/`LONG`;
   - Watcom's `_READ`/`_WRITE`; `winsock.h`; `x86intrin.h` breaking the `_rotl` template.
   Found on the way: glibc defines `BIG_ENDIAN` on every machine, and `FIXED.H`/`DEFINES.H` read that as "big-endian
   host": Linux would have compiled the big-endian layouts of `fixed` and the cell/coordinate unions. Fixed in `ra_compat.h`
   and guarded by an `#error` in `wire_layout_check.cpp`.
4. Replaced the Watcom `#pragma aux` assembly of `JSHELL.H` with bit-exact C++ (`CODE/compat/watcom_aux.h`), and
   `RGB.H`/`MPU.H`'s with no-ops/RDTSC. Added `tests/jshell_asm_test.cpp`, which runs the original instruction sequences
   as GCC inline asm and compares them with the C++ on 200 000 inputs. (The test caught a wrong expectation of mine,
   not a bug in the port.)
5. Reconstructed `CODE/PALETTE.H`, `CODE/PALETTE.CPP` and `CODE/PCX.H` from every use (details and sources in
   compat-notes.md). Added `PALETTE.CPP` to the game sources; `RA_GAME_MISSING_OBJECTS` is now empty.
6. Measured the wire structs with GCC and MinGW (template trick: `Show<sizeof(X)>` in a compile error) and wrote
   `CODE/wire_layout_check.cpp`. Result before merging other sessions: **197 of 286** on Linux GCC; 193/286 with MinGW.
7. Orchestrator requests during the session: `MouseQX`/`MouseQY` declared in `EXTERNS.H` (S07-C); `SHAPE_TRANS` added to
   `WIN32LIB/INCLUDE/SHAPE.H` (S07-B); `GVPLayout` (S07-A's mirror in `WIN32LIB/DRAWBUFF/GVPORT.H`) asserted against the
   real `GraphicViewPortClass`; `Get_Offset()` returning `long` noted as a 64-bit follow-up; `SoundObject` and
   `PrimaryBufferPtr` removed from `EXTERNS.H` and their two uses in `INIT.CPP` (S09); `WIN32`/`_WIN32` no longer defined
   on non-Windows hosts, and SDL3 shielded from the 1-byte packing (S09's SDL3 backend).
8. Merged `claude/cool-bohr-9vg7ac` (S07-A/B/C/D, S08, S09) into this branch; no conflicts. Result: **236 of 319**.
9. Wrote ADR-0014, ADR-0015, compat-notes.md, this log; updated `docs/building.md` and the S06 brief.

## Decisions
- [ADR-0014](../decisions/0014-compiler-compat-layer-and-packing.md) (Proposed): force-included compat header + shim
  directories; minimal Win32 stand-in headers for non-Windows hosts (declarations only, SDK values); **keep global 1-byte
  packing**, shield system and SDL headers from it; per-compiler `static_assert`s on the wire structs; `WIN32`/`_WIN32` only
  on Windows hosts.
- [ADR-0015](../decisions/0015-no-mingw-ci-job.md) (Proposed): **no MinGW CI job**; MinGW stays a local check
  (command in the ADR). `.github/workflows/build.yml` is unchanged.
- Small: native `bool` everywhere (via `TRUE_FALSE_DEFINED`, no header edits); `-fpermissive` from S03 left in place
  (it does not hide the errors S06b–e must fix; S06e should try removing it); no `-Wno-narrowing` (fix tables with casts).

## Files changed
| File / folder | Change |
|---------------|--------|
| `CODE/compat/` (new) | `ra_compat.h`, `ra_crt_compat.h`, `watcom_aux.h`, `dos/*`, `posix/*`, `win32/*` |
| `cmake/RABuildConfig.cmake` | Compat layer wiring; `WIN32`/`_WIN32` only on Windows; `RA_PACK_STRUCTS` define |
| `cmake/RASources.cmake` | `CODE/PALETTE.CPP`, `CODE/wire_layout_check.cpp` added; missing-objects list emptied |
| `CODE/PALETTE.H`, `CODE/PALETTE.CPP`, `CODE/PCX.H` (new) | Reconstructed |
| `CODE/wire_layout_check.cpp` (new) | Layout `static_assert`s |
| `CODE/JSHELL.H`, `RGB.H`, `MPU.H`, `WATCOM.H` | Watcom pragmas only for Watcom; portable versions |
| `CODE/FTIMER.H`, `INT.H`, `SEARCH.H`, `MIXFILE.H`, `LIST.H`, `VECTOR.H`, `FUNCTION.H`, `EXTERNS.H` | Standard C++ fixes (see compat-notes.md) |
| `CODE/INIT.CPP` | Two lines removed (`SoundObject`/`PrimaryBufferPtr`, S09 request) |
| `WIN32LIB/INCLUDE/` `DESCMGMT.H`, `PLAYCD.H`, `SHAPE.H`, `SOSRES.H`, `WWLIB32.H`, `WWMEM.H`, `WW_WIN.H` | Trailing Ctrl-Z removed (shared, minimal) |
| `WIN32LIB/INCLUDE/PALETTE.H` | `extern "C" extern` → `extern "C"` (shared, minimal) |
| `WIN32LIB/INCLUDE/WWSTD.H` | `#define VOID void` → `#include <windows.h>` (shared, minimal) |
| `WIN32LIB/INCLUDE/SHAPE.H` | `SHAPE_TRANS = 0x0040` (shared, minimal; S07-B request) |
| `WIN32LIB/MEM/WWMEM.H`, `WIN32LIB/MISC/WWLIB32.H`, `WIN32LIB/PLAYCD/PLAYCD.H`, `WINVQ/INCLUDE/VQA32/*.H` (4), `WINVQ/VQA32/*.H` (4), `WIN32LIB/DIPTHONG/_DIPTABL.CPP` | Trailing Ctrl-Z removed |
| `tests/jshell_asm_test.cpp` (new), `tests/tests.cmake` | Test for the `JSHELL.H` asm replacements (`# --- S06a ---` block) |
| `docs/decisions/0014-…`, `0015-…`, `docs/codebase/compat-notes.md` (new) | Documentation |
| `docs/building.md`, `docs/sessions/S06-compiler-compat.md` | Defines/packing text; checkboxes ticked with "S06a", status In progress |

## Verification
```sh
python3 scripts/compile_progress.py                 # before: 10 of 284
python3 scripts/compile_progress.py                 # after S06a alone: 197 of 286 (redalert 150/214)
# after merging claude/cool-bohr-9vg7ac (S07-S09), fresh build dir:
cmake -S . -B build -G Ninja -DRA_SDL3_HEADLESS=ON
python3 scripts/compile_progress.py --no-configure  # 236 of 319 (redalert 155/219, win32lib 59/74,
                                                    #  vqa32wp 7/9, vqm32wp 15/17)
cmake --build build --target ra_tests && ctest --test-dir build   # 100% tests passed, 0 failed out of 31
cmake -S . -B build -DRA_HAVE_SDL3=ON
ninja -C build CMakeFiles/win32lib.dir/WIN32LIB/AUDIO/audio_sdl3.cpp.o   # compiles (failed before:
                                                    #  process.h via _WIN32, then SDL_alignment_test under -fpack-struct=1)
python3 scripts/compile_progress.py --build-dir build-mingw --cmake-arg=-DCMAKE_SYSTEM_NAME=Windows \
  --cmake-arg=-DCMAKE_C_COMPILER=i686-w64-mingw32-gcc --cmake-arg=-DCMAKE_CXX_COMPILER=i686-w64-mingw32-g++ \
  --cmake-arg=-DRA_BUILD_TESTS=OFF                  # 228 of 319
```
`CODE/wire_layout_check.cpp`, `PALETTE.CPP`, `RGB.CPP`, `WRITEPCX.CPP` compile with both GCC and MinGW. MSVC was not
available in this session: the MSVC CI job is the first check of the `_MSC_VER` paths and of the `_WIN32` layout numbers.

## Determinism impact
- **`bool` is now the native 1-byte type** instead of Watcom 10.6's `typedef int bool` (4 bytes). Struct layouts that
  contain `bool` change size (save games, some wire structs), and a `bool` now always holds 0 or 1: original code that
  stored a non-0/1 value in a `bool` and compared it with `true`, or did arithmetic on it, behaves differently. Not
  audited file by file. S14 must cover it.
- **Enums are 4 bytes** (compiler default; Watcom used the smallest type). Not changed by S06a, but now measured:
  `EventClass` is 32 bytes (Watcom 20), so a game packet carries 16 events (`MAX_IPX_PACKET_SIZE` 512 / 32) instead of 26.
  Compressed events store a 4-byte type (`QUEUE.CPP` uses `sizeof(EventClass::EventType)` consistently). Allowed by
  ADR-0009/0013; pinned in `wire_layout_check.cpp`.
- **`Fixed_To_Cardinal`/`Cardinal_To_Fixed`** (simulation: FLY, INFANTRY, TECHNO, UNIT, VESSEL) and `Set_Bit`/`Get_Bit`/
  `First_True_Bit`/`First_False_Bit` (BooleanVectorClass) are now C++. Verified bit-exact against the original
  instruction sequences (`tests/jshell_asm_test.cpp`).
- **`BIG_ENDIAN` from glibc is removed.** Without this, Linux builds would have used the big-endian variants of `fixed`
  (`FIXED.H`) and the `CELL`/`COORDINATE` unions (`DEFINES.H`). Windows was never affected.
- Packing (`/zp1`-like) and signed `char` (`-fsigned-char`) are unchanged.
- Not ported: `calcx`/`calcy` in `COORD.CPP` (`#pragma aux`, simulation, `Move_Point`). Per-file work (S06b), must be
  bit-exact.

## Problems & open questions
- MSVC could not be run here. If the MSVC job's numbers for `SpecialClass`/`GlobalPacketType.ScenarioInfo`/
  `SerialPacketType` differ from MinGW's, update the `_WIN32` block of `wire_layout_check.cpp`.
- Remaining non-game errors in component directories owned by other sessions: trailing Ctrl-Z in
  `WIN32LIB/AUDIO/SOSRES.H`, `WIN32LIB/SHAPE/SHAPE.H`, `WIN32LIB/WW_WIN/WW_WIN.H` and several WIN32LIB `.CPP` files;
  `extern "C" extern` in `WIN32LIB/PALETTE/PALETTE.H`; `WIN32LIB/KEYBOARD`/`WW_WIN` files include `keyboard.h` before
  `windows.h`. Listed in compat-notes.md.
- S07-C's new `.CPP` files declare their own prototypes because the headers did not compile on GCC. They do now; S07-C may
  switch to the headers.
- Global packing is kept; moving to per-struct packing is possible later (ADR-0014, consequences).

## Next steps
- **S06b–e**, proposed split of the 64 game files still failing on Linux GCC (alphabetical, 16 failing files each; each
  group owns every `CODE/*.CPP` in its range):
  - **S06b `2…`–`EV`** (`2KEYFRAM` … `EVENT`): AIRCRAFT ANIM BUILDING BULLET CCINI CDFILE CELL CONQUER CONST COORD DIALOG
    DISPLAY DYNAVEC EDIT EGOS EVENT (COORD's 508 narrowing errors are one table; `calcx`/`calcy` must be bit-exact).
  - **S06c `EX`–`MA`**: EXPAND FACTORY FOOT GLOBALS HOUSE INFANTRY INI INIT INT IOMAP IOOBJ KEY LAYER LIST LOADDLG MAP.
  - **S06d `MI`–`SK`**: MISSION MIXFILE MONOC MSGBOX NETDLG OBJECT RADAR RAWFILE READLINE REINF RULES SAVELOAD SCENARIO
    SCORE SESSION SKIRMDLG.
  - **S06e `SL`–`Z` and `_…`**: SLIDER SPECIAL STATBTN TACTION TEAMTYPE TECHNO TEXTBTN THEME TXTLABEL UDPADDR UNIT
    UTRACKER VESSEL VORTEX WINSTUB WSPUDP (+ `_WSPROTO`).
- Main error kinds left: narrowing in brace initialisers, Watcom `for` scope (watch for `index` becoming POSIX `index()`),
  missing `template<>`, `bool` increment, non-const reference to temporaries, enum/int conversions.
- S10: everything declared in `CODE/compat/win32/` and still called is a link error to resolve.

## Review & merge notes (orchestrator)

- Merged into `claude/cool-bohr-9vg7ac` on 2026-10-14 with no conflicts (the session had already merged S07–S09 into its branch).
- Verified on the merged tree (GCC `-m32`, `-DRA_HAVE_SDL3=ON`): `ctest` reports 31/31 passing; compile progress is 236 of 319 (74%);
  `WIN32LIB/AUDIO/audio_sdl3.cpp` now compiles, so `RA_HAVE_SDL3` was switched to default ON.
- Spot-checked the `BIG_ENDIAN` fix in `CODE/compat/ra_compat.h`, and the `#error` guard.
- Recorded the S06b–e split in the S06 brief, and the size changes in the S14 brief.
