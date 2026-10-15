# Game Code (`CODE/`): Subsystems, Boot Sequence, Feature Switches

Made in session S01. This page explains how the game code in `CODE/` is organised, how the Win32 build
starts up and runs, which preprocessor switches shape it, and which code matters for deterministic lockstep.
The per-file table of platform dependencies is in [platform-dependencies.md](platform-dependencies.md).
For the networking stack, see S04's pages. For `WIN32LIB/`, `WINVQ/` and the assembly files, see S02's pages.

> **Update from S05 (2026-10-14).** The DOS branches, native IPX, modem play, Westwood Online, the WChat/DDE path
> and TEN/MPath described on this page were removed in S05; `WIN32`, `WINSOCK_IPX` and `WOLAPI_INTEGRATION` are no
> longer tested anywhere, and `TEN`/`MPATH` no longer exist. The skirmish dialog moved to `SKIRMDLG.CPP`.
> See [removed-code.md](removed-code.md).

## Summary

- **What gets built.** `CODE/MAKEFILE` builds `ra95.exe` from **250 object files**: 242 `.CPP`, 6 `.ASM`, and 2 sources that
  are **missing from the release** (`PALETTE.CPP` with `PaletteClass`, and `PASSEDIT.CPP`). Two more files are compiled through
  `#include`. 41 sources in `CODE/` are not in the Win32 build at all (DOS-only, TEN/MPath, experiments).
- **The build is a "release" build.** `DEFINES.H:58` sets `RELEASE_VERSION`, so the **scenario editor and cheat keys are
  compiled out**. `MAPEDIT*.CPP` and `DEBUG.CPP` compile to empty files. `WOLAPI_INTEGRATION` is on, so the game always
  contains the Westwood Online client, and some *older* internet/LAN code paths are compiled out by `#ifndef WOLAPI_INTEGRATION`.
- **The game is about 100,000 live lines** (non-blank, non-comment, compiled) in 252 files. The biggest parts are simulation
  (52 files plus 13 type-data files), UI (37), WOL (20) and the network core (18).
- **Boot path:** `WinMain` (`STARTUP.CPP:109`) → `Main_Game` (`CONQUER.CPP:215`) → `Init_Game` (`INIT.CPP:197`) →
  loop { `Select_Game` (`INIT.CPP:420`) → loop { `Main_Loop` (`CONQUER.CPP:2139`) } }.
  The Win32 message pump is hidden inside the keyboard class (`KEY.CPP:521`), and sockets are driven by window messages
  (`WINSTUB.CPP:184-186`).
- **Determinism:** the simulation uses only integers, fixed-point math and one seeded LCG (`Scen.RandomNumber`).
  There is no floating point in simulation files. The risks for a port are type widths (`long`, `bool`), struct packing
  and bit-fields of network events, and a few Watcom `#pragma aux` math helpers (`calcx`/`calcy`, `Fixed_To_Cardinal`)
  that must be rewritten bit-exactly.
