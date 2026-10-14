# ADR-0004: SDL3 as the platform layer; drop DirectX

- **Status:** Proposed (awaiting project owner)
- **Date:** 2026-10-14
- **Session:** S00

## Context
The game uses DirectDraw (DirectX 5), DirectSound, Win32 windows and messages, and the proprietary HMI SOS audio library.
None of these are portable, and the old DirectX SDK isn't available.

## Options considered
1. **Modern Windows-only APIs:** Windows only, and a lot of code.
2. **SDL3:** one library for windowing, input, audio and timers on Windows, Linux and macOS. Widely used and zlib-licensed (GPL-compatible).
3. **SDL2:** same idea and very mature, but SDL3 is the current major version.

## Decision
Use SDL3 behind a thin `platform/` layer, so the game code never calls SDL directly.
Fall back to SDL2 if SDL3 causes problems with 32-bit builds. That fallback would need its own ADR.

## Consequences
The game keeps its 8-bit software renderer. We only present the finished frame. Rendering improvements remain possible later.
