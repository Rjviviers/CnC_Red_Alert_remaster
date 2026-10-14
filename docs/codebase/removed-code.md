# Removed code

Everything deleted from the original source tree, why, and where to find it again.
Started in session S05 ([brief](../sessions/S05-strip-dead-platforms.md),
[log](../logs/2026-10-14-S05-strip-dead-platforms.md)). Scope: [ADR-0005](../decisions/0005-multiplayer-first-scope.md).

**How to get something back.** Each section names the *last commit that still has the code*. To look at
a file: `git show <commit>:<path>`. To restore it: `git checkout <commit> -- <path>`. To find any removed
file by name: `git log --diff-filter=D --name-only -- '<path>'`.

The original makefiles (`CODE/MAKEFILE`, `WIN32LIB/*/MAKEFILE`, `WINVQ/*/MAKEFILE`) were **not** edited:
they still name some deleted files and are kept as a record of the original build
([ADR-0012](../decisions/0012-hand-maintained-source-lists.md)).

## Summary

S05 deleted **1,209 files** (2,143 → 937 files in the repository, counting the few it added) and about **417,000 lines** (net about 415,500;
counts from `git diff --shortstat 7a69f38 807268f`; binary files count as files, not lines). Every source-level
change is listed below; the commits are:

| Commit | What | Section |
|--------|------|---------|
| `2557f1d` | Hand-maintained CMake source lists, ADR-0012 | (build) |
| `e87f703` | DOS trees, prebuilt tools, debug copies, unused asm | [1](#1-dos-trees-tools-debug-copies-and-unused-assembly) |
| `98d4fee` | `unifdef` collapse of `WIN32`/DOS/`WINSOCK_IPX` branches | [2](#2-dos-branches-inside-the-remaining-sources-preprocessor-collapse) |
| `59ea2e1` | Native IPX removed; LAN switched to `UDPInterfaceClass` | [3](#3-native-ipx) |
| `064a13a` | Skirmish dialog moved to `SKIRMDLG.CPP` (pure move) | [4](#4-modem--null-modem-serial-play-and-greenleaf) |
| `8b5bb75` | Modem/null-modem play and Greenleaf removed | [4](#4-modem--null-modem-serial-play-and-greenleaf) |
| `ca5523e` | Westwood Online, WChat/DDE, statistics, TEN, MPath removed | [5](#5-westwood-online-the-wchatdde-internet-path-ten-and-mpath) |
| `807268f` | Remaining DOS-only sources in `CODE/` | [6](#6-remaining-dos-only-sources-in-code) |

**ADR-0005 status:** Accepted, and implemented by S05 for the DOS build, native IPX, modem/null-modem play,
Westwood Online and TEN/MPath. Not done yet: stubbing the cutscene movies (the `WINVQ/` player and
`CODE/INTERPAL.CPP` are still built; S08/S09 decide how to stub them).

## 1. DOS trees, tools, debug copies and unused assembly

**Last commit with this code: `2557f1d`.** Evidence: [libraries.md](libraries.md) §4 and §6,
[assembly-inventory.md](assembly-inventory.md) §2.

| Removed | What it was | Why it could go |
|---------|-------------|-----------------|
| `WWFLAT32/` (400 files) | DOS version of the support library (VESA/MCGA, real-mode keyboard and timer, HMI SOS audio), incl. prebuilt `SOSDW1PS.LIB` copies | Only the DOS makefile branch used it (`CODE/MAKEFILE:62`, `:433`) |
| `VQ/` (183 files) | DOS VQA movie player | Only the DOS include/link lines (`CODE/MAKEFILE:104`, `:537`) |
| `LAUNCH/` | `LAUNCH.ASM`, DOS/4GW real-mode launcher stub | Not referenced by any makefile |
| `LAUNCHER/` (47 files) | Separate Windows patcher/launcher for the WOL auto-update flow (RTPatch) | Separate program; the port will have its own update story; RTPatch is proprietary |
| `TOOLS/` except `TOOLS/MIX/MIXFILE.CPP` and `TOOLS/AUDIOMAK/AUDIOMAK.CPP` | Prebuilt DOS/Win tool executables (`MIXFILE.EXE`, `KEYFRAME.EXE`, …), Borland project files, `SCODE.ASM` | Binaries, not built from this tree. The two `.CPP` files stay as file-format references |
| `WIN32LIB/SRCDEBUG/` | 93 asm + C++ copies of library sources for debugging (60 byte-identical copies, 33 old DOS files) | Never built |
| `WIN32LIB/EXAMPLE/` | Sample program for the library | Never built into the game |
| `WIN32LIB/MOVIE/` | DirectShow MPEG player (`mpeg.lib`) | Not linked (the game linked a prebuilt `mpgdll.lib` that is not in the repo); movies are stubbed (ADR-0005) |
| `WIN32LIB/AUDIO/OLD/`, `MEM/MSVC/`, `MEM/OLDMEM/`, `DRAWBUFF/TEST/`, `WW_WIN/OLD/`, `KEYBOARD/OLD/`, `KEYBOARD/TEST/`, `KEYBOARD/OLDTEST/` | Old versions and test programs | Not in any library makefile |
| `WINVQ/VQA32/OLD/`, `WINVQ/VPLAY32/`, `WINVQ/VQAVIEW/` (incl. prebuilt `VQAVIEW.EXE`) | Old player code, stand-alone player, Windows viewer tool | Not part of `vqa32wp.lib`/`vqm32wp.lib` |
| `WINVQ/LIB/` (`SOSDBLC.LIB`, `SOSDW1CR.LIB`, `SOSDW1CS.LIB`, `SOSDW1PR.LIB`, `SOSDW1PS.LIB`, `SOSMBLC.LIB`, `README.TXT`) | Prebuilt proprietary HMI SOS audio libraries | The Win32 link never referenced them; only the ADPCM codec (asm source) was used |
| `CODE/KEYFBUFF.ASM`, `CODE/TXTPRNT.ASM` | DOS versions of the shape and text blitters | DOS-only objects (`CODE/MAKEFILE:422-424`); Win32 uses `2KEYFBUF.ASM`/`2TXTPRNT.ASM` |
| `CODE/2SUPPORT.ASM`, `CODE/SUPPORT.ASM`, `CODE/COORDA.ASM` | Fading table, `strtrim`, fixed/cardinal helpers | In no object list; C++ or inline versions exist (`JSHELL.CPP:467`, `READLINE.CPP:31`, `JSHELL.H:292-315`) |
| `WIN32LIB/DRAWBUFF/FTPUTPIX.ASM`, `SHADOW.ASM`, `PLAYCD/PLAYCD.ASM`, `SHAPE/DRAWSHP.ASM`, `SHAPE/DS_*.ASM` (13), `SHAPE/SETSHAPE.ASM` | Asm in library folders that no makefile builds | Not library members |
| `WINVQ/VQA32/UNVQXMDE.ASM`, `WINVQ/VQM32/PORTIO.ASM` | Mode X decoder, port I/O | Not library members |
| Library members never pulled or shadowed: `WIN32LIB/DRAWBUFF/` `FILLQUAD`, `GETCLIP`, `SZREGION`, `TXTPRNT`; `FONT/TEXTPRNT`; `IFF/` `LCWCOMP`, `PACK2PLN`; `MISC/` `CRC`, `DETPROC`, `FACING8`, `FACING16`, `FACINGFF`, `OPSYS`, `SHAKESCR`; `WINVQ/VQA32/UNVQVESA`; `WINVQ/VQM32/` `CRC`, `DRAWCHAR`, `FILLRECT`, `HUFFDCMP`, `LCWCOMP`, `LCWUNCMP`, `MONO`, `PALETTE`, `TEXTPRNT`, `VB`, `VESABUF`, `XMODE` (all `.ASM`, 27 files) | See rows 13-65 of the inventory | Assembly-inventory delete list, item 1. The C++ versions of `Desired_Facing*` and `Calculate_CRC` stay the simulation authority (ADR-0010) |

**Deliberately kept** from the inventory's delete list, for later sessions:
`WIN32LIB/PROFILE/APROFILE.ASM` (row 48: `CODE/WINSTUB.CPP` still calls `Stop_Profiler`, and
`WIN32LIB/TIMER/TIMERINI.CPP` and `WWLIB32.H` include `profile.h`; remove the whole `PROFILE` component in one
go, S08) and `WINVQ/VQM32/MCGABUF.ASM` (row 58: still referenced by compiled `WINVQ/VQA32/DRAWER.CPP`; goes when
the movie player is stubbed). `CODE/WINASM.ASM` is edited in the modem section below, not deleted.

## 2. DOS branches inside the remaining sources (preprocessor collapse)

**Last commit with this code: `e87f703`.** 132 files, 5,437 lines removed, nothing added.

Done with `unifdef` 2.12 (not by hand), on every `.CPP`/`.H`/`.C` file under `CODE/`, `WIN32LIB/`, `WINVQ/`
and `TOOLS/`, resolving the symbols the way the shipped Win32 build (`wmake -DWIN32`) did:

```sh
unifdef -m -DWIN32=1 -D_WIN32 -DWINSOCK_IPX -UDOS4G -UGF_WATCOM_S -UPHARLAP -UPHARLAP_TNT -UINTERNET_OFF FILE
```

| Symbol | Resolved as | Why |
|--------|-------------|-----|
| `WIN32`, `_WIN32` | defined | Makefile `/DWIN32=1 /D_WIN32`; every `#ifndef WIN32` / `#else // WIN32` branch was DOS-only |
| `WINSOCK_IPX` | defined | Makefile; removes the THIPX32/real-mode IPX branches of the connection layer |
| `DOS4G`, `GF_WATCOM_S` | undefined | DOS/4GW and Greenleaf DOS settings |
| `PHARLAP`, `PHARLAP_TNT` | undefined | DOS extender; the WINVQ makefiles set `PHARLAP_TNT=0` (asm only) |
| `INTERNET_OFF` | undefined | Commented out in `CODE/DEFINES.H:134`; needed so `#if defined(WIN32) && !defined(INTERNET_OFF)` collapses |

What went: DOS keyboard/mouse/timer/video code, the DOS `KeyboardClass` variant (`JSHELL.H`), DOS file and
CD handling (`RAWFILE.CPP`, `CDFILE.CPP`), real-mode IPX in the connection layer (`IPXMGR.CPP`, `IPXCONN.CPP`,
`IPX.CPP`), DOS Greenleaf modem code in `NULLMGR.CPP`/`NULLDLG.CPP`, `RESFACTOR 1` (`DEFINES.H`), and header
guards such as `#ifndef WIN32 / #define WIN32 1` in `WIN32LIB` headers. `#if 0` / `#if (0)` blocks were left alone
(unifdef was run without `-k`). Two files could not be parsed and were left unchanged: `CODE/RAWOLAPI.H`
(an apostrophe inside `#error`; deleted later with WOL) and `WIN32LIB/INCLUDE/VIDEO.H` (unterminated comment;
it has none of these symbols).

## 3. Native IPX

**Last commit with this code: `98d4fee`.** Evidence: [networking.md](networking.md#what-s05-can-remove-and-what-must-stay),
[libraries.md](libraries.md) §4. Decision: ADR-0005, ADR-0007 (LAN over UDP).

| Removed | What it was | Why it could go |
|---------|-------------|-----------------|
| `IPX/` (52 files) | Sources of the Win95 IPX thunk DLLs `THIPX32.DLL`/`THIPX16.DLL` and the import library `wwipx32.lib` | Native IPX is dropped. The Win32 link line named `ipx\wwipx32.lib` (`CODE/MAKEFILE:561`); CMake never linked it, and the comment in `CMakeLists.txt` now says it is gone |
| `CODE/IPX95.CPP`, `CODE/IPX95.H` | Loads `THIPX32.DLL` with `LoadLibrary` and holds the function pointers into it | DLL loading code for native IPX; its only callers were in `#ifndef WINSOCK_IPX` branches (collapsed in section 2) |
| `CODE/IPX.CPP` | Real-mode IPX through DPMI interrupts | Linked but unreachable; no callers left |
| `CODE/IPXPROT.ASM`, `CODE/IPXREAL.ASM` | Real-mode IPX callback blob (`Get_RM_IPX_Address`/`Size`) | Only caller was in a DOS branch of `IPXMGR.CPP` |
| `CODE/WSPIPX.CPP`, `CODE/WSPIPX.H` | `IPXInterfaceClass`: IPX over Winsock (`AF_IPX`, `wsipx.h`, `wsnwlink.h`) | Native IPX transport. **Replaced at its only use**: `CODE/INIT.CPP` (`GAME_IPX`, was line 1255) now creates `UDPInterfaceClass`, in the same commit |
| `CODE/FAKESOCK.H` | Fake Winsock declarations for the DOS build | Only included from DOS branches |
| Prototypes in `CODE/IPX.H` for `IPX.CPP` functions; `Get_RM_IPX_*` prototypes in `CODE/IPXMGR.H`; `_IPX_Initialise` externs in `CODE/WINASM.ASM`; `Get_OS_Version()` in `CODE/STARTUP.CPP` (it only set `WindowsNT` for `IPX95.CPP`) | | Only used by the removed files |

**Kept on purpose** (the LAN game uses them, despite the "IPX" names): `IPXMGR`, `IPXCONN`, `IPXGCONN`,
`IPXADDR`, `CONNECT`, `COMQUEUE`, `COMBUF`, `CONNMGR.H`, the typedefs in `IPX.H` (`NetNumType`, `NetNodeType`,
`IPXHeaderType`, `ECBType`), `WSPROTO`, `WSPUDP`, `_WSPROTO`, `UDPADDR`, and `GAME_IPX` itself.
`IPXManagerClass` still has the now-empty `Alloc_RealMode_Mem`/`Free_RealMode_Mem` stubs and the
`RealModeDataType` members; they are harmless and left for S12 to clean up with the socket rewrite.

**Behaviour change:** the LAN menu entry now creates the UDP transport instead of IPX. Over UDP it does not
work yet (no real broadcast, fixed port, own-address filter; see [networking.md](networking.md)). S12/S13 fix that.

## 4. Modem / null-modem (serial) play and Greenleaf

**Last commit with this code: `064a13a`** (the complete `NULLDLG.CPP`, before the skirmish dialog was moved out,
is at `59ea2e1`). Evidence: [networking.md](networking.md#what-s05-can-remove-and-what-must-stay),
[platform-dependencies.md](platform-dependencies.md) (Greenleaf), [libraries.md](libraries.md) (`WINCOMM`). Decision: ADR-0005.

**Moved first, not deleted** (commit `064a13a`, a pure move): `Com_Scenario_Dialog` (the skirmish setup dialog) and
`Find_Local_Scenario` (used by the LAN lobby, `NETDLG.CPP`) went from `NULLDLG.CPP` to the new `CODE/SKIRMDLG.CPP`.
Then the modem paths inside `Com_Scenario_Dialog` were removed (everything that used `NullModem`, the serial
packets, `Session.ModemType`, `ModemRXString`, `TheirName/TheirColor/TheirHouse`, the `goto` retry label and the
`SHOW_MONO` debug code). The `bool skirmish` parameter and the remaining `!skirmish` UI branches were kept to keep
the diff small; callers always pass `true`. S11 can simplify the function.

| Removed | What it was | Why it could go |
|---------|-------------|-----------------|
| `CODE/NULLDLG.CPP` (rest) | Modem/null-modem dialogs: com settings, phone book, dial/answer, `Com_Show_Scenario_Dialog` (modem client), `Select_Serial_Dialog`, reconnect, plus debug helpers `Smart_Printf`, `Hex_Dump_Data`, `itoh`, `Log_*_Time` (only used by modem code) | Modem play dropped (ADR-0005) |
| `CODE/NULLMGR.CPP/.H` | `NullModemClass`, the serial connection manager (the only real Greenleaf user) | Modem play dropped |
| `CODE/NULLCONN.CPP/.H` | Serial connection (`commlib.h`) | Modem play dropped |
| `CODE/SEQCONN.CPP/.H`, `CODE/NOSEQCON.CPP/.H` | Old sequenced / non-sequenced connections | Not built; nothing else includes them. (`COMQUEUE.*`, also not built, is kept on purpose: the session brief lists it with the connection layer) |
| `WIN32LIB/WINCOMM/` (`WINCOMM.CPP`, `MODEMREG.CPP`, headers, makefile), `WIN32LIB/INCLUDE/WINCOMM.H`, `WIN32LIB/INCLUDE/MODEMREG.H` | `WinModemClass`/`WinNullModemClass` (Win32 serial API) and the modem registry reader | Only used by the modem code |
| `CODE/FUNCTION.H`: `#include <modem.h>`, `#include <fast.h>` (Greenleaf), `#include "phone.h"`, `#include "nullmgr.h"`, the `NULLDLG.CPP` prototypes | Greenleaf GCL headers (not in the repository) and modem declarations | Every file included them; the Greenleaf headers and `phone.h` are missing from the release, so this also removes two "missing header" causes for S06 |
| `CODE/WINASM.ASM`: the 15 empty Greenleaf stubs (`HMAnswer_`, `PortOpenGreenleafFast_`, …, with their `global`s and shared `ret`) and `_AbortModemFunctionPtr` | Link stubs | No callers left |
| `CODE/SESSION.H/.CPP`: modem constants (`PORTBUF_MAX` … `MODEM_NAME_MAX`, `PACKET_TIMING_TIMEOUT`), enums `DetectPortType`, `DialStatusType`, `DialMethodType`, `CallWaitStringType`, `ModemGameType`, struct `SerialSettingsType`, members `ModemService`, `CurPhoneIdx`, `SerialDefaults`, `ModemType`, `PhoneBook`, `InitStrings`, statics `SerialPacketNames`, `DialMethodCheck`, `CallWaitStrings`, and reading/writing the `[SerialDefaults]`, `[InitStrings]`, `[PhoneBook]` and `PhoneIndex` INI entries | Modem settings | Modem play dropped. Existing INI sections are now ignored, not deleted |
| `GameType` values `GAME_MODEM`, `GAME_NULL_MODEM` (`SESSION.H`) | Session types | Removed with every use. **The numeric values of the remaining `GameType` constants changed** (`GAME_IPX` 3 → 1, `GAME_INTERNET` 4 → 2, `GAME_SKIRMISH` 5 → 3, …). Checked: `Session.Type` is not saved in save games or recordings and is not sent over the network; the only ordered comparison (`DISPLAY.CPP`, inside `#ifndef FIXIT_VERSION_3`, dead) now starts at `GAME_IPX` |
| Modem branches in `QUEUE.CPP` (`Process_Serial_Packet`, `NullModem` packet buffer, serial reconnect in `Handle_Timeout`, emergency-save-on-hang-up, `Destroy_Null_Connection` on EXIT / out-of-sync, local `RC_SERIAL_PROCESSED`/`RC_PLAYER_LEFT`/`RC_HUNG_UP`), `CONQUER.CPP` (`Modem_Signoff`, serial chat messages, `NullModem.Service()` in `Call_Back`), `INIT.CPP` (`GAME_MODEM`/`GAME_NULL_MODEM` cases), `MPLAYER.CPP` (the "Modem/Serial" button; the dialog is 11*RESFACTOR shorter), `SCORE.CPP` (serial timing packets on the score screen), `SENDFILE.CPP` (the `gametype == 0` serial file transfer), `NETDLG.CPP` (two `case` labels), `GLOBALS.CPP`/`EXTERNS.H` (`NullModem`, `Debug_Modem_Dump`, `Debug_Smart_Print`) | | Modem play dropped. Every LAN (`GAME_IPX`), internet and skirmish path is unchanged |

**Kept:** `SerialCommandType`, `SerialPacketType` and `RemoteFileTransferType` (`SESSION.H`): the network file
transfer in `SENDFILE.CPP` sends `SERIAL_FILE_CHUNK` packets, so these are part of the LAN path. `MAX_SERIAL_PACKET_SIZE`
stays for the same reason. The `TXT_MODEM_*` string IDs in `CONQUER.H` and the modem-related `FIXIT_*` switches in
`DEFINES.H` were left alone. Code inside `#ifdef NEVER` (e.g. the old `Write_MultiPlayer_Settings` body) was not touched.

## 5. Westwood Online, the WChat/DDE internet path, TEN and MPath

**Last commit with this code: `8b5bb75`.** Evidence: [networking.md](networking.md#what-s05-can-remove-and-what-must-stay),
[platform-dependencies.md](platform-dependencies.md), [game-code.md](game-code.md) (feature switches). Decision: ADR-0005.

**How the preprocessor switches were resolved** (all with `unifdef`, then hand edits where WOL code had to go):

| Symbol | Resolved as | Notes |
|--------|-------------|-------|
| `TEN`, `MPATH` | `0` (`unifdef -DTEN=0 -DMPATH=0`, every `#if(TEN)` / `#if(MPATH)`) | As in `CODE/DEFINES.H`. Their `#define`s were then deleted |
| `WOLAPI_INTEGRATION` | **defined** (`unifdef -DWOLAPI_INTEGRATION`) | As in the shipped build. This deletes every `#ifndef WOLAPI_INTEGRATION` block (the older WChat/DDE internet path, which the shipped game never compiled). *Before* that, the pure-WOL `#ifdef WOLAPI_INTEGRATION ... #endif` blocks (no `#else`) in the kept files were deleted by hand (a small script listed them; each was reviewed). The few `#ifdef ... #else` blocks were handled one by one, see below |

The `#ifdef WOLAPI_INTEGRATION ... #else ... #endif` blocks in kept files:
- **Kept the compiled (WOL) branch**, because it is not WOL functionality: `DIALOG.CPP` (text spacing for `TPF_TYPE`),
  `TEXTBTN.CPP` (disabled-button style), `SESSION.H` (`ShortFileName[13]` in two packet structs: wire layout),
  `NETDLG.CPP` (the matching `strcpy`). Also kept, although they had no `#else`: `DropListClass::Flag_To_Redraw`
  (`DROP.CPP/.H`, a UI fix used by every dialog) and `HouseClass::InitialName` (`HOUSE.H`, set in `SCENARIO.CPP`):
  removing it would change `HouseClass`'s layout, which save games depend on.
- **Removed the WOL button / WOL-only input**, which gave code identical to the `#else` text: `MPLAYER.CPP` (the
  "Internet" button; the dialog keeps the shipped game's centred position and is 11*RESFACTOR shorter),
  `GAMEDLG.CPP` (the WOL options button, only shown when logged in to WOL), `CONQUER.CPP` (`PAGE_RESPOND_KEY`,
  used only to answer WOL pages).

| Removed | What it was | Why it could go |
|---------|-------------|-----------------|
| `CODE/WOL_CGAM.CPP`, `WOL_CHAT.CPP`, `WOL_DNLD.CPP`, `WOL_GSUP.CPP/.H`, `WOL_LOGN.CPP`, `WOL_MAIN.CPP`, `WOL_OPT.CPP` | Westwood Online lobby, chat, login, game setup, patch download, options | WOL service is gone (ADR-0005) |
| `CODE/WOLAPIOB.CPP/.H`, `CODE/RAWOLAPI.CPP/.H`, `CODE/WOLAPI/` (`WOLAPI.H`, `WOLAPI_I.C`, `*DEFS.H`) | COM wrapper and event sinks for the WOLAPI DLL | Same |
| `CODE/WOLEDIT.*`, `CODE/WOLDEBUG.H`, `CODE/W95TRACE.*`, `CODE/COMINIT.*`, `CODE/TOOLTIP.*`, `CODE/ICONLIST.*`, `CODE/BIGCHECK.*`, `CODE/SEDITDLG.*`, `CODE/DIBAPI.H`, `CODE/DIBFILE.CPP`, `CODE/DIBUTIL.*` | WOL-only UI controls, bitmap loading, debug trace and COM setup | Only used by WOL screens (checked: no other file uses them) |
| `CODE/STATS.CPP`, `CODE/PACKET.*`, `CODE/FIELD.*` | Game-statistics packets sent to the WOL/WChat servers | Only used for `GAME_INTERNET`; every call (`Register_Game_Start_Time`, `Register_Game_End_Time`, `Send_Statistics_Packet`, `GameStatisticsPacketSent`, `PacketLater`, `ConnectionLost`) was removed from `CONQUER.CPP` and `QUEUE.CPP` |
| `CODE/TCPIP.*`, `CODE/INTERNET.*`, `CODE/DDE.*`, `CODE/CCDDE.*` | Pre-WOL internet play through WChat: TCP manager (`Winsock` object), DDE server, spawn/launch WChat | Not compiled in the shipped game except the DDE server, which only received WChat start packets and gave the single-instance check (below) |
| `CODE/TENMGR.*`, `CODE/CCTEN.CPP`, `CODE/CCMPATH.CPP`, `CODE/MPMGRD.*`, `CODE/MPMGRW.*`, `CODE/MPLIB.CPP`, `CODE/MPLPC.CPP` | TEN and MPath online services (all code was `#if(TEN)` / `#if(MPATH)`, both 0), and the DOS MPath library | Dead; services gone |
| `GAME_TEN`, `GAME_MPATH` (`SESSION.H`) and their runtime checks in `QUEUE.CPP`, `INIT.CPP`, `STARTUP.CPP` | | Never set |
| `CODE/STARTUP.CPP`: the `wolsetup.exe` / registry clean-up after the WOL patch, and the **single-instance check** (`RA95AlreadyRunning`, set by the DDE server constructor) | | **Behaviour change (not simulation):** a second copy of the game now starts instead of switching to the first one. That is what LAN testing on one machine needs (S12/S13) |
| `CODE/INIT.CPP`: the WOL entry (`WOL_Main`), loading `WOLAPI.MIX`, the "returning from a WOL game" shortcut, a WOL debug loop | | WOL |
| `CODE/QUEUE.CPP`: WOL tournament "disconnect pinging", the shorter resync timeout for WOL games with more than 2 players, the "cancel means forfeit" rule | | Only active with a WOL connection (`pWolapi`), which never exists now |
| `CODE/NETDLG.CPP`: the WOL tournament "cancel means forfeit" text on the reconnect dialog; `Winsock.Get_Connected()` in `Shutdown_Network` (the WChat TCP connection) replaced by nothing (it was always false in a LAN game; a comment marks the spot) | | S04 listed this call under `SENDFILE.CPP`; the two `SENDFILE.CPP` calls were in `#ifndef WINSOCK_IPX` branches and went with section 2 |
| `CODE/SENDFILE.CPP`: three WOLAPI message pumps during scenario download | | WOL |
| `CODE/WOLSTRNG.CPP/.H`: all strings except the 12 still used | | The draw-proposal strings (LAN games, `FIXIT_VERSION_3`) and the expansion-mission captions are still used, so the file stays, trimmed |
| `CODE/EXTERNS.H`: `PlanetWestwood*` (except the port), `WChatHWND`, the statistics and WChat prototypes; includes of the deleted headers in 12 files | | |
| `CMakeLists`/`cmake/RABuildConfig.cmake`: the `WOLAPI_INTEGRATION` and `WINSOCK_IPX` compile definitions; `CODE/PASSEDIT.OBJ` from the missing-objects list (it was the WOL password edit control) | | No source tests the symbols any more |

**Moved, not deleted:**
- `PlanetWestwoodPortNumber` (and `ShowCommand`, also defined there) from `INTERNET.CPP` to `GLOBALS.CPP`; `WSPUDP.CPP` binds to it.
- `GAME_VERSION` (`0x00030003`) from `RAWOLAPI.H` to `VERSION.CPP` (the LAN version check).
- `bSpecialAftermathScenario()` from `WOL_GSUP.CPP` to `SESSION.CPP` (used by the LAN lobby and the skirmish dialog).

**Deliberately kept** (WOL-era, but not WOL code, or removing it would touch the simulation):
- `GAME_INTERNET`: nothing can select it any more, but its branches remain in the connection/queue code and in the
  unit-tracker statistics inside simulation files (`HOUSE.CPP`, `TECHNO.CPP`, `BUILDING.CPP`, `CELL.CPP`). Removing
  those touches simulation files and `HouseClass`; leave it to S12, which decides the internet design.
- `UTRACKER.*` (`UnitTrackerClass`): members of `HouseClass`; same reason.
- `SpecialClass::IsFromWChat`: a bit inside `SpecialClass`, which is sent in game-options packets (wire layout).
- `#ifndef FIXIT_VERSION_3` blocks that mention WChat but only use symbols that still exist (e.g. `SCENARIO.CPP`);
  those that used deleted symbols were removed. `FIXIT_*` switches themselves are untouched (ADR-0011).
- `GETCPU.CPP` (`CPUType` was reported in WOL statistics, but the file is general).

## 6. Remaining DOS-only sources in `CODE/`

**Last commit with this code: `ca5523e`.** Evidence: [platform-dependencies.md](platform-dependencies.md)
("Files in `CODE/` that are not in the Win32 build").

| Removed | What it was | Why it could go |
|---------|-------------|-----------------|
| `CODE/KEYFRAME.CPP` | DOS version of the keyframe shape decoder | DOS object list only (`CODE/MAKEFILE:421-424`); Win32 uses `2KEYFRAM.CPP` |
| `CODE/DPMI.CPP`, `CODE/DPMI.H` | DOS protected-mode interface helpers (`#pragma aux`) | DOS only; nothing includes `DPMI.H` |
| `CODE/CWSTUB.C` | DOS/4GW extender stub (`DOS4GPATH`) | DOS only |

**Not removed** from that list: the old connection code `COMQUEUE.*` (kept on purpose, see section 4), the unbuilt
experiments (`ADPCM.CPP`, `LCWUNCMP.CPP` and the others: S07 may reuse some of them, they are not platform code),
the unbuilt Win32 `CODE/KEYBOARD.CPP`, and the IDE project files (`*.PJT`, `*.IDE`), which are not code.
