# Vision & Scope

## The goal

Our own modern version of Command & Conquer: Red Alert, built from the original source code,
**focused on multiplayer**.

1. **Short term:** play over LAN with a few friends on modern PCs.
2. **Medium term:** play over the internet with friends (direct connect, then a lobby).
3. **Long term:** public matchmaking and ranked play.

## In scope

- Building the original code with modern compilers on modern operating systems.
- Multiplayer: skirmish setup, LAN, internet, lobbies, matchmaking, ranking.
- Skirmish vs AI. It is not a goal in itself, but we need it to test the game on one machine.
- Quality-of-life improvements that help multiplayer (resolution, stability, reconnect, replays) — each as its own decision later.

## Out of scope (for now)

- The single-player campaign. We don't deliberately break it, but we don't fix or test it either.
- Cutscene movies (VQA). These are campaign-only, so they get stubbed out.
- DOS builds, IPX networking, modem/serial play, Westwood Online, and the TEN/MPath services.
- Distributing game assets. Players must own the game (for example the C&C Ultimate Collection) and point our build at their files.

## Milestones

| Milestone | What it means | How we know it's done |
|-----------|---------------|------------------------|
| **M1 — It boots** | A modern build starts, loads the player's MIX files and reaches the main menu | A screenshot of the main menu from a CI-built binary |
| **M2 — It plays** | A skirmish against AI can be played to the end | A full skirmish game without crashes, logged in the session log |
| **M3 — LAN with friends** | 2–4 players on one LAN can find each other, start a game, and finish it without desyncs | A playtest report with at least 3 full LAN games |
| M4 — Internet with friends | Direct-connect play across the internet | Planned after M3 |
| M5 — Public lobby & matchmaking | A server that players can find each other through | Planned after M4 |
| M6 — Ranked | Accounts, ratings, ladders | Planned after M5 |

M4–M6 are deliberately not broken into sessions yet. We'll plan them once M3 shows us what the networking code really needs.
