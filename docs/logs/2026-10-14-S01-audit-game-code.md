# Session log: S01 — Audit: game code

- **Date:** 2026-10-14
- **Session brief:** [S01](../sessions/S01-audit-game-code.md)
- **Branch / PR:** `worktree-agent-a499dad99c633fb5e` (agent worktree; the orchestrator merges it) / no PR yet
- **Status at end:** Done (in review)

## Goal
Know exactly which files in `CODE/` belong to the Win32 build and what each one depends on, so that later sessions
(S05, S06, S08, …) can be planned from the inventory.

## What was done
1. The worktree had been created from `0dc09bb`, which has no `docs/`. I fast-forwarded it to `claude/cool-bohr-9vg7ac`
   (`f839023`) without switching branches, then read `CLAUDE.md`, the wiki, the ADRs and the brief.
2. **Build list.** Wrote a parser for `CODE/MAKEFILE` that evaluates its `!ifdef` blocks with `WIN32` defined. Result:
   250 unique objects (180 common + 18 Win32-only in `ra95.exe`, 2 in `jshell.lib`, 50 in `tech.lib`; `INI.OBJ` is listed twice).
   They map to 242 `.CPP`, 6 `.ASM`, and **2 missing sources** (`PALETTE.CPP`, `PASSEDIT.CPP`). `MAPEDSEL.CPP` and
   `WOLAPI/WOLAPI_I.C` are compiled through `#include`. 41 sources in `CODE/` are not built.
3. **Preprocessor-aware dependency scan.** A first grep pass showed the counts in `overview.md` were misleading: `gcl` matched
   `BuildingClass`, and most "IPX"/"Winsock" files only mention those names in DOS branches. So I wrote a tri-state
   (`#if` true / false / unknown) evaluator using the Win32 release configuration (makefile `/D` flags + `DEFINES.H`), and
   grepped only compiled lines. I fixed three scanner bugs found while spot-checking:
   - string literals were matched (`"Sleep"` in `CONST.CPP`);
   - directive lines inside dead blocks were counted (`EGOS.CPP` DOS `#pragma aux Vsync`);
   - line numbers drifted after `\` continuations, and a file's `#define` leaked into later files.
4. **Spot checks by hand:** `CONQUER`, `INIT`, `STARTUP`, `NULLMGR`, `IPXMGR`, `IPX`, `IPX95`, `COORD`, `EGOS`, `MONOC`, `SCORE`,
   `SESSION`, `CONST`. I confirmed that `IPX_SPX_Installed`/`IPX95` callers are compiled out (`IPXMGR.CPP:149`, `:1138`) and that
   `WINASM.ASM:253-298` contains empty Greenleaf stubs.
5. **Subsystem classification** by hand (17 groups). For uncertain files I checked call sites: `TOOLTIP`, `ICONLIST`, `DIB*` are WOL-only;
   `Com_Scenario_Dialog` (skirmish) is in `NULLDLG.CPP`; the pre-WOL `TCPIP`/`DDE` paths have no live callers.
6. **Boot sequence:** traced `WinMain` → `Main_Game` → `Init_Game` → `Select_Game` → `Main_Loop` → `Sync_Delay`. Found that the Win32
   message pump is in `WWKeyboardClass::Fill_Buffer_From_System` (`KEY.CPP:521`), and that sockets are serviced from the
   window procedure (`WINSTUB.CPP:184-186`).
7. **Feature switches:** listed every platform and feature switch, its value in the shipped build, its use count, and what it controls.
8. **Determinism:** checked RNG usage (`Scen.RandomNumber` vs `NonCriticalRandomNumber` vs `CryptRandom` vs `rand()`), floating point
   (none in simulation), fixed point, the asm math helpers, the CRC sync check, and the network event layout.
9. Wrote `docs/codebase/game-code.md` and `docs/codebase/platform-dependencies.md`. The second one embeds the two scripts, and
   I verified that the embedded copies regenerate the 252-row table byte for byte.

