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
| [0007](0007-lan-udp-transport.md) | LAN: keep the connection layer, rewrite the UDP transport | Accepted |
| [0008](0008-case-insensitive-include-shim.md) | Case-insensitive include shim for Linux builds | Accepted |
| [0009](0009-own-network-protocol.md) | Our own network protocol, no compatibility with original clients | Accepted (amended by 0013) |
| [0010](0010-asm-porting-policy.md) | Port only the assembly the game needs; prefer existing C++ | Accepted |
| [0011](0011-expansions-in-multiplayer.md) | Counterstrike and Aftermath content in multiplayer | Accepted |
| [0012](0012-hand-maintained-source-lists.md) | Hand-maintained CMake source lists | Accepted |
| [0013](0013-lan-scope-port-crossplay-naming.md) | LAN: port 19960, no cross-play for M3, rename "IPX" only when needed | Accepted |
| [0014](0014-compiler-compat-layer-and-packing.md) | Compiler compatibility layer, Win32 stand-in headers for Linux, and struct packing | Accepted |
| [0015](0015-no-mingw-ci-job.md) | No MinGW-w64 cross-compile job in CI (use it locally as a Windows-header check) | Accepted |
| [0016](0016-drop-video-memory-icon-cache.md) | Drop the DirectDraw video-memory icon cache | Accepted |
| [0019](0019-one-lcw-decoder-and-asm-oracle-tests.md) | One LCW decoder; check asm ports against the original instructions in tests | Accepted |
| [0020](0020-sdl3-via-fetchcontent.md) | Get SDL3 with CMake FetchContent from a pinned release tarball | Accepted |
| [0021](0021-explicit-template-instantiation.md) | Explicit template instantiation in the defining `.CPP`, defined stand-ins for Watcom "instantiation declarations" | Accepted |
| [0022](0022-portable-file-io-and-win32-replacements.md) | Portable file I/O on C runtime descriptors, one Win32 shim module, and no VQA player in the link | Proposed |
| [0023](0023-data-folder-and-first-run.md) | Data folder, first-run configuration and start-up checks | Proposed |
