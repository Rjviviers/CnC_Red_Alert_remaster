# ADR-0008: Generated case-insensitive include shim instead of renaming files

- **Status:** Proposed
- **Date:** 2026-10-14
- **Session:** S03

## Context
The original code was written on DOS/Windows, where file names are case-insensitive and `\` is a
path separator. Almost every file is named in upper case (`FUNCTION.H`) but included in lower or
mixed case (`#include "function.h"`, `#include "PassEdit.h"`), and some includes use backslashes
(`<vqm32\all.h>`, `"wolapi\wolapi.h"`, `<sys\stat.h>`). On Linux these fail: in S03, without any
workaround, 175 of 316 files stopped at `function.h` and only 4 compiled.

Constraints: session S03 must not modify game or library sources (that is S06/S07), several agents
work in parallel on the same tree, and the Windows/MSVC build must keep working unchanged.

## Options considered
1. **Rename files / rewrite every `#include`.** The clean end state, but it touches hundreds of files,
   conflicts with every parallel session, and ruins `git blame`/history for the original code. Out of
   scope for S03.
2. **Build on a case-insensitive file system** (e.g. ext4 casefold, a mounted image). No code changes,
   but it needs root or special setup on every developer machine and CI runner, and does not fix
   backslashes.
3. **Generated shim in the build tree.** At configure time a script scans every `#include` spelling
   and creates symlinks with those exact spellings that point to the real files, one shim directory
   per source/include directory, added to the include path. Backslashes are just characters in a
   Linux file name, so a link named `vqm32\all.h` works too. System headers spelled oddly
   (`<sys\stat.h>`, `<WINDOWS.H>`) get one-line forwarding headers searched last.
   No source changes, no setup; but a subtle include-order difference is possible (below).
4. **Compiler wrapper that rewrites paths.** Hard to make reliable across GCC and Clang, and invisible.

## Decision
Option 3. `tools/make_case_shim.py` generates `build/case_shim/` on every CMake configure, on
non-Windows hosts only (`RA_CASE_SHIM`, default OFF on Windows). Wiring in `cmake/RACaseShim.cmake`:
each include directory `D` is followed by its shim on the `-I` path; each source file gets
`-iquote <shim of its own directory>` so its quoted includes still find sibling headers first;
`_system/` forwarders are added with `-idirafter`.

## Consequences
- Linux builds work without touching the original files; S03 measured 10 files compiling instead of 4,
  and the remaining errors are real porting problems (`windows.h`, `dos.h`, the pre-standard `bool` enum).
- Search order is close to, but not exactly, Watcom's. A header reached through a shim resolves its own
  quoted includes in the shim directory first. Where two directories contain a header with the same name
  (e.g. `CODE/FUNCTION.H` and `WIN32LIB/INCLUDE/FUNCTION.H`), a header in one directory could pick up the
  other directory's copy. If S06 sees "wrong header" errors on Linux only, suspect this.
- Adding or changing an `#include` needs a CMake re-run (the progress tool re-configures by default).
- Requires Python 3 at configure time on Linux, and symlink support (fine on Linux/macOS).
- This is a **transitional** measure. Once S06 normalises `#include` spellings to match the real
  file names (or renames files to lower case), the shim becomes a no-op and should be removed by a
  new ADR that supersedes this one.
