# ADR-0013: LAN scope: port 19960, no Windows ↔ Linux cross-play for M3, rename "IPX" only when needed

- **Status:** Accepted (decided by the project owner)
- **Date:** 2026-10-14
- **Session:** Wave 1 review (questions raised by S04)
- **Amends:** [ADR-0009](0009-own-network-protocol.md) (the cross-platform wire-layout requirement)

## Context
S04's LAN design left three questions for the owner: the default port, whether Windows and Linux builds must be able
to play together, and whether to rename the many "IPX" identifiers that the UDP path keeps using.

## Decision
1. **Default port: 19960/udp**, with fallback through 19960–19963 for several instances on one machine, as in the LAN design.
2. **Windows ↔ Linux cross-play is not required** for M3 (LAN with friends). Players on the same platform must be able
   to play together. Wire structs still get `static_assert`s, so their layout can't drift by accident within a platform.
   Making layouts match across compilers is a later, optional task.
3. **Rename "IPX" identifiers only when needed.** The names stay (`GAME_IPX`, `IPXManagerClass`, …) unless a
   session is already changing that code and the old name causes real confusion. User-visible text says "LAN".

## Consequences
- S06 no longer has to make MSVC and GCC produce identical wire layouts. It only has to keep each compiler's
  layout stable and asserted. This removes the riskiest part of S06's layout work (enum underlying types and
  `-mms-bitfields`), which can wait until cross-play is wanted.
- The Linux build stays in CI as a portability check, but M3 playtests run on Windows.
- No mass-rename PRs; renames happen opportunistically and get noted in the session log.
