# S05 — Strip dead platforms

**Status:** Not started · **Wave:** 2 · **Depends on:** S01, S02, S03 · **Parallel with:** S04
**Owns:** deletions across the tree; `docs/codebase/removed-code.md`

## Goal
Shrink the codebase to what we will actually port, so later sessions don't waste time on dead code.

## Tasks
- [ ] Delete DOS-only trees confirmed by S02 (expected: `WWFLAT32/`, `VQ/`, `IPX/`, `LAUNCH/`).
- [ ] Remove modem/serial play and Greenleaf usage (`GAME_MODEM`, `GAME_NULL_MODEM`, `NULLCONN`, `SEQCONN` if only modem uses it).
- [ ] Remove native IPX (`IPX*.CPP`, `IPXPROT.ASM`, `IPXREAL.ASM`, `WSPIPX`). **Keep** the UDP path, even though the game still calls it the "IPX game".
- [ ] Remove Westwood Online (`WOL_*.CPP`, `WOLAPI*`, `RAWOLAPI`) and TEN/MPath code.
- [ ] Collapse `#ifdef WIN32` / DOS branches so only the Win32 branch remains.
- [ ] Record each removal and why in `docs/codebase/removed-code.md`, plus ADR-0005 status.

## Done when
No DOS, IPX, modem or WOL code remains. The removal record lets anyone find it again in git history.
