# ADR-0022: Portable file I/O on C runtime descriptors, one Win32 shim module, and no VQA player in the link

- **Status:** Proposed
- **Date:** 2026-10-15
- **Session:** S10b

## Context
After S10a, linking `redalert` on Linux left 80 unresolved symbols, all outside the templates: the Win32 API (file
I/O in `RAWFILE.CPP`, the window and message loop in `STARTUP.CPP`/`WINSTUB.CPP`, the registry, `FindFirstFile`, …),
Winsock, `main`, and the leaves of the VQA movie player (`UnVQ_4x2`, the SOS audio codec, `AudioUnzap`,
`DirectSoundCreate`, `timeSetEvent`, `MCGA_*`, `WaitVB`, `Mono_*`). Three questions needed an answer:

1. How does `RawFileClass`, the bottom of every file the game reads (`CCFileClass`, `CDFileClass`, MIX files), talk to
   the OS on every host? It must behave exactly like the Win32 handle code, and on Linux it must find the player's
   data files whatever their case (`REDALERT.MIX`, `redalert.mix`, `Redalert.mix`).
2. What happens to the other ~30 Win32 calls scattered over game files that the brief asks us to edit as little as
   possible (`CONQUER.CPP`, `INIT.CPP`, `SESSION.CPP`, `2KEYFRAM.CPP`, `WSPROTO.CPP`, `GETCD.CPP`, …)?
3. Movies are stubbed (ADR-0005). Where should the stub cut?

## Options considered
**File I/O**
1. **Implement the Win32 file calls (`CreateFile`, `ReadFile`, `SetFilePointer`, …) on POSIX** and leave `RAWFILE.CPP`
   alone. No game edit, but two code paths (real Win32 on Windows, emulation elsewhere) and a fake `HANDLE` type.
2. **stdio `FILE *`** in `RawFileClass`. One code path, but stdio buffers: the size of a file being written is not
   visible until a flush, and C forbids reading right after writing on the same stream without a seek. Both are
   things the Win32 handle allowed.
3. **C runtime file descriptors** (`open`/`read`/`write`/`lseek`; `_open`/`_read`/… on Windows) behind a tiny module
   `CODE/compat/ra_file.{h,cpp}`. Unbuffered and positioned like a Win32 handle, one code path on every host, and the
   handle is an `int` with `-1` for "closed", which is what the DOS version of `RawFileClass` used.

**Other Win32 calls**
1. Rewrite each caller. Many small edits in files other sessions own, for calls that are mostly dead (CD volume
   labels, the installer's registry flags).
2. **One clearly named module per kind:** real portable implementations in `CODE/compat/win32_api.cpp`
   (`FindFirstFile`, `GetSystemTime`, `GlobalMemoryStatus`, `GetLastError`, …) and stubs in
   `CODE/stubs/win32_stubs.cpp` (registry, `GetVolumeInformation`, `SendMessage`, `ShowCursor`), both compiled only
   off Windows. Windows keeps the real API, which is the simplest thing there.

**Movies**
1. Stub the player's leaves (`UnVQ_4x2`, `VQA_sosCODEC*`, `AudioUnzap`, `DirectSoundCreate`, `timeSetEvent`,
   `MCGA_*`, `WaitVB`, `Mono_*`). The player would still run, decode nothing and show garbage or black frames, and we
   would have stubbed eleven functions to get there.
2. **Do not link `vqa32wp`; stub the VQA API the game calls** (`VQA_Alloc` returns NULL, so every caller skips the
   movie). Nine trivial functions, no dead player code in the executable. The library still builds as its own
   target. Turn off `MPEGMOVIE` (the DirectDraw DVD player, `mpgdll`, not in the repository).

## Decision
- `RawFileClass` uses `CODE/compat/ra_file.h` on every host: C runtime descriptors, Win32 `CreateFile` semantics per
  access mode (READ = open existing, WRITE and READ|WRITE = create or truncate), and on non-Windows hosts
  case-insensitive path resolution (`\` → `/`, drive letter dropped, each missing component matched
  case-insensitively, first match in byte order).
- The remaining Win32 calls get portable implementations or documented stubs in one module each, non-Windows only.
- `redalert` no longer links `vqa32wp`; `CODE/stubs/movie_stubs.cpp` makes movie playback a no-op. `MPEGMOVIE` is off.
- Winsock is stubbed off Windows until S12 (WSAStartup fails, so the game reports no network).

## Consequences
- One file I/O path to test (`tests/s10b_file_test.cpp`); behaviour on Windows changes from Win32 handles to CRT
  descriptors, which is visible only in error codes (the error handlers ignore them).
- Lookup of a name that does not exist with the exact case costs a directory scan per component. The game opens its
  files rarely (MIX files are opened once and cached), so this is not measurable.
- Two files differing only in case: the one that sorts first byte-wise wins, on every machine.
- Bringing movies back later means relinking `vqa32wp` and porting its decoder and audio path (a new ADR superseding
  the movie part of this one and ADR-0005).
- `CODE/compat/win32_api.cpp` and `CODE/stubs/win32_stubs.cpp` shrink as callers are rewritten; delete them, and the
  `CODE/compat/win32/` headers, when nothing calls the Win32 API off Windows.
