# Building

> **Current state (session S03, SDL3 added in S08):** the build system works, but the game does not compile yet.
> The code was written for Watcom C++ 10.6 and TASM; making it compile is sessions
> [S06](sessions/S06-compiler-compat.md) (C++) and [S07](sessions/S07-asm-to-cpp.md) (assembly).
> Until then, the useful thing to run is the **compile-progress report**, which tries every
> source file on its own and shows how many compile. A normal `cmake --build` is expected to fail.

## What you need

| | Windows | Linux |
|---|---------|-------|
| Compiler | Visual Studio 2022 (or Build Tools) with "Desktop development with C++" | GCC (or Clang) **with 32-bit support** |
| CMake | 3.20 or newer (ships with Visual Studio) | 3.20 or newer |
| Generator | Ninja (ships with Visual Studio) | Ninja (recommended) or Make |
| Python | 3.8+ (for the progress report) | 3.8+ (also needed to configure, see [case shim](#file-name-case-on-linux)) |
| Game assets | Not needed to build. Never commit them ([ADR-0006](decisions/0006-no-game-assets.md)). | |

The target is **32-bit x86** ([ADR-0003](decisions/0003-toolchain.md), accepted).
**SDL3** ([ADR-0004](decisions/0004-platform-library.md)) is downloaded and built automatically when you configure;
see [SDL3](#sdl3) below. Configuring needs internet access the first time (or a local SDL source tree).

## Linux

1. Install the tools (Debian/Ubuntu):

   ```sh
   sudo apt install cmake ninja-build python3 gcc-multilib g++-multilib
   # 32-bit development files SDL3 needs for a real window and sound (headers only;
   # SDL loads the libraries at run time):
   sudo dpkg --add-architecture i386 && sudo apt update
   sudo apt install libx11-dev:i386 libxext-dev:i386 libxrandr-dev:i386 libxcursor-dev:i386 \
                    libxi-dev:i386 libxfixes-dev:i386 libasound2-dev:i386
   ```

   `gcc-multilib`/`g++-multilib` provide the 32-bit C/C++ runtime headers and libraries for `-m32`.
   Without them, configuring stops with a message telling you to install them.
   Without the `:i386` X11 packages SDL's configure stops too; add `-DRA_SDL3_HEADLESS=ON` to build
   without a real window instead (see [SDL3](#sdl3)).

2. From the repository root, run the progress report (configures `./build` on first use):

   ```sh
   python3 scripts/compile_progress.py
   ```

3. Open `build/compile-progress.md`.

Doing the steps by hand:

```sh
cmake -S . -B build -G Ninja                 # configure (32-bit by default)
cmake --build build --target compile-progress  # = python3 scripts/compile_progress.py --no-configure
cmake --build build                          # real build: fails until S06/S07
```

No 32-bit support available? Configure natively instead (64-bit; layout-sensitive code will
misbehave, so use it only to look at compile errors):

```sh
python3 scripts/compile_progress.py --build-dir build-native --cmake-arg=-DRA_TARGET_ARCH=native
```

## Windows

1. Install Visual Studio 2022 (Community is fine) with the **Desktop development with C++**
   workload, and Python 3.
2. Open **"x86 Native Tools Command Prompt for VS 2022"** (Start menu). It must be the *x86* one:
   the target is 32-bit, and configuring with a 64-bit compiler stops with an error.
3. From the repository root:

   ```bat
   python scripts\compile_progress.py
   ```

   This configures `build\` with Ninja and writes `build\compile-progress.md`.

By hand:

```bat
cmake -S . -B build -G Ninja
cmake --build build --target compile-progress
```

The Visual Studio IDE generator also works for browsing the code
(`cmake -S . -B build-vs -G "Visual Studio 17 2022" -A Win32`), but it does not write
`compile_commands.json`, so the progress report needs a Ninja (or NMake) build directory.

## The compile-progress report

`scripts/compile_progress.py` reads `compile_commands.json` from the build directory, runs every
compile command independently and in parallel, never stops on errors, and writes:

| File | Contents |
|------|----------|
| `build/compile-progress.md` | Totals per target, most common first errors, per-file pass/fail with the first error line |
| `build/compile-progress.json` | The same data for scripts (S06 can diff runs) |
| `build/compile-progress-logs/<target>/<file>.log` | Full compiler output for each file |

Useful options: `--filter WIN32LIB/` (only some files), `-j N` (parallel jobs),
`--build-dir DIR`, `--no-configure`, `--cmake-arg=-DNAME=VALUE` (repeatable).

It exits with status **0 whenever the report was produced**, even if nothing compiles.
It exits with **2** if the build system is broken (configure failed, no `compile_commands.json`,
compiler could not be started). CI relies on that.

## SDL3

SDL3 is the only third-party dependency ([ADR-0004](decisions/0004-platform-library.md)). `cmake/RASDL3.cmake`
fetches the pinned release tarball (**SDL 3.4.16**, checked by SHA-256) with CMake `FetchContent` at configure time and
builds it as a **static** library with the same compiler and architecture as the game
([ADR-0020](decisions/0020-sdl3-via-fetchcontent.md)). The first configure of a build directory downloads about 15 MB, and the
first build compiles SDL (1-2 minutes). Targets that use it:

| Target | What |
|--------|------|
| `SDL3::SDL3` | static SDL3 (platform layer; audio in S09) |
| `raplatform` | the platform layer, `platform/` ([platform-layer.md](codebase/platform-layer.md)). Real target: must always build |
| `platform_demo` | test program: test pattern, palette cycling, input echo |

```sh
cmake --build build --target raplatform platform_demo
./build/platform_demo                                  # Esc quits, F11/Alt+Enter fullscreen
SDL_VIDEO_DRIVER=offscreen ./build/platform_demo --frames 120   # no display needed
```

| Option | Default | Meaning |
|--------|---------|---------|
| `RA_SDL3_FROM_SYSTEM` | `OFF` | Use an installed SDL3 (`find_package(SDL3)`) instead of building the pinned one. It must be 32-bit for `RA_TARGET_ARCH=x86`. |
| `RA_SDL3_HEADLESS` | `OFF` | Linux: build SDL with only its `offscreen`/`dummy` video drivers (no X11/Wayland headers needed). Everything builds and the tests run; no window can open. |
| `FETCHCONTENT_SOURCE_DIR_SDL3` | — | Standard CMake: path to an unpacked SDL source tree, to configure offline. |
| `RA_SDL3_URL` / `RA_SDL3_SHA256` | the pinned release | Change both together to update SDL. |

**Linux, 32-bit.** SDL chooses its video and audio back ends from the development headers it finds. For the `-m32` build these
are the `:i386` packages listed under [Linux](#linux). With them you get the X11 back end and ALSA audio, which also reaches
PulseAudio/PipeWire through their ALSA plugins. `libpulse-dev:i386` cannot be installed next to the 64-bit packages on Ubuntu 24.04,
so there is no native PulseAudio back end. Wayland is not built for 32-bit; the X11 back end works under XWayland.
SDL's optional X11 extensions XTest, XShape, XScrnSaver, Xdbe and XSync are switched off to keep that package list short.

**CI** installs the same `:i386` packages and runs the tests and `platform_demo` (ctest `platform_demo_offscreen`) on SDL's
**`offscreen` video driver** (`SDL_VIDEO_DRIVER=offscreen`), because CI machines have no display. The Windows job builds SDL
with MSVC x86 and uses the same `offscreen` driver for the test.

## Build options

All architecture, standard and flag choices live in one file:
[`cmake/RABuildConfig.cmake`](../cmake/RABuildConfig.cmake). Change them there, not in the targets.

| Option | Default | Meaning |
|--------|---------|---------|
| `RA_TARGET_ARCH` | `x86` | `x86` = 32-bit (`-m32` on GCC/Clang; the x86 MSVC environment on Windows). `native` = compiler default. Needs a fresh build directory to change. |
| `RA_CXX_STANDARD` | `14` | C++ standard (GNU extensions on). |
| `RA_LANGUAGE` | `ENGLISH` | Language define (`ENGLISH`/`GERMAN`/`FRENCH`), as in `CODE/MAKEFILE`. |
| `RA_PACK_STRUCTS` | `ON` | 1-byte struct packing everywhere, like Watcom `/zp1` (`/Zp1`, `-fpack-struct=1`). S06 decides the final policy. |
| `RA_CASE_SHIM` | `ON` except on Windows | Generate the case-insensitive include shim ([ADR-0008](decisions/0008-case-insensitive-include-shim.md)). |

## Targets and where the source lists come from

| CMake target | Original | Sources |
|--------------|----------|---------|
| `raplatform`, `platform_demo` | new in S08 (`platform/platform.cmake`) | `platform/` — always built for real, see [SDL3](#sdl3) |
| `redalert` (output `ra95`) | `CODE/MAKEFILE` → `ra95.exe` | `OBJECTS` (+ the `!ifdef WIN32` additions), `LIBFILES` (jshell.lib) and `TECHFILES` (tech.lib) |
| `win32lib` | `WIN32LIB/MAKEFILE` → `win32lib.lib` | `OBJECTS` of each of the 18 component libraries in `LIBS` |
| `vqa32wp` | `WINVQ/VQA32/MAKEFILE` | `OBJECTS` |
| `vqm32wp` | `WINVQ/VQM32/MAKEFILE` | `OBJECTS` |

The lists are in [`cmake/RASources.cmake`](../cmake/RASources.cmake). They are **hand-maintained**
([ADR-0012](decisions/0012-hand-maintained-source-lists.md)): S03 generated them from the original
makefiles with `scripts/gen_cmake_sources.py`, and S05 froze that output and deleted the generator
when it started deleting files. When you add, delete or rename a source file, edit
`cmake/RASources.cmake` in the same commit. The original makefiles (`CODE/MAKEFILE`, the
`WIN32LIB/*/MAKEFILE`s, ...) are kept for reference only; they still name files that S05 deleted.

Assembly (`.ASM`, TASM syntax) files are kept in `RA_*_ASM_SOURCES` but not compiled; S07
replaces them. Objects named in the original makefile without a source file in the repository are listed
in `RA_*_MISSING_OBJECTS` (currently `CODE/PALETTE`; `CODE/PASSEDIT` went with Westwood Online in S05).

### Defines and include paths

Mirrored from the original makefiles, minus the Watcom-specific options (full mapping in the
header of `cmake/RABuildConfig.cmake`):

- All targets: `WIN32=1`, `_WIN32`.
- Game: `ENGLISH=1`; includes `WIN32LIB/INCLUDE`, `WINVQ/INCLUDE`. (The original also defined
  `WOLAPI_INTEGRATION` and `WINSOCK_IPX`; S05 collapsed the sources as if they were defined and
  dropped them, see [removed-code.md](codebase/removed-code.md).)
- `win32lib`: includes `WIN32LIB/INCLUDE`. `vqa32wp`/`vqm32wp`: includes `WINVQ/INCLUDE`.
- Dropped: Watcom's own headers, the DirectX 5 SDK (`..\dxsdk\inc`; the Windows SDK has
  `ddraw.h`/`dsound.h`) and Greenleaf (`..\gcl510\H`, modem code, removed in S05). None are in the repo.

## File-name case on Linux

The sources were written on DOS/Windows, where file names are case-insensitive and `\` is a path
separator. They write `#include "function.h"` for `FUNCTION.H`, and `#include <vqm32\all.h>`.
On Linux, without help, 175 files fail on `function.h` alone.

We do not rename any source file. Instead, at configure time `scripts/make_case_shim.py` creates
`build/case_shim/`: one directory of symlinks per source/include directory, named exactly as the
code spells them (`function.h → CODE/FUNCTION.H`, a file literally named `vqm32\all.h → WINVQ/INCLUDE/VQM32/ALL.H`).
System headers spelled in upper case or with backslashes (`<WINDOWS.H>`, `<sys\stat.h>`) get a
one-line forwarding header in `build/case_shim/_system/`. Details and trade-offs:
[ADR-0008](decisions/0008-case-insensitive-include-shim.md).

The shim is regenerated on every configure. If you add or change an `#include`, re-run CMake
(the progress tool does this automatically unless `--no-configure` is given).

## Unit tests

Unit tests live in `tests/`. Each test is a small executable that uses the dependency-free framework in
`tests/ra_test.h` (`RA_TEST`, `RA_CHECK`, `RA_CHECK_EQ`, `RA_CHECK_MEM_EQ`, `RA_TEST_MAIN`).
Tests are built with the same settings as the game (32-bit, struct packing, defines), so they see the same layouts.

```sh
cmake --build build --target ra_tests            # build every test
ctest --test-dir build --output-on-failure       # run them
```

To add a test, add a `ra_add_test(...)` call to `tests/tests.cmake`, under the marker comment for your session:

```cmake
ra_add_test(NAME lcw_test
            SOURCES tests/lcw_test.cpp CODE/LCW.CPP
            INCLUDES CODE WIN32LIB/INCLUDE)
```

Paths are relative to the repo root. `tests/tests.cmake` is pulled in with `include()`, not `add_subdirectory()`, so the
existing helpers resolve paths correctly. Test targets are excluded from the compile-progress report, which only
counts `redalert`, `win32lib`, `vqa32wp` and `vqm32wp` (`PROGRESS_TARGETS` in `scripts/compile_progress.py`;
add any new game library target there). Turn tests off with `-DRA_BUILD_TESTS=OFF`.

In CI, **failing unit tests fail the job**, unlike compile errors in the progress report.

## Continuous integration

[`.github/workflows/build.yml`](../.github/workflows/build.yml) runs on every push and pull request:

| Job | Runner | Toolchain |
|-----|--------|-----------|
| `linux-gcc-x86` | `ubuntu-24.04` | GCC + `gcc-multilib`, `-m32`, Ninja; i386 X11/ALSA headers for SDL3 |
| `windows-msvc-x86` | `windows-2022` | MSVC x86 (via `ilammy/msvc-dev-cmd`), Ninja |

Each job configures (which fetches SDL3), **builds the platform layer** (`raplatform`, `platform_demo`; a
failure fails the job), runs the progress tool,
writes the totals and top errors to the job summary, and uploads `compile-progress.md`,
`.json` and the per-file logs as an artifact (`compile-progress-<job>`). A job fails only if
the build system is broken, never because game sources fail to compile.

## Appendix: the original build

For reference (the tools are not available and not needed):

- `CODE/MAKEFILE` (Watcom `wmake`): `wmake -DWIN32` builds `ra95.exe`; without `WIN32` it builds
  the DOS `game.dat` (DOS/4GW). Objects go to `obj\win32\ENGLISH`. The exe links the game objects,
  `jshell.lib` and `tech.lib` (both built from the same directory), `win32lib.lib`,
  `vqa32wp.lib`, `vqm32wp.lib`, `ipx\wwipx32.lib`, `mpgdll.lib`, DirectX 5 and DirectShow libraries.
- `WIN32LIB/MAKEFILE` runs `wmake` in each component directory, then merges the component
  libraries into `win32lib.lib`. Compiler options come from `WIN32LIB/PROJECT.CFG`. The component
  makefiles copy headers into `WIN32LIB/INCLUDE` (so that directory is a copy, and some headers exist twice).
  `RAWFILE` is installed but not part of `LIBS`, so it is not built.
- `WINVQ/VQA32` and `WINVQ/VQM32` each have a `MAKEFILE` (Watcom) plus Borland variants (`MAKEFILE.BOR`).
