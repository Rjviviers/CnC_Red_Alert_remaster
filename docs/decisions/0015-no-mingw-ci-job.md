# ADR-0015: No MinGW-w64 cross-compile job in CI (use it locally as a Windows-header check)

- **Status:** Accepted (approved by project owner 2026-10-15)
- **Date:** 2026-10-14
- **Session:** S06a

## Context
S03 suggested a MinGW-w64 i686 cross-compile job: it sees real Windows headers from a Linux runner, and before S06a it
was a better scoreboard than plain Linux GCC (27/316 vs 10/316). [ADR-0014](0014-compiler-compat-layer-and-packing.md)
gave Linux its own Win32 stand-in headers, so the question is whether a third compiler in CI is still worth it.

## Options considered
1. **Add a `mingw-x86` CI job.** Exercises the real Windows SDK headers with GCC diagnostics. But MinGW is not a compiler we
   ship ([ADR-0003](0003-toolchain.md): MSVC on Windows, GCC/Clang on Linux), and its headers differ from MSVC's where the
   old code cares: in C++ MinGW does not define the `min`/`max` macros the game uses (14 files fail only on MinGW because of
   this), and its `dos.h`/`direct.h` differ from both Watcom's and MSVC's. Its wire layout is a third one to pin
   (ADR-0013). Another 3–5 minutes of CI per push.
2. **No CI job; use MinGW locally (chosen).** The MSVC job already covers the real Windows headers in CI. MinGW stays useful
   on Linux machines for a quick check that a change does not break the Windows-host paths of the compatibility layer.

## Decision
Option 2. `.github/workflows/build.yml` is unchanged. To run the check locally (Debian/Ubuntu: `apt install g++-mingw-w64-i686`):

```sh
python3 scripts/compile_progress.py --build-dir build-mingw \
  --cmake-arg=-DCMAKE_SYSTEM_NAME=Windows \
  --cmake-arg=-DCMAKE_C_COMPILER=i686-w64-mingw32-gcc \
  --cmake-arg=-DCMAKE_CXX_COMPILER=i686-w64-mingw32-g++ \
  --cmake-arg=-DRA_BUILD_TESTS=OFF
```

S06a measured 228/319 this way, against 236/319 for Linux GCC.

## Consequences
- MSVC-specific breakage is only seen in the MSVC CI job, which S06b–e must check.
- `CODE/wire_layout_check.cpp` uses MinGW's measured numbers for all `_WIN32` compilers (MinGW defaults to MSVC bit-field
  layout). If the first MSVC CI run disagrees, MSVC's numbers replace them there.
- Revisit if MSVC CI becomes too slow or unavailable, or if we ever ship a MinGW build.
