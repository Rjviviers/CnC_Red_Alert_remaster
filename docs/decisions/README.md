# Architecture Decision Records

One short document per decision. Use [`../templates/adr.md`](../templates/adr.md).
Numbers are never reused. To change an accepted decision, write a new ADR that supersedes it.

| ADR | Title | Status |
|-----|-------|--------|
| [0001](0001-port-this-codebase.md) | Port this codebase directly | Accepted |
| [0002](0002-docs-as-code-wiki.md) | Docs-as-code wiki in `docs/`, every step documented | Accepted |
| [0003](0003-toolchain.md) | CMake, modern compilers, 32-bit x86 first | Accepted |
| [0004](0004-platform-library.md) | SDL3 as the platform layer; drop DirectX | Accepted |
| [0005](0005-multiplayer-first-scope.md) | Multiplayer-first scope: drop DOS, IPX, modem, WOL, movies | Accepted |
| [0006](0006-no-game-assets.md) | Never ship game assets | Accepted |
| [0007](0007-lan-udp-transport.md) | LAN: keep the connection layer, rewrite the UDP transport | Proposed |
| [0008](0008-case-insensitive-include-shim.md) | Case-insensitive include shim for Linux builds | Proposed |
| [0009](0009-own-network-protocol.md) | Our own network protocol, no compatibility with original clients | Accepted |
