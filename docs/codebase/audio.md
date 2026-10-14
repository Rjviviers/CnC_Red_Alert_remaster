# Audio (`WIN32LIB/AUDIO`)

Written in session S09 (2026-10-14). Covers the original design, the AUD format, the decoders, and the
portable replacement (software mixer + backend interface) that replaced DirectSound.

## 1. The original design (Win95, DirectSound)

The game calls a small C API declared in `WIN32LIB/INCLUDE/AUDIO.H` (a copy of `WIN32LIB/AUDIO/AUDIO.H`):

| Function | Used by the game for |
|----------|----------------------|
| `Audio_Init(HWND, bits, stereo, rate, reverse)` | start-up, `CODE/STARTUP.CPP:272` (mono, 16-bit, 22050 Hz) |
| `Sound_End` | shutdown |
| `Play_Sample(sample, priority, volume, pan)` / `Play_Sample_Handle` | every sound effect and speech (`CODE/AUDIO.CPP:466`, `:708`) |
| `File_Stream_Sample` / `File_Stream_Sample_Vol` | music (`CODE/THEME.CPP:338`) |
| `Sound_Callback` | called every game frame (`CODE/CONQUER.CPP:1307`, `THEME.CPP:213`) to keep streams fed |
| `Stop_Sample`, `Sample_Status`, `Is_Sample_Playing`, `Stop_Sample_Playing`, `Fade_Sample` | sample control |
| `Set_Sound_Vol`, `Set_Score_Vol` | master volumes (0..255) |
| `Load_Sample`, `Free_Sample`, `Load_Sample_Into_Buffer`, `Sample_Read`, `Sample_Length` | loose `.AUD` files |
| `Start/Stop_Primary_Sound_Buffer`, `Restore_Sound_Buffers`, `Set_Primary_Buffer_Format` | focus loss/gain (`CODE/WINSTUB.CPP`) |
| `Get_Digi_Handle`, `Get_Free_Sample_Handle` | misc |
| globals `SampleType`, `SoundType`, `StreamLowImpact`, `Misc`, `Audio_Focus_Loss_Function` | `SampleType != 0` means "sound works" all over `CODE/` |

Internals:

- **5 sample trackers** (`MAX_SFX`, `SOUND.H`), `LockedData.SampleTracker[]` (`SOUNDINT.H`). A handle is a tracker index.
- Each tracker owned a **32 KB DirectSound secondary buffer**. `Play_Sample_Handle` decoded the first 8 KB into it
  and started it looping; `maintenance_callback` (a 40 Hz `timeSetEvent` timer, `MAINTENANCE_RATE`) refilled it a
  quarter (8 KB) at a time through `Sample_Copy`, and stopped it when it played past the end.
- **Priority**: `Get_Free_Sample_Handle` takes the highest-numbered free tracker; if none is free it takes the
  *lowest-numbered* tracker whose priority is `<=` the new sound's, and stops it; otherwise the sound is dropped (-1).
  A free tracker that last held a score is skipped once, to keep a channel for music (see quirk below).
- **Volume**: `volume` (0..255) times the master volume / 256, converted by `Convert_HMI_To_Direct_Sound_Volume`
  to a DirectSound attenuation: `-(exp((255-v)/27.686) - 1)` hundredths of a dB. The curve is very flat:
  128 is only -0.97 dB, 50 is -16 dB, 0 is silent. `Set_Score_Vol` updates the playing score; `Set_Sound_Vol` only
  affects sounds started later. `panloc` was ignored on Win95.
- **Fades**: `Fade_Sample(handle, ticks)` sets `Reducer = Volume/ticks + 1` (`Volume` = vol << 7); each maintenance
  tick subtracts it; `Sound_Callback` stops the sample once silent.
- **Music streaming**: a 16 × 2176-byte ring (`StreamBufferCount`, `StreamBufferSize = 8192/4 + 128`). `File_Stream_Preload`
  reads 12 blocks (2 per call with `real_time_start`), patches the AUD header size to the first block and starts it with
  `Play_Sample_Handle`; the next block is attached as `QueueBuffer`. `Sound_Callback` → `File_Callback` reads more
  blocks; `maintenance_callback` moves the next pending block into `QueueBuffer` when the old one is used up.
  `Simple_Copy` reads across the block boundary.
