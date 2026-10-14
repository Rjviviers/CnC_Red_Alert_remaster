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
