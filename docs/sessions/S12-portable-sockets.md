# S12 — Portable UDP sockets

**Status:** Not started · **Wave:** 5 · **Depends on:** S04, S10 · **Parallel with:** S11
**Owns:** `WSPROTO`, `WSPUDP`, `TCPIP` (as needed), the new `net/` socket wrapper

## Goal
Make the UDP transport work without Windows window messages, on Windows and Linux.

## Design
Implement [docs/design/lan-multiplayer.md § Transport design](../design/lan-multiplayer.md#transport-design-s12) (ADR-0007).
Today `UDPInterfaceClass` is only used by Westwood Online, and it can't do LAN as written: it has no real broadcast,
one fixed port for every peer, and it drops packets from local IPs.

## Inputs from S05
- `GAME_INTERNET` and the unit trackers (`UTRACKER`) were left in, because removing them touches simulation files
  and `HouseClass`. Decide here whether to remove them or reuse `GAME_INTERNET` for internet play later.
- `INIT.CPP` already creates `UDPInterfaceClass` for the LAN game (S05). The second instance on one machine is no
  longer blocked: the DDE single-instance check is gone.

## Tasks
- [ ] A small socket wrapper (Winsock / BSD sockets) for non-blocking UDP, broadcast, and address helpers.
- [ ] Replace `WSAAsyncSelect` notifications with a `Poll()` called from `IPXManagerClass::Service()`, plus harmlessly from the main loop. Polling from the main loop alone is not enough: the game blocks in nested wait loops that never return to it (S04 finding).
- [ ] Keep the packet interface `UDPInterfaceClass` presents to the connection layer unchanged.
- [ ] Port configuration: default 19960/udp, falling back through 19960–19963 so several instances can run on one machine; `REDALERT.INI [Network]` and `-PORT=` (see the design doc).
- [ ] Unit tests: two sockets on localhost exchange packets and broadcasts.

## Done when
The transport tests pass on Windows and Linux CI, and the game links with the new transport.
