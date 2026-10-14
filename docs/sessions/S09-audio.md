# S09 — Audio via SDL

**Status:** In review · **Wave:** 3 · **Depends on:** S02, S03, S05 · **Parallel with:** S06, S07, S08
**Owns:** `WIN32LIB/AUDIO/`, the sound code in `CODE/` (identified in S01/S02)

## Goal
Replace HMI SOS and DirectSound with SDL audio. Sound effects and music must play.

## Inputs from S02
- The HMI SOS driver is compiled out on Win32; only its ADPCM codec is used (C++ version exists: `CODE/ADPCM.CPP`).
- DirectSound is used only in `WIN32LIB/AUDIO` and `WINVQ/VQA32/AUDIO.CPP` (movies, which are stubbed).

## Tasks
- [x] Step 1: a no-op audio backend, so S10 isn't blocked on audio. (Backend interface + null backend; DirectSound removed.)
- [x] Step 2: decode Westwood AUD (IMA ADPCM / Westwood ADPCM) in C++, with tests. (Also took over package S07-E.)
- [ ] Step 3: mix effects and stream music through SDL, keeping the existing volume and priority behaviour.
  Mixer, volume/priority behaviour and streaming are done and tested on the null backend; the SDL3 backend is written
  (`WIN32LIB/AUDIO/audio_sdl3.cpp`) but behind `RA_HAVE_SDL3` (OFF) until S08's SDL3 is merged. Remaining: switch it on and listen.
- [x] Document it in `docs/codebase/audio.md`.

## Done when
In-game sounds and music play on Windows and Linux.

Session log: [2026-10-14-S09-audio.md](../logs/2026-10-14-S09-audio.md).
