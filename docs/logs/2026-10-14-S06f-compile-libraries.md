# Session log: S06f — Compiler compatibility: support libraries

- **Date:** 2026-10-14 (finished 2026-10-15)
- **Session brief:** [S06](../sessions/S06-compiler-compat.md) ("Status after S06a"; S06f is the library sub-task)
- **Branch / PR:** `worktree-agent-a11735ce96a7e1aff` (worktree of `claude/cool-bohr-9vg7ac` at `99742a1`) / not pushed
- **Status at end:** Done

## Goal
Make each of the 19 failing `WIN32LIB/` and `WINVQ/` files compile with GCC `-m32`, or take it out of the build with a
documented, evidence-based reason. No behaviour change, no `-fpermissive`-style shortcuts, no edits to `CODE/*.CPP`.

## What was done
1. Fast-forwarded to `claude/cool-bohr-9vg7ac` (`99742a1`, S06a merged). Baseline: **236 / 319** (win32lib 59/74,
   vqa32wp 7/9, vqm32wp 15/17).
2. For every function each file defines, grepped `CODE/`, `WIN32LIB/`, `WINVQ/` and `tests/` for callers, and checked
   whether calls sit inside `#if` switches that are `0`. Rule applied: **fix a file that live code calls; remove a file
   that has no live caller or that the game shadows with its own copy.**
