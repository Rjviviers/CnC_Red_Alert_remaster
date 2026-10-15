# Session log: S06b — Compiler compatibility: game files group 1

- **Date:** 2026-10-14 (finished 2026-10-15, after an API usage-limit interruption)
- **Session brief:** [S06](../sessions/S06-compiler-compat.md) ("Status after S06a", group S06b)
- **Branch / PR:** `worktree-agent-ad6e54a88bbf1644d` (worktree `.claude/worktrees/agent-ad6e54a88bbf1644d`) / not pushed, merged by the orchestrator
- **Status at end:** Done

## Goal
Make the 16 game files `AIRCRAFT ANIM BUILDING BULLET CCINI CDFILE CELL CONQUER CONST COORD DIALOG DISPLAY DYNAVEC EDIT
EGOS EVENT` (in `CODE/`) compile with GCC `-m32`, without changing behaviour. Port `COORD.CPP`'s inline-asm `calcx`/`calcy`
(simulation code) bit-exactly and test them against the original instructions.

## What was done
1. `git merge --ff-only claude/cool-bohr-9vg7ac` (head `99742a1`). Configured `build/` (Ninja, `-DRA_TARGET_ARCH=x86`),
   baseline `python3 scripts/compile_progress.py`: **236 / 319**, all 16 files failing.
2. Small files, one cause each (see the table below): `for`-scope, `bool` vs `int` parameter in `Take_Damage`
   definitions, temporaries bound to `Straw &`/`Pipe &`, `TBLACK` as a null pointer, `this->` for a dependent base,
   `house->Name` without `()`, `_HARDERR_FAIL`, a member written in a `const` function.
3. `BULLET.CPP`: `ScenarioInit--` on a `bool`. `ScenarioInit` is a nesting counter (it was an `int` under Watcom's
   `typedef int bool`), so it became `int` in `EXTERNS.H` instead of rewriting `--` as `= false`. S06c/S06e reached the
   same conclusion in parallel; on merging the updated base the base's `EXTERNS.H` line was kept (same change) and
   `GLOBALS.CPP` already defines it as `int`.
