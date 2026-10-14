# Session log: S03 — CMake + CI skeleton

- **Date:** 2026-10-14
- **Session brief:** [S03](../sessions/S03-build-system-ci.md)
- **Branch / PR:** `worktree-agent-a0e52ee14db6493eb` / not opened yet (not pushed)
- **Status at end:** Done except the SDL task, which is deferred to S08 (ADR-0004 not approved). Brief set to *In review*.

## Goal
A modern build system and CI that try to compile the original sources and report how far they get.
Expected result: a working build system plus a compile-progress report, close to 0% compiling.

## What was done
1. The worktree started at the pre-wiki commit (`0dc09bb`); fast-forwarded it to `0ebf96c`
   (wiki + session plan) so the docs and brief were present.
2. Read `CODE/MAKEFILE`, `WIN32LIB/MAKEFILE`, `WIN32LIB/PROJECT.CFG`, the 18 component makefiles in
   `WIN32LIB/*/MAKEFILE`, and `WINVQ/VQA32/MAKEFILE` + `WINVQ/VQM32/MAKEFILE`. Findings:
   - The Win32 `ra95.exe` links `OBJECTS` (plus an `!ifdef WIN32` block of 18 more objects) and two
     libraries built from the same directory: `jshell.lib` (`LIBFILES`) and `tech.lib` (`TECHFILES`).
     `INI.OBJ` appears in both `OBJECTS` and `TECHFILES`.
   - `CODE/PASSEDIT.CPP` and `CODE/PALETTE.CPP` are named in the makefile but **do not exist** in the repo.
   - The game links `vqa32wp.lib` and `vqm32wp.lib` (WINVQ/VQA32 and WINVQ/VQM32). `WINVQ/VPLAY32`
     and `WINVQ/VQAVIEW` are standalone tools, not used by the game.
   - `WIN32LIB/MAKEFILE` `LIBS` lists 18 components; `RAWFILE` is installed but **not** in `LIBS`.
   - `WIN32LIB/PROJECT.CFG` defines no macros at all (`/W3 /d1 /otxan /s /4 /j`); Watcom's NT
     target supplied the platform macros. WINVQ also defines none (only TASM gets `PHARLAP_TNT=0`).
3. Wrote `scripts/gen_cmake_sources.py`: a small parser for the wmake subset used (comments, `&`
   continuations, `=`/`+=`, `!ifdef/!ifndef/!else/!endif` with `WIN32` defined). It resolves each
   object to `.CPP`/`.C`/`.ASM` case-insensitively and writes `cmake/RASources.cmake`
   (C++ list, ASM list, missing list per target). Result: game 242 C++ + 6 ASM + 2 missing;
   win32lib 48 C++ + 42 ASM; vqa32wp 9 + 2; vqm32wp 17 + 15. `--check` mode for CI.
4. Wrote `cmake/RABuildConfig.cmake`: the **single place** for architecture, standard, defines and
   flags (ADR-0003 is Proposed). Two stages: `-m32` must be set before `project()`; the rest after.
   It verifies the pointer size and that `<stdio.h>` compiles (catches missing multilib early).
5. Wrote the root `CMakeLists.txt`: targets `redalert` (output `ra95`), `win32lib`, `vqa32wp`,
   `vqm32wp`, plus the `compile-progress` custom target. All sources compiled as C++ (wpp386 did the
   same). SDL left as a commented `FetchContent` placeholder.
6. First Linux configure: almost every file failed on `function.h` (the file is `FUNCTION.H`).
   Wrote `scripts/make_case_shim.py` + `cmake/RACaseShim.cmake` (ADR-0008): a generated directory of
   symlinks named exactly as the code spells includes, incl. literal-backslash names like
   `vqm32\all.h`. Without shim: 4/316 compile; with shim: 10/316, and the errors become real ones.
7. Wrote `scripts/compile_progress.py`: reads `compile_commands.json`, compiles each TU in parallel,
   keeps going, writes `.md`/`.json` reports and per-file logs, groups first errors into
   categories, appends a summary to `$GITHUB_STEP_SUMMARY`. Exit 0 if a report was produced, 2 if
   the build system is broken.
8. Tried C++ standards 98/14/17: identical results (missing headers dominate). Kept 14 (the
   oldest standard MSVC accepts).
9. Extra experiment (not in CI): cross-compiled with MinGW-w64 i686 to see what code reaches with
   real Windows headers: 27/316 compile, and the next wall is `<bios.h>` from `WIN32LIB/INCLUDE/WWSTD.H`.
10. Wrote `.github/workflows/build.yml` (Linux GCC `-m32` and Windows MSVC x86 jobs), `docs/building.md`,
    ADR-0008, `.gitignore` for build directories.

## Decisions
- [ADR-0008](../decisions/0008-case-insensitive-include-shim.md) (Proposed): case-insensitive
  include shim generated in the build tree instead of renaming files.
