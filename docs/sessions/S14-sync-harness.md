# S14 — Sync & determinism harness

**Status:** Not started · **Wave:** 6 · **Depends on:** S11 · **Parallel with:** S13

## Goal
Automatically detect simulation divergence, so we catch desyncs before players do.

## Inputs from Wave 1
- S02: the Win32 screen shake calls `Sim_Random_Pick`. If it runs on only one machine, it desyncs. Check it with the harness.
- S01: three inline-asm helpers (`calcx`/`calcy`, `Fixed_To_Cardinal`) feed unit movement. Verify their C++ replacements here.

- S06a: `bool` is now 1 byte (Watcom: 4), and `EventClass` is 32 bytes (Watcom: 20), so 16 events fit in a packet instead of 26.
  Fine within one compiler (ADR-0013), but measure packet throughput in the LAN tests.
- S06a: glibc's `BIG_ENDIAN` define had made Linux compile the big-endian `FIXED.H`/`DEFINES.H` layouts. This is now fixed and guarded
  by `#error`. Make sure the harness would have caught a bug of this kind (compare a Linux and a Windows replay once cross-checks exist).

## Tasks
- [ ] Record and replay a game's events (the original `Queue_Record`/playback can be a starting point).
- [ ] Log a per-frame CRC of the game state; a tool compares two logs and reports the first frame that differs.
- [ ] A CI test: replay a recorded game twice (and on both compilers) and assert identical CRCs.
- [ ] Clearer desync handling in-game: report the frame number and save debug info.
- [ ] Document it in `docs/codebase/determinism.md`.

## Done when
CI catches an intentionally introduced desync.
