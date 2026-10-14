# Session log: S05 — Strip dead platforms

- **Date:** 2026-10-14
- **Session brief:** [S05](../sessions/S05-strip-dead-platforms.md)
- **Branch / PR:** `worktree-agent-a058690d1ddd32087` (based on `claude/cool-bohr-9vg7ac` at `7a69f38`) / not pushed
- **Status at end:** Done (in review)

## Goal
Shrink the codebase to what we will port: delete the DOS build, native IPX, modem play, Westwood Online and
TEN/MPath, collapse the DOS `#ifdef` branches, keep the LAN path (UDP transport + "IPX"-named connection layer),
lockstep, the lobby, skirmish, the expansions and the single-player code, and record every removal.

## What was done
In this order, one commit per step (hashes in [removed-code.md](../codebase/removed-code.md#summary)):

1. **Worktree brought up to date** with `git merge --ff-only claude/cool-bohr-9vg7ac` (head `7a69f38`).
   Installed `unifdef` 2.12 (`apt-get install -y unifdef`); `gcc-multilib`/`g++-multilib` were already present.
2. **Baseline compile-progress run** (see Verification): 10 of 316 translation units compile.
3. **Hand-maintained source lists (`2557f1d`).** Done first, so that every later deletion commit could update
   `cmake/RASources.cmake` and keep CMake configuring. Froze the generated file (new header comment), deleted
   `scripts/gen_cmake_sources.py`, removed the `--check` steps from `.github/workflows/build.yml`, updated
   `docs/building.md`, wrote [ADR-0012](../decisions/0012-hand-maintained-source-lists.md) (Proposed). Every later
   deletion pruned the list with a small script that drops entries whose file no longer exists.
4. **DOS trees and dead files (`e87f703`):** `WWFLAT32/`, `VQ/`, `LAUNCH/`, `LAUNCHER/`, the `TOOLS/` binaries (kept
   `MIXFILE.CPP`, `AUDIOMAK.CPP` as format references), `WIN32LIB/SRCDEBUG|EXAMPLE|MOVIE` and the `OLD/TEST/MSVC/OLDMEM`
   folders, `WINVQ/VPLAY32|VQAVIEW|VQA32/OLD|LIB` (the HMI SOS `.LIB`s), and 52 assembly files from S02's delete list.
   `IPX/` was left for the IPX commit.
5. **`unifdef` collapse (`98d4fee`)**, before touching IPX/modem/WOL, so that the later hand edits work on the
   code the real game compiled. Symbol set, over every `.CPP/.H/.C` in `CODE/`, `WIN32LIB/`, `WINVQ/`, `TOOLS/`:
   `-DWIN32=1 -D_WIN32 -DWINSOCK_IPX -UDOS4G -UGF_WATCOM_S -UPHARLAP -UPHARLAP_TNT -UINTERNET_OFF`.
   132 files, 5,437 lines, no insertions. `INTERNET_OFF` had to be included because seven
   `#if defined(WIN32) && !defined(INTERNET_OFF)` lines would not collapse otherwise (it is commented out in
   `DEFINES.H`, so undefined is the real build). `CODE/RAWOLAPI.H` and `WIN32LIB/INCLUDE/VIDEO.H` could not be
   parsed by unifdef and were left unchanged (the first is deleted later; the second has none of the symbols).
6. **Native IPX (`59ea2e1`):** deleted `IPX/`, `IPX.CPP`, `IPX95.*`, `IPXPROT.ASM`, `IPXREAL.ASM`, `WSPIPX.*`,
   `FAKESOCK.H`; **in the same commit** `INIT.CPP` (`GAME_IPX`, old line 1255) now does
   `PacketTransport = new UDPInterfaceClass;`. Removed the prototypes for the deleted functions (`IPX.H`, `IPXMGR.H`,
   `WINASM.ASM` externs) and `Get_OS_Version()` (only fed `IPX95.CPP`). The `wwipx32.lib` link existed only in the
   original makefile; CMake never linked it (comment updated).
7. **Skirmish dialog extracted (`064a13a`)**, a pure move: `Com_Scenario_Dialog` and `Find_Local_Scenario` (used
   by the LAN lobby) from `NULLDLG.CPP` (around old line 2617) into the new `CODE/SKIRMDLG.CPP`.
8. **Modem / Greenleaf (`8b5bb75`):** deleted the rest of `NULLDLG.CPP`, `NULLMGR.*`, `NULLCONN.*`, the unbuilt
   `SEQCONN.*`/`NOSEQCON.*` and `WIN32LIB/WINCOMM/`; removed `GAME_MODEM`/`GAME_NULL_MODEM` and every branch using
   them (QUEUE, CONQUER, INIT, MPLAYER, SCORE, SENDFILE, NETDLG, SESSION, GLOBALS, EXTERNS), the modem settings in
   `SessionClass`, the Greenleaf/`phone.h`/`nullmgr.h` includes in `FUNCTION.H`, and the Greenleaf stubs in
   `WINASM.ASM`. Stripped the modem host paths from `SKIRMDLG.CPP`. Removal was done with a small line-range
   tool (delete block / keep the else branch) driven by line numbers read from the code, not by regex rewriting.
9. **WOL, WChat/DDE, TEN, MPath (`ca5523e`):**
   - `unifdef -DTEN=0 -DMPATH=0` over all sources, then removed the remaining `GAME_TEN`/`GAME_MPATH` checks.
   - Listed every `#ifdef WOLAPI_INTEGRATION` block in the kept files with a script (`#else` or not). Deleted the
     pure-WOL blocks (no `#else`) by hand after reviewing each; kept four that are not WOL functionality
     (`DROP.CPP/.H`, `HOUSE.H` `InitialName`, `SCENARIO.CPP`); `CONQUER.CPP` was done by hand because a `#endif`
     inside a comment confused the script. Hand-edited the `#ifdef/#else` button layouts in `MPLAYER.CPP` and
     `GAMEDLG.CPP`. Then `unifdef -DWOLAPI_INTEGRATION` over all sources.
   - Moved `PlanetWestwoodPortNumber` (and `ShowCommand`, also defined in `INTERNET.CPP`) to `GLOBALS.CPP`,
     `GAME_VERSION` to `VERSION.CPP`, `bSpecialAftermathScenario` (found in `WOL_GSUP.CPP`, used by `NETDLG.CPP` and
     the skirmish dialog) to `SESSION.CPP`. Removed the statistics calls, WOL pumps, tournament rules and the
     `Winsock.Get_Connected()` call in `NETDLG.CPP` (`Shutdown_Network`).
   - Deleted 50 WOL/internet files and 8 TEN/MPath files (list in removed-code.md §5). Trimmed `WOLSTRNG.CPP/.H` to
     the 12 strings still used. Dropped the `WOLAPI_INTEGRATION`/`WINSOCK_IPX` compile definitions.
10. **Remaining DOS-only sources (`807268f`):** `KEYFRAME.CPP`, `DPMI.CPP/.H`, `CWSTUB.C`.
11. **Documentation (this commit):** `removed-code.md` (summary and ADR-0005 status), `overview.md` directory table,
    "Update from S05" notes on `networking.md`, `platform-dependencies.md`, `libraries.md`, `assembly-inventory.md`,
    `game-code.md`, an implementation note on ADR-0005, this log, the brief.

Checks used after each source commit: a brace/parenthesis/`#if` balance script compared each edited file with its
previous version (for the WOL collapse, against the old file run through the same `unifdef`); a missing-include
scan; a grep for every removed symbol; and the compile-progress run.

## Decisions
- [ADR-0012](../decisions/0012-hand-maintained-source-lists.md) (new, Proposed): hand-maintained CMake source lists;
  generator deleted; the original makefiles stay untouched as a record.
- **Order:** the `WIN32`/DOS collapse came before the IPX/modem/WOL deletions (the brief's example order had it
  later); this let `WINSOCK_IPX` take the real-mode IPX code out mechanically instead of by hand.
- **`WOLAPI_INTEGRATION` resolved as defined**, as instructed. The only places where the result equals the old
  `#else` text are the WOL button layouts (`MPLAYER.CPP`, `GAMEDLG.CPP`) and `PAGE_RESPOND_KEY` in `CONQUER.CPP`:
  there, removing the WOL button/key from the compiled branch gives exactly the `#else` text. `MPLAYER.CPP` keeps the
  shipped game's centred dialog position rather than the `#else` one.
- **`GameType` lost `GAME_MODEM`, `GAME_NULL_MODEM`, `GAME_TEN`, `GAME_MPATH`**, so the remaining values changed
  (`GAME_IPX` 3 → 1, `GAME_INTERNET` 4 → 2, `GAME_SKIRMISH` 5 → 3). `Session.Type` is not saved, recorded or sent.
- **Kept on purpose:** `COMQUEUE.*` (brief), `GAME_INTERNET` and `UTRACKER.*` (their remaining uses are in simulation
  files and `HouseClass`; left for S12), `SpecialClass::IsFromWChat` (wire layout), `HouseClass::InitialName` (save
  layout), `SerialPacketType`/`SerialCommandType`/`RemoteFileTransferType` (used by the LAN file transfer),
  `WIN32LIB/PROFILE`, `MONO`, `PLAYCD`, `APROFILE.ASM`, `MCGABUF.ASM`, the `ModeX_Blit`/`Set_Palette_Register` asm,
  the unbuilt experiments in `CODE/` and the IDE project files.
- The `skirmish` parameter of `Com_Scenario_Dialog` and its `!skirmish` UI branches stay (now dead) to keep the diff
  small; S11 can simplify.
- `#if 0`, `#ifdef NEVER`, `FIXIT_*` and other non-platform switches were not touched (ADR-0011).

## Files changed
| File / folder | Change |
|---------------|--------|
| `WWFLAT32/`, `VQ/`, `IPX/`, `LAUNCH/`, `LAUNCHER/` | Deleted |
| `TOOLS/` | Binaries and project files deleted; `MIX/MIXFILE.CPP`, `AUDIOMAK/AUDIOMAK.CPP` kept |
| `WIN32LIB/` | `SRCDEBUG/`, `EXAMPLE/`, `MOVIE/`, `WINCOMM/`, old/test folders and unused asm deleted (138 asm files in all); DOS branches collapsed |
| `WINVQ/` | `VPLAY32/`, `VQAVIEW/`, `VQA32/OLD/`, `LIB/` and unused asm deleted (19 asm files in all); DOS/PharLap branches collapsed |
| `CODE/` | 86 files deleted (IPX, modem, WOL, WChat/DDE, stats, TEN/MPath, DOS, unused asm); `SKIRMDLG.CPP` new; 87 files edited (most only by unifdef) |
| `cmake/RASources.cmake` | Hand-maintained; deleted files removed, `SKIRMDLG.CPP` added, `PASSEDIT.OBJ` dropped |
| `cmake/RABuildConfig.cmake`, `CMakeLists.txt` | `WOLAPI_INTEGRATION`/`WINSOCK_IPX` defines dropped; comments |
| `.github/workflows/build.yml` | `gen_cmake_sources.py --check` steps removed |
| `scripts/gen_cmake_sources.py` | Deleted (ADR-0012) |
| `docs/codebase/removed-code.md` | New: every removal, why, last commit that has it |
| `docs/decisions/0012-hand-maintained-source-lists.md` | New ADR (Proposed) |
| `docs/decisions/0005-multiplayer-first-scope.md` | Implementation note added (status unchanged) |
| `docs/codebase/overview.md` | Directory table and network notes updated |
| `docs/codebase/{networking,platform-dependencies,libraries,assembly-inventory,game-code}.md` | "Update from S05" notes |
| `docs/building.md` | Hand-maintained lists, defines |
| `docs/sessions/S05-strip-dead-platforms.md` | Tasks ticked, status In review |

## Verification
```sh
python3 scripts/compile_progress.py          # before (at 7a69f38)
# -> 10 of 316 translation units compile (3.2%)
#    redalert 7/242, win32lib 1/48, vqa32wp 0/9, vqm32wp 2/17; 65 ASM files listed
python3 scripts/compile_progress.py          # after (807268f, fresh build/ directory)
# -> -- Configuring done ... 10 of 284 translation units compile (3.5%)
#    redalert 7/212, win32lib 1/46, vqa32wp 0/9, vqm32wp 2/17; 37 ASM files listed
```
The same 10 files compile before and after (checked by comparing the two `compile-progress.json` files:
`BASE64`, `LCW`, `LZO1X_C`, `LZO1X_D`, `MPU`, `RANDOM`, `_WSPROTO` in `CODE/`; `REGIONSZ`; `VQM32/MEM`, `VQM32/VERTAG`).
The run was also repeated after the modem and the WOL commits with the same result. CMake configure succeeds.
The most common first error is still `missing header: windows.h` (171 files, was 201).

```sh
git diff --shortstat 7a69f38 807268f                 # 1333 files changed, 1720 insertions(+), 417264 deletions(-)
git diff --shortstat --diff-filter=D 7a69f38 807268f # 1209 files deleted, 320 of them .ASM
#   (S02's delete list had 322; APROFILE.ASM and MCGABUF.ASM were kept)
grep -rn "GAME_MODEM\|NullModem\|pWolapi\|WOLAPI_INTEGRATION\|WINSOCK_IPX\|GAME_TEN\|#if(TEN)" CODE WIN32LIB WINVQ  # only comments/makefile
```

## Determinism impact
None expected for the simulation. Details:
- No change to simulation code paths that run in LAN or skirmish games. The removed branches only ran for modem,
  WOL/internet, TEN or MPath sessions (or never: DOS, `#ifndef WINSOCK_IPX`, `#ifndef WOLAPI_INTEGRATION`).
  The only simulation-file edits are the TEN/MPath blocks in `HOUSE.CPP` (`#if(TEN)`, dead).
- `GameType` numeric values changed (see Decisions). Not part of the simulation or of any saved/sent data.
- `SessionClass` lost its modem and TEN/MPath members (layout change). `SessionClass` is not sent raw; its save
  routine writes selected fields only, none of the removed ones.
- Wire structs were not changed: `SerialPacketType`, `GlobalPacketType` (including `ShortFileName[13]`) and
  `SpecialClass` keep their layout. `EventClass` untouched.
- `HouseClass` layout unchanged (`InitialName` and the unit trackers kept).
- **Non-simulation behaviour changes:** the LAN menu now uses UDP (does not work yet; S12/S13); the multiplayer menu
  lost the Modem/Serial and Internet buttons (dialog 22*RESFACTOR shorter); a second game instance now starts
  (the DDE-based single-instance check is gone); the `[SerialDefaults]`/`[InitStrings]`/`[PhoneBook]` INI sections
  are no longer read or written.

## Problems & open questions
- S04's note that `SENDFILE.CPP:156,441` call `Winsock.Get_Connected()` was off: both were in `#ifndef WINSOCK_IPX`
  (never compiled). The compiled call was `NETDLG.CPP` `Shutdown_Network`; it was removed (always false on LAN).
- `CODE/CONQUER.CPP` has a `#endif` inside a `/* */` comment, which breaks naive `#if` counting (unifdef handles it).
- unifdef cannot parse `CODE/RAWOLAPI.H` (apostrophe in `#error`) or `WIN32LIB/INCLUDE/VIDEO.H` (unterminated
  comment); neither needed changes.
- `GAME_INTERNET` remains as an unselectable game type with dead branches in shared and simulation code. Whether
  to delete it or reuse it for internet play is for S12 (ADR-0009, own protocol).
- Movie stubbing (ADR-0005) is not done; `WINVQ/` still builds.

## Next steps
- **S06:** fewer files (284 TUs, was 316). `phone.h` no longer needs recreating (include removed); `PASSEDIT` is gone;
  `PALETTE.CPP`/`palette.h` and `pcx.h` are still missing. The Greenleaf `<modem.h>`/`<fast.h>` includes are gone
  from `FUNCTION.H`. Nothing tests `WIN32`, `WINSOCK_IPX`, `WOLAPI_INTEGRATION`, `TEN` or `MPATH` any more, so S06 can
  decide freely whether to keep defining `WIN32`/`_WIN32`. Source lists are now edited by hand (ADR-0012).
- **S07:** the asm delete list is done except `APROFILE.ASM`, `MCGABUF.ASM` and the non-modem dead parts of
  `WINASM.ASM` (`ModeX_Blit`, `Set_Palette_Register`); 37 asm files remain listed (5 in `CODE/`).
- **S08/S09:** decide `WIN32LIB/PROFILE`, `MONO`, `PLAYCD` and the movie stubs (`WINVQ/`, `INTERPAL.CPP`).
- **S11:** simplify `Com_Scenario_Dialog` in `SKIRMDLG.CPP` (drop the `skirmish` parameter and dead branches).
- **S12/S13:** the LAN path now starts `UDPInterfaceClass`; `PlanetWestwoodPortNumber` is in `GLOBALS.CPP`; decide
  `GAME_INTERNET` and the unit trackers.
- Orchestrator: update `docs/sessions/README.md`, `docs/logs/README.md`, `docs/decisions/README.md` (ADR-0012).