- Small decisions (no ADR):
  - Source lists are **generated** from the original makefiles and committed; regenerate with
    `python3 scripts/gen_cmake_sources.py`. Once S05/S06 start deleting files, switch to hand-maintained
    lists and delete the generator.
  - `jshell.lib`/`tech.lib` are folded into the `redalert` target rather than separate libraries.
  - `WIN32=1`/`_WIN32` are defined for all four targets (the original libs got them from Watcom's NT
    target; the game includes the library headers with them defined).
  - `RA_PACK_STRUCTS` (Watcom `/zp1`) defaults to ON. On MSVC this needs
    `WINDOWS_IGNORE_PACKING_MISMATCH`, otherwise the Windows SDK's packing assert would hide every
    real error. Whether to keep global packing is for S06 (it is an ABI hazard with system headers).
  - Dropped Watcom-only flags (`/5r /otxan /of+ /s /ri /fh /zq /bt=NT`), warnings-as-errors (`/we`),
    and include paths not in the repo (Watcom headers, DirectX 5 SDK, Greenleaf).
  - SDL3 not added (ADR-0004 still Proposed); moves to S08.
  - Windows CI uses Ninja inside the x86 MSVC developer environment (`ilammy/msvc-dev-cmd`), because
    the Visual Studio generator does not write `compile_commands.json`.

## Files changed
| File / folder | Change |
|---------------|--------|
| `CMakeLists.txt` | New: root build, four targets, `compile-progress` target, SDL placeholder |
| `cmake/RABuildConfig.cmake` | New: the one place for arch/standard/defines/flags |
| `cmake/RACaseShim.cmake` | New: wires the case shim into targets |
| `cmake/RASources.cmake` | New, generated: source lists from the original makefiles |
| `scripts/gen_cmake_sources.py` | New: makefile → CMake source list generator (`--check` for CI) |
| `scripts/make_case_shim.py` | New: generates `build/case_shim/` |
| `scripts/compile_progress.py` | New: compile-progress report |
| `.github/workflows/build.yml` | New: Linux GCC x86 + Windows MSVC x86 jobs |
| `.gitignore` | New: ignore `build/`, `build-*/`, `out/` |
| `docs/building.md` | New: how to build and run the report on Windows and Linux |
| `docs/decisions/0008-case-insensitive-include-shim.md` | New ADR (Proposed) |
| `docs/sessions/S03-build-system-ci.md` | Status *In review*, tasks ticked (SDL deferred) |

Not edited (owned by the coordinator): `docs/sessions/README.md`, `docs/logs/README.md`,
`docs/decisions/README.md`. They need the S03 status/log link and an ADR-0008 row when merging.

## Verification
Environment: Ubuntu 24.04 container, GCC 13.3.0, CMake 3.28.3, Ninja, Python 3.13.
32-bit multilib was **not** installed initially (`-m32` failed on `bits/libc-header-start.h`);
installed it with `apt-get update && apt-get install -y gcc-multilib g++-multilib`, after which
`g++ -m32` produces 4-byte pointers. All numbers below are real `-m32` runs.

```sh
python3 scripts/gen_cmake_sources.py --check        # -> cmake/RASources.cmake is up to date
rm -rf build && python3 scripts/compile_progress.py # configures build/ with Ninja, ~3 s
cmake --build build --target compile-progress     # same result through CMake
```

Result (`build/compile-progress.md`, GNU 13.3.0, x86, pointer size 4, C++14):

| Target | Compiles | Fails | C++ files | Progress | ASM files (not compiled) |
|--------|---------:|------:|----------:|---------:|-------------------------:|
| redalert | 7 | 235 | 242 | 2.9% | 6 |
| win32lib | 1 | 47 | 48 | 2.1% | 42 |
| vqa32wp | 0 | 9 | 9 | 0.0% | 2 |
| vqm32wp | 2 | 15 | 17 | 11.8% | 15 |
| **All** | **10** | 306 | 316 | **3.2%** | 65 |

Files that compile: `CODE/BASE64.CPP`, `LCW.CPP`, `LZO1X_C.CPP`, `LZO1X_D.CPP`, `MPU.CPP`,
`RANDOM.CPP`, `_WSPROTO.CPP`, `WIN32LIB/DRAWBUFF/REGIONSZ.CPP`, `WINVQ/VQM32/MEM.CPP`, `VERTAG.CPP`.

Most common first errors (Linux GCC):

| Files | First error |
|------:|-------------|
| 201 | missing header: `windows.h` |
| 26 | missing header: `dos.h` |
| 26 | `expected identifier before 'false'` (headers declare `enum {false=0,true=1}; typedef int bool;` unless `__BORLANDC__`) |
| 16 | missing header: `ddraw.h` |
| 11 | missing header: `io.h` |
| 9 | missing header: `mem.h` |
| 3 | missing header: `pharlap.h` |
| 3 | `expected initializer before …` (calling-convention keywords such as `__cdecl` / Watcom `cdecl`) |
| 3 | missing header: `new.h` |
| 2 | missing header: `conio.h` |
| 1 each | stray `\32` (DOS Ctrl-Z EOF byte in `_DIPTABL.CPP`), `strcmpi` / `_MAX_PATH` undeclared, `objbase.h`, `direct.h`, `iostream.h` |

