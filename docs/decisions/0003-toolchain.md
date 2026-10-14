# ADR-0003: CMake, modern compilers, 32-bit x86 first

- **Status:** Accepted (approved by project owner 2026-10-14)
- **Date:** 2026-10-14
- **Session:** S00

## Context
The original build uses Watcom C++ 10.6, TASM 4.0 and Watcom `wmake` makefiles, none of which are usable today.
The code assumes 32-bit: `long` is 32 bits, pointers are cast to integers, and structs are byte-packed (`/zp1`).

## Options considered
1. **Go straight to 64-bit.** Future-proof, but hundreds of subtle size bugs, some of which would cause desyncs.
2. **32-bit x86 first, 64-bit later.** Much smaller initial change. Still runs on every modern Windows and on Linux with multilib.

## Decision
- Build system: CMake.
- Compilers: MSVC (Windows) and GCC/Clang (Linux). Both are built in CI.
- Target: 32-bit x86 first. 64-bit becomes a separate future session, once the sync harness (S14) can verify it.
- Language: compile as C++ with the most permissive modern standard setting that works. Refactor only when needed.

## Consequences
Linux builds need 32-bit SDL and multilib. macOS (no 32-bit support) has to wait for the 64-bit work.
