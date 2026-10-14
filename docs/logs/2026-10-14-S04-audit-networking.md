# Session log: S04 — Audit: networking → LAN design

- **Date:** 2026-10-14
- **Session brief:** [S04](../sessions/S04-audit-networking.md)
- **Branch / PR:** `worktree-agent-adca5c861a88bebca` / not opened (not pushed, by instruction)
- **Status at end:** Done; in review (ADR-0007 is Proposed and needs the owner's decision)

## Goal
Understand exactly how the original multiplayer works, and write the LAN design that S12–S14 implement.

## What was done
1. **Worktree setup.** The worktree had been created from `main` (`0dc09bb`), which has no `docs/`.
   It was fast-forwarded to `claude/cool-bohr-9vg7ac` (`0ebf96c`, the same base as the other Wave-1
   worktrees) with `git merge --ff-only`. No branch switch.
2. Read `CLAUDE.md`, the wiki pages, ADR-0003/0004/0005, and the S04, S05, S12, S13 and S14 briefs.
3. Read the stack bottom-up. Transport:
   - `WSPROTO.*`, `WSPUDP.*`, `WSPIPX.*`, `_WSPROTO.*`, `UDPADDR.CPP`;
   - the `WINSTUB.CPP` window procedure and the `KEY.CPP` message pump.
4. Connection layer: `IPXMGR.*`, `IPXCONN.*`, `IPXGCONN.*`, `IPXADDR.*`, `CONNECT.*`, `COMBUF.H`,
   `CONNMGR.H`. Checked `MAKEFILE` membership: `SEQCONN`, `NOSEQCON` and `COMQUEUE` are not built.
5. Session, lobby and lockstep:
   - `SESSION.H/.CPP`, `NETDLG.CPP` (discovery, join, options, ping, GO, scenario transfer);
   - `QUEUE.CPP` (whole multiplayer path, compression, CRC, desync and timeout handling);
   - `EVENT.H/.CPP`, `VERSION.H/.CPP`, `MPLAYER.CPP`, `INIT.CPP` (game-type switch and transport
     creation), `CONQUER.CPP` (main loop, `IPX_Call_Back`), `SENDFILE.CPP`;
   - the WOL setup in `WOL_GSUP.CPP`.
6. **Main finding (dead end for the original plan):** the LAN menu entry uses native IPX, not UDP.
   `INIT.CPP:1255` creates `IPXInterfaceClass`, and the UDP alternative is commented out.
   `INIT.CPP:1022`, cited by S00 and the S04 brief, is inside `#ifndef WOLAPI_INTEGRATION` and is not
   compiled. `UDPInterfaceClass` is only used by Westwood Online. Corrected the line in
   `docs/codebase/overview.md` and added a note to the S04 brief.
7. Found why UDP can't do LAN or same-machine play as it stands:
   - no broadcast (only an `IP.INI` unicast list);
   - no `SO_BROADCAST`;
   - one fixed port (1234) used for bind and as every destination;
   - every packet from a local IP is dropped;
   - `WSAEWOULDBLOCK` loses packets.
8. Computed the wire struct sizes with a model program (below). Watcom `/zp1` with smallest-type enums
   and MSVC-style bitfields was compared against 4-byte enums and GCC SysV bitfields. The layout differs
   between modern compilers, so pinned layouts and static_asserts became part of the design.
9. Wrote [`docs/codebase/networking.md`](../codebase/networking.md):
   - layers, component diagram, host/join/start/frames sequence diagram;
   - every packet type with fields, offsets and sizes;
   - ports;
   - lockstep, desync and disconnect behaviour;
   - Windows dependencies;
   - game-type sharing;
   - the S05 keep/remove list.
10. Wrote [`docs/design/lan-multiplayer.md`](../design/lan-multiplayer.md):
    - keep/replace;
    - polled non-blocking transport;
    - address port bytes;
    - port 19960 with fallback;
    - broadcast targets;
    - two instances on one PC;
    - version checks;
    - layout static_asserts;
    - test plan, risks, the internet outlook, and the work split.

    Wrote [ADR-0007](../decisions/0007-lan-udp-transport.md).
11. Rendered all five Mermaid diagrams with mermaid-cli to check them (one fix: semicolons inside
    sequence-diagram messages are statement separators).

## Decisions
- [ADR-0007](../decisions/0007-lan-udp-transport.md) (Proposed): keep the connection layer and lockstep,
  and replace only the transport internals with a polled, portable UDP socket. The port goes into the
  address, the default base port is 19960 with a 4-port span, and versions must match exactly.
  **It is not in `docs/decisions/README.md`** (that file is not this session's to edit); the integrator
  should add the row.
- Smaller decisions in the design doc, no separate ADR:
  - keep the `IPX*` and `GAME_IPX` names for M3;
  - poll from `IPXManagerClass::Service()` rather than only once per frame;
  - normalise local source addresses to `127.0.0.1`;
  - join by IP via the existing `Session.IsBridge`/`BridgeNet` path;
  - debug drop/delay knobs.

## Files changed
| File / folder | Change |
|---------------|--------|
| `docs/codebase/networking.md` | New: full audit of the original network stack |
| `docs/design/lan-multiplayer.md` | New: target LAN design for M3 |
| `docs/decisions/0007-lan-udp-transport.md` | New: ADR, Proposed |
| `docs/logs/2026-10-14-S04-audit-networking.md` | New: this log |
| `docs/sessions/S04-audit-networking.md` | Tasks ticked, status `In review`, outcome section with the correction |
| `docs/codebase/overview.md` | One bullet corrected: LAN does not use UDP yet |

No source code was changed.

## Verification
Struct sizes, run in the session's scratchpad (`sizes.c` is reproduced below):
```
$ gcc -m32 -mms-bitfields -o s_ms sizes.c && ./s_ms      # Watcom-like: 1/2-byte enums, MSVC bitfields
CommHeaderType 7, GlobalHeaderType 9, IPXAddressClass 10, SpecialClass 4,
EventClass 20 (Data at 5, 15 bytes), GlobalPacketType 139 (union at 14, 125 bytes),
ScenarioInfo 120, ScenarioInfo.Seed at 69, .Special at 73, MAX_IPX_PACKET_SIZE 520 (26 events)
$ gcc -m32 -o s_gnu sizes.c && ./s_gnu                    # GCC SysV bitfields
SpecialClass 2, ScenarioInfo 115, ScenarioInfo.Special at 70 (everything else as above)
$ gcc -m32 -mms-bitfields -o si_ms sizes_int.c && ./si_ms  # all enums 4 bytes
EventClass 32 (Data at 8, 24 bytes), GlobalPacketType 144, MAX_IPX_PACKET_SIZE 512 (16 events)
```
`RemoteFileTransferType` = 2 + 2 + 2 + 192 = 198 was computed by hand from `SESSION.H:329-335`.

Mermaid, all five diagrams (the networking.md component and sequence diagrams, and the design doc's
target, polling and discovery diagrams), each rendered successfully to PNG:
```
$ npx -y @mermaid-js/mermaid-cli@11.4.2 -p puppeteer.json -i <diagram>.mmd -o <diagram>.png
Generating single mermaid chart
```
(`puppeteer.json` = `{"executablePath":"/opt/pw-browsers/chromium-1194/chrome-linux/chrome","args":["--no-sandbox"]}`.)

<details><summary><code>sizes.c</code>: struct size model</summary>

```c
/* Model of Red Alert network structs to compute their /zp1 sizes.
 * Enums are modelled with their Watcom default (smallest integral type).
 * Build: gcc -m32 -mms-bitfields sizes.c   (Watcom/MSVC-like bitfields)
 *        gcc -m32 sizes.c                  (default SysV bitfields, for comparison)
 * sizes_int.c = the same with every enum typedef changed to int32_t.
 */
#include <stdio.h>
#include <stddef.h>
#include <stdint.h>
#pragma pack(push,1)
typedef int8_t  HousesType;  typedef int8_t PlayerColorType;
typedef int16_t NetCommandType; typedef int16_t SerialCommandType;
typedef int8_t  EventType; typedef int8_t RTTIType; typedef int8_t MissionType;
typedef int8_t  SpeedType; typedef uint8_t MPHType; typedef int8_t AnimType;
typedef int16_t CELL; typedef uint32_t COORDINATE;
typedef struct { uint32_t t; } xTargetClass;          /* TARGET_COMPOSITE: long */
typedef struct { unsigned a:1,b:1,c:1,d:1,e:1,f:1,g:1,h:1,i:1; } SpecialClass;
typedef struct { uint16_t MagicNumber; uint8_t Code; uint32_t PacketID; } CommHeaderType;
typedef struct { CommHeaderType Header; uint16_t ProductID; } GlobalHeaderType;
typedef struct { uint8_t NetworkNumber[4]; uint8_t NodeAddress[6]; } IPXAddressClass;
typedef struct {
	EventType Type;
	unsigned Frame:26; unsigned ID:5; unsigned IsExecuted:1;
	union {
		struct { SpecialClass Data; } Options;
		struct { CELL Cell; } SellCell;
		struct { xTargetClass Whom; } Target;
		struct { AnimType What; HousesType Owner; COORDINATE Where; } Anim;
		struct { int32_t Value; } General;
		struct { xTargetClass Whom; MissionType Mission; xTargetClass Target; xTargetClass Destination; } MegaMission;
		struct { xTargetClass Whom; MissionType Mission; xTargetClass Target; xTargetClass Destination; SpeedType Speed; MPHType MaxSpeed; } MegaMission_F;
		struct { xTargetClass Whom; MissionType Mission; } Mission;
		struct { xTargetClass Whom; xTargetClass Where; } NavCom;
		struct { xTargetClass Whom; xTargetClass Target; } TarCom;
		struct { RTTIType Type; int32_t ID; } Specific;
		struct { RTTIType Type; CELL Cell; } Place;
		struct { int32_t ID; CELL Cell; } Special;
		struct { uint32_t CRC; uint16_t CommandCount; uint8_t Delay; } FrameInfo;
		struct { uint32_t Pointer; uint32_t Size; } Variable; /* void* on 32-bit */
		struct { uint16_t DesiredFrameRate; uint16_t MaxAhead; } Timing;
		struct { uint16_t AverageTicks; } ProcessTime;
	} Data;
} EventClass;
typedef struct {
	NetCommandType Command;
	char Name[12];
	union {
		struct { unsigned int IsOpen:1; } GameInfo;
		struct { HousesType House; PlayerColorType Color; uint32_t NameCRC; uint32_t MinVersion; uint32_t MaxVersion; int32_t CheatCheck; } PlayerInfo;
		struct { char Scenario[44]; unsigned int Credits;
			unsigned int IsBases:1, IsTiberium:1, IsGoodies:1, IsGhosties:1, OfficialScenario:1;
			uint8_t BuildLevel, UnitCount, AIPlayers; int32_t Seed; SpecialClass Special;
			unsigned int GameSpeed; uint32_t Version; unsigned int FileLength; char ShortFileName[13]; uint8_t FileDigest[32]; } ScenarioInfo;
		struct { char Buf[120]; PlayerColorType Color; uint32_t NameCRC; } Message;
		struct { int32_t OneWay; } ResponseTime;
		struct { int32_t Why; } Reject;
		struct { uint32_t ID; PlayerColorType Color; } Chat;
	} u;
} GlobalPacketType;
#pragma pack(pop)
#define P(t) printf("%-22s %4zu\n", #t, sizeof(t))
#define O(t,m) printf("  %-34s off %3zu size %3zu\n", #m, offsetof(t,m), sizeof(((t*)0)->m))
int main(void){
	P(CommHeaderType); P(GlobalHeaderType); P(IPXAddressClass); P(SpecialClass); P(EventClass); P(GlobalPacketType);
	O(EventClass,Data); O(EventClass,Data.FrameInfo); O(EventClass,Data.MegaMission_F); O(EventClass,Data.Timing);
	O(GlobalPacketType,u); O(GlobalPacketType,u.PlayerInfo); O(GlobalPacketType,u.ScenarioInfo); O(GlobalPacketType,u.Message);
	O(GlobalPacketType,u.ScenarioInfo.BuildLevel); O(GlobalPacketType,u.ScenarioInfo.Special);
	printf("MAX_IPX_PACKET_SIZE    %4zu  (events per packet %zu)\n",
		((546 - sizeof(CommHeaderType)) / sizeof(EventClass)) * sizeof(EventClass),
		(546 - sizeof(CommHeaderType)) / sizeof(EventClass));
	return 0;
}
```
</details>

## Determinism impact
None. No code was changed. The design calls for wire-struct layout changes (fixed enum underlying
types, bitfield layout) in S06/S12. Those change in-memory sizes but not arithmetic, and must be checked
with the S14 harness, as the design doc says.

## Problems & open questions
- **Watcom enum sizing is inferred, not verified.** There is no Watcom compiler here. The code's own
  1-byte event-type assumption supports it.
- **For the owner:**
  - accept ADR-0007;
  - accept the default port 19960 (IANA status unverified);
  - should cross-play between Windows and Linux builds be an M3 requirement (it drives the layout work)?
  - keep or drop the Aftermath flag in the version check?
  - is renaming `IPX*`/`GAME_IPX` wanted at all?
- `docs/decisions/README.md` needs a row for ADR-0007, and `docs/sessions/README.md` needs S04's status
  and log link. Both are left to the integrator, as instructed.
- **Plan changes for other sessions:**
  - **S12's brief says "poll once per frame from the main loop".** The design says poll in
    `IPXManagerClass::Service()`, because the game blocks in nested wait loops.
  - **S05** must keep `PlanetWestwoodPortNumber` (`INTERNET.CPP`) and `GAME_VERSION` (`RAWOLAPI.H`),
    stub `Winsock.Get_Connected()` in `SENDFILE.CPP`, and extract the skirmish dialog from `NULLDLG.CPP`.
  - **S13** must also switch `GAME_IPX` to `UDPInterfaceClass`, handle duplicate handles for two
    instances, and keep simulating when the window loses focus.
  - **S06** owns the wire-layout work.
- Unverified details are marked as such in `networking.md` and `lan-multiplayer.md`.

## Next steps
- Owner review of ADR-0007 and the open questions above.
- S05: use the keep/remove list in `networking.md`.
- S12/S13/S14: implement from `docs/design/lan-multiplayer.md` (work split at its end).
