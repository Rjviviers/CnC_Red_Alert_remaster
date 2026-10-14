# S10 — Link & boot to main menu → **Milestone M1**

**Status:** Not started · **Wave:** 4 · **Depends on:** S06, S07, S08, S09

## Goal
Link the executable, load the player's MIX files, and reach the main menu.

## Tasks
- [ ] Resolve every unresolved symbol, adding documented stubs where a feature is out of scope (movies, WOL, …).
- [ ] Asset path configuration: the player points the game at their Red Alert data folder (CLI flag + INI). Document it in `docs/playing.md`.
- [ ] Skip or stub the intro movies (ADR-0005).
- [ ] Detect whether the Counterstrike/Aftermath MIX files are present (ADR-0011).
- [ ] Fix startup crashes until the main menu shows and responds to the mouse.
- [ ] CI produces downloadable builds for Windows and Linux.

## Done when
**M1:** a CI-built binary reaches the main menu. The session log includes a screenshot.
