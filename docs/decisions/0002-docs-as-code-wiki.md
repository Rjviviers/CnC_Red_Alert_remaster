# ADR-0002: Docs-as-code wiki in `docs/`, every step documented

- **Status:** Accepted
- **Date:** 2026-10-14
- **Session:** S00

## Context
The project owner requires proper documentation of every step. Several agents will work in parallel,
and each of them needs written context to work from.

## Options considered
1. **GitHub Wiki:** a separate git repository, not reviewed in PRs, easily drifts out of date.
2. **`docs/` folder in the repo:** versioned and reviewed together with the code, and readable by agents.

## Decision
Use `docs/` as the wiki. Every session has a brief and a log, decisions are ADRs, and
knowledge about the old code goes in `docs/codebase/`. The rules are enforced through `CLAUDE.md`.

## Consequences
Every PR carries documentation, so PRs get a bit bigger, and reviewers check documentation as well as code.
