# S09 — Audio via SDL

**Status:** Not started · **Wave:** 3 · **Depends on:** S02, S03, S05 · **Parallel with:** S06, S07, S08
**Owns:** `WIN32LIB/AUDIO/`, the sound code in `CODE/` (identified in S01/S02)

## Goal
Replace HMI SOS and DirectSound with SDL audio. Sound effects and music must play.

## Inputs from S02
- The HMI SOS driver is compiled out on Win32; only its ADPCM codec is used (C++ version exists: `CODE/ADPCM.CPP`).
- DirectSound is used only in `WIN32LIB/AUDIO` and `WINVQ/VQA32/AUDIO.CPP` (movies, which are stubbed).

## Tasks
- [ ] Step 1: a no-op audio backend, so S10 isn't blocked on audio.
- [ ] Step 2: decode Westwood AUD (IMA ADPCM / Westwood ADPCM) in C++, with tests.
- [ ] Step 3: mix effects and stream music through SDL, keeping the existing volume and priority behaviour.
- [ ] Document it in `docs/codebase/audio.md`.

## Done when
In-game sounds and music play on Windows and Linux.
