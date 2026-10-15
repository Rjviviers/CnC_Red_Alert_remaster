# Session log: S10b — Link: platform entry point, Win32 replacements and stubs

- **Date:** 2026-10-15
- **Session brief:** [S10](../sessions/S10-link-and-boot.md) (split S10a/S10b/S10c, this is S10b)
- **Branch / PR:** `worktree-agent-a47e4eb28a7a45d1e` (based on `claude/cool-bohr-9vg7ac` with S10a merged) / not pushed
- **Status at end:** Done. `redalert` (`build/ra95`) links on Linux (GCC `-m32`) with 0 unresolved symbols.

## Goal
Resolve every unresolved symbol that is not a template (S10a's part): replace `WinMain` with a portable `main` on the
SDL platform layer, remove the Win32 window/message-loop code, make file I/O portable (case-insensitive on Linux),
and stub what is out of scope (movies, Winsock until S12, asm leftovers), with documentation.

## What was done
1. Fast-forwarded to `claude/cool-bohr-9vg7ac` (`97674ba`), configured (`-DRA_TARGET_ARCH=x86`, SDL from a local
   source copy via `FETCHCONTENT_SOURCE_DIR_SDL3`) and linked: **393 unresolved**, identical to
   `docs/codebase/link-unresolved-2026-10-15.txt`; 80 of them mine. Mapped each to the object files that reference it
   with `nm -C --undefined-only` (script in the scratchpad, not committed).
2. **File I/O** (`CODE/compat/ra_file.{h,cpp}`, ADR-0022): C runtime descriptors on every host, Win32 open
   semantics per access mode, case-insensitive path resolution off Windows, DOS date/time conversion.
   `RAWFILE.H`/`RAWFILE.CPP` switched over (handle `int`, `-1` closed). Unit test `tests/s10b_file_test.cpp`.
3. **Remaining Win32 calls off Windows**: real implementations in `CODE/compat/win32_api.cpp` (`FindFirstFile` sorted
   like NTFS, `GetSystemTime`, `GlobalMemoryStatus`, `GetLastError`, `GetDriveType`, `OutputDebugString`, read-only
   `CreateFile`/`CloseHandle`) and stubs in `CODE/stubs/win32_stubs.cpp` (registry, `GetVolumeInformation`,
   `SendMessage`, `ShowCursor`). Windows links the real API (`ws2_32 winmm advapi32 user32`).
4. **Movies**: first planned to stub the VQA player's leaves; instead `redalert` no longer links `vqa32wp`, and
   `CODE/stubs/movie_stubs.cpp` provides the nine VQA functions the game calls (`VQA_Alloc` returns NULL, so every
   movie is skipped). That removed `UnVQ_4x2`, `VQA_sosCODEC*`, `AudioUnzap`, `DirectSoundCreate`, `timeSetEvent`,
   `timeKillEvent`, the priority-class calls, `MCGA_*`, `WaitVB`/`WaitNoVB` and all `Mono_*` from the link without
   stubbing them. `MPEGMOVIE` turned off in `DEFINES.H` (DirectDraw DVD player); `MPGSET.CPP` then failed to compile
   (`PlayMpegMovie` undeclared), so its `#ifdef DVD` became `#if defined(DVD) && defined(MPEGMOVIE)`.
5. **Winsock** (`CODE/stubs/winsock_stubs.cpp`, non-Windows): `WSAStartup` fails, so the network reports
   "not available" until S12. `closesocket` is a real `close`.
6. **asm leftovers** (`CODE/stubs/asm_stubs.cpp`): `CPUType`, `VendorID`, `Detect_MMX_Availability`, `Force_VM_Page_In`.
7. **Entry point** (`STARTUP.CPP`): `WinMain` → `main` (see platform-layer.md "How `main` works"). `InitDDraw` is the
   only video set-up; its dead `GBC_VIDEOMEM`/`GetCaps` branch and `DDSCAPS` are gone; its failure path no longer
   deletes `WindowsTimer` (the caller's `Game_Shutdown` does, which would otherwise have been a double free).
   `Emergency_Exit` calls `Game_Shutdown(true)` instead of posting `WM_DESTROY`.
8. **Window code** (`WINSTUB.CPP`): deleted `Create_Main_Window`, `Windows_Procedure`, `Window_Dialog_Box`,
   `Colour_Debug`, `CCFocusMessage`. Added `Install_Platform_Callbacks` (focus callback, quit poll hook, focus-loss
   pointers), `Game_Shutdown` (the `WM_DESTROY` body), `Got_Focus` (the `CCFocusMessage` body). `Check_For_Focus_Loss`
   pumps the platform instead of `PeekMessage`. `Memory_Error_Handler` and `Assert_Failure` use `Game_Shutdown` +
   `exit` (and `Assert_Failure` writes `ASSERT.TXT` with stdio).
9. `KEY.CPP`/`KEY.H`: deleted `WWKeyboardClass::Message_Handler` (its `DefWindowProc` was unresolved).
10. Mid-session the orchestrator merged S10a; I merged it (fast-forward, my work was uncommitted) and re-ran CMake
    configure for `ENUMOPS.H`.
11. The first full link then failed on a **duplicate `TickCount`** (it was already in the first link log, hidden among
    the undefined references): `CODE/GLOBALS.CPP` and `WIN32LIB/TIMER/TIMERINI.CPP` each define one, of different
    types. Watcom/MSVC mangle the type into a variable's name; GCC does not. Renamed the library's with a macro in
    both `WIN32LIB` `TIMER.H` copies. Fixed `extern BOOL GameInFocus` → `extern bool` in `MOUSE.CPP` (same class of
    bug: the definition is a 1-byte `bool`).
12. Smoke-ran the binary: without `REDALERT.INI` it shows "Run SETUP program first" and exits 0; with a minimal
    lower-case `redalert.ini` (found case-insensitively) it opens the window (offscreen) and crashes in
    `MixFileClass::Offset` → `strupr` on a string literal (`Init_Fonts`). That is S10c's crash-fixing; noted in
    compat-notes.md.
13. Docs: ADR-0022, `docs/codebase/stubs.md`, platform-layer.md (integration list ticked, "How `main` works"),
    compat-notes.md "S10b notes".

## Decisions
- [ADR-0022](../decisions/0022-portable-file-io-and-win32-replacements.md) (Proposed): CRT-descriptor file I/O with
  case-insensitive lookup, one Win32 shim module per kind off Windows, no VQA player in the link.
- Closing the window now exits the game (`Emergency_Exit(EXIT_SUCCESS)` from a poll hook). The original ignored
  `SC_CLOSE` (Alt+F4 did nothing).
- `GameInFocus` starts `true` once the window is open: the platform reports focus *changes* only, and under an
  offscreen/headless driver focus may never arrive, which would hide the cursor forever and make
  `Check_For_Focus_Loss` suspend the music every frame.
- `Check_For_Focus_Loss` sleeps 10 ms per call while out of focus (the original spun on `PeekMessage`).
- The low-disk-space yes/no question became a warning (the platform message box has only OK); the game continues.
- `main` still `chdir`s to the executable's directory, as the original did; S10c decides with the asset path.

## Files changed
| File / folder | Change |
|---------------|--------|
| `CODE/STARTUP.CPP` | `WinMain` → portable `main`; `InitDDraw` cleanup; `Emergency_Exit` via `Game_Shutdown` |
| `CODE/WINSTUB.CPP` | Window class/procedure/dialog/colour-debug code removed; focus callback, quit hook, `Game_Shutdown`; portable `Assert_Failure`, `Memory_Error_Handler` |
| `CODE/RAWFILE.CPP`, `CODE/RAWFILE.H` | Win32 file API → `ra_file.h`; handle is `int`, `-1` closed |
| `CODE/KEY.CPP`, `CODE/KEY.H` | `Message_Handler` removed (other session's file; minimal) |
| `CODE/DEFINES.H` | `MPEGMOVIE` no longer defined (shared header; one line) |
| `CODE/MPGSET.CPP` | Guard `DVD && MPEGMOVIE` (one line) |
| `WIN32LIB/KEYBOARD/MOUSE.CPP` | `extern bool GameInFocus` (one line) |
| `WIN32LIB/INCLUDE/TIMER.H`, `WIN32LIB/TIMER/TIMER.H` | `#define TickCount WWLib_TickCount` for library code |
| `CODE/compat/ra_file.h`, `CODE/compat/ra_file.cpp` | New: portable file I/O |
| `CODE/compat/win32_api.cpp` | New: Win32 calls on non-Windows hosts |
| `CODE/stubs/{movie,winsock,win32,asm}_stubs.cpp` | New: documented stubs |
| `cmake/RASources.cmake` | The six new sources in `RA_GAME_CXX_SOURCES` |
| `CMakeLists.txt` | `redalert` drops `vqa32wp`; Windows links `ws2_32 winmm advapi32 user32`, MSVC `/ENTRY:mainCRTStartup` |
| `tests/s10b_file_test.cpp`, `tests/tests.cmake` | New unit test |
| `docs/decisions/0022-…md`, `docs/codebase/stubs.md` | New |
| `docs/codebase/platform-layer.md`, `docs/codebase/compat-notes.md` | Updated |

## Verification
```
cmake -S . -B build -G Ninja -DRA_TARGET_ARCH=x86 -DFETCHCONTENT_SOURCE_DIR_SDL3=<local SDL 3.4.16 source>
cmake --build build --target redalert -- -k 0
  before (97674ba):            393 unresolved (= link-unresolved-2026-10-15.txt), 1 multiple definition (TickCount)
  after S10a merge (938a463):  80 unresolved, all S10b's
  after S10b:                  0 unresolved, 0 multiple definitions; build/ra95 (6.2 MB) produced
python3 scripts/compile_progress.py --build-dir build --no-configure
  313 of 313 translation units compile (100.0%)      (307 + 6 new files)
cmake --build build --target ra_tests && ctest --test-dir build
  100% tests passed, 0 tests failed out of 33        (s10b_file_test: 5 tests, 0 failed)
SDL_VIDEO_DRIVER=offscreen SDL_AUDIO_DRIVER=dummy ./build/ra95
  "Red Alert: Run SETUP program first."  exit 0      (no REDALERT.INI next to the binary)
  with a minimal redalert.ini: window opens, then SIGSEGV in strupr <- MixFileClass::Offset <- Init_Fonts (S10c)
i686-w64-mingw32-g++ -std=gnu++14 tests/s10b_file_test.cpp CODE/compat/ra_file.cpp   compiles (Windows path; not run)
```
MSVC was not built locally: check the first CI run (`/ENTRY:mainCRTStartup`, the Windows branch of `ra_file.cpp`).

## Determinism impact
No simulation code, struct layout of network events, random number generator or iteration order of simulation
objects changed. Things near it:
- `RawFileClass` now reads through `ra_file`. The bytes returned (RULES.INI, scenarios, MIX contents) are the same;
  only error codes differ (ignored by every `Error()` override).
- `FindFirstFile` (Linux) returns `*.PKT`/`*.MPR` in case-insensitive name order, the NTFS order, so the multiplayer
  map list is built in the same order as on Windows. Without sorting it would follow `readdir` order.
- `GetSystemTime` seeds `CryptRandom` (key exchange, not the lockstep `Random`).
- `GlobalMemoryStatus` sets `UseBigShapeBuffer` (shape cache in rendering only; > 16 MB as before).
- `TickCount` rename: the library's `TimerClass` (palette fades, memory-node ages) and the game's
  `TTimerClass<SystemTimerClass>` are separate objects again, as on Watcom/MSVC. Neither drives the lockstep frame count.
- `RawFileClass` changes size? No: `HANDLE` (pointer) → `int`, both 4 bytes on the 32-bit build; it is never saved.
- `KEY.H`: a non-virtual member function removed; no layout change.

## Problems & open questions
- `TickCount`-style collisions (same variable name, different types) may exist elsewhere and only show at run time
  if both sides are `extern`-declared with matching but wrong types (like `GameInFocus`). Worth a scan.
- `_dos_findfirst` (`dos/dos.h`) returns `readdir` order; `INIT.CPP` registers `SC*.MIX`/`SS*.MIX` with it.
- `ReadyToQuit` is `bool` but the code stores 0..3 in it; harmless now that nothing waits on it.
- `vqm32wp` is still linked; nothing checked whether the game uses anything from it.

## Next steps
For S10c (boot):
- Crash at boot: `MixFileClass::Offset` upper-cases its `filename` argument in place (`MIXFILE.CPP:538`), a string
  literal → SIGSEGV. Copy to a buffer first; check the other `strupr` calls.
- `REDALERT.INI` is required (missing → "Run SETUP program first", exit). Ship or generate a default.
- `main` `chdir`s to the executable's directory; data files are then looked up there (any case). Decide with the asset path.
- Expansion detection: the registry stubs make CS/AM read as "not installed"; replace with MIX detection (ADR-0011).
- No CD drives are ever found (`GetDriveType` stub); CD-request dialogs should never be reached once data comes from a folder.
- Run headless with `SDL_VIDEO_DRIVER=offscreen SDL_AUDIO_DRIVER=dummy`; `GameInFocus` starts true so the cursor draws.

## Review & merge notes (orchestrator)

- Merged into `claude/cool-bohr-9vg7ac` on 2026-10-15 with no conflicts. Verified on the merged tree (GCC `-m32`): the **full build
  succeeds**, including `ra95` (ELF 32-bit); 313/313 compile; `ctest` 33/33. Running `ra95` with no INI under
  `SDL_VIDEO_DRIVER=offscreen SDL_AUDIO_DRIVER=dummy` prints "Red Alert: Run SETUP program first." and exits 0.
- The Windows link (`/ENTRY:mainCRTStartup`, the Windows branch of `ra_file.cpp`) is first checked by CI on this push.
- Behaviour changes accepted: closing the window exits the game; `GameInFocus` starts true.

## CI fix after merge: `s10b_file_test` on MSVC (orchestrator, 2026-10-15)
- **Symptom:** CI run 32 (Windows/MSVC) failed `s10b_file_test`, case `resolve_missing`: `RA_Resolve_Path(..., must_exist=true)`
  returned true for a file that doesn't exist. Linux passed.
- **Root cause:** the `_WIN32` branch of `RA_Resolve_Path` passed every name through and ignored `must_exist`. The POSIX branch
  checks existence. A real bug, not a test problem: callers such as `RA_File_Open` (read) rely on it.
- **Fix:** with `must_exist`, the Windows branch now `stat`s the name and returns false if it's missing. The header comment says so too.
- **Checked here:** Linux test passes; the Windows branch compiles and links with MinGW (i686), and the new binary contains the extra
  `stat` call. No Wine is available to run it, so CI on this push is the run-time check.
