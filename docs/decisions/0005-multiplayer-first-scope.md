# ADR-0005: Multiplayer-first scope: drop DOS, IPX, modem, WOL, movies

- **Status:** Proposed (awaiting project owner)
- **Date:** 2026-10-14
- **Session:** S00

## Context
The project focuses on multiplayer. The codebase contains a lot of code for platforms and services that no longer exist.

## Decision
Remove:
- the DOS build and its libraries (`WWFLAT32/`, `VQ/`, `IPX/`, `LAUNCH/`, Greenleaf);
- native IPX, modem and null-modem play;
- Westwood Online (`WOL_*`) and the TEN/MPath services.

Stub out the cutscene movies (VQA playback); they only matter for the campaign.

Keep the single-player campaign code but don't maintain it. Keep skirmish vs AI as the test bed.

## Consequences
The codebase is much smaller and quicker to port. The online service will be our own design (M5+), not a revival of WOL.
Removed code stays available in git history; `docs/codebase/removed-code.md` lists it.
