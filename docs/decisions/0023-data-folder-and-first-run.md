# ADR-0023: Data folder, first-run configuration and start-up checks

- **Status:** Proposed
- **Date:** 2026-10-15
- **Session:** S10c

## Context
Players supply the game data themselves (ADR-0006). The original game found its data through an installer: files on the
hard drive next to `RA95.EXE`, the rest on the CD (`CDFileClass` search drives, `?:\`), the expansions through registry
values, and `REDALERT.INI` written by `SETUP.EXE`. None of that exists in the port. At the end of S10b the game:

- exited with "Run SETUP program first" when `REDALERT.INI` was missing;
- looked for data only in the executable's folder (after `chdir`), and would have shown "insert CD" dialogs (there are no
  CD drives) whenever the code asked for a specific CD (`RequiredCD` 0..5);
- with a missing or damaged MIX file, hit `assert` (whose handler draws a dialog with fonts that are not loaded yet) or,
  without asserts, crashed later on a NULL pointer;
- treated a first run (`[Intro] PlayIntro` missing) as "started from the installer": intro movie, then straight into the
  first Allied mission without the main menu.

## Options considered
1. **Ship a `REDALERT.INI` and require the data in the executable's folder.** Simple, but the CI artifact would have to
   carry an INI, and players with an existing installation (Ultimate Collection, freeware release) would have to copy
   gigabytes next to every build.
2. **Data folder setting with a fixed search order, generated INI, start-up checks** (chosen).
3. **Per-user configuration directory** (`SDL_GetPrefPath`, `%APPDATA%`) for the INI and saves. Better for installed
   builds, but a bigger change (saved games, recordings and every `CONFIG_FILE_NAME` user would move) that no one needs
   before there is an installer; it can supersede the "INI next to the executable" part later.

## Decision
- **Data folder:** `-data=<folder>` (also `--data=`, `-data <folder>`; taken out of `argv` before `Parse_Command_Line`,
  which upper-cases arguments) or `[Data] Path=` in `REDALERT.INI` (relative to the game folder). The command line wins.
- **Search order:** data folder → game folder (the executable's folder; the current directory after `main`'s `chdir`)
  → start-up folder (the current directory when the game started). Duplicates are dropped. Registered with
  `CCFileClass::Set_Search_Drives`. File names stay case-insensitive (ADR-0022).
- **MIX files are searched in that order; other files keep the original "current directory first" order.** A bare
  `*.MIX` name skips the current-directory check in `CDFileClass::Set_Name` (the game folder is in the list anyway).
  Otherwise a `REDALERT.INI` or saved game inside the player's data folder (an old installation) would be read instead
  of the one the game writes next to the executable (`OPTIONS.CPP` reads `REDALERT.INI` through `CCFileClass`).
- **First run:** if `REDALERT.INI` does not exist in the game folder, write one with documented defaults
  (`RA_Boot_Default_Config`, docs/playing.md). If it cannot be written, say so and exit.
- **`[Intro] PlayIntro` is ignored** (neither read nor written): every start shows the main menu. Movies are not played
  (ADR-0005), and `Play_Movie` returns at once, so nothing waits on them.
- **No CDs:** `Force_CD_Available` always succeeds. A file that is really missing is reported where it is loaded.
- **Start-up checks:** before the window opens, `REDALERT.MIX` (or a loose `LOCAL.MIX`) and `MAIN.MIX` (or a loose
  `CONQUER.MIX`) must be in the search list; otherwise a message names the missing files and every folder searched (in
  order, marking folders that do not exist) and the game exits with code 1. During `Init_*`, the asserts on
  `LOCAL.MIX`/`HIRES.MIX`, a missing font or `CONQUER.MIX` become the same kind of message (`Boot_Data_Error`); a MIX
  file whose header cannot describe the file is treated as missing instead of being read. `HIRES1.MIX` (Aftermath art)
  is reported on stderr and skipped. The message is a platform message box, and also goes to stderr; on the headless
  video drivers (`offscreen`, `dummy`) it goes to stderr only, so CI and servers never block on a dialog.
- **Expansions** are detected from their MIX files (ADR-0011): Counterstrike = `EXPAND.MIX`, Aftermath = `EXPAND2.MIX`,
  the names the original code used before it switched to the registry (`EXPAND.CPP`, `CONQUER.CPP`, `INIT.CPP`).

## Consequences
- A CI build runs from any folder: `ra95 -data=<your Red Alert folder>`. docs/playing.md is the player's reference.
- The game folder must be writable (INI, saves). If that becomes a problem, option 3 supersedes this part.
- `CDFileClass` searches MIX files in a different order than the original when the same MIX exists in several folders;
  the player's data folder wins.
- The boot smoke test (`ctest -R s10c_boot_smoke`) checks the no-data path on every CI run.
- The original "from install" flow (`Special.IsFromInstall`) is now reachable only through its hidden command-line switch.