- HMI SOS itself was compiled out; only its ADPCM codec was used.

## 2. AUD file format

All little-endian. 12-byte header (`AUDHeaderType`, packed):

| Offset | Size | Field |
|-------:|-----:|-------|
| 0 | 2 | sample rate (Hz) |
| 2 | 4 | `Size`: bytes of data after the header |
| 6 | 4 | `UncompSize`: bytes of PCM after decoding |
| 10 | 1 | flags: bit 0 stereo (`AUD_FLAG_STEREO`), bit 1 16-bit (`AUD_FLAG_16BIT`) |
| 11 | 1 | compression: 0 none, 1 Westwood ADPCM (`SCOMP_WESTWOOD`), 99 IMA ADPCM (`SCOMP_SOS`) |

Uncompressed data follows directly. Compressed data is a sequence of frames, each with an 8-byte header:
compressed size (2), uncompressed size (2), magic `0x0000DEAF` (4), then the compressed bytes. If both sizes are equal
the frame is stored raw. Red Alert's IMA files use 512-byte frames (2048 bytes of 16-bit PCM).
Rates between 20000 and 24000 are played at exactly 22050 Hz (an original fudge, kept).

## 3. Decoders

All three were TASM files; S09 replaced them with C++ in place (`.CPP`, `.ASM` deleted), as package S07-E planned.

### IMA ADPCM (`SOSCODEC.CPP`, `OLSOSDEC.CPP`)

Standard IMA ADPCM: per 4-bit code, `diff = step>>3 (+step if bit 2) (+step>>1 if bit 1) (+step>>2 if bit 0)`, negated
if bit 3; the predictor is clamped to 16 bits; the step index moves by `{-1,-1,-1,-1,2,4,6,8}` and is clamped to 0..88.
Low nibble first. The predictor and step index carry over between frames.

- `sosCODECDecompressData` (fast path, 16-bit mono only) adopts the existing `CODE/ADPCM.CPP` (table-driven:
  combined index `step*16 + code`, `DiffTable`/`IndexTable`) per ADR-0010, with three fixes: the size argument
  counts **output** bytes (what `Sample_Copy` passes and the asm expected), it returns that size (ADPCM.CPP returned 0),
  and the state lives in `wIndex`/`dwPredicted`. The tables are computed at start-up; a test checks them against
  `CODE/DTABLE.CPP`/`ITABLE.CPP`. `CODE/ADPCM.CPP` is still not built.
- `General_sosCODECDecompressData` handles mono/stereo and 8/16-bit output. Stereo input alternates bytes (even = left,
  odd = right, two samples per byte); 8-bit output is the high byte xor 0x80. The nibble position resets every call.
- `sosCODECInitStream` existed in both asm files (which copy the linker kept is unknown); the port sets the union of
  both: index 0, step 7, predictor 0, sample index 0, for both channels.
- Safety fixes: a zero (or odd stereo) size no longer runs away.

### Westwood ADPCM (`AUDUNCMP.CPP`, `Decompress_Frame`)

Unsigned 8-bit output, previous sample starts at 0x80 each frame. Code byte = 2-bit command (bits 6-7) + 6-bit argument:

| Command | Meaning |
|---------|---------|
| 0 | argument+1 bytes of four 2-bit deltas `{-2,-1,0,1}` (bits 0-1 first), clamped to 0..255 |
| 1 | argument+1 bytes of two 4-bit deltas `{-9,-8,-6,-5,-4,-3,-2,-1,0,1,2,3,4,5,6,8}` (low nibble first), clamped |
| 2 | bit 5 set: signed 5-bit delta, **not clamped** (8-bit wrap, as the asm did); else argument+1 raw bytes |
| 3 | repeat the previous sample argument+1 times |

