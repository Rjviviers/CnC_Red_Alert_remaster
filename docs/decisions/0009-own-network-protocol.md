# ADR-0009: Our own network protocol, no compatibility with original clients

- **Status:** Accepted (decided by the project owner)
- **Date:** 2026-10-14
- **Session:** Wave 1 review (questions raised by S01 and S04)

## Context
The original game sends raw, byte-packed C structs over the network. Their layout depends on the Watcom
compiler (enum sizes, bitfield layout, 4-byte `bool`, `/zp1` packing). S01 and S04 asked whether our builds must
stay wire-compatible with original Red Alert clients (the 1990s executables or other ports).

## Options considered
1. **Stay compatible with original clients.** We'd have to reproduce the Watcom layout and protocol exactly, forever.
   That blocks protocol changes we will need for internet play, matchmaking and ranked.
2. **Our own protocol.** Only our builds play each other. We're free to change packet formats, versioning and transport.

## Decision
Option 2: we make our own protocol. Original clients are not supported.

## Consequences
- The original Watcom struct sizes become a convenient **starting point**, not a requirement. S06/S12 may keep them
  where that's easiest, or change them.
- ~~What stays required: every build we ship (MSVC on Windows, GCC/Clang on Linux) uses an **identical** wire layout,
  enforced with `static_assert`s, so Windows and Linux players can play together.~~ **Amended by
  [ADR-0013](0013-lan-scope-port-crossplay-naming.md):** cross-play isn't required for M3. Each platform's layout must
  only be stable and asserted.
- Every protocol change bumps our own `NET_PROTOCOL_VERSION` (ADR-0007 / LAN design), so mismatched builds refuse to
  connect instead of desyncing.
