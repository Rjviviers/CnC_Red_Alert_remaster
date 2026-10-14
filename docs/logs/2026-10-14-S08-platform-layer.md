# Session log: S08 — Platform layer: SDL video, input, timer

- **Date:** 2026-10-14
- **Session brief:** [S08](../sessions/S08-platform-layer.md)
- **Branch / PR:** `worktree-agent-ab0d5082bf54248b1` (from `claude/cool-bohr-9vg7ac` at `66de4c5`) / not pushed
- **Status at end:** Done (the platform layer is in review; game boot wiring is the S10 list)

## Goal
Replace DirectDraw, the Win32 window and message loop, and the Windows timers with a thin SDL3 layer behind `platform/platform.h`.
The game keeps drawing into its 8-bit paletted buffers. A small test program must open a window, show a palette image and read input.

## What was done
1. Read the brief, ADR-0003/0004, game-code.md (boot sequence, the message pump in `KEY.CPP`), libraries.md,
   platform-dependencies.md and the assembly layout traps.
2. **Checked the real timer rate.** `WIN32LIB/TIMER/TIMERINI.CPP` calls `timeSetEvent(1000/freq)` with `freq = 60`, which is
   **16 ms**, so the "60 Hz" tick really runs at 62.5 Hz. The port keeps that.
3. **Checked the real resolution:** 640x400 (`GLOBALS.CPP`), or 640x480 with `Resolution=1` in the INI and the view
   40 lines down (`STARTUP.CPP` `InitDDraw`). The 320-wide ModeX path is dead.
4. **SDL3 in CMake** (`cmake/RASDL3.cmake`): FetchContent from the 3.4.16 release tarball with a SHA-256 pin, static.
   3.4.18 was 12 days old, so the previous release was taken.
   - Dead end 1: configuring 32-bit failed because SDL found no 32-bit X11 and refuses to build without X11/Wayland. Installing
     `libx11-dev:i386` and `libxext-dev:i386` (after `dpkg --add-architecture i386`) worked on Ubuntu 24.04.
   - Dead end 2: SDL then required XTest. The unneeded X11 extensions (XTest, XShape, XScrnSaver, Xdbe, XSync) are now switched off.
   - `libpulse-dev:i386` cannot be installed (dependency breaks), so audio is ALSA only.
   - Added `RA_SDL3_HEADLESS` (offscreen/dummy only) for machines without the i386 headers, and `RA_SDL3_FROM_SYSTEM`.
5. **Platform library** `raplatform` (`platform/`): public header without SDL types (packing-proof structs with `static_assert`s),
   SDL back end, pure key translation and viewport code, and a `std::chrono` timer. Built without the game's 1-byte packing and
   WIN32 defines. `win32lib` links it `PUBLIC`.
6. **`platform_demo`** and the unit tests (`platform_keymap_test`, `platform_viewport_test`, `platform_timer_test`) plus a ctest that
   runs the demo on the offscreen driver.
7. **Integration** (rewired, not just planned):
   - `TIMERINI.CPP` `WinTimerClass` now runs on the platform clock.
   - `GBUFFER.CPP`: system-memory surfaces, and the visible page is registered with the platform.
   - `ICONCACH.CPP`: the cache is inert.
   - `DDRAW.CPP`: `Set_Video_Mode`, `Set_DD_Palette`, `Wait_Vert_Blank` and caps now go to the platform layer.
   - `MOUSE.CPP`: a poll hook instead of the timer thread, and platform cursor position and clip.
   - `CODE/KEY.CPP`: the message pump is replaced by `Platform_Pump_Events`, plus modifiers, `To_ASCII` and `Down`.
   - `CODE/STARTUP.CPP`: removed the DirectDraw `GetCaps` check, which would now dereference `NULL`.
8. Mid-session notes from the orchestrator:
   - S07-C merged: `Set_DD_Palette` must exist. It does, `extern "C"`, with the same signature `PAL.CPP` calls.
   - The 320-wide `Blit_Display`, `Colour_Debug`, `ModeX_Blit` and `Set_Palette_Register` cleanup is listed for S10.
   - S07-A merged: `IsDirectDraw` stays `FALSE`, which is the design here. The icon-cache deletion and `STMPCACH.CPP` are listed for S10 (they touch S07-A's files and
     `CDATA.CPP`). `GVPORT.H` is unaffected: no layout changed.
   - Neither branch touches the files changed here, so merging should be clean apart from neighbouring blocks in `tests/tests.cmake`.
9. Verification under Xvfb with scripted input (xdotool) and a screenshot. KEY.CPP was compiled against stub headers and
   driven end to end.
10. Docs: [platform-layer.md](../codebase/platform-layer.md), [ADR-0020](../decisions/0020-sdl3-via-fetchcontent.md),
    building.md "SDL3", a note in libraries.md, the brief ticked and set to `In review`.

## Decisions
- [ADR-0020](../decisions/0020-sdl3-via-fetchcontent.md) (Proposed): SDL3 via FetchContent, pinned tarball and hash, static, not vcpkg.
  ADR-0021 was not needed.
