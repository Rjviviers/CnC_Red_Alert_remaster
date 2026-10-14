# ADR-0020: Get SDL3 with CMake FetchContent from a pinned release tarball

- **Status:** Proposed
- **Date:** 2026-10-14
- **Session:** S08

## Context
[ADR-0004](0004-platform-library.md) (accepted) chose SDL3 behind a thin `platform/` layer. S08 has to make SDL3 a real
dependency. The constraints are:

- Both CI targets are **32-bit** ([ADR-0003](0003-toolchain.md)): MSVC Win32 and Linux GCC `-m32`. Prebuilt SDL3
  packages are almost always 64-bit. Ubuntu has no `libsdl3-dev:i386` in 24.04, and SDL 3 is newer than most distro packages.
- The build must be reproducible, so we pin an exact release.
- The build must work from a plain `cmake -S . -B build` with no extra tool installed, because that is how the
  compile-progress tool and CI already work ([building.md](../building.md)).
- S09 (audio) uses the same SDL3.

## Options considered
1. **FetchContent from the official release tarball, pinned by version and SHA-256, built as a static library.**
   - Pros: no extra tool. It builds with the same compiler and flags, so `-m32`/Win32 just works. The hash pins exactly what
     we build. It is offline-capable via the standard `FETCHCONTENT_SOURCE_DIR_SDL3`. Static linking means no `SDL3.dll` to copy next to tests.
   - Cons: SDL (about 15 MB of source) is compiled in every fresh build directory, which takes 1-2 minutes. On Linux, the 32-bit X11/ALSA
     **development headers** must be installed to get a real window (SDL loads the libraries at run time).
2. **vcpkg** (manifest mode, triplets `x86-windows-static` / `x86-linux`).
   - Pros: binary caching, and a common approach for Windows projects.
   - Cons: it is another tool to bootstrap on every machine and in CI. The `x86-linux` triplet is community-supported, and on
     Linux it still needs the same i386 X11 headers. The SDL version follows the vcpkg baseline commit, which is less direct than a pinned tarball.
     It also changes how people configure (`CMAKE_TOOLCHAIN_FILE`), and every existing instruction would change.
3. **System packages** (`find_package(SDL3)`).
   - Pros: fastest.
   - Cons: no 32-bit SDL3 package on the CI distro, and the version is not pinned. We keep it only as an opt-in (`RA_SDL3_FROM_SYSTEM`).
4. **Vendoring the SDL source in the repo.** This adds 15 MB+ to the repo and makes updates noisy. Rejected.

## Decision
Use option 1: `cmake/RASDL3.cmake` fetches `SDL3-3.4.16.tar.gz` (released 2026-09-02) from the SDL GitHub release, checks
its SHA-256, and builds it static with unneeded subsystems off (camera, sensor, haptic, HIDAPI, dialog, tests, examples and
install). `SDL3::SDL3` is the target the platform layer (and S09) links. `RA_SDL3_FROM_SYSTEM=ON` uses an installed SDL3.
`RA_SDL3_HEADLESS=ON` builds SDL with only its offscreen/dummy video drivers, for Linux machines without 32-bit X11 headers.

Linux CI installs the i386 X11 (`x11`, `xext`, `xrandr`, `xcursor`, `xi`, `xfixes`) and ALSA development packages, so the CI SDL is
the same as a developer's. Tests and the demo run there on SDL's `offscreen` video driver, because there is no display.
To keep that package list short, `RASDL3.cmake` turns off the X11 extensions the game does not need (XTest, XShape,
XScrnSaver, Xdbe, XSync).

## Consequences
- A fresh build directory spends 1-2 minutes compiling SDL. CI does this on every run, and it could later use `actions/cache` on `build/_deps`.
- Updating SDL means changing `RA_SDL3_VERSION` and `RA_SDL3_SHA256` together in `cmake/RASDL3.cmake`.
- PulseAudio/PipeWire development packages are not installable for i386 next to the amd64 ones on Ubuntu 24.04, so the Linux
  32-bit SDL only has ALSA (plus disk/dummy) audio. ALSA reaches PulseAudio/PipeWire through their ALSA plugins. This affects S09.
- If vcpkg is adopted for other dependencies later, this ADR should be superseded instead of running two mechanisms.