Returns the number of *source* bytes read (the caller ignores it). The asm could write past the output buffer on a
bad frame; the port never does but reads the source identically.

## 4. New design

```
 game thread                                     audio thread (SDL) or game thread (null)
 ───────────                                     ───────────────────────────────────────
 Play_Sample, File_Stream_*, Stop_Sample ...     AudioBackendClass  ── pulls ──┐
   SOUNDIO.CPP: trackers, priority, streaming                                  │
   (file reads happen here, unlocked)            AudioMixerClass::Mix  <───────┘
                    │                               │ per voice, when its 8 KB stage is empty:
                    └── Audio lock (recursive) ─────┤   Audio_Voice_Fill → Sample_Copy → decoders
                                                    │ every 1/40 s of output: maintenance_callback (fades,
                                                    │   attach next stream block)
```

- **Mixer** (`audio_mixer.h/.cpp`): 8 voices (5 used, voice N = tracker N), each with its own rate/channels/bit depth,
  linear-interpolation resampling to the output rate, 16.16 per-voice and master gain, clipping, mono/stereo output,
  `reverse_channels`. Voices pull data through a fill callback in 8 KB requests (the old quarter-buffer size), so
  `Sample_Copy` and the frame logic are unchanged. A tick callback runs `maintenance_callback` at 40 Hz of *output*
  time. Integer-only. One recursive mutex (heap-allocated: with 1-byte struct packing a member mutex could be misaligned)
  serializes `Mix` with the game-side API; it replaces `GlobalAudioCriticalSection`.
- **Backend interface** (`audio_backend.h`): `Open(format, mix callback)`, `Close`, `Pause`, `Set_Gain`, `Update`.
  Select one with `Audio_Set_Backend()` before `Audio_Init()`; default is SDL3 when built with `RA_HAVE_SDL3`, else null.
  If `Open` fails, `Audio_Init` returns FALSE and the game runs silent (`SampleType == SAMPLE_NONE`), as before.
- **Null backend** (`audio_null.cpp`): opens without a device. `Update()` (called from `Sound_Callback` and
  `Sample_Status`) mixes the wall-clock time since the last call (capped at 250 ms) and discards it, so sounds still
  start and end on time and the game's `while (Is_Speaking()) {}` busy-waits (`CODE/SCENARIO.CPP:822`) terminate.
  `Pump()` lets tests mix by hand.
- **SDL3 backend** (`audio_sdl3.cpp`, compiled only with `-DRA_HAVE_SDL3=ON`, OFF by default): an `SDL_AudioStream`
  on the default playback device in the mixer's format (S16, 1 or 2 channels, the rate given to `Audio_Init`); SDL
  converts to the device. Its callback mixes on SDL's audio thread. Uses only SDL's audio subsystem, so it builds
  headless. Syntax-checked against SDL 3.2.0 headers; not yet built or run against a real SDL (S08 landed in parallel).
- **Portability**: the library no longer includes `windows.h`, `dsound.h` or `wwstd.h`. `audio_compat.h` supplies
  `BOOL`/`TRUE`/`FALSE`/`far`/`__cdecl`, declares `HWND` like `windows.h` does off Windows (`struct HWND__ *`), and the
  file-function prototypes (`Open_File` etc., defined by the game in `CODE/CCFILE.CPP`). AUD headers are read byte by
  byte, so `long` size and packing don't matter (the tests also pass as a 64-bit build). Memory is `malloc`/`free`.

### Behaviour changes, function by function

