# ADR-0011: Counterstrike and Aftermath content in multiplayer

- **Status:** Accepted (project owner: "if possible")
- **Date:** 2026-10-14
- **Session:** Wave 1 review (question raised by S01 and S04)

## Context
The code already contains the Counterstrike and Aftermath units and rules (`FIXIT_CSII`, `FIXIT_ANTS`,
`FIXIT_CARRIER`, … are on), and a runtime switch per multiplayer game (`bAftermathMultiplayer`, `FIXIT_VERSION_3`).
The network version check carries an "Aftermath" flag (bit 31 of `MinVersion`/`Version`, `NETDLG.CPP:3082-3091`).
The content needs the expansion MIX files, which the C&C Ultimate Collection includes.

## Decision
Expansion content is in scope for multiplayer **when every player has the expansion data**:
- keep the `FIXIT_*` expansion switches on;
- keep the Aftermath flag in the lobby/version check, so a game only enables the expansions if all players have them;
- if a player lacks the data, the game falls back to the base units (the original behaviour) rather than refusing to start.

## Consequences
S10 must detect whether the expansion MIX files are present. S13 shows the expansion setting in the lobby, and tests
a mixed game (one player without the expansions). S11/S15 playtests include expansion units.