Comparison runs (same tool, separate build directories):

| Variant | Compiles |
|---------|---------:|
| Linux GCC `-m32` (default) | 10 / 316 |
| Linux GCC native 64-bit (`-DRA_TARGET_ARCH=native`) | 10 / 316 |
| Without case shim (`-DRA_CASE_SHIM=OFF`) | 4 / 316 (175 fail on `function.h`) |
| `-DRA_CXX_STANDARD=98` / `17` | 10 / 316 each |
| MinGW-w64 i686 cross (`-DCMAKE_SYSTEM_NAME=Windows -DCMAKE_CXX_COMPILER=i686-w64-mingw32-g++`) | 27 / 316 (227 of the failures stop at `bios.h`) |
| Unix Makefiles generator instead of Ninja | 10 / 316 (report identical) |

Also checked: the tool exits 2 with a clear message when `compile_commands.json` is missing;
`--summary FILE` appends the totals and top errors; the workflow parses with
`python3 -c "import yaml; yaml.safe_load(open('.github/workflows/build.yml'))"` (jobs
`linux-gcc-x86`, `windows-msvc-x86`).

**Not verified:** the Windows/MSVC path and the GitHub Actions run itself (no Windows machine here,
and the branch is not pushed). The first CI run on GitHub is the real test of the Windows job.

## Determinism impact
None. No game or library source was changed. Note for S06: `RA_PACK_STRUCTS` and `-fsigned-char`
mirror Watcom's `/zp1` and `/j`; both affect struct layout / arithmetic and therefore network
events and sync, so do not change them without the sync harness (S14) or a careful review.

## Problems & open questions
- **Linux native GCC mostly measures "no `windows.h`".** 201 of 306 failures stop there, so the Linux
  number says little until S08 abstracts the platform. Open question for the owner: add a third CI job
  that cross-compiles with MinGW-w64 i686 on Linux? It gets real Windows headers (27/316 today) and is a
  better scoreboard for S06 on Linux.
- **Global 1-byte packing** (`/Zp1`, `-fpack-struct=1`) also packs system-header structs (Windows SDK,
  glibc). Fine for compile progress, but an ABI hazard at link/run time. S06 should move to
  `#pragma pack` around the game's own structs.
- **Missing sources:** `CODE/PASSEDIT.CPP` and `CODE/PALETTE.CPP` are in the makefile but not the repo.
  S01 should check whether anything still needs them (`WOL_LOGN.CPP` includes `PassEdit.h`, also missing).
- **Watcom/DOS headers** (`dos.h`, `bios.h`, `io.h`, `mem.h`, `conio.h`, `pharlap.h`, `new.h`, `iostream.h`) are
  the next wall after `windows.h`. Mostly S05 (dead DOS code) and S06 (compat header).
- Case shim limitation: when the same header name exists in two directories, a header reached through a
  shim may resolve a quoted include to the other directory's copy (ADR-0008).
- The Windows CI job is unverified until the first push.
- **`tools/` vs the original `TOOLS/`:** the new scripts live in `tools/` as assigned, but the repo already
  has an original `TOOLS/` directory. On case-insensitive file systems (Windows, macOS) both share one
  directory on checkout. No file names clash, so it works, but it is confusing; consider renaming the new
  one (e.g. `scripts/`) before merging. Paths to update: `CMakeLists.txt`, `cmake/RACaseShim.cmake`,
  `.github/workflows/build.yml`, `docs/building.md`, ADR-0008, and the generator's header text.

## Next steps
- Coordinator: add S03 status/log link to `docs/sessions/README.md`, the log to `docs/logs/README.md`,
  and ADR-0008 to `docs/decisions/README.md`; push and check the first CI run (especially Windows).
- S05/S06: use `build/compile-progress.md` as the scoreboard. Suggested first wins: a compat header for
  the pre-standard `bool` enum and calling-convention keywords, and removing DOS-only includes.
- S07: the 65 ASM files are listed in `RA_*_ASM_SOURCES` in `cmake/RASources.cmake`.
- S08: add SDL3 at the placeholder in `CMakeLists.txt` once ADR-0004 is approved, and record the
  FetchContent-vs-vcpkg choice in an ADR.

## Review & merge notes (orchestrator)

- Merged into `claude/cool-bohr-9vg7ac` after review on 2026-10-14.
- Renamed the new `tools/` folder to `scripts/`. On case-insensitive file systems (Windows, macOS),
  `tools/` and the original `TOOLS/` would have become one folder. All references were updated, and this log
  now uses the new paths.
- Updated the comments that called ADR-0003/0004 "Proposed". The owner accepted both on 2026-10-14.
- Re-verified after the rename: `python3 scripts/gen_cmake_sources.py --check` reports the lists are up to date, and
  `python3 scripts/compile_progress.py` again gives 10 of 316 (3.2%).
- The open questions (MinGW CI job, global `/zp1` packing, missing `PASSEDIT.CPP`/`PALETTE.CPP`) were added to
  the S06 brief under "Inputs from S03".
