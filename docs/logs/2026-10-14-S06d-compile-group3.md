# Session log: S06d — Compiler compatibility: game files group 3

- **Date:** 2026-10-14 (finished 2026-10-15, after an API-limit interruption)
- **Session brief:** [S06](../sessions/S06-compiler-compat.md) (sub-session S06d, files `MI`–`SK`)
- **Branch / PR:** `worktree-agent-ae3a8428b05d24f97` (not pushed) / —
- **Status at end:** Done (all 16 files compile; S06 as a whole stays In progress)

## Goal
Make `CODE/` MISSION, MIXFILE, MONOC, MSGBOX, NETDLG, OBJECT, RADAR, RAWFILE, READLINE, REINF, RULES, SAVELOAD,
SCENARIO, SCORE, SESSION and SKIRMDLG compile with GCC `-m32`, without changing behaviour.

## What was done
1. Fast-forwarded to `claude/cool-bohr-9vg7ac` (`99742a1`). Baseline: 236 of 319 translation units compile.
2. Fixed each file; the fixes fall into a few patterns (details in [compat-notes](../codebase/compat-notes.md#s06d-notes)):

| File | Error | Fix |
|------|-------|-----|
| MISSION | `static ... Is_Recruitable_Mission(...) const` | dropped the `const` on the out-of-class definition of a static member (meaningless; Watcom ignored it) |
| MIXFILE | same for `MixFileClass<T>::Offset(...)` | same |
| MONOC | Watcom `#pragma aux output` (6845 CRT port I/O for the MDA debug display); narrowing in the line-draw table | pragma kept under `#ifdef __WATCOMC__`, otherwise a documented no-op `static inline output()`; `(char)0xDA` casts |
| MSGBOX | `bool retval` holds button index 2 (compiled, but wrong) | `int retval` |
| MSGBOX, SKIRMDLG | `Fancy_Text_Print(TXT_NONE, 0, 0, TBLACK, ...)`: an enumerator with value 0 as the `RemapControlType *` argument | `(RemapControlType *)NULL` (the same null pointer Watcom passed) |
| NETDLG, SKIRMDLG, REINF, SAVELOAD, SCENARIO, SCORE | for-loop variable used after the loop (`index` becomes POSIX `index()`) | declaration hoisted in front of the loop |
| OBJECT | `IsToDisplay = false` inside a `const` member; `Take_Damage(..., int forced)` vs header `bool forced` | `const_cast<ObjectClass *>(this)->IsToDisplay`; definition takes `bool` (only tested for truth) |
| RADAR | implicit `int` (`static _last_pos = -1;`) | `static int` |
| RAWFILE | `SEM_FAILCRITICALERRORS`, `BY_HANDLE_FILE_INFORMATION`, `GetFileInformationByHandle` missing | added to the Win32 stand-in `windows.h` (SDK values/layout, declaration only) |
| READLINE, SESSION, SCENARIO, SAVELOAD | temporary bound to a non-const reference (`ini.Load(CCFileClass("X"))`, `Read_Line(FileStraw(file), ...)`) | named local object |
| REINF (and SCENARIO, ...) | `ScenarioInit++` / `--` on a `bool` | `ScenarioInit` declared `int` in `EXTERNS.H` (see Decisions) |
| RULES | `Sub_Saturate(1, 1)`: in-class friend found only by ADL, no `fixed` argument | `Sub_Saturate(fixed(1), 1)` (same overload Watcom picked) |
| SCENARIO | `bool BGMessageBox` vs `int BGMessageBox` in `FUNCTION.H`; narrowing in `_scorepal` | definition (and its local `retval`) made `int`; `(char)` casts |
| SCORE | narrowing in `_bluepal`/`_redpal`/`_yellowpal`; `index` and `i` after their loops | `(char)` casts; hoisted |
| SESSION | `FILE_ATTRIBUTE_TEMPORARY` missing | added to the stand-in `windows.h` (0x100) |

3. Audited the 16 files for `bool` variables that hold more than 0/1 (Watcom `bool` = 4-byte `int`): searched for
   `++`/`--`/`+=`/`-=` and assignments of 2..9 to every `bool` local/parameter, and for `return` of other values.
   Found one that compiled silently: `WWMessageBox::Process` (MSGBOX) kept the pressed button in `bool retval` and set it
   to 2 for the third button, which a real `bool` turns into 1. Made it `int`.
4. Override check (after S06e found `VesselClass::Take_Damage` no longer overriding): re-ran OBJECT, MISSION and
   SCENARIO (which includes nearly every class header) with `-fsyntax-only -Woverloaded-virtual=2`. The only `bool`/`int`
   parameter mismatch in the whole hierarchy is `VesselClass::Take_Damage(..., int)` vs `FootClass`/`ObjectClass`
   `(..., bool)` (S06e's file). The other hidden virtuals (`BulletClass::Overlap_List()`/`AnimClass::Overlap_List()` vs
   `ObjectClass::Overlap_List(bool)`, `BuildingClass::Remap_Table()` non-const, `IPXGlobalConnClass` packet functions,
   heap `ID`/`Logical_ID`) differ in arity or `const`, so they were not overrides on Watcom either: original behaviour,
   unchanged. Nothing in the `ObjectClass`/`MissionClass` declarations I touched changes an override.
5. Full report, unit tests, docs.

## Decisions
- **`ScenarioInit` is `int`, not `bool`** (`CODE/EXTERNS.H`). It is a nesting counter: 143 uses, many `ScenarioInit++` …
  `ScenarioInit--` pairs that nest (`Create_One_Of` inside scenario loading), and `int save_init = ScenarioInit;` /
  `int old = ScenarioInit;` save/restore it. On Watcom `bool` was `typedef int`, so it counted. With the real 1-byte
  `bool` (ADR-0014) a nested `--` would switch it off while the outer level is still active. `int` restores the original
  behaviour exactly. **`GLOBALS.CPP:469` (`bool ScenarioInit;`, S06c's file) must become `int ScenarioInit;`**; I did
  not touch it. Side effect: BULLET, FACTORY, TACTION and VESSEL (S06b/c/e files) now compile without any change.
- **`BGMessageBox` returns `int`.** `FUNCTION.H` already said `int`; the body sets `retval` to 0, 1 or 2 ("more") and
  compares it with `morebutton-1`, so a real `bool` would have broken the "more" button.
- **MONOC no-op:** user-mode port I/O is impossible; only the hardware cursor of the monochrome debug screen is lost.
  (`MonoClass::Raw_Ptr` still points at 0xB0000 and would crash if `MonoClass::Enable()` were ever called on a modern OS.
  Not a compile problem; noted for whoever ports the debug output.)
- No ADR needed (0024 not used).

## Files changed
| File / folder | Change |
|---------------|--------|
| `CODE/MISSION.CPP` … `CODE/SKIRMDLG.CPP` (the 16 files) | fixes above |
| `CODE/EXTERNS.H` (header) | `extern int ScenarioInit;` |
| `CODE/compat/win32/windows.h` (header) | `BY_HANDLE_FILE_INFORMATION`, `GetFileInformationByHandle`, `FILE_ATTRIBUTE_TEMPORARY`, `SEM_FAILCRITICALERRORS` |
| `docs/codebase/compat-notes.md` | `## S06d notes` |
| `docs/logs/2026-10-14-S06d-compile-group3.md` | this log |

## Verification
```
python3 scripts/compile_progress.py          # before: 236 of 319 (74.0%)   after: 256 of 319 (80.3%)
                                             # all 16 S06d files: pass; + BULLET, FACTORY, TACTION, VESSEL
cmake --build build --target ra_tests && ctest --test-dir build
                                             # 100% tests passed, 0 tests failed out of 31
```
No previously passing file fails (16 + 4 newly passing = +20).

## Determinism impact
- **`ScenarioInit` `bool` → `int`:** restores the Watcom behaviour (counter). Simulation-relevant (it gates
  `Unlimbo`/reveal/trigger logic during scenario setup and reinforcement creation); with `bool` it would have diverged
  from the original. `sizeof` is 4 again; it is a plain global, not in a network packet. It is not written to save
  games directly (`SAVELOAD.CPP` only sets it to 0).
- **`ObjectClass::Take_Damage(..., bool forced)`** (simulation): `forced` is only used as `forced ||` and `!forced`;
  any non-zero int maps to `true`, so the result is identical.
- **`Sub_Saturate(fixed(1), 1)`** (RULES, simulation constants): the same overload (`fixed const &, unsigned`) Watcom
  chose for `(1, 1)`, raw value 255; bit-exact.
- **OBJECT `const_cast`**: same store as before.
- For-loop hoisting: the variables keep the values they had on Watcom; no iteration order changed.
- `NETDLG`/`SESSION`: no packet struct or protocol change (one hoisted loop variable; a named `RawFileClass` for
  the config file; two stand-in constants).
- **Counter check (`bool++`/`--`):** the only `bool` used as a counter in these files is `ScenarioInit` → `int`.
  `MSGBOX`'s `retval` (0/1/2) → `int`; UI only (3-button message boxes), but it restores the original return value.
- **Override check:** no virtual in `OBJECT`/`MISSION` stopped overriding because of `bool` vs `int` (see step 4); the
  known one is `VesselClass::Take_Damage` (S06e).
- Everything else (palettes, MONOC, message boxes) is UI only.

## Problems & open questions
- **For S06c:** change `GLOBALS.CPP:469` to `int ScenarioInit;` (otherwise a conflicting declaration).
- **For S06b:** `CARRY.CPP:140` `bool oldscen = ScenarioInit;` truncates the counter when saving/restoring; make it
  `int` (the Watcom original was effectively `int`). Fix `ScenarioInit++/--` errors elsewhere by relying on the
  `int` declaration, **not** by rewriting them to `= true/false`.
- `BUILDING.CPP`, `INFANTRY.CPP`, `UNIT.CPP` define `Take_Damage(..., int forced)` against `bool` in their headers:
  same fix as OBJECT (S06b/c/e).
- `EDIT.CPP` (S06b) and `LIST.H` have the same `Fancy_Text_Print(TXT_NONE, 0, 0, TBLACK, ...)` pattern.

## Next steps
Orchestrator: merge with S06b/c/e; apply the `GLOBALS.CPP` one-liner if S06c did not.

## Review & merge notes (orchestrator)

- Merged into `claude/cool-bohr-9vg7ac` on 2026-10-15, after S06e, S06c and S06f. Conflicts: `CODE/EXTERNS.H` (the third identical
  `ScenarioInit` → `int` change; kept one) and `compat-notes.md` (kept all sections).
- The follow-ups this session asked for were already done: `GLOBALS.CPP:469` is `int` (S06c), `CARRY.CPP:140` is `int` (orchestrator, during
  the S06e merge), `INFANTRY`/`UNIT` `Take_Damage` match (S06c/S06e). `BUILDING.CPP:1194` still defines `int forced`; that's S06b's file and
  S06b has been told.
- On the merged tree (GCC `-m32`): `ctest` 31/31; compile progress 292 of 307. The 15 failing files are exactly S06b's group.
