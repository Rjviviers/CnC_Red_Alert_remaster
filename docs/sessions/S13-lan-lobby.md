# S13 — LAN discovery, lobby, game start

**Status:** Not started · **Wave:** 6 · **Depends on:** S12 · **Parallel with:** S14
**Owns:** `NETDLG.CPP`, `SESSION.CPP`, and connection-layer files per the S04 design

## Goal
Players on the same network can see each other's games, join a lobby, choose settings, and start.

## Design
Implement [docs/design/lan-multiplayer.md](../design/lan-multiplayer.md) (work split: the S13 row). Tests 4–9 come from its test plan.

## Tasks
- [ ] Switch the LAN game (`GAME_IPX`) from `IPXInterfaceClass` to `UDPInterfaceClass` (`INIT.CPP:1255`), and rename the menu entry from "IPX" to "LAN".
- [ ] Game discovery via broadcast; the lobby shows players, colours, countries and map.
- [ ] Allow two instances on one machine (different ports) for testing.
- [ ] Handle the edge cases: a player leaves the lobby, the host leaves, version mismatch (our own `NET_PROTOCOL_VERSION`).
- [ ] Two instances that share one INI also share a player name: add `-NAME=` and handle duplicate names.
- [ ] Keep simulating when the window loses focus (otherwise the other player stalls).
- [ ] Join by IP: a text field in the join dialog (see the design doc).
- [ ] Expansion content (ADR-0011): show the Aftermath setting in the lobby, and only enable it when every player has the data. Test a mixed game.
- [ ] Document the lobby flow in `docs/codebase/networking.md`.

## Done when
Two instances (on one machine or two machines) can find each other, start a game, and play.