- **Surprises that change later sessions:** the LAN game uses **Winsock IPX** (`AF_IPX`), not UDP, in the shipped build.
  The **skirmish setup dialog lives in the modem file** `NULLDLG.CPP`. Greenleaf is only needed for **headers**, not
  libraries. Details are in [Implications for later sessions](#implications-for-later-sessions).

## How the build list was derived

`OBJECTS` (`MAKEFILE:157-337`), the Win32 additions (`MAKEFILE:401-419`), `LIBFILES` → `jshell.lib` (`MAKEFILE:343-345`) and
`TECHFILES` → `tech.lib` (`MAKEFILE:348-399`) are all linked by `win95.lnk` (`MAKEFILE:544-570`). A script parsed these lists
with `WIN32` defined and mapped each object to its source (method and script in
[platform-dependencies.md](platform-dependencies.md#method-re-runnable)).

| Part | Objects | Sources |
|------|--------:|---------|
| `OBJECTS` (common) | 180 | 178 `.CPP`, 1 `.ASM` (`IPXPROT`), 1 missing (`PASSEDIT`) |
| `OBJECTS +=` under `!ifdef WIN32` | 18 | 14 `.CPP`, 4 `.ASM` (`2KEYFBUF`, `CPUID`, `WINASM`, `2TXTPRNT`) |
| `LIBFILES` (`jshell.lib`) | 2 | `ROTBMP.CPP`, `SPRITE.CPP` |
| `TECHFILES` (`tech.lib`) | 51 listed, 50 unique | 48 `.CPP`, 1 `.ASM` (`LCWCOMP`), 1 missing (`PALETTE`). `INI.OBJ` is listed in both `OBJECTS` and `TECHFILES` |
| **Total unique** | **250** | **242 `.CPP` + 6 `.ASM` + 2 missing** |
| Compiled via `#include` | — | `MAPEDSEL.CPP` (from `MAPEDIT.CPP:2172`), `WOLAPI/WOLAPI_I.C` (from `RAWOLAPI.CPP:27`) |

The DOS build uses `KEYFBUFF.ASM`, `TXTPRNT.ASM` and `KEYFRAME.CPP` instead of the Win32 `2KEYFBUF.ASM`, `2TXTPRNT.ASM`
and `2KEYFRAM.CPP` (`MAKEFILE:421-424`).

Some files are in the build but compile to (almost) nothing because of switches: `MAPEDIT.CPP`, `MAPEDDLG.CPP`,
`MAPEDPLC.CPP`, `MAPEDTM.CPP`, `MAPEDSEL.CPP` and `DEBUG.CPP` (editor/cheats off), `MCI.CPP` and `MCIMOVIE.CPP` (`MCIMPEG` off),
`MPU.CPP` (Borland-only body), `ENDING.CPP` (`#ifdef NEVER`), and `CREW.CPP` (header only).

## Subsystem map

Every Win32-build file is assigned to exactly one subsystem. **Lines** are physical lines. **Live** lines are non-blank,
non-comment lines that the Win32 build actually compiles (see [method](platform-dependencies.md#method-re-runnable)).

| Subsystem | Files | Lines | Live | What it is | Files |
|-----------|------:|------:|-----:|------------|-------|
| **Simulation** | 52 | 76,458 | 32,028 | Game objects and their AI: the class hierarchy `AbstractClass` → `ObjectClass` → `MissionClass` → `RadioClass` → `TechnoClass` → `FootClass` → unit types; houses (players), map cells, pathfinding, combat, triggers and teams, scenario loading, rules | `ABSTRACT AIRCRAFT ANIM BASE BUILDING BULLET CARGO CELL COMBAT COORD CRATE CREW DOOR DRIVE FACE FACING FACTORY FINDPATH FLASHER FLY FOOT FUSE HOUSE INFANTRY LAYER LOGIC MAP MISSION OBJECT OVERLAY RADIO REINF RULES SCENARIO SMUDGE SUPER TACTION TARGET TEAM TEAMTYPE TECHNO TEMPLATE TERRAIN TEVENT TRACKER TRIGGER TRIGTYPE UNIT VESSEL VORTEX WARHEAD WEAPON` |
| **Sim: type data** | 13 | 17,740 | 12,810 | Static tables describing every unit/building/weapon type (`*DATA.CPP`) and constants | `AADATA ADATA BBDATA BDATA CDATA CONST HDATA IDATA ODATA SDATA TDATA UDATA VDATA` |
| **Network core** | 18 | 17,582 | 6,125 | Lockstep queue (`QUEUE`), events (`EVENT`), session (`SESSION`), connection managers (`CONNECT`, `COMBUF`, `IPX*`), Winsock transports (`WSP*`, `UDPADDR`), scenario download (`SENDFILE`) | `QUEUE EVENT SESSION CONNECT COMBUF IPX IPX95 IPXPROT.ASM IPXMGR IPXCONN IPXGCONN IPXADDR UDPADDR WSPROTO WSPUDP WSPIPX _WSPROTO SENDFILE` |
| **Multiplayer UI** | 3 | 10,475 | 4,000 | Game-type menu, LAN lobby dialogs, in-game chat messages | `MPLAYER NETDLG MSGLIST` |
| **Modem / serial** | 3 | 10,735 | 5,194 | Null-modem and modem play. **`NULLDLG.CPP` also contains the skirmish setup dialog** | `NULLCONN NULLMGR NULLDLG` |
| **Internet (pre-WOL)** | 8 | 4,587 | 1,458 | Older WChat/DDE-based internet play, and game statistics reporting | `TCPIP INTERNET DDE CCDDE STATS PACKET FIELD UTRACKER` |
| **WOL** | 20 | 17,325 | 10,383 | Westwood Online client (COM `WOLAPI`), its screens and helpers | `WOL_* RAWOLAPI WOLAPIOB WOLEDIT PASSEDIT SEDITDLG ICONLIST TOOLTIP DIBFILE DIBUTIL COMINIT BIGCHECK WOLSTRNG WOLAPI_I.C` |
| **UI** | 37 | 21,551 | 8,670 | Gadget/button system, dialogs, main menu, options, sidebar, radar, power bar, map scrolling, mouse | `BAR CHECKBOX CHEKLIST COLRLIST CONTROL CREDITS DIAL8 DIALOG DROP EDIT EGOS GADGET GAMEDLG GAUGE GOPTIONS HELP LINK LIST LOADDLG MENUS MOUSE MSGBOX OPTIONS POWER RADAR SCROLL SHAPEBTN SIDEBAR SLIDER SOUNDDLG SPECIAL STATBTN TAB TEXTBTN TOGGLE TXTLABEL VISUDLG` |
| **Rendering** | 12 | 12,838 | 3,275 | Tactical map drawing (`DISPLAY`), page flipping, shape/sprite blitting, palettes, 2x interpolation, PCX screenshots | `DISPLAY GSCREEN ROTBMP SPRITE RGB HSV PALETTE(missing) INTERPAL WRITEPCX 2KEYFRAM 2KEYFBUF.ASM 2TXTPRNT.ASM` |
| **Audio** | 2 | 1,382 | 641 | Sound effects, speech, music (on top of WIN32LIB audio) | `AUDIO THEME` |
| **File I/O** | 13 | 11,508 | 3,742 | Files, MIX archives, CD search, INI parsing, save/load | `CCFILE CCINI INI PROFILE READLINE RAMFILE MIXFILE CDFILE BFIOFILE RAWFILE IOMAP IOOBJ SAVELOAD` |
| **Compression / crypto** | 33 | 10,236 | 3,478 | "Pipe/Straw" stream framework, LCW/LZO/LZW compression, Blowfish, SHA, public-key (`PK`, `MP`) for MIX headers | `PIPE STRAW XPIPE XSTRAW LCW* LZO* LZW* PK* MP INT BLOW* BLOWFISH SHA* CRC* B64* BASE64 RNDSTRAW` |
| **Core / boot** | 16 | 15,388 | 5,282 | Main loop, init, globals, heaps, containers, fixed-point, RNG, version, mono debug screen | `CONQUER INIT GLOBALS HEAP JSHELL DYNAVEC VECTOR CCPTR FIXED RANDOM VERSION BUFF RECT BENCH MONOC MPU` |
| **Win32 platform** | 7 | 4,102 | 960 | `WinMain`, window procedure, keyboard/mouse, CPU detection, asm stubs | `STARTUP WINSTUB KEY GETCPU CPUID.ASM WINASM.ASM W95TRACE` |
| **Campaign-only** | 6 | 3,525 | 1,618 | Mission carry-over, expansion mission picker, campaign map selection, ending, intro/side selection, score screen (`SCORE.CPP` also has the multiplayer score screen) | `CARRY EXPAND MAPSEL ENDING INTRO SCORE` |
| **Movies** | 3 | 1,329 | 255 | MPEG (DVD) movie settings; MCI player (compiled out). VQA playback is in `CONQUER.CPP` (`Play_Movie`) | `MPGSET MCI MCIMOVIE` |
| **Editor / debug** | 6 | 8,804 | 0 | Scenario editor and cheat/debug keys. **All compiled out in the release build** | `MAPEDIT MAPEDDLG MAPEDPLC MAPEDTM MAPEDSEL DEBUG` |

Things a newcomer should know about the architecture:

- **The global `Map` is the whole screen.** `MouseClass Map;` (`GLOBALS.CPP:441`) sits at the bottom of a single inheritance chain:
  `GScreenClass` → `MapClass` (cells: simulation state) → `DisplayClass` (tactical view) → `RadarClass` → `PowerClass` →
  `SidebarClass` → `TabClass` → `HelpClass` → `ScrollClass` → `MouseClass`. Simulation state and UI live in one object.
  Separating them (for example, for a headless server or sync tests) is a real refactor.
- **Everything is global.** `GLOBALS.CPP` defines the global objects (`Map`, `Scen`, `Session`, `Rule`, the object heaps
  `Units`, `Infantry`, …), and `EXTERNS.H` declares them. Game objects come from fixed-size heaps (`HEAP.CPP`), and
  network/save code refers to them by heap index (`TARGET`), not by pointer.
- **One master header.** Almost every `.CPP` includes only `FUNCTION.H`, which pulls in `<windows.h>`, WIN32LIB (`<wwlib32.h>`),
  the VQA player, the Greenleaf headers, and all game headers. Header cleanup (S06a) unblocks everything else.
- **The "IPX" name is historical.** `IPXManagerClass Ipx` is the network manager for every packet-based game type.
  Since `WINSOCK_IPX`, it sends through `PacketTransport` (a `WinsockInterfaceClass`: `IPXInterfaceClass` or `UDPInterfaceClass`).

## Boot sequence (Win32 build)

Step by step, with what each step depends on. File:line references are to the original source.

1. **Static constructors run before `WinMain`.** This includes `DDEServerClass DDEServer` (`CCDDE.CPP:62`), whose constructor
   sets `RA95AlreadyRunning` when another copy is already running. All the global objects in `GLOBALS.CPP` are also constructed here.
2. **`WinMain`** (`STARTUP.CPP:109`):
   1. If another instance is running, bring it to the front and exit (`STARTUP.CPP:127-136`; Win32 `FindWindow`).
   2. Require 7 MB free RAM (`Ram_Free`, WIN32LIB).
   3. Split the Win32 command line into `argv` by hand, `chdir` to the exe's folder (`_dos_getdrive`/`_dos_setdrive`: Watcom runtime).
   4. WOL install housekeeping: delete `wolsetup.exe`, read/clear registry values (`STARTUP.CPP:272-306`).
   5. **`Parse_Command_Line`** (`INIT.CPP:1741`) sets `Special`/`Session` options from the arguments.
   6. **Timer:** `WindowsTimer = new WinTimerClass(60, FALSE)` (`STARTUP.CPP:351`), a 60 Hz Windows multimedia timer
      (WIN32LIB). `Sleep(1000)` checks that it ticks. All game timers (`TickCount`, `FrameTimer`) read it through
      `SystemTimerClass` (`JSHELL.H:359-369`).
   7. **Keyboard:** `Keyboard = new KeyboardClass()` (`STARTUP.CPP:375`; `KEY.CPP`, Win32 messages).
   8. Choose compressed or uncompressed shapes based on RAM, then check free disk space.
   9. Read `REDALERT.INI` (`CONFIG_FILE_NAME`) and `Read_Setup_Options` (screen size, video options).
   10. **Window:** `Create_Main_Window` (`WINSTUB.CPP:435`) registers the class and creates the window with
       `Windows_Procedure` (`WINSTUB.CPP:166`).
   11. **Audio:** `Audio_Init(MainWindow, 16, false, 22050, 0)` (`STARTUP.CPP:422`; WIN32LIB DirectSound).
   12. **Video:** with `MPEGMOVIE` defined, `InitDDraw()` (`STARTUP.CPP:445`, body at `:703`) sets the DirectDraw mode
       (640x400, falling back to 640x480) and creates `VisiblePage`/`HiddenPage` (video memory when possible). `SeenBuff`/`HidPage` are
       640x400 viewports onto them (offset 40 lines down when the mode is 640x480).
   13. Mouse: `WWMouse = new WWMouseClass(&SeenBuff, 48, 48)` (`STARTUP.CPP:558`). CD drive detection.
   14. Read the intro flag from the INI, then **`Main_Game(argc, argv)`** (`STARTUP.CPP:629`).
   15. On return: clear the pages, post `WM_DESTROY`, and pump messages until the window procedure has cleaned up (`ReadyToQuit`).
3. **`Main_Game`** (`CONQUER.CPP:215`):
   1. **`Init_Game`** (`INIT.CPP:197`):
      `Init_Keys` → `Bootstrap` (`INIT.CPP:3112`: waits for focus, opens the bootstrap MIX files, fonts, loads `CONQUER.ENG`
      strings and the palette, `Init_Expansion_Files`) → `Init_Mouse` → `Init_CDROM_Access` → `Init_Secondary_Mixfiles` →
      sets up the type heaps (`*Types.Set_Heap`, `*TypeClass::Init_Heap`) → loads **`RULES.INI`** (and `AFTRMATH.INI` if
      Aftermath is installed) into `Rule` → `Init_Heaps` (object heaps) → `Anim_Init` (VQA player setup; this is where the
      DirectSound objects are handed over) → creates `MpgSettings` if DVD → **`Play_Intro`** → color remaps → title page →
      `Init_Bulk_Data` → `Options.Load_Settings`.
   2. **Outer loop:** `while (Select_Game(fade))`.
      - **`Select_Game`** (`INIT.CPP:420`) runs the main menu, `Main_Menu` (`MENUS.CPP:463`), and handles the selection.
        For campaign it picks a side or expansion mission (`Expansion_Dialog`). For skirmish it runs `Com_Scenario_Dialog(true)`
        (`NULLDLG.CPP:2904`). For LAN it creates `IPXInterfaceClass` (`INIT.CPP:1255`) and calls `Init_Network`/`Remote_Connect`
        (`NETDLG.CPP:952`, `:1246`; lobby dialogs). For modem it runs `Select_Serial_Dialog`. For internet it creates
        `UDPInterfaceClass` (`INIT.CPP:1202`) and runs `WOL_Main()`. Then it calls **`Init_Random()`** (`INIT.CPP:2398`;
        see Determinism), **`Start_Scenario`** (`SCENARIO.CPP:308` → `Read_Scenario`, `:416`), sets up the message list, and
        `Session.Create_Connections()` for multiplayer.
      - **Inner loop:** `for (;;) { if (Main_Loop()) break; …special dialogs (options, surrender, draw)… }`.
      - After a game: statistics packet (internet), modem sign-off or `Shutdown_Network`, back to `Select_Game`.
4. **`Main_Loop`** (`CONQUER.CPP:2139`), once per game frame:
   1. `Check_For_Focus_Loss()` (`WINSTUB.CPP:117`) pumps Windows messages and pauses while the window is unfocused.
   2. `Reallocate_Big_Shape_Buffer()`.
   3. Pick the frame delay. Multiplayer uses `TIMER_SECOND / Session.DesiredFrameRate`. Single player uses `Options.GameSpeed` adjusted by difficulty.
   4. **Input and render:** `Map.Input()` → `Keyboard_Process()` → `Map.Render()`.
   5. Recording/playback (`Do_Record_Playback`).
   6. Sort the ground layer, then **`Logic.AI()`** (`LOGIC.CPP`): the simulation step for every object.
   7. `Session.Messages.Manage()`, then **`Queue_AI()`** (`QUEUE.CPP:355`): the **lockstep step**. It sends and receives events,
      waits for other players' frames, executes events scheduled for this frame, and checks the CRC (`Compute_Game_CRC`, `QUEUE.CPP:3793`).
   8. `Call_Back()` (`CONQUER.CPP:1616`): audio and music AI, `IPX_Call_Back()` → `Ipx.Service()` (network I/O), modem service,
      and WOL message pumping.
   9. Win, lose, restart and draw checks. Then `Frame++`.
   10. **`Sync_Delay()`** (`CONQUER.CPP:2086`): busy-loops until `FrameTimer` expires, calling `Call_Back`, `Map.Input` and `Map.Render`
       on the way, so the UI stays responsive while frames are throttled.
5. **Where Windows messages are pumped.** `WWKeyboardClass::Fill_Buffer_From_System` (`KEY.CPP:521`) runs
   `PeekMessage`/`GetMessage`/`DispatchMessage` whenever the game polls the keyboard (`Keyboard->Check()`), and
   `Check_For_Focus_Loss` does the same. `Windows_Procedure` forwards socket notifications to `PacketTransport->Message_Handler`
   (`WINSTUB.CPP:184-186`; `WSAAsyncSelect`) and keyboard/mouse to `Keyboard->Message_Handler`. **Network packets are only
   received while messages are being pumped**, which S08 (event pump) and S12 (polled sockets) must keep in mind together.
6. **Shutdown:** `WM_DESTROY` in `Windows_Procedure` → `Prog_End` (`STARTUP.CPP:843`): sound off, timers deleted.

**In the port** (S10b/S10c): `WinMain` is a portable `main` ([platform-layer.md, "How `main` works"](platform-layer.md#how-main-works)).
Before the window opens, `Boot_Setup_Data` creates `REDALERT.INI` if needed, sets the data search folders and checks
for `REDALERT.MIX`/`MAIN.MIX` ([ADR-0023](../decisions/0023-data-folder-and-first-run.md)). The intro movie is skipped
(`Play_Movie` returns at once), the CD checks always pass, and `[Intro] PlayIntro` is ignored, so every start goes
`Init_Game` → main menu. Missing or damaged data found during `Init_*` is reported by `Boot_Data_Error` instead of an
assert. The data files `Init_Game` loads, in order, are listed in [playing.md](../playing.md#the-order-the-game-loads-them).
Without data the port gets as far as `Init_Bootstrap_Mixfiles` (window, timer, audio, keyboard and mouse are up).

## Feature switches

Values are for the shipped Win32 build. "Uses" counts `#if`/`#ifdef`/`#ifndef`/`#elif` lines in `CODE/*.CPP|*.H` (lines / files).

### Platform and build switches (from the makefile)

| Switch | Value | Uses | What it controls |
|--------|-------|-----:|------------------|
| `WIN32` | defined (`/DWIN32=1`) | 513 / 95 | Windows vs DOS: video, timer, keyboard, file paths, networking transport, resolution (`RESFACTOR`), screen sizes. Every DOS branch is dead in our build |
| `_WIN32` | defined | 0 / 0 | Only for SDK headers |
| `WOLAPI_INTEGRATION` | defined | 113 / 43 | Westwood Online client in menus, chat, game setup and statistics. **Also disables** the older WChat/DDE internet path (`#ifndef`) |
| `WINSOCK_IPX` | defined | 40 / 12 | Network packets go through Winsock (`PacketTransport`) instead of the THIPX32 real-IPX DLL |
| `ENGLISH` / `GERMAN` / `FRENCH` | `ENGLISH=1` | 25 / 62 / 68 uses | Language strings, a few UI layouts, intro/credits differences. `FOREIGN_VERSION` if not English (`DEFINES.H:214-215`) |
| `DOS4G`, `GF_WATCOM_S` | DOS build only | 0 | Only for Greenleaf/DOS4GW headers; never tested in `CODE/` |

### Game switches (from `CODE/DEFINES.H`)

| Switch | Value | Uses | What it controls |
|--------|-------|-----:|------------------|
| `RELEASE_VERSION` / `INTERNAL_VERSION` / `PLAYTEST_VERSION` | release | 1 / 2 / 2 | Selects which of the next two are on (`DEFINES.H:56-58`) |
| `SCENARIO_EDITOR` | **off** | 52 / 30 | Map editor (`MAPED*.CPP`), editor hooks in objects, triggers and teams |
| `CHEAT_KEYS`, `VIRGIN_CHEAT_KEYS` | **off** | 86 / 56, 3 / 3 | Debug keys, debug command-line flags, benchmarks, `DEBUG.CPP` |
| `FIXIT_CSII`, `FIXIT_ANTS`, `FIXIT_CARRIER`, `FIXIT_PHASETRANSPORT`, `FIXIT_ENGINEER` | on | 214 / 46 … | Counterstrike/Aftermath units and rules. **Changes gameplay; must stay on** for compatibility |
| `FIXIT_VERSION_3` | on | 153 / 32 | "Version 3" reunification: one exe for RA/CS/AM; Aftermath enabled per game (`bAftermathMultiplayer`), draw proposals |
| other `FIXIT_*` (`DEFINES.H:75-125`) | on | small | Bug fixes (score crash, multiplayer save, random seed, …) |
| `FIXIT_ENGINEER_CAPTURE`, `FIXIT_HELI_LANDING`, `FIXIT_MINE_PASSABLE` | **off** | 1 each | Fixes that "will cause the game to go out of sync" (`DEFINES.H:112-115`). Turning them on changes simulation |
| `FIXIT_FAST_LOAD` | off | 3 | Faster INI loading |
| `DVD`, `MPEGMOVIE` | on | 17 / 6, 11 / 6 | DVD edition: MPEG movies through `mpgdll` (DirectDraw), and the `InitDDraw` startup path |
| `MCIMPEG` | off | 22 / 11 | MCI-based MPEG player (`MCI.CPP`, `MCIMOVIE.CPP` compile empty) |
| `MOVIE640` | on | 7 / 2 | 640x400 VQA movies |
| `TEN`, `MPATH` | `0` | 46 / 16, 39 / 15 | TEN and MPath online services. All code is `#if(TEN)` / `#if(MPATH)`, so it is dead |
| `INTERNET_OFF` | off | 13 / 3 | Would hide internet play |
| `TIMING_FIX` | `1` | 5 / 5 | Network timing fix in the queue code |
| `PARTIAL`, `SORTDRAW` | on, off | 8, 10 | Rendering optimisations (partial object redraw, draw sorting) |
| `MIRROR_QUEUE` | off | 14 / 4 | Debug copy of the event queue |
| `DONGLE`, `BOGUSCD` | off | 3, 3 | Copy protection |
| `STEVES_LOAD_OVERRIDE` | on | 1 | Loads `CONQUER.ENG` from disk if present (`INIT.CPP` `Bootstrap`) |
| `RESFACTOR` | `2` under `WIN32` | (macro) | Coordinate scale between 320x200 artwork logic and 640x400 screen (`DEFINES.H:224-229`) |
| `NEVER`, `OBSOLETE`, `__BORLANDC__`, `TIBERIAN_SUN` | undefined | 48, 44, 31, 1 | Dead code |

Local switches defined inside single files: `VIC` (`ANIM.CPP:58`), `FATSHIP` (`BDATA.CPP:66`), `DIAGONAL` and `TEST`
(`FINDPATH.CPP:67,77`), `SHOW_MONO 0` (`NETDLG`, `NULLDLG`, `QUEUE`), `CONN_DEBUG 0` (`CONNECT.H:110`).

## Determinism: code that matters for lockstep

Red Alert multiplayer is lockstep: every machine runs the same simulation from the same inputs. Anything below that differs
between machines or builds causes a desync. **Rule for porting sessions:** if you change any of this, say so in your session log and
test it with the S14 sync harness.

### Random numbers

| Generator | Where | Used for | Notes |
|-----------|-------|----------|-------|
| `Scen.RandomNumber` (`RandomClass`, `SCENARIO.H:65`) | `RANDOM.CPP`: LCG `Seed = Seed*0x41C64E6D + 0x3039`, returns bits 10..24 | **All simulation randomness**: `Random_Pick`, `Percent_Chance` (`INLINE.H:955-962`). About 230 lines in 31 files (`HOUSE`, `BUILDING`, `CELL`, `INFANTRY`, `QUEUE`, `VORTEX`, …) | Seeded from `Seed` in `Init_Random` (`INIT.CPP:2398`). The host picks `Seed` with `srand(time)/rand()` (`NETDLG.CPP:4332`, `NULLDLG.CPP:3348`, `WOL_GSUP.CPP:626`) and sends it to the clients |
| `NonCriticalRandomNumber` (`GLOBALS.CPP:285`) | same class | `Sim_Random_Pick`: unit voice responses, screams, tesla-bolt drawing, cell flashing (`AIRCRAFT`, `DRIVE`, `INFANTRY`, `TECHNO`, `CELL`) | Cosmetic only (checked by hand). Must never be used for simulation decisions |
| `CryptRandom` (`RandomStraw`) | `RNDSTRAW.CPP` | Crypto keys (MIX headers, `INIT.CPP:2408-2425` seeds it from the clock) | Not simulation |
| C `rand()` | | Only seed picking and WOL UI flavour (`RAWOLAPI`, `WOLAPIOB`) | Not simulation |

**Porting hazard:** `RandomClass::Seed` is `unsigned long` (`RANDOM.H:67`). The *output* only depends on the low 32 bits,
so the numbers are the same on 64-bit. But the type is 8 bytes on LP64 (Linux/macOS 64-bit), which changes the layout of
`ScenarioClass` (save games), so it should become `uint32_t`.

### Math

- **No floating point in the simulation.** A search for `float`, `double`, `sqrt`, `sin`, `cos` and `atan` in all build `.CPP` files
  found only comments ("double click", "double precision", …) outside the WOL files. The only float in a header is
  `WOLAPIOB.H:411` (WOL UI).
- **Fixed point:** `fixed` (`FIXED.H`/`FIXED.CPP`), an unsigned 8.8 value used throughout rules and simulation. INI values
  are parsed with integer `atoi` (`FIXED.CPP:126-137`), not `atof`.
- **Lookup tables instead of trig:** `Move_Point` (`COORD.CPP:430`) uses a 256-entry cosine table plus the **Watcom inline
  asm helpers `calcx`/`calcy`** (`COORD.CPP:405-427`: 16x16 multiply with a specific shift/rounding). These must be
  re-implemented bit-exactly.
- **`Fixed_To_Cardinal` / `Cardinal_To_Fixed`** are `#pragma aux` asm in `JSHELL.H:293-318`. `Fixed_To_Cardinal` is used by the
  simulation (`FLY`, `INFANTRY`, `TECHNO`, `UNIT`, `VESSEL`). Rounding and overflow behaviour must be copied exactly.
- `Set_Bit` / `Get_Bit` (asm in `JSHELL.H:211-232`) back `BooleanVectorClass` (`VECTOR.CPP`).
- Signed right shifts and signed/unsigned `char`: the original compiles with `/j` (signed `char`, `MAKEFILE:115`). On ARM,
  `char` is unsigned by default, so build with `-fsigned-char` or the equivalent. Not verified file by file.

### Sync check and wire format

- `Compute_Game_CRC` (`QUEUE.CPP:3793`) folds the coordinates, facing, speed, mission and target of every infantry, unit, vessel,
  building and house into `GameCRC` with `Add_CRC` (`QUEUE.CPP:3895`, `unsigned long`). The result is exchanged in events and compared.
  **Iteration order of the object heaps is part of the CRC**, so heap allocation order must stay identical.
- `EventClass` (`EVENT.H`) is sent over the network as raw bytes. It relies on `/zp1` byte packing (`MAKEFILE:113`), on Watcom
  bit-field layout (`Frame:26`, `ID:5`, `IsExecuted:1`, `EVENT.H:113-123`) and on `unsigned long` fields (`EVENT.H:194`, `:207`).
  A modern build must pin this layout with `#pragma pack(1)`, fixed-width types and `static_assert`s (S06; S04 owns the protocol).
- `bool` is `typedef int bool` in the original (`WATCOM.H:82-87`, included first by `FUNCTION.H`). Any struct containing
  `bool` that is saved or sent gets **smaller** with a modern compiler's 1-byte `bool`. This matters for save games and for
  any network struct with `bool` members (S04/S06 to audit).

### Other things to audit in S14

- Code that branches on the **local player** (`PlayerPtr`, `IsOwnedByPlayer`) and then touches `Scen.RandomNumber`
  or simulation state. That runs differently on each machine. Not audited in S01.
- `Layer[LAYER_GROUND].Sort()` runs every frame before `Logic.AI()` (`CONQUER.CPP` `Main_Loop`). The sort must stay
  deterministic and stable, with no pointer comparisons. Not verified.
- Timers: game logic counts frames (`Frame`), and real time is only used for pacing (`FrameTimer`, `Sync_Delay`).
  Replacing the Windows timer (S08) should not affect simulation, as long as `TickCount`-based code in the
  simulation (`Session.ProcessTimer` statistics) stays non-simulation. Unverified beyond `Main_Loop`.

## Implications for later sessions

- **S05 (strip dead platforms)**
  - The LAN game in the shipped build uses **Winsock IPX** (`IPXInterfaceClass`, `WSPIPX.CPP`, `INIT.CPP:1255`). There is a
    commented-out line that used UDP instead (`INIT.CPP:1258`). Removing `WSPIPX` means **switching the LAN game to
    `UDPInterfaceClass`** at that point, not just deleting files. Keep `IPXManagerClass`, `IPXConnClass`, `IPXGlobalConnClass` and
    `IPXAddressClass`: they are the transport-independent network layer.
  - **Do not delete `NULLDLG.CPP`.** It contains `Com_Scenario_Dialog`, the skirmish setup dialog. Move it out (for
    example to a new `SKIRMDLG.CPP`) before removing modem code.
  - Greenleaf is only *headers* in Win32: remove `#include <modem.h>`/`<fast.h>` from `FUNCTION.H:296-297` and `commlib.h`
    from `NULLMGR.H`/`NULLCONN.H`. The empty Greenleaf stubs in `WINASM.ASM:253-298` can go with them.
  - `IPX.CPP`, `IPX95.CPP` and `IPXPROT.ASM` are linked but unreachable; they can be removed together with `WSPIPX`.
  - The pre-WOL internet code (`TCPIP`, `INTERNET`, `DDE`, `CCDDE`) is almost entirely dead. **But** `CCDDE.CPP`'s global
    `DDEServer` provides the single-instance check (`RA95AlreadyRunning`), and `STATS`/`PACKET`/`FIELD`/`UTRACKER` are
    used by WOL games only.
  - 41 files in `CODE/` are not in the build at all (list in platform-dependencies.md).
- **S06 (compiler compatibility)**
  - First restore the **missing sources**: `PALETTE.CPP` and `palette.h` (`PaletteClass`), `phone.h`, `pcx.h` (and `PASSEDIT.*`,
    unless WOL is gone first). Without them nothing compiles. The PaletteClass interface can be derived from its users
    (`CONQUER`, `JSHELL`, `OPTIONS`, `RGB`, `VORTEX`, …).
  - Port the `#pragma aux` helpers in `JSHELL.H`, `COORD.CPP`, `RGB.H`, `MPU.H` and `MONOC.CPP` to C++, bit-exactly.
  - Provide shims for `<dos.h>`, `<mem.h>`, `<i86.h>`, `<conio.h>`, `<direct.h>`, `<io.h>`, `<share.h>` and the `_dos_*` calls
    (12 files; see the table).
  - Replace `typedef int bool` (`WATCOM.H`) carefully: check every saved or sent struct.
- **S07 (asm → C++):** the six `.ASM` files in `CODE/` are `2KEYFBUF` (shape drawing incl. MMX, 4,848 lines), `WINASM`
  (palette interpolation, ModeX blit, Greenleaf stubs), `2TXTPRNT` (font blitter), `LCWCOMP` (LCW compressor), `CPUID` (MMX
  detection) and `IPXPROT` (dead). S02 has the details.
- **S08 (platform layer):** the Win32 surface in the game code is small and concentrated in `STARTUP.CPP` (`WinMain`, `InitDDraw`),
  `WINSTUB.CPP` (window procedure, focus handling), `KEY.CPP` (keyboard *and* the message pump) and `CONQUER.CPP`
  (`PlayMpegMovie`). 34 dialog files poll `AllSurfaces.SurfacesRestored` (DirectDraw surface loss). Keep that flag
  (always false) or remove it everywhere. The SDL event pump replaces `Fill_Buffer_From_System` and `Check_For_Focus_Loss`.
- **S12 (portable sockets):** sockets are serviced through window messages (`WINSTUB.CPP:184-186`) and through
  `Ipx.Service()` from `Call_Back`. Polling inside `Call_Back` is the natural replacement.

## Open questions

- Is the release really missing `PALETTE.CPP`, `PASSEDIT.CPP`, `phone.h` and `pcx.h`, or are they somewhere else in EA's
  repositories? (Other open-source ports of this code had to recreate them. That is unverified here.)
- Should the port keep **wire compatibility** with original Red Alert 3.03 clients? If not (likely), the event format can
  be redesigned freely in S12/S13. That needs a decision (ADR) before S06 freezes struct layouts.
- *(Answered: yes, when all players have the data; see [ADR-0011](../decisions/0011-expansions-in-multiplayer.md).)* Is Counterstrike/Aftermath content (`FIXIT_CSII`, …) in scope for multiplayer? It is compiled in and switched at runtime
  (`bAftermathMultiplayer`), but needs the expansion MIX files.
