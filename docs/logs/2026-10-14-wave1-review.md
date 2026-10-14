# Session log: Wave 1 review & merge (orchestrator)

- **Date:** 2026-10-14
- **Sessions covered:** S01, S02, S03, S04 (run in parallel by four agents, each in its own git worktree)
- **Branch:** `claude/cool-bohr-9vg7ac`
- **Status at end:** Wave 1 done; Wave 2 (S05) unblocked

## What was done
1. Started four agents, one per session, each in an isolated worktree, with instructions not to push and not to edit
   the shared indexes.
2. S01's and S04's worktrees were created from the original commit, before the wiki existed. Both agents were told
   to fast-forward to the wiki branch; S02 and S03 did the same on their own.
3. Reviewed and merged each session as it finished (S03 → S04 → S01 → S02). Each merge commit records the
   review changes. Those changes are also listed at the end of each session's log.
4. Folded every agent's "plan changes" into the briefs they affect ("Inputs from SNN" sections), because agents
   weren't allowed to edit other sessions' briefs.

## Corrections to earlier documentation
- **LAN never used UDP** (S04, confirmed by S01 and checked by hand): `INIT.CPP:1022` is compiled out under
  `WOLAPI_INTEGRATION`; LAN used native IPX (`INIT.CPP:1255`). The S00 log and the overview were corrected.
- **Greenleaf**: used by 3 files, not ~66 (substring false matches). The overview was corrected.
- `tools/` was renamed to `scripts/`, to avoid clashing with `TOOLS/` on case-insensitive file systems.

## Owner decisions during the review
| Decision | Recorded in |
|----------|-------------|
| Accept ADR-0003 (toolchain), ADR-0004 (SDL3), ADR-0005 (scope) | ADR files, decisions index |
| Missing sources (`PALETTE.CPP`, `PASSEDIT.CPP`, headers): no other copy exists, so S06 rewrites them | S06 brief |
| No compatibility with original clients; our own protocol | [ADR-0009](../decisions/0009-own-network-protocol.md) |
| Counterstrike/Aftermath in multiplayer "if possible" | [ADR-0011](../decisions/0011-expansions-in-multiplayer.md) |
| Accept ADR-0007 (LAN transport), ADR-0008 (include shim), ADR-0010 (asm porting policy) | ADR files, decisions index |

## Still open for the owner
- Default LAN port 19960/udp
- Windows ↔ Linux cross-play as a requirement for M3
- Rename the "IPX" identifiers later, or never

## Key numbers after Wave 1
- Win32 game build: 250 objects (242 `.CPP`, 6 `.ASM`, 2 missing sources).
- Compile progress: 10 of 316 files compile with GCC `-m32` (baseline for S06). CI runs on Windows and Linux.
- Assembly: 357 files in the repo, 66 in the Win32 link, **25 really need porting** (~8,150 lines).

## Determinism impact
None. Documentation only.

## Next steps
Start Wave 2: S05 (strip dead platforms), using the delete lists from S01/S02 and the keep list from S04.
