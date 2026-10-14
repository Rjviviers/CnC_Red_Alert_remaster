# S10 — Link & boot to main menu → **Milestone M1**

**Status:** Not started · **Wave:** 4 · **Depends on:** S06, S07, S08, S09

## Goal
Link the executable, load the player's MIX files, and reach the main menu.

## Inputs from S09
- *(Update: S06a fixed the `_WIN32` problem and `RA_HAVE_SDL3` now defaults to ON. Test real playback in S10.)* Audio boots with the **null backend** by default. To get real sound, enable `-DRA_HAVE_SDL3=ON` (S09's `audio_sdl3.cpp`, linked to
  `SDL3::SDL3`). On 2026-10-14 it did **not** compile on Linux, because the build still defined `_WIN32` there, so SDL took its Windows
  path (`process.h`). S06a was asked to stop defining `WIN32`/`_WIN32` off Windows. Once that lands, turn the option on by default and
  test playback, then mark S09 Done.
- `CODE/EXTERNS.H:127-128` / `CODE/INIT.CPP:1202-1203` reference the removed `SoundObject`/`PrimaryBufferPtr` (sent to S06a).

## Inputs from S08
- Work through the "Remaining integration for S10" list in [platform-layer.md](../codebase/platform-layer.md): replace `WinMain`
  with `main`, remove the old window code, wire the focus callback (the cursor won't draw until `GameInFocus` is set), handle quit/shutdown,
  clean up `InitDDraw`, disable the DirectDraw MPEG movie path, delete the icon cache and `STMPCACH.CPP`, remove the 320-wide
  leftovers, and add video options to the INI.
- The timer runs at the original's real rate: 62.5 Hz (16 ms), not 60 Hz.
- `GBUFFER.CPP`, `ICONCACH.CPP`, `DDRAW.CPP`, `MOUSE.CPP` and `STARTUP.CPP` were rewired by S08 but reviewed only by hand, because they need
  S06's `windows.h`/`ddraw.h` shims to compile. Compile and check them first.
- MSVC has not built the platform layer locally; check the first CI result.

## Tasks
- [ ] Resolve every unresolved symbol, adding documented stubs where a feature is out of scope (movies, WOL, …).
- [ ] Asset path configuration: the player points the game at their Red Alert data folder (CLI flag + INI). Document it in `docs/playing.md`.
- [ ] Skip or stub the intro movies (ADR-0005). S05 did **not** stub movie playback; it is still to do here.
- [ ] Detect whether the Counterstrike/Aftermath MIX files are present (ADR-0011).
- [ ] Visually check the ported shape renderer against the original game (S07-B): stealth tank, aircraft shadows, fading, remapped house colours.
- [ ] Fix startup crashes until the main menu shows and responds to the mouse.
- [ ] CI produces downloadable builds for Windows and Linux.

## Done when
**M1:** a CI-built binary reaches the main menu. The session log includes a screenshot.
