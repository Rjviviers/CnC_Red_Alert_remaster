# ADR-0006: Never ship game assets

- **Status:** Accepted
- **Date:** 2026-10-14
- **Session:** S00

## Context
The source code is GPLv3, but the game's data files (art, sound, maps in `.MIX` archives) are not.

## Decision
No game assets are committed or distributed. Players point the game at their own legally owned copy
(for example the C&C Ultimate Collection on Steam or the EA App).

## Consequences
Installation needs a "point at your game folder" step (S10). CI can't run tests that need real assets
unless the test data is freely licensed or generated.
