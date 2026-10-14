# Session log: S00 — Project wiki & plan

- **Date:** 2026-10-14
- **Session brief:** [S00](../sessions/S00-wiki-and-plan.md)
- **Branch / PR:** `claude/cool-bohr-9vg7ac` / not opened yet
- **Status at end:** Done (waiting for the owner to review the proposed ADRs)

## Goal
Set up a project wiki with tasks and a visual plan for the first steps, broken into sessions
that several agents can work on in parallel.

## What was done
1. Discussed direction with the project owner. They chose to port this repository rather than rebase onto
   Vanilla Conquer or switch to OpenRA (ADR-0001).
2. Surveyed the source tree: file counts per directory, the `CODE/MAKEFILE` Win32 vs DOS configuration,
   and the usage of DirectX, Winsock, IPX, Greenleaf and Watcom pragmas.
3. Found that LAN over UDP already exists (`UDPInterfaceClass`, created in `CODE/INIT.CPP`) and that
   multiplayer is lockstep (`CODE/QUEUE.CPP`).
4. Wrote the wiki: home, vision, roadmap with Mermaid diagrams, codebase overview, session board,
   briefs S00–S15, ADRs 0001–0006, templates.
5. Added `CLAUDE.md`, so every future session (human or agent) follows the documentation rules.

## Decisions
- Accepted: ADR-0001 (port this repo), ADR-0002 (docs in `docs/`), ADR-0006 (no assets).
- Proposed, waiting for the owner: ADR-0003 (toolchain), ADR-0004 (SDL3), ADR-0005 (scope cuts).

## Files changed
| File / folder | Change |
|---------------|--------|
| `CLAUDE.md` | New: project rules and documentation requirements |
| `docs/` | New: the whole wiki |
| `README.md` | Added a fork notice linking to the wiki |

## Verification
Documentation only. Checked that every link on the session board points to an existing file.

## Determinism impact
None.

## Problems & open questions
- The owner needs to accept or change ADR-0003/0004/0005 before Wave 2 starts. Wave 1 is unaffected.

## Next steps
Start Wave 1: S01, S02, S03 and S04 in parallel, one agent each.
