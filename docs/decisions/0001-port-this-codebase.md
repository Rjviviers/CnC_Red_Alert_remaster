# ADR-0001: Port this codebase directly

- **Status:** Accepted
- **Date:** 2026-10-14
- **Session:** S00

## Context
The goal is a modern, multiplayer-focused Red Alert. There are existing alternatives: Vanilla Conquer
(an already-modernised fork of this same code) and OpenRA (a full C# reimplementation).

## Options considered
1. **Rebase onto Vanilla Conquer.** Fastest route to a working build, but much of the work and design would be someone else's.
2. **Move to OpenRA.** Online play already exists, but it's a different engine and not this codebase.
3. **Port this repository ourselves.** Slowest start, but full ownership and understanding of every line.

## Decision
Port this repository ourselves (option 3). This was the project owner's explicit choice.

## Consequences
Getting a first build takes significant effort (Phases 1–2 of the roadmap). We can still read other
open-source ports for reference where their licences allow, and note it in the session log when we do.
