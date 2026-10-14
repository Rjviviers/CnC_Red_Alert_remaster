# S12 — Portable UDP sockets

**Status:** Not started · **Wave:** 5 · **Depends on:** S04, S10 · **Parallel with:** S11
**Owns:** `WSPROTO`, `WSPUDP`, `TCPIP` (as needed), the new `net/` socket wrapper

## Goal
Make the UDP transport work without Windows window messages, on Windows and Linux.

## Tasks
- [ ] A small socket wrapper (Winsock / BSD sockets) for non-blocking UDP, broadcast, and address helpers.
- [ ] Replace `WSAAsyncSelect` notifications with polling once per frame from the main loop (hook from S08).
- [ ] Keep the packet interface `UDPInterfaceClass` presents to the connection layer unchanged.
- [ ] Make the port configurable, defaulting to the port chosen in the S04 design.
- [ ] Unit tests: two sockets on localhost exchange packets and broadcasts.

## Done when
The transport tests pass on Windows and Linux CI, and the game links with the new transport.