3. Fixes (details in [compat-notes.md "S06f notes"](../codebase/compat-notes.md#s06f-notes)): trailing Ctrl-Z bytes in
   `DELAY.CPP`, `PALETTE.CPP`, `WINDOWS.CPP`, `WINHIDE.CPP` and the component headers `SHAPE/SHAPE.H`, `WW_WIN/WW_WIN.H`;
   `extern "C" extern` in `PALETTE/PALETTE.CPP` and the component `PALETTE/PALETTE.H`; a Watcom `for` scope in
   `ICONCACH.CPP`; a `GetPriorityClass` declaration in the stand-in `windows.h` for `TASK.CPP`. `GETSHAPE.CPP` compiled once
   `SHAPE.H` was clean.
4. Removals (details and evidence in [removed-code.md §7](../codebase/removed-code.md#7-dead-or-shadowed-support-library-files-s06f)):
   deleted `WRITELBM`, `IFF/WRITEPCX`, `KEYBOARD`, `FINDARGV`, `IRANDOM`, `LOADPAL`, `MORPHPAL` and the whole
   `WIN32LIB/MONO/` directory; took `WPROFILE.CPP` (+ `APROFILE.ASM`), `CAPTION.CPP`, `VIDEO.CPP` and `VESAVID.CPP` out of
   `cmake/RASources.cmake` but kept the files, because source still names their functions.
5. Dead end: `LOADPAL.CPP` compiled after the `PALETTE.H` fix, but it has no caller, so it was removed under the same rule
   as `MORPHPAL.CPP`.

| File | Decision | Reason |
|------|----------|--------|
| `DRAWBUFF/ICONCACH.CPP` | fixed | Called from `CODE/CDATA.CPP`, `CODE/WINSTUB.CPP` (not owned here), so the cache and `STMPCACH.CPP` cannot be deleted yet (platform-layer.md item 10) |
| `IFF/WRITELBM.CPP` | removed + deleted | no caller |
| `IFF/WRITEPCX.CPP` | removed + deleted | shadowed by `CODE/WRITEPCX.CPP` |
| `KEYBOARD/KEYBOARD.CPP` | removed + deleted | shadowed by `CODE/KEY.CPP` (S08 item 8) |
| `MISC/DELAY.CPP` | fixed | `Delay()` called by `MENUS`, `INIT`, `TECHNO` |
| `MISC/FINDARGV.CPP` | removed + deleted | no caller |
| `MISC/IRANDOM.CPP` | removed + deleted | no caller; game RNG is `CODE/RANDOM.CPP` (untouched) |
| `MONO/MONO.CPP` | removed + deleted (dir) | shadowed by `CODE/MONOC.CPP` |
| `PALETTE/LOADPAL.CPP` | removed + deleted | no caller |
| `PALETTE/MORPHPAL.CPP` | removed + deleted | no caller |
| `PALETTE/PALETTE.CPP` | fixed | `Fade_Palette_To` called by `CODE/PALETTE.CPP` |
| `PROFILE/WPROFILE.CPP` | removed (file kept) | developer tool; `Start_Profiler` never called |
| `SHAPE/GETSHAPE.CPP` | fixed (header) | `Extract_Shape`, `Get_Shape_Width`, … used by the game and `WWMOUSE.CPP` |
| `WW_WIN/WINDOWS.CPP` | fixed | `WindowList`, `Change_Window` used by the game |
| `WW_WIN/WINHIDE.CPP` | fixed | `Window_Hide_Mouse` used by `CODE/DESCDLG.CPP` |
| `VQA32/CAPTION.CPP` | removed (file kept) | callers under `VQACAPTIONS_ON` = 0 |
| `VQA32/TASK.CPP` | fixed (header) | `VQA_Alloc`/`VQA_Play` called by `CONQUER`/`INTRO` until movies are stubbed |
| `VQM32/VESAVID.CPP` | removed (file kept) | DOS VESA BIOS; only caller `VESABLIT.CPP` is behind `VQAVESA_ON` = 0 |
| `VQM32/VIDEO.CPP` | removed (file kept) | DOS `int 10h`; callers under `VQAVIDEO_ON` = 0 |

## Decisions
No ADR needed (0026 unused): every removal follows ADR-0005 (movies stubbed), ADR-0012 (hand-maintained lists) and the
libraries.md verdicts. Small decision: dead library files are removed rather than fixed, even when the fix is trivial.

## Files changed
| File / folder | Change |
|---------------|--------|
| `cmake/RASources.cmake` | 13 entries removed (12 C++, 1 asm), with pointer comments |
| `WIN32LIB/DRAWBUFF/ICONCACH.CPP`, `MISC/DELAY.CPP`, `PALETTE/PALETTE.CPP`, `WW_WIN/WINDOWS.CPP`, `WW_WIN/WINHIDE.CPP` | compile fixes |
| **Header edits:** `WIN32LIB/SHAPE/SHAPE.H`, `WIN32LIB/WW_WIN/WW_WIN.H` (Ctrl-Z), `WIN32LIB/PALETTE/PALETTE.H` (`extern "C" extern`), `CODE/compat/win32/windows.h` (`+ GetPriorityClass`) | minimal |
| `WIN32LIB/IFF/WRITELBM.CPP`, `IFF/WRITEPCX.CPP`, `KEYBOARD/KEYBOARD.CPP`, `MISC/FINDARGV.CPP`, `MISC/IRANDOM.CPP`, `PALETTE/LOADPAL.CPP`, `PALETTE/MORPHPAL.CPP`, `WIN32LIB/MONO/` | deleted |
| `docs/codebase/removed-code.md` | §7 added |
| `docs/codebase/compat-notes.md` | "S06f notes" appended |
| `docs/codebase/libraries.md` | S06f update note; MONO/PROFILE verdicts |
| `docs/codebase/platform-layer.md` | items 8 and 14 annotated |

## Verification
```
python3 scripts/compile_progress.py
  before: 236 of 319 (win32lib 59/74, vqa32wp 7/9, vqm32wp 15/17, redalert 155/219)
  after:  243 of 307 (win32lib 65/65, vqa32wp 8/8, vqm32wp 15/15, redalert 155/219 unchanged)
cmake --build build --target win32lib vqa32wp vqm32wp      -> all three static libraries build
cmake --build build --target raplatform platform_demo ra_tests && ctest --test-dir build
  -> 100% tests passed, 0 tests failed out of 31
```

## Determinism impact
None. No simulation code was touched; `IRANDOM.CPP` was never called and the game RNG (`CODE/RANDOM.CPP`) is unchanged.

## Problems & open questions
- `CODE/WINSTUB.CPP:229` `Stop_Profiler()` is now an unresolved symbol at link time. It sits in `Windows_Procedure`,
  which S10 deletes (platform-layer.md item 14).
- `TASK.CPP` needs `GetPriorityClass`/`SetPriorityClass` at link time; they disappear when the movie player is stubbed.

## Next steps
- S10: delete the `Stop_Profiler()` call; remove the icon cache (`ICONCACH.CPP`, `STMPCACH.CPP`, the `GBUFFER.H` branch and the
  `CDATA`/`WINSTUB` calls) per ADR-0016; stub the VQA player per ADR-0005, after which the whole `WINVQ/` tree can go.

## Review & merge notes (orchestrator)

- Merged into `claude/cool-bohr-9vg7ac` on 2026-10-15 after S06e and S06c (only conflict: appended sections in `compat-notes.md`; kept all).
- Verified on the merged tree (GCC `-m32`): `win32lib`, `vqa32wp` and `vqm32wp` **build completely** as targets; `ctest` 31/31;
  compile progress 276 of 307 (the total drops because of the removed files).
- `Stop_Profiler()` in `CODE/WINSTUB.CPP:229` (now unresolved) is left for S10, as recorded in `platform-layer.md`.
