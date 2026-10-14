# Project rules for Claude (and every agent working in this repo)

This is a fork of the original Command & Conquer: Red Alert source code. The goal is a
modern, **multiplayer-focused** port: LAN first, then internet play, public matchmaking and
ranked. Single player is not a priority. Read `docs/README.md` (the project wiki) before starting.

## Documentation is mandatory — every step is documented

The project owner requires proper documentation of every step. A change is **not done** until
its documentation is done. For every work session:

1. **Before starting**, read the session brief in `docs/sessions/` and mark it `In progress`
   in `docs/sessions/README.md`.
2. **While working**, record anything learned about the old code in `docs/codebase/`
   (one page per subsystem; update existing pages instead of duplicating).
3. **Every non-obvious decision** (library choice, removing a feature, changing a file format or
   network protocol) gets an Architecture Decision Record in `docs/decisions/` using
   `docs/templates/adr.md`. Never silently reverse an accepted ADR — write a new one that supersedes it.
4. **At the end**, write a session log in `docs/logs/` using `docs/templates/session-log.md`
   (`YYYY-MM-DD-SNN-short-title.md`): goal, what was done, decisions, files changed, how it was
   verified (exact commands + results), problems, and next steps.
5. Tick the task checkboxes in the session brief, update its status in `docs/sessions/README.md`,
   and update the roadmap diagram in `docs/roadmap.md` if the plan changed.
6. Documentation goes in the **same commit/PR** as the code it describes.

## Working rules

- Never commit game assets (`*.MIX`, `*.AUD`, `*.VQA`, …). Players supply them from their own copy.
- Preserve original copyright headers in files you modify; the repo is GPLv3 with additional terms (see `LICENSE.md`).
- The multiplayer simulation is deterministic lockstep (`CODE/QUEUE.CPP`). Any change that can affect
  simulation (math, random numbers, struct layout of network events, iteration order) must be called
  out in the session log and checked with the sync harness once it exists (session S14).
- Prefer small, reviewable PRs — one session (or one sub-task of a session) per PR.
- When agents work in parallel, each one owns only the files listed in its session brief; touch
  shared files (CMake lists, `docs/sessions/README.md`) with minimal, merge-friendly edits.
