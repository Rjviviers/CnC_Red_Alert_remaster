# Codebase Overview

A first map of the original source tree, made in session S00. The deeper audits (S01, S02, S04)
add detailed pages next to this one.

## Top-level directories

After session S05 (strip dead platforms). Counts are files in the repository.

| Directory | Files (cpp / asm / h) | What it is | Used by the Win32 build? |
|-----------|----------------------|------------|-----------------------------|
| `CODE/` | 236 / 5 / 207 | The game itself: simulation, UI, networking (LAN over UDP), skirmish, campaign | Yes |
| `WIN32LIB/` | 54 / 28 / 82 | Westwood's Win32 support library: drawing, shapes, audio, keyboard, timer, files | Yes (`win32lib.lib`) |
| `WINVQ/` | 26 / 4 / 67 | Win32 VQA movie player | Yes (`vqa32wp.lib`), movies only (to be stubbed, ADR-0005) |
| `TOOLS/` | 2 / 0 / 0 | `MIX/MIXFILE.CPP` and `AUDIOMAK/AUDIOMAK.CPP`, kept as file-format references | No |

**Removed in S05** (still in git history; details and the last commit that has each one in
[removed-code.md](removed-code.md)):

| Directory | Files (cpp / asm / h) before | What it was |
|-----------|-----------------------------|-------------|
| `WWFLAT32/` | 76 / 118 / 94 | DOS version of the support library |
| `VQ/` | 26 / 19 / 98 | DOS VQA movie player |
| `IPX/` | 10 / 17 / 2 | Win95 IPX thunk DLL (`THIPX32.DLL`, `wwipx32.lib`) and real-mode IPX driver |
| `LAUNCH/` | 0 / 1 / 0 | DOS launcher stub |
| `LAUNCHER/` | 17 cpp, 1 c | Separate Windows launcher/patcher for Westwood Online updates |

Also removed from `CODE/`, `WIN32LIB/` and `WINVQ/`: native IPX, modem/null-modem play (`WIN32LIB/WINCOMM/`),
Westwood Online, the WChat/DDE internet path, TEN and MPath, debug copies (`WIN32LIB/SRCDEBUG/`), old/test folders,
prebuilt tools and HMI SOS libraries, unused assembly, and every DOS `#ifdef` branch.

## Original build

- Makefile: `CODE/MAKEFILE` (Watcom `wmake`). `WIN32` selects the Windows build, otherwise DOS/4GW.
- Compiler: Watcom `wpp386` with `/5r` (register calling convention), `/zp1` (**byte-packed structs**),
  `/j` (signed `char`), `/we` (warnings as errors). Assembler: TASM.
- Win32 defines: `WIN32`, `_WIN32`, `WOLAPI_INTEGRATION`, `WINSOCK_IPX`, plus a language define (`ENGLISH`, …).
- External SDKs the build expects: DirectX 5 SDK (`..\dxsdk`), Watcom headers, and Greenleaf GCL (`..\gcl510`, DOS/modem).
  HMI SOS was used for DOS audio.

## Platform dependencies (rough counts from `CODE/`)

| Dependency | Files mentioning it | Plan |
|------------|---------------------|------|
| DirectDraw | ~10 | Replace with the SDL platform layer (S08) |
| DirectSound | 1 (+ `WIN32LIB/AUDIO`) | Replace with SDL audio (S09) |
| Winsock | ~30 | Keep UDP, made portable (S12) |
| IPX | ~36 | Native IPX removed in S05; the "IPX"-named connection layer stays and runs over UDP |
| Greenleaf (`gcl`) | 3 (`FUNCTION.H` includes the headers; `NULLMGR.CPP` and `NULLDLG.CPP` use them). Headers only; the library isn't linked. *Corrected by S01; the earlier "~66" was substring false matches.* | Removed with modem play in S05 |
| Watcom `#pragma aux` | `COORD.CPP`, `MONOC.CPP`, `JSHELL.H`, `MPU.H`, `RGB.H` (`DPMI.CPP` and the DOS `Vsync` in `EGOS.CPP` were removed in S05) | Rewrite (S06/S07) |

## Multiplayer architecture (first look)

- **Lockstep simulation:** `CODE/QUEUE.CPP` collects each player's events, runs them on the same frame
  on every machine, and checks sync. Only commands go over the network, not game state.
- **Game types** (`CODE/SESSION.H`, after S05): `GAME_NORMAL`, `GAME_IPX` (LAN), `GAME_INTERNET` (no longer
  selectable; its old WOL branches remain, see removed-code.md), `GAME_SKIRMISH`. `GAME_MODEM`,
  `GAME_NULL_MODEM`, `GAME_TEN` and `GAME_MPATH` were removed in S05.
- **The LAN game now creates the UDP transport** (S05). Before S05 the LAN game (`GAME_IPX`) created the
  native-IPX `IPXInterfaceClass` (`INIT.CPP:1255`) and only Westwood Online used `UDPInterfaceClass`
  (`WSPUDP.CPP`). S05 deleted native IPX and switched `GAME_IPX` to `UDPInterfaceClass`; it does not work over
  UDP yet (S12/S13). It's driven by Windows messages (`WSAAsyncSelect`), which has to be replaced with polling
  to be portable (S12). See [networking.md](networking.md).
- **Lobby UI:** `CODE/NETDLG.CPP`; skirmish setup: `CODE/SKIRMDLG.CPP`. Internet/WOL (`TCPIP.CPP`, `WOL_*.CPP`,
  `WOLAPI/`) was removed in S05.

S04 takes this further and documents the whole stack.
