# S05 — Strip dead platforms

**Status:** Done · **Wave:** 2 · **Depends on:** S01, S02, S03 · **Parallel with:** S04
**Owns:** deletions across the tree; `docs/codebase/removed-code.md`

## Goal
Shrink the codebase to what we will actually port, so later sessions don't waste time on dead code.

## Inputs from S04
- Before deleting `INTERNET.CPP` and `RAWOLAPI.H`, move `PlanetWestwoodPortNumber` and `GAME_VERSION` out of them. The LAN code still uses both.
- `SENDFILE.CPP` calls into the TCP/WOL layer at one point. Stub that call; don't delete the file.
- The skirmish dialog lives in `NULLDLG.CPP` (the modem dialog file). Extract it into its own file before removing the modem code.

## Inputs from S02
- Use the delete lists in [assembly-inventory.md § Delete list](../codebase/assembly-inventory.md#2-delete-list) and
  [libraries.md](../codebase/libraries.md).
- `IPX/` is **not** DOS-only: the Win32 build links `wwipx32.lib` and loads `THIPX32.DLL` at runtime. It still goes,
  because native IPX is removed, but remove the link and the DLL loading code too.
- Unused HMI SOS binary libraries are committed in the repo. Delete them.

## Inputs from S01
- When you delete `WSPIPX`, change `INIT.CPP:1255` in the same commit to create `UDPInterfaceClass` instead of
  `IPXInterfaceClass`, so the LAN path never points at deleted code. S12/S13 make it actually work over UDP.
- Greenleaf is headers only: remove its `#include`s from `FUNCTION.H`; `NULLMGR.CPP` is the only real user.
- Full per-file classification: [platform-dependencies.md](../codebase/platform-dependencies.md).

## Tasks
- [x] Delete DOS-only trees confirmed by S02 (expected: `WWFLAT32/`, `VQ/`, `IPX/`, `LAUNCH/`).
- [x] Remove modem/serial play and Greenleaf usage (`GAME_MODEM`, `GAME_NULL_MODEM`, `NULLCONN`, `SEQCONN` if only modem uses it).
- [x] Remove native IPX (`IPX.CPP`, `IPX95.CPP`, `IPXPROT.ASM`, `IPXREAL.ASM`, `WSPIPX`). **Keep** the UDP transport (`WSPROTO`, `WSPUDP`) and the whole connection layer, even though it is named "IPX" (`IPXMGR`, `IPXCONN`, `IPXGCONN`, `IPXADDR`). The LAN game uses it. Follow the keep/remove list in [networking.md](../codebase/networking.md#what-s05-can-remove-and-what-must-stay).
- [x] Remove Westwood Online (`WOL_*.CPP`, `WOLAPI*`, `RAWOLAPI`) and TEN/MPath code.
- [x] Collapse `#ifdef WIN32` / DOS branches so only the Win32 branch remains.
- [x] Record each removal and why in `docs/codebase/removed-code.md`, plus ADR-0005 status.

## Done when
No DOS, IPX, modem or WOL code remains. The removal record lets anyone find it again in git history.
