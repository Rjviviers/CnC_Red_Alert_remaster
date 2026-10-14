# Codebase Overview

A first map of the original source tree, made in session S00. The deeper audits (S01, S02, S04)
add detailed pages next to this one.

## Top-level directories

| Directory | Files (cpp / asm / h) | What it is | Used by the Win32 build? |
|-----------|----------------------|------------|-----------------------------|
| `CODE/` | 277 / 12 / 242 | The game itself: simulation, UI, networking, campaign | Yes |
| `WIN32LIB/` | 157 / 166 / 111 | Westwood's Win32 support library: drawing, shapes, audio, keyboard, timer, files | Yes (`win32lib.lib`) |
| `WINVQ/` | 45 / 23 / 88 | Win32 VQA movie player | Yes (`vqa32wp.lib`), movies only |
| `WWFLAT32/` | 76 / 118 / 94 | DOS version of the support library | No: DOS build only |
| `VQ/` | 26 / 19 / 98 | DOS VQA movie player | No: DOS build only |
| `IPX/` | 10 / 17 / 2 | Real-mode IPX networking driver for DOS | No (to be confirmed in S02) |
| `LAUNCH/` | 0 / 1 / 0 | DOS launcher stub | No |
| `LAUNCHER/` | 17 cpp, 1 c | Separate Windows launcher/patcher program | Separate program (S02 decides) |
| `TOOLS/` | 2 / 1 / 0 | Small build tools | Possibly (S02) |

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
| IPX | ~36 | Remove native IPX (S05) |
| Greenleaf (`gcl`) | ~66 | Modem/serial: remove (S05) |
| Watcom `#pragma aux` | `COORD.CPP`, `DPMI.CPP`, `EGOS.CPP`, `MONOC.CPP`, `JSHELL.H`, `MPU.H`, `RGB.H` | Rewrite (S06/S07) |

## Multiplayer architecture (first look)

- **Lockstep simulation:** `CODE/QUEUE.CPP` collects each player's events, runs them on the same frame
  on every machine, and checks sync. Only commands go over the network, not game state.
- **Game types** (`CODE/SESSION.H`): `GAME_NORMAL`, `GAME_MODEM`, `GAME_NULL_MODEM`, `GAME_IPX` (LAN),
  `GAME_INTERNET`, `GAME_SKIRMISH`, `GAME_TEN`, …
- **A UDP transport exists, but LAN doesn't use it yet** (corrected in S04). The LAN game (`GAME_IPX`)
  creates the native-IPX `IPXInterfaceClass` (`INIT.CPP:1255`). `UDPInterfaceClass` (`WSPUDP.CPP`) is only
  used by Westwood Online (`INIT.CPP:1202`; line 1022 is compiled out). It's driven by Windows messages
  (`WSAAsyncSelect`), which has to be replaced with polling to be portable (S12).
  See [networking.md](networking.md).
- **Lobby UI:** `CODE/NETDLG.CPP`. **Internet/WOL:** `TCPIP.CPP`, `WOL_*.CPP`, `WOLAPI/`.

S04 takes this further and documents the whole stack.
