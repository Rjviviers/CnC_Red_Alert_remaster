# Session Board

Status values: `Not started` · `In progress` · `Blocked` · `In review` · `Done`.
Update this table at the start and end of every session (see [`CLAUDE.md`](../../CLAUDE.md)).
The dependency graph is in the [roadmap](../roadmap.md).

| ID | Session | Wave | Depends on | Parallel-safe with | Status | Log |
|----|---------|------|------------|--------------------|--------|-----|
| S00 | [Project wiki & plan](S00-wiki-and-plan.md) | 0 | — | — | Done | [log](../logs/2026-10-14-S00-wiki-and-plan.md) |
| S01 | [Audit: game code](S01-audit-game-code.md) | 1 | S00 | S02, S03, S04 | Done | [log](../logs/2026-10-14-S01-audit-game-code.md) |
| S02 | [Audit: support libraries & assembly](S02-audit-libraries.md) | 1 | S00 | S01, S03, S04 | Done | [log](../logs/2026-10-14-S02-audit-libraries.md) |
| S03 | [CMake + CI skeleton](S03-build-system-ci.md) | 1 | S00 | S01, S02, S04 | Done | [log](../logs/2026-10-14-S03-build-system-ci.md) |
| S04 | [Audit: networking → LAN design](S04-audit-networking.md) | 1 | S00 | S01, S02, S03 | Done | [log](../logs/2026-10-14-S04-audit-networking.md) |
| S05 | [Strip dead platforms](S05-strip-dead-platforms.md) | 2 | S01, S02, S03 | S04 | Done | [log](../logs/2026-10-14-S05-strip-dead-platforms.md) |
| S06 | [Compiler compatibility](S06-compiler-compat.md) | 3 | S05 | S07, S08, S09 | Done | [b](../logs/2026-10-14-S06b-compile-group1.md), [a](../logs/2026-10-14-S06a-shared-headers.md), [c](../logs/2026-10-14-S06c-compile-group2.md), [d](../logs/2026-10-14-S06d-compile-group3.md), [e](../logs/2026-10-14-S06e-compile-group4.md), [f](../logs/2026-10-14-S06f-compile-libraries.md) |
| S07 | [Assembly → C++](S07-asm-to-cpp.md) | 3 | S02, S03 | S06, S08, S09 | Done (A–D + audio in S09) | [A](../logs/2026-10-14-S07A-surface-primitives.md), [B](../logs/2026-10-14-S07B-shape-renderer.md), [C](../logs/2026-10-14-S07C-text-palette-cursor.md), [D](../logs/2026-10-14-S07D-data-codecs.md) |
| S08 | [Platform layer: video, input, timer](S08-platform-layer.md) | 3 | S01, S03, S05 | S06, S07, S09 | Done | [log](../logs/2026-10-14-S08-platform-layer.md) |
| S09 | [Audio via SDL](S09-audio.md) | 3 | S02, S03, S05 | S06, S07, S08 | In review (SDL backend on by default; playback not yet heard, so check in S10) | [log](../logs/2026-10-14-S09-audio.md) |
| S10 | [Link & boot to main menu → **M1**](S10-link-and-boot.md) | 4 | S06, S07, S08, S09 | — | Not started | |
| S11 | [Skirmish vs AI playable → **M2**](S11-skirmish.md) | 5 | S10 | S12 | Not started | |
| S12 | [Portable UDP sockets](S12-portable-sockets.md) | 5 | S04, S10 | S11 | Not started | |
| S13 | [LAN discovery, lobby, game start](S13-lan-lobby.md) | 6 | S12 | S14 | Not started | |
| S14 | [Sync & determinism harness](S14-sync-harness.md) | 6 | S11 | S13 | Not started | |
| S15 | [LAN release & playtest → **M3**](S15-lan-release.md) | 7 | S13, S14 | — | Not started | |

## Running sessions with multiple agents

- Each agent gets **one session brief** (or one sub-task of it) and works on its own branch.
- Agents in the same wave must not edit the same source files. Every brief lists the files it **owns**.
- Shared files (`CMakeLists.txt`, this board) get small, append-only edits so merges stay trivial.
- An agent's PR must include the session log. Reviewers reject PRs without documentation.
