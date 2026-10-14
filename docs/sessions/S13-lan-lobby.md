# S13 — LAN discovery, lobby, game start

**Status:** Not started · **Wave:** 6 · **Depends on:** S12 · **Parallel with:** S14
**Owns:** `NETDLG.CPP`, `SESSION.CPP`, and connection-layer files per the S04 design

## Goal
Players on the same network can see each other's games, join a lobby, choose settings, and start.

## Tasks
- [ ] Rename the LAN menu entry (it currently says "IPX") and make sure it uses the UDP transport.
- [ ] Game discovery via broadcast; the lobby shows players, colours, countries and map.
- [ ] Allow two instances on one machine (different ports) for testing.
- [ ] Handle the edge cases: a player leaves the lobby, the host leaves, version mismatch.
- [ ] Document the lobby flow in `docs/codebase/networking.md`.

## Done when
Two instances (on one machine or two machines) can find each other, start a game, and play.
