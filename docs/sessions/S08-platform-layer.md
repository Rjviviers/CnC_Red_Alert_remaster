# S08 — Platform layer: SDL video, input, timer

**Status:** Done · **Wave:** 3 · **Depends on:** S01, S03, S05 · **Parallel with:** S06, S07, S09
**Owns:** `WIN32LIB/` video, keyboard/mouse and timer parts; the new `platform/` directory

## Goal
Replace DirectDraw, the Win32 window/message loop and the Windows timers with a thin SDL3 layer.
The game keeps drawing into its 8-bit paletted buffers as before.

## Inputs from S02
- Don't change the layout of `GraphicViewPortClass`/`WWMouseClass` before S07-A/S07-C. Remove the video-memory icon
  cache (`STMPCACH`) together with S07-A. If S08 uses the OS cursor, `WWMOUSE` asm can be dropped instead of ported.

## Inputs from S01
- The Windows message pump is inside the keyboard code (`KEY.CPP:521`), not just the main loop.
- Sockets are serviced through window messages (`WINSTUB.CPP:184`). S12 replaces this with polling.
- Boot sequence and main loop: [game-code.md](../codebase/game-code.md).

## Tasks
- [x] Window creation, palette, and presenting the 8-bit buffer through an SDL texture, with scaling to modern resolutions.
- [x] Keyboard and mouse: feed SDL events into the existing keyboard buffer (`KEYBOARD/`, `KEYFBUFF`).
- [x] Timer: replace the Windows multimedia timers with an SDL- or `std::chrono`-based tick at the original rate.
- [x] Replace the `WndProc`/`GetMessage` main loop with SDL event pumping. Keep a hook for socket polling (S12).
- [x] Document the layer in `docs/codebase/platform-layer.md`.

## Result (S08)
Log: [2026-10-14-S08-platform-layer](../logs/2026-10-14-S08-platform-layer.md). Layer: [platform-layer.md](../codebase/platform-layer.md).
SDL3: [ADR-0020](../decisions/0020-sdl3-via-fetchcontent.md). What S10 still has to wire: platform-layer.md, "Remaining integration for S10".

## Done when
A small test program can open a window, show a palette image and read input through the new layer.
