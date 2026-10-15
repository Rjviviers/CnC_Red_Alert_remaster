# Playing: getting a build and pointing it at your Red Alert data

This page is for players and testers. Building from source is in [building.md](building.md).

> **Status (2026-10-15, session S10c):** the game builds, links and starts on Linux and Windows. Without data it
> reaches the data check and exits with a clear message. Whether it reaches the main menu with real data has **not**
> been tested yet (milestone M1): the project has no game data, so the first run with real data is yours.
> Please report what happens (see [Troubleshooting](#troubleshooting)).

## 1. Get a build

Every push to the repository builds the game in CI (GitHub Actions, workflow `build`). Open the run, scroll to
**Artifacts**, and download:

| Artifact | Contents |
|----------|----------|
| `ra95-windows-x86` | `ra95.exe`, the Visual C++ runtime DLLs it needs, `README.txt` |
| `ra95-linux-x86` | `ra95-linux-x86.tar.gz` (keeps the executable bit) with `ra95` and `README.txt` |

Unpack it into a folder you can write to: the game creates `REDALERT.INI` (and later saved games) next to itself.

**Linux:** the program is 32-bit x86. On 64-bit Debian/Ubuntu install the 32-bit runtime:

```sh
sudo dpkg --add-architecture i386 && sudo apt update
sudo apt install libc6:i386 libstdc++6:i386 libgcc-s1:i386 libx11-6:i386 libxext6:i386 \
     libxrandr2:i386 libxcursor1:i386 libxi6:i386 libxfixes3:i386 libasound2t64:i386
tar -xzf ra95-linux-x86.tar.gz && cd ra95-linux-x86
```

(`libasound2:i386` instead of `libasound2t64:i386` before Ubuntu 24.04 / Debian 13.) The CI build of SDL supports X11
and ALSA only.

## 2. Get the data

The data files are **not** part of this project and never will be ([ADR-0006](decisions/0006-no-game-assets.md)).
Use your own copy of Red Alert, for example:

- the **C&C Ultimate Collection** (EA app / Steam): the Red Alert folder of the installation;
- the **2008 freeware release** of Red Alert (version 3.03, with Counterstrike and Aftermath), which EA made available
  for free: install it and use the installed folder (if you take the files from its disc images instead, you need
  both `REDALERT.MIX` and `MAIN.MIX`; where they sit on the discs has not been checked by this project);
- an original Windows 95 installation (with the CD contents copied to the hard drive).

### Files the game needs

Names are matched without regard to upper/lower case (`redalert.mix` is fine).

| File | Needed? | What it is |
|------|---------|------------|
| `REDALERT.MIX` | **required** | Holds `LOCAL.MIX` (fonts, palettes, strings), `HIRES.MIX` (interface art) and `NCHIRES.MIX` |
| `MAIN.MIX` | **required** | Holds `CONQUER.MIX` (game art), `GENERAL.MIX`, `SCORES.MIX` (music), `SOUNDS.MIX`, `SPEECH.MIX`, `RUSSIAN.MIX`, `ALLIES.MIX`, `MOVIES1.MIX`/`MOVIES2.MIX` |
| `HIRES1.MIX` | recommended | Interface art added with Aftermath (`FIXIT_CSII` code in `INIT.CPP`). Missing: a warning, the game carries on |
| `EXPAND.MIX` | optional | **Counterstrike** missions and units |
| `EXPAND2.MIX` | optional | **Aftermath** missions and units |
| `SC*.MIX`, `SS*.MIX` | optional | Extra scenario packs; looked for in the game folder only (as in the original) |

Instead of `REDALERT.MIX`/`MAIN.MIX` the game also accepts the files inside them as loose files (`LOCAL.MIX`,
`HIRES.MIX`, `CONQUER.MIX`, ...). The expansions are detected from `EXPAND.MIX` and `EXPAND2.MIX` (the names come from
`CODE/INIT.CPP` and `CODE/EXPAND.CPP`; the original installer used registry values instead,
[ADR-0011](decisions/0011-expansions-in-multiplayer.md)). Multiplayer only enables expansion content when every player
has it.

### The order the game loads them

Read from the code (`CODE/STARTUP.CPP` `main`, `CODE/INIT.CPP` `Init_Game`), so you know what to check if it stops early:

1. `REDALERT.INI` in the game folder (created if missing). The start-up check looks for `REDALERT.MIX` and `MAIN.MIX`.
2. Window, timer, sound, keyboard and mouse start.
3. `Bootstrap` → `Init_Bootstrap_Mixfiles`: `EXPAND2.MIX` (if present), `HIRES1.MIX`, `EXPAND.MIX` (if present),
   `REDALERT.MIX`, then from inside it `LOCAL.MIX` and `HIRES.MIX` (read into memory) and `NCHIRES.MIX`.
4. `Init_Fonts`: `12METFNT.FNT`, `HELP.FNT`, `6POINT.FNT`, `GRAD6FNT.FNT`, `EDITFNT.FNT`, `8POINT.FNT`, `3POINT.FNT`,
   `SCOREFNT.FNT`, `LED.FNT`, `VCR.FNT`; then `CONQUER.ENG` (all game text), `DEBUG.ENG`, `TEMPERAT.PAL`; then
   `SC*.MIX`/`SS*.MIX` in the game folder.
5. `Init_Mouse`: `MOUSE.SHP`.
6. `Init_Secondary_Mixfiles`: `MAIN.MIX`, then from inside it `CONQUER.MIX`, `GENERAL.MIX`, `MOVIES1.MIX` (or
   `MOVIES2.MIX`), `SCORES.MIX` (music list), `SPEECH.MIX`, `SOUNDS.MIX`, `RUSSIAN.MIX`, `ALLIES.MIX`.
7. `RULES.INI`, and `AFTRMATH.INI` when Aftermath is present.
8. The intro movie is skipped (movies are not played, [ADR-0005](decisions/0005-multiplayer-first-scope.md)).
9. `PALETTE.CPS` (colour remaps), `TITLE.PCX` (title screen with "Stand by..."), then `Init_Bulk_Data`: caches
   `CONQUER.MIX` (and the sound MIX files), reads `TUTORIAL.INI`, and loads the shapes of every unit, building and
   terrain type.
10. `Options.Load_Settings` (reads `REDALERT.INI` again), then the main menu (`Select_Game` → `Main_Menu`).

## 3. Run it

Either copy the `.MIX` files into the game's folder and start `ra95` / `ra95.exe`, or tell the game where they are:

```sh
./ra95 -data=/path/to/redalert                 # Linux
ra95.exe -data="C:\Games\Red Alert"            # Windows (quotes because of the space)
```

or set `Path=` in the `[Data]` section of `REDALERT.INI`.

### Where the game looks

In this order; the first folder that has the file wins:

1. **the data folder**: `-data=<folder>`, otherwise `[Data] Path=` from `REDALERT.INI`;
2. **the game folder**: the folder that holds `ra95`/`ra95.exe` (the game makes it the current directory, as the
   original did);
3. **the start-up folder**: the current directory when you started the game.

This order applies to the MIX files. Other files (`REDALERT.INI`, saved games, a loose `RULES.INI` override) are read
from the game folder first, because that is where the game writes them ([ADR-0023](decisions/0023-data-folder-and-first-run.md)).

### Command-line options

| Option | Meaning |
|--------|---------|
| `-data=<folder>`, `--data=<folder>`, `-data <folder>` | Data folder (see above). Relative paths are relative to where you start the game. Added in S10c |
| `-480` | Original option for 640x480. `Resolution=` in `REDALERT.INI` is read after it and wins (as in the original), so use `Resolution=1` instead |
| `-CD<folder>` | Original option: extra search folder(s), `;`-separated. Searched before the data folder. Prefer `-data` |
| `-ATTRACT`, `-STEALTH`, `-MESSAGES`, `-SOCKET<n>`, `-DESTNET<addr>` | Original options (demo mode, hidden names, outside messages, IPX). The network options do nothing until S12 |
| `-?` | Prints the options and exits |

### `REDALERT.INI`

Created on the first run in the game folder with these keys:

| Section | Key | Default | Meaning |
|---------|-----|---------|---------|
| `[Data]` | `Path` | empty | Data folder. Empty: game folder, then start-up folder. Relative paths are relative to the game folder. `-data` overrides it |
| `[Options]` | `Resolution` | `0` | `0` = 640x400 (the original default), `1` = 640x480 with the 640x400 picture centred |

The game adds its own sections when it saves settings (`[Options]` game speed, volumes, ...; `[MultiPlayer]` name,
colour, ...), which also removes the comments. Delete the file to get the defaults back. An `[Intro] PlayIntro` key from
an old installation is ignored.

## Troubleshooting

Messages appear in a message box and in the terminal (stderr). Every start also prints the folders searched and the
files found, for example:

```
Red Alert: data search 1: /home/me/ra (data folder, from -data)
Red Alert: data search 2: /home/me/ra95-linux-x86 (game folder)
Red Alert: found /home/me/ra/redalert.mix
Red Alert: Aftermath (EXPAND2.MIX): found
```

| Message | What to do |
|---------|------------|
| "The Red Alert game data was not found" | The listed files are not in any listed folder. Check the folder names, or use `-data=` |
| "The data folder does not exist" | Typo in `-data=` or `[Data] Path=` |
| "Could not create REDALERT.INI in ..." | Put the game in a folder you can write to |
| "LOCAL.MIX could not be read", "HIRES.MIX ...", "CONQUER.MIX ...", "The game fonts ..." | `REDALERT.MIX` or `MAIN.MIX` is damaged, truncated, or from another game/version. Copy it again |
| "HIRES1.MIX ... not found or damaged" (terminal only) | Your data probably predates Aftermath; the game continues (untested) |
| "assert '...' failed at line N in module X" | A bug. The text is also in `ASSERT.TXT` in the game folder. Please report it |
| The game crashes or hangs | Please report it (below) |

**Headless / no display:** `SDL_VIDEO_DRIVER=offscreen SDL_AUDIO_DRIVER=dummy ./ra95 ...` runs without a window or sound;
messages then go to stderr only.

**Reporting a problem:** open an issue with the terminal output, `ASSERT.TXT` if there is one, which data you used
(Ultimate Collection, freeware 3.03, ...), the `dir`/`ls` listing of your data folder, and for a crash on Linux a
backtrace (`gdb -batch -ex run -ex bt --args ./ra95 -data=...`).