- **Software surfaces everywhere.** `GBC_VIDEOMEM`/`GBC_VISIBLE` buffers are system memory and `IsDirectDraw` is always false.
  The visible page is presented by the platform layer from the event pump, at most every 16 ms and right after palette changes, because the
  game never flips.
- **Software cursor kept** (S07-C ported `WWMOUSE`). It is driven by a poll hook instead of a timer thread, and the OS cursor is hidden over the window.
- **Single-threaded.** No timer or mouse threads. The tick count is computed on read.
- **Default presentation:** windowed, FIT scaling, 4:3 aspect correction (pixels 1.2x tall, like a CRT). Alt+Enter toggles fullscreen
  (desktop fullscreen, no mode change).
- **Faithful key semantics**, including two quirks found in `KEY.CPP` (see platform-layer.md): caps lock and num lock never set the shift bit,
  and ASCII translation ignores caps lock. US-layout ASCII for now.
- **Poll-hook list** for S12. The layer never touches sockets.

## Files changed
| File / folder | Change |
|---------------|--------|
| `platform/` (new) | `platform.h`, `platform.cmake`, `src/{platform_sdl,platform_timer,keymap,viewport}.*`, `demo/platform_demo.cpp` |
| `cmake/RASDL3.cmake` (new) | SDL3 FetchContent, options |
| `CMakeLists.txt` | SDL section replaced by the two includes; `win32lib` links `raplatform` |
| `tests/tests.cmake` (`# --- S08 ---`), `tests/platform_*_test.cpp` (new) | 3 unit tests, plus the demo smoke test |
| `.github/workflows/build.yml` | i386 X11/ALSA dev packages (Linux), "Build platform layer" step (both jobs) |
| `WIN32LIB/TIMER/TIMERINI.CPP` | `WinTimerClass` on the platform clock |
| `WIN32LIB/DRAWBUFF/GBUFFER.CPP` | `DD_Init`/`Un_Init`/`Lock`/`Unlock`/`Attach_DD_Surface`/`DD_Linear_Blit_To_Linear` without DirectDraw |
| `WIN32LIB/DRAWBUFF/ICONCACH.CPP` | `IconCacheClass` without DirectDraw (never caches) |
| `WIN32LIB/MISC/DDRAW.CPP` | Whole file on the platform layer (same functions and globals) |
| `WIN32LIB/KEYBOARD/MOUSE.CPP` | Poll hook, platform cursor position and clip, no critical section or `timeSetEvent` |
| **`CODE/KEY.CPP`** (game file, outside S08's ownership) | `#include "platform.h"`; `Put_Key_Message` modifiers via `Platform_Modifier_Bits`; `To_ASCII` body via `Platform_Key_To_ASCII`; `Down` body via `Platform_Key_Down`; `Fill_Buffer_From_System` body pumps the platform layer |
| **`CODE/STARTUP.CPP`** (game file, outside S08's ownership) | `InitDDraw`: removed the `VisiblePage.Get_DD_Surface()->GetCaps` "video memory" check (15 lines replaced by a 3-line comment) |
| `docs/codebase/platform-layer.md` (new), `docs/decisions/0020-…` (new), `docs/images/s08-platform-demo.png` (new) | Docs and screenshot |
| `docs/building.md`, `docs/codebase/libraries.md`, `docs/sessions/S08-platform-layer.md` | Updates |

No layout of `GraphicViewPortClass`, `GraphicBufferClass`, `WWMouseClass`, `WinTimerClass` or `WWKeyboardClass` changed. No asm file or
S07 replacement was touched.

## Verification
All on Ubuntu 24.04, GCC 13.3, CMake 3.28, `-m32`:

```sh
sudo dpkg --add-architecture i386 && sudo apt-get update
sudo apt-get install libx11-dev:i386 libxext-dev:i386 libxrandr-dev:i386 libxcursor-dev:i386 \
                     libxi-dev:i386 libxfixes-dev:i386 libasound2-dev:i386
cmake -S . -B build -G Ninja -DRA_TARGET_ARCH=x86
#   -- Video drivers: dummy offscreen x11(dynamic) / Audio drivers: alsa(dynamic) disk dummy
#   -- SDL3: building pinned release 3.4.16 (static) ...
cmake --build build --target raplatform platform_demo ra_tests   # no warnings in our code (-Wall -Wextra)
ctest --test-dir build --output-on-failure
#   100% tests passed, 0 tests failed out of 5
#   (smoke_test, platform_keymap_test, platform_viewport_test, platform_timer_test, platform_demo_offscreen)
file build/platform_demo     # ELF 32-bit LSB pie executable, Intel 80386
SDL_VIDEO_DRIVER=offscreen build/platform_demo --frames 120 --dump out.bmp   # exit 0
SDL_VIDEO_DRIVER=dummy build/platform_demo --frames 30                       # exit 0
cmake -S . -B build-headless -G Ninja -DRA_SDL3_HEADLESS=ON && cmake --build build-headless --target ra_tests && ctest --test-dir build-headless
#   Video drivers: dummy offscreen; 5/5 tests pass
python3 scripts/compile_progress.py --build-dir build --no-configure
#   10 of 284 translation units compile (unchanged); the edited WIN32LIB/CODE files still stop at
#   windows.h/ddraw.h/dos.h, which is S06's work
```

**Under Xvfb (real X11 driver) with xdotool input** (`Xvfb :99 -screen 0 1600x1000x24`, then `xdotool` mouse moves, clicks and keys):
the window opens at 640x480 (640x400 at 4:3). Output excerpt:

```
MOUSE key=0x0001 down at (320,200)      <- centre of the window
MOUSE key=0x0002 down at (0,0)          <- window corner
MOUSE key=0x0001 down at (639,399)      <- opposite corner: clamped to the frame
KEY   key=0x0141 vk=0x41 SHIFT ascii=65 'A'
KEY   key=0x0131 vk=0x31 SHIFT ascii=33 '!'
KEY   key=0x0243 vk=0x43 CTRL ascii=3
KEY   key=0x0024 vk=0x24 ascii=0        <- KP_Home with num lock off = VK_HOME
platform_demo: 3906 frames, 354 ticks in 5665 ms (period 16 ms)   <- 62.5 ticks/s
```
Screenshot: [s08-platform-demo.png](../images/s08-platform-demo.png) (cropped from `import -window root`).

**Integration harnesses** (scratch only, not committed: they need stand-in `windows.h`/`wwstd.h` that S06 will provide properly):
- `WIN32LIB/TIMER/{TIMER,TIMERINI,TIMERDWN}.CPP` compiled `-m32 -fpack-struct=1` with stub headers and linked to `raplatform`:
  `ticks in ~1000 ms: 62, TickCount=62 TimerClass=62 countdown(31)=0`.
- `CODE/KEY.CPP` compiled the same way (with `bool` restored on `WWKeyboardClass::Down` in the copy, an S06 fix) and driven
  under Xvfb by xdotool through `Keyboard.Check()/Get()`:
  `key 0x0001 mouse (320,200)`, `key 0x0142 ascii 66 down(now)=1`, `key 0x0242 ascii 2`, `key 0x0026 … down(now)=1`, `key 0x001B ascii 27`.
  So the pump → queue → `WWKeyboardClass` buffer → `MouseQX/QY` path works.
- Not compiled: `GBUFFER.CPP`, `ICONCACH.CPP`, `DDRAW.CPP`, `MOUSE.CPP`, `STARTUP.CPP`. Their headers need DirectDraw types
  (`ddraw.h`), which is S06a's shim. The changes were reviewed by hand.
- The MSVC Win32 build of SDL and `raplatform` was not run here (no Windows machine); CI covers it.

## Determinism impact
None. The tick rate is unchanged: 16 ms per tick, 62.5 ticks/s, exactly what `timeSetEvent(1000/60)` gave. Ticks only pace
frames and UI; lockstep runs on frame numbers (`QUEUE.CPP`). No simulation code, random numbers, struct layouts or network
structures changed. Input arrives through the same `WWKeyboardClass` buffer in the same encoding.

## Problems & open questions
- Linux 32-bit audio: SDL gets only ALSA (no `libpulse-dev:i386`). Mention to S09.
- `GameInFocus` must be set from the platform focus callback (S10), or `WWMouseClass::Process_Mouse` never draws the cursor.
  `GameInFocus` is also declared `bool` in the game and `BOOL` in the library (S06/S10).
- `KEY.H` still has `typedef int bool` (unless `__BORLANDC__`). The platform API returns real `bool`, so S06's removal of that
  typedef is required anyway before linking.
- Non-US keyboard text entry uses a US ASCII table (as the original effectively did for US Windows); SDL text input could replace it.
- `KEY.CPP` and `STARTUP.CPP` are S06's compile-fix territory, so S06 may see small merge conflicts in those functions.

## Next steps
- S10: the 14-item list in [platform-layer.md, "Remaining integration for S10"](../codebase/platform-layer.md#remaining-integration-for-s10)
  (entry point, window/WndProc removal, focus callback, quit, `InitDDraw` leftovers, MPEG movie path, icon-cache deletion,
  320-wide leftovers, video options).
- S12: register socket polling with `Platform_Add_Poll_Hook`.
- S09: link `SDL3::SDL3` for audio (already available), aware of ALSA-only on 32-bit Linux.
- Optional: cache `build/_deps` in CI to skip rebuilding SDL.

## Review & merge notes (orchestrator)

- Merged into `claude/cool-bohr-9vg7ac` on 2026-10-14 with no conflicts. The merged tree configures with SDL 3.4.16 (FetchContent),
  builds `raplatform` and `platform_demo`, and `ctest` reports 27/27 passing (GCC `-m32`, Linux). Compile progress is unchanged at 36 of 311:
  the rewired WIN32LIB files still wait on S06's shims.
- Passed the SDL/ALSA/headless notes to S09, and added this session's S10 handoff to the S10 brief ("Inputs from S08").