4. `CELL`, `DISPLAY`, `EGOS`: the Watcom `for`-scope errors (`index` turning into POSIX `index()`, `cell`, `i`) were
   fixed with a small script that hoists the declaration of the governing `for` (the nearest `for (TYPE NAME =` whose
   enclosing block contains the use, i.e. Watcom's scope) and refuses `for`s that are the body of an unbraced `if`.
   Every hoist was reviewed in the diff. First attempt read GCC's columns wrongly (tabs count as 8 by default); fixed with
   `-fdiagnostics-column-unit=byte` and the bad edit reverted.
5. `EGOS.CPP` then failed with "`DynamicVectorClass<EgoClass *> EgoList` has incomplete type": `FUNCTION.H` has
   `class DynamicVectorClass<EgoClass *>;` (and five similar lines), which standard C++ reads as an explicit
   specialisation that is never defined. Wrapped in `#ifdef __WATCOMC__`.
6. `CONST.CPP`: 11 cheat-code CRCs ≥ 0x80000000 in `long` tables → `(long)0x...`.
7. `COORD.CPP`: the `#pragma aux calcx`/`calcy` block is now under `#ifdef __WATCOMC__`; other compilers include the new
   `CODE/compat/coord_aux.h`. The four 256-entry tables: 508 entries ≥ 0x80 written as `(signed char)0xNN`, and the two
   tables declared plain `char` are now `signed char` (Watcom `/j` made `char` signed). A script parsed the old and new
   file and confirmed the table values are identical (1028 hex literals, equal sequence). Trap found: the original
   ends the block with a `//` comment ending in `\`, which swallows the following line; a blank line keeps `#else` alive.
8. `tests/s06b_coord_test.cpp` (registered under `# --- S06b ---` in `tests/tests.cmake`): runs the original instruction
   sequence as GCC inline asm (32-bit x86 only) with random garbage in the upper half of `EAX`, and compares the low
   16 bits and `short += result` with the C++ — exhaustively for table values −128..127 × all 65 536 distances, plus
   2 000 000 random full-16-bit pairs; fixed expectations run everywhere. A deliberate mutation of `coord_aux.h` made all
   three tests fail (999 596 mismatches), so the oracle is live.
9. Checks asked for by the orchestrator: (a) no other `bool++`/`bool--` in my files (GCC's `bool` operand diagnostics,
   errors and C++14 deprecation warnings, are gone); (b) override mismatches: compiled all 16 files with
   `-Woverloaded-virtual=1`; the only "hidden" diagnostics are pre-existing ones in `VECTOR.H`/`HEAP.H` (`operator=`,
   `ID`, `Logical_ID` overloads, not `bool`/`int` related). `AIRCRAFT`/`BUILDING` `Take_Damage` definitions said
   `int forced` against `bool forced` in the headers; these were hard errors and now say `bool`.
10. Committed, then merged the updated `claude/cool-bohr-9vg7ac` (S06c and S06e merged meanwhile) into this branch
    (one conflict, `EXTERNS.H` `ScenarioInit`, same change on both sides) and re-verified.

## Decisions
No ADR needed (ADR-0014 patterns). Small decisions:
- `ScenarioInit` → `int` (restores the Watcom semantics) rather than `= true`/`= false`.
- Move_Point's tables declared `signed char` instead of `char`, so their values do not depend on `-fsigned-char`/`/J`.
- `calcx`/`calcy` return the 16-bit result sign-extended; the original's upper 16 bits were undefined and never used.
- `EVENT.CPP` `house->Name` → `house->Name()`: the original printed the function's code bytes as a string.

## Files changed
| File / folder | Change |
|---------------|--------|
| `CODE/AIRCRAFT.CPP` | `face` hoisted out of its `for`; `Take_Damage(..., bool forced)` to match `AIRCRAFT.H` |
| `CODE/ANIM.CPP` | `const_cast` for `IsToDisplay` in `Render() const`; `index` hoisted |
| `CODE/BUILDING.CPP` | `Take_Damage(..., bool forced)` (`ScenarioInit--` fixed by the `EXTERNS.H` change) |
| `CODE/BULLET.CPP` | nothing (fixed by `EXTERNS.H`) |
| `CODE/CCINI.CPP` | named `FileStraw`/`FilePipe` locals instead of temporaries bound to `Straw &`/`Pipe &` |
| `CODE/CDFILE.CPP` | `_HARDERR_FAIL` (3, Watcom `<dos.h>`) defined locally |
| `CODE/CELL.CPP` | `index` hoisted (2 places, one is `Goodie_Check`) |
| `CODE/CONQUER.CPP` | `x`, `y` hoisted in the shape-bounds function |
| `CODE/CONST.CPP` | `(long)` casts on 11 CRCs ≥ 0x80000000 |
| `CODE/COORD.CPP` | asm under `#ifdef __WATCOMC__`, `#include "coord_aux.h"` otherwise; 508 `(signed char)` casts; 2 tables `char` → `signed char` |
| `CODE/DIALOG.CPP`, `CODE/EDIT.CPP` | `(RemapControlType *)NULL` instead of `TBLACK` for the pointer argument |
| `CODE/DISPLAY.CPP` | `index` (×4), `cell` (×2), `i` hoisted |
| `CODE/DYNAVEC.CPP` | `this->IsAllocated`, `this->VectorMax` |
| `CODE/EGOS.CPP` | `index` hoisted |
| `CODE/EVENT.CPP` | `house->Name()` |
| `CODE/EXTERNS.H` (header) | `ScenarioInit` `bool` → `int` (identical to the base's S06c/S06e change after the merge) |
| `CODE/FUNCTION.H` (header) | six bare `class DynamicVectorClass<...>;` lines under `#ifdef __WATCOMC__` |
| `CODE/compat/coord_aux.h` | new: C++ `calcx`/`calcy` |
| `tests/s06b_coord_test.cpp`, `tests/tests.cmake` | new test, registered at the end under `# --- S06b ---` |
| `docs/codebase/compat-notes.md` | `## S06b notes` appended |

## Verification
- `python3 scripts/compile_progress.py --no-configure -j 16` (GCC 13.3, `-m32`):
  - on the S06a base `99742a1`: **236 → 255 / 319**; all 16 S06b files pass (redalert 155 → 174). The 3 extra
    are other groups' files whose only error was `ScenarioInit--`, fixed by the `EXTERNS.H` change. The library
    targets are unchanged (win32lib 59, vqa32wp 7, vqm32wp 15) and every still-failing game file is in another group's range.
  - after merging the current base `22de8d5` (S06c + S06e): **284 / 319** (redalert 203/219). The remaining 16 failing
    game files are all in S06d's range (`MISSION`…`SKIRMDLG`).
- after merging the base again with S06c–f (`3b2d5da`): **307 / 307 (100 %)**: redalert 219, win32lib 65, vqa32wp 8,
  vqm32wp 15 (S06f removed some library files, hence the smaller total).
- `cmake --build build --target ra_tests && ctest --test-dir build`: **32/32 passed** (including `s06b_coord_test`, 0.56 s).
- Mutation check of the oracle: a perturbed `coord_aux.h` compiled against the test → 3/3 tests fail.
- Table values: scripted comparison of the old and new `COORD.CPP` literals (signed-char interpretation) → identical.

## Determinism impact
- **`calcx`/`calcy` (`Move_Point`, `Normal_Move_Point`, so `Coord_Move` and everything that moves):** C++ port, low
  16 bits bit-exact with the original instructions (exhaustive over the table range × every distance; random over the
  full 16-bit range). The upper 16 bits were undefined in the original and every caller truncates to `short`.
- **`COORD.CPP` tables:** values unchanged (−3 for `0xfd`, as Watcom `/j` stored). The two `char` tables are now
  `signed char`, which removes a dependency on the compiler's `char` signedness; same values with `-fsigned-char`/MSVC.
- **`ScenarioInit` is `int` again:** this restores the Watcom behaviour (nested `++`/`--`). With a 1-byte `bool` the
  nested brackets would have ended early. `CARRY.CPP:140` (not mine) saving it in a `bool` was already noted by S06e.
- **`Take_Damage(..., bool forced)` in `AIRCRAFT.CPP`/`BUILDING.CPP`:** definitions now match the headers. They
  failed to compile before, so no silent override change; callers pass `true`/`false`, so the value is the same.
  Override scan with `-Woverloaded-virtual=1`: no `bool`/`int` mismatch in my files.
- **`for`-scope hoists in `CELL.CPP` (`Overlap_Down`, `Goodie_Check` — crate selection is simulation), `ANIM.CPP`,
  `AIRCRAFT.CPP`:** the variable has exactly the scope and value Watcom gave it; no evaluation-order change.
- **`CONST.CPP` codes:** same 32-bit patterns (cheat/editor passwords, not simulation).
- Everything else (UI text, dialogs, credits, `DYNAVEC` name lookup, `const_cast` in `AnimClass::Render`): no simulation effect.
- Integer widths/signedness otherwise unchanged; no float/fixed math touched.

## Problems & open questions
- The session's scratchpad directory is shared with other agents (a helper script of mine was overwritten by another
  agent's); use a per-session subdirectory.
- `FUNCTION.H` change is shared: if another group meets "incomplete type" for `DynamicVectorClass<TeamMissionClass *>`
  and friends, it is the same fix (already applied to all six lines here).

## Next steps
- S06d's files (`MISSION`…`SKIRMDLG`) are the last failing game files on GCC.
- Run the sync harness (S14) over movement once it exists; `s06b_coord_test` covers the asm helpers only.

## Review & merge notes (orchestrator)

- Merged into `claude/cool-bohr-9vg7ac` on 2026-10-15, last of S06b–f, with no conflicts (the session had already merged S06c–f).
- Verified on the merged tree (GCC `-m32`): **307 of 307** translation units compile; `win32lib`, `vqa32wp`, `vqm32wp`, `raplatform` and
  `platform_demo` build; `ctest` 32/32 (including `s06b_coord_test`, which checks `calcx`/`calcy` against the original instructions).