## Decisions
- No ADR written (read-only session). **Suggested ADRs** for the orchestrator or owner:
  1. *LAN transport:* the shipped LAN game uses Winsock IPX (`IPXInterfaceClass`, `INIT.CPP:1255`), and only WOL uses
     `UDPInterfaceClass`. Proposal: LAN switches to `UDPInterfaceClass` in S05 (there is already a commented-out line for this at
     `INIT.CPP:1258`), keeping the transport-independent `IPXManagerClass` layer. This sharpens ADR-0005.
  2. *Wire compatibility with original RA 3.03 clients:* yes or no. This decides whether S06 must keep the `EventClass`
     byte layout (packing, Watcom bit-fields, `unsigned long` fields) or may redesign it.
- Small decisions: the scripts live in an appendix of `platform-dependencies.md`, because S01 owns only documentation files.
  S03 may move them to `tools/`.

## Files changed
| File / folder | Change |
|---------------|--------|
| `docs/codebase/game-code.md` | New: build composition, subsystem map, boot sequence, feature switches, determinism, implications |
| `docs/codebase/platform-dependencies.md` | New: summary, method, dependency overview, headers/missing files, 252-row table, unbuilt files, scripts |
| `docs/sessions/S01-audit-game-code.md` | Ticked tasks, status → In review |
| `docs/logs/2026-10-14-S01-audit-game-code.md` | This log |

No source files were modified.

## Verification
- `python3 -I scan_deps.py <repo> deps.tsv && python3 -I gen_table.py deps.tsv`: 252 rows, every file classified
  (no `??` subsystem). Extracting the scripts from the doc and re-running them produced an **identical** table.
- Cross-checks: `ls CODE/*.CPP | wc -l` = 277, i.e. 242 built + 1 included (`MAPEDSEL`) + 34 not built. `.ASM`: 12 = 6 built + 6 not built.
- `grep` of `#include` targets against `CODE/`, `CODE/WOLAPI/`, `WIN32LIB/INCLUDE/` and `WINVQ/INCLUDE/` found the missing headers
  (`modem.h`, `fast.h`, `commlib.h`, `phone.h`, `pcx.h`, `PassEdit.h`; `cbn_.h` and the TEN/MPath headers are only used by dead code).
- Manual review of the matched lines in 13 files (listed above).

## Determinism impact
None (documentation only). The determinism-relevant findings are recorded in `game-code.md` (Determinism section) for S06, S07
and S14.

## Problems & open questions
- `overview.md` (owned by S00/orchestrator) has two statements that this audit corrects. I did not edit it; suggested changes:
  - "Greenleaf (`gcl`) ~66 files" should be: only `NULLMGR.CPP` calls Greenleaf, plus the `FUNCTION.H` headers; the library is not linked in Win32.
  - "LAN over UDP already exists (`INIT.CPP` ~1022)": that code is inside `#ifndef WOLAPI_INTEGRATION` and is dead. The live
    LAN path is Winsock IPX (`INIT.CPP:1255`); UDP is only used by WOL (`INIT.CPP:1202`).
- Missing sources (`PALETTE.CPP`/`PaletteClass`, `phone.h`, `pcx.h`, `PASSEDIT.*`, Greenleaf headers) block compilation. S06 must
  recreate them. Are they available anywhere else (owner question)?
- Wire compatibility with original clients: owner decision needed (see Decisions).
- Not verified: local-player-dependent branches that consume `Scen.RandomNumber`; stability of `LayerClass::Sort`. Left for S14.

## Next steps
- Orchestrator: merge, update `docs/sessions/README.md` and `docs/logs/README.md`, and consider updating `overview.md` with the two corrections above.
- S05: use the "Implications" section of `game-code.md`. Extract the skirmish dialog from `NULLDLG.CPP` before removing modem code,
  and switch LAN to UDP before removing `WSPIPX`.
- S06a: restore the missing headers and sources, and remove the Greenleaf includes from `FUNCTION.H` first.
- S08: replace the message pump in `KEY.CPP` and `WINSTUB.CPP` (`Check_For_Focus_Loss`), and keep a socket-polling hook in `Call_Back`.
