# ADR-0007: LAN play over UDP by keeping the original connection layer and replacing only the transport

- **Status:** Proposed
- **Date:** 2026-10-14
- **Session:** S04

## Context
M3 needs 2–4 players on a LAN, on Windows and Linux, plus several instances on one PC for testing.
The S04 audit ([networking.md](../codebase/networking.md)) found:

- The LAN menu entry (`GAME_IPX`) uses native IPX through Winsock (`INIT.CPP:1255`), which modern
  systems don't have. A UDP transport (`UDPInterfaceClass`) exists, but only Westwood Online used it.
- The connection layer between the game and the transport is transport-agnostic despite its "IPX" names:
  - `IPXManagerClass` and `IPXConnClass` (ACK/retry, ordering, demultiplexing);
  - the Global Channel lobby protocol;
  - `QUEUE.CPP` lockstep.

  All of it talks to an abstract `PacketTransport`.
- The UDP transport is driven by Windows messages (`WSAAsyncSelect` on `MainWindow`). It has a fixed
  port shared by every peer, no real broadcast, and drops all packets from local IPs. That rules out LAN
  discovery and two instances on one PC.
- The wire format is raw byte-packed structs whose layout depends on Watcom's enum and bitfield rules.
  Modern compilers change it, differently on MSVC and GCC.

## Options considered
1. **Keep everything above the transport; rewrite the transport internals** (non-blocking UDP, polling,
   port in the address, proper broadcast).
   - Pros: smallest change; the lockstep and lobby protocol are proven; no simulation risk; S12 and S13
     stay small.
   - Cons: we keep 1990s code and naming (`IPX*`, `GAME_IPX`); raw-struct wire format needs layout
     discipline.
2. **Replace the connection layer with a modern library** (ENet, GameNetworkingSockets, yojimbo) under
   the lockstep.
   - Pros: NAT punching and relays come built in for later.
   - Cons: a new dependency; it re-implements what `ConnectionClass` already does; the lobby and
     `QUEUE.CPP` would need adapting to its API; more risk before M3. It can still be adopted later
     *under* `UDPInterfaceClass` if M4/M5 need it.
3. **Rewrite networking and the lobby from scratch.**
   - Pros: a clean design.
   - Cons: the most work, the most desync risk, and it throws away a working protocol. Not justified for M3.

## Decision
Option 1.
- `GAME_IPX` uses `UDPInterfaceClass`.
- Its public interface stays the same, but internally it uses a new portable `net::` socket wrapper
  (Winsock 2 / BSD). It is non-blocking and **polled from `IPXManagerClass::Service()`**; no window
  messages.
- Addresses keep the 10-byte `IPXAddressClass` layout: IPv4 in node bytes 0–3 and the **UDP port in
  node bytes 4–5**.
- The default base port is **19960/udp**. An instance binds the first free port in 19960–19963. It is
  configurable via `REDALERT.INI [Network] Port=` and the `-PORT=` command line.
- Discovery keeps the original query protocol, broadcast to `255.255.255.255`, the directed broadcasts,
  `127.0.0.1` and configured targets on every port in the span. Packets from local IPs are normalised to
  `127.0.0.1`; only our own port is filtered out.
- Compatibility is an exact match of a new `NET_PROTOCOL_VERSION` through the existing reject path.
- Every wire struct's size and key offsets are pinned with `static_assert`s to the original Watcom values,
  on every compiler.

Details: [`docs/design/lan-multiplayer.md`](../design/lan-multiplayer.md).

## Consequences
- S12 is a contained transport rewrite with unit tests. S13 is mostly lobby UI and edge cases.
- The lockstep code is untouched, so desync risk comes only from layout and compiler changes, which S14
  and the static_asserts guard.
- The wire format changes (port bytes in addresses), and it is not compatible with the original game.
  We never had that compatibility over UDP anyway.
- Enums in wire structs need fixed underlying types and bitfields need MSVC-style layout (S06/S12).
- The `IPX*` names stay for now. A rename is a separate, later cleanup.
- Internet play (M4+) can reuse join-by-IP and the port-carrying addresses. NAT traversal and relays can
  be added under the transport without touching the connection layer. IPv6 needs a wider address and a
  new ADR.
