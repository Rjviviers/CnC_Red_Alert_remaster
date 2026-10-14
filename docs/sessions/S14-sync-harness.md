# S14 — Sync & determinism harness

**Status:** Not started · **Wave:** 6 · **Depends on:** S11 · **Parallel with:** S13

## Goal
Automatically detect simulation divergence, so we catch desyncs before players do.

## Tasks
- [ ] Record and replay a game's events (the original `Queue_Record`/playback can be a starting point).
- [ ] Log a per-frame CRC of the game state; a tool compares two logs and reports the first frame that differs.
- [ ] A CI test: replay a recorded game twice (and on both compilers) and assert identical CRCs.
- [ ] Clearer desync handling in-game: report the frame number and save debug info.
- [ ] Document it in `docs/codebase/determinism.md`.

## Done when
CI catches an intentionally introduced desync.
