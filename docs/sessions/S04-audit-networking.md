# S04 — Audit: networking → LAN design

**Status:** Done · **Wave:** 1 · **Depends on:** S00 · **Parallel with:** S01, S02, S03
**Type:** investigation + design spec; no code changes
**Owns:** `docs/codebase/networking.md`, `docs/design/lan-multiplayer.md`

## Goal
Understand exactly how the original multiplayer works, and write the design for LAN play.

## Known starting points
- Lockstep event queue: `CODE/QUEUE.CPP` (`Queue_AI_Multiplayer`, DoList, frame sync)
- Game types: `CODE/SESSION.H` (`GAME_IPX` is the LAN path, `GAME_INTERNET`, `GAME_SKIRMISH`, …)
- Transport selection: `CODE/INIT.CPP` around line 1022 creates `UDPInterfaceClass` for LAN
- UDP/IPX transports: `WSPUDP.CPP`, `WSPIPX.CPP`, `WSPROTO`, `TCPIP.CPP`
- Connection layer: `COMBUF`, `COMQUEUE`, `CONNECT`, `IPXCONN`, `IPXGCONN`, `IPXMGR`, `SEQCONN`
- Lobby UI: `CODE/NETDLG.CPP`

## Tasks
- [x] Document the layers top to bottom: lobby UI → session → connection manager → packet transport → sockets.
- [x] Document every packet and message type, with its struct layout and size (watch out for `/zp1` packing).
- [x] Document how LAN discovery works today (broadcast, ports, game advertisement).
- [x] Document frame sync: `MaxAhead`, frame rate negotiation, how desyncs are detected (CRC), and what happens on disconnect.
- [x] List everything Windows-specific in the path (`WSAAsyncSelect`, window messages, `MainWindow`).
- [x] Write `docs/design/lan-multiplayer.md`: the target LAN design, what to keep, what to replace, default port, and a test plan. Add an ADR for the network approach.

## Done when
S12 and S13 can be implemented from the design doc without re-reading the whole network stack.

## Outcome
- [Networking audit](../codebase/networking.md), [LAN design](../design/lan-multiplayer.md),
  [ADR-0007](../decisions/0007-lan-udp-transport.md), [session log](../logs/2026-10-14-S04-audit-networking.md).
- Correction to the starting points above: `INIT.CPP:1022` is compiled out. LAN (`GAME_IPX`) creates
  the native-IPX `IPXInterfaceClass` (`INIT.CPP:1255`); only Westwood Online uses `UDPInterfaceClass`.