| Function | Change |
|----------|--------|
| `Audio_Init` | Opens the backend; `window` and `bits_per_sample` are unused. A second call is a no-op (the original disabled sound). Can be called again after `Sound_End`. |
| `Play_Sample_Handle` | Starts a mixer voice. The `Any_Locked()` check (DirectDraw surface locked → return 0) is gone. Returns -1 if the voice can't start (the original returned the handle with the tracker inactive). |
| `Sample_Status` | Asks the mixer; marks a finished tracker stopped. Advances the null backend. |
| `Sample_Copy` | Rewinds to the frame start (instead of desynchronising) when a frame doesn't fit or isn't fully loaded yet; rejects frames larger than the staging buffer. Identical output for well-formed, loaded data. |
| `Start_Primary_Sound_Buffer` | Resumes the backend. Instead of `GameInFocus`, a non-forced call fails after `Stop_Primary_Sound_Buffer` until a forced one, which is what the game's focus code does. |
| `Restore_Sound_Buffers`, `Suspend/Resume_Audio_Thread` | No-ops. `Set_Primary_Buffer_Format` returns whether a device is open. |
| `File_Stream_Sample_Vol` | Closes the file when no tracker is free (the original leaked it). |
| removed (internal) | `Sound_Timer_Callback`, `Sound_Thread`, `Attempt_Audio_Restore`, `Print_Sound_Error`, DirectSound globals `SoundObject`, `PrimaryBufferPtr`, `GlobalAudioCriticalSection` |

Kept on purpose: the volume curve, priority rules, `Set_Sound_Vol` not touching playing sounds, ignored `panloc`, the
22050 Hz fudge, and the `StartingFileStream` quirk (once a tracker is skipped to keep a channel for music, the flag
stays set until the next stream starts, so the reservation only works once).

## 5. Determinism

Audio never feeds the simulation. Checked in S09: the game reads audio state only through `Is_Speaking`,
`Is_Sample_Playing`, `Sample_Status`/`Theme.Still_Playing` and `Sample_Length`, used for UI waits (speech, intro,
ending, score screen) and for picking the next music track, which uses `Sim_Random_Pick` → `NonCriticalRandomNumber`,
the non-simulation RNG (`CODE/INLINE.H:1005`). All other `Sim_Random_Pick` uses are sound/visual choices.
`EVENT.CPP:844` (EXIT event) waits for speech before ending the game, which is local presentation only.

## 6. What's left

- **Switch on the SDL3 backend** after S08: configure with `-DRA_HAVE_SDL3=ON` (or default it ON); the CMake block in
  `CMakeLists.txt` adds `RA_HAVE_SDL3=1` to `win32lib` and links `SDL3::SDL3` (rename if S08's target differs).
  Then play a sound by hand on Windows and Linux (32-bit Linux SDL audio is ALSA only).
- **Game side (S06/S10)**: `CODE/EXTERNS.H:127-128` declares and `CODE/INIT.CPP:1202-1203` uses `SoundObject`/`PrimaryBufferPtr`
  to hand DirectSound to the movie player; those globals no longer exist. Remove them with the movie stub (ADR-0005).
  `HWND`: if S08's `windows.h` replacement defines it, it must be `struct HWND__ *` (or change `audio_compat.h` to match),
  or `Audio_Init` won't link. Code that wants to choose a backend includes `audio_backend.h` from `WIN32LIB/AUDIO`.
- `WINVQ` movie audio still uses DirectSound and its own `SOSCODEC.ASM` copy; out of scope (movies are stubbed).
- `WIN32LIB/INCLUDE/DIFFTB.INC`, `INDEXTB.INC`, `NYBBTB.INC` are now unused copies of the asm tables.
- Not verified against real game `.AUD` files (no assets in CI; ADR-0006). Worth a manual listen once the game runs.
- `TEST.CPP`/`TST.CPP` in `WIN32LIB/AUDIO` are old DOS test programs, not built.

## Tests

`tests/s09_decoder_test.cpp` (hand-derived ADPCM vectors, table check against `CODE/DTABLE.CPP`, fast/general/reference
agreement), `tests/s09_mixer_test.cpp` (synthetic PCM: passthrough, formats, gains, clipping, resampling, tick rate),
`tests/s09_sound_api_test.cpp` (the `audio.h` API on the null backend with in-memory files: playback bit-exact,
IMA/Westwood/raw AUDs, priority, volume curve, fades, focus, `Load_Sample`, music streaming across blocks, a second
mixing thread, real-time busy-wait). Run: `cmake --build build --target ra_tests && ctest --test-dir build`.
