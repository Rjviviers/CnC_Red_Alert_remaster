/*
**	Command & Conquer Red Alert(tm) - multiplayer port. GPLv3 or later, see LICENSE.md.
**	Layout checks for the structures that go over the network (session S06a, ADR-0014).
**
**	This program is free software: you can redistribute it and/or modify
**	it under the terms of the GNU General Public License as published by
**	the Free Software Foundation, either version 3 of the License, or
**	(at your option) any later version.
**
**	This program is distributed in the hope that it will be useful,
**	but WITHOUT ANY WARRANTY; without even the implied warranty of
**	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
**	GNU General Public License for more details.
**
**	You should have received a copy of the GNU General Public License
**	along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

/*
**	Network packets are the raw memory of these structs (docs/design/lan-multiplayer.md,
**	"Keeping network structs layout-stable"). ADR-0013: Windows <-> Linux cross-play is not
**	required, so each compiler may have its own layout, but that layout must never change by
**	accident. The numbers below are what each compiler produced when S06a pinned them
**	(32-bit x86, 1-byte packing, native 1-byte bool, 4-byte enums). The comment on each line
**	gives the original Watcom 10.6 size from the LAN design, which we no longer match
**	(ADR-0009: compatibility with the 1996 clients is not required).
**
**	If an assert fires, a change altered the wire format. Either undo it, or (deliberately)
**	update the number here AND bump the network protocol version, and say so in the session
**	log. This file is compiled into the game, so the build is the test.
*/

#include	"function.h"

#include	<stddef.h>
#include	"../WIN32LIB/DRAWBUFF/GVPORT.H"		// S07-A's mirror of GraphicViewPortClass

/*
**	glibc defines BIG_ENDIAN even on little-endian machines; CODE/compat/ra_compat.h removes
**	it, because the game reads "#ifdef BIG_ENDIAN" as "this machine is big-endian"
**	(DEFINES.H, FIXED.H). Catch it if it ever comes back.
*/
#ifdef BIG_ENDIAN
#error "BIG_ENDIAN is defined: the game would use its big-endian struct layouts (see ra_compat.h)"
#endif

static_assert(sizeof(bool) == 1, "bool is 1 byte (the original typedef int bool was 4)");
static_assert(sizeof(char) == 1 && (char)-1 < 0, "char must be signed (Watcom /j; build with -fsigned-char)");

/*
**	The game is 32-bit (ADR-0003). The sizes below contain pointers and longs, so they are
**	only pinned for 32-bit x86.
*/
#if defined(__i386__) || defined(_M_IX86)

static_assert(sizeof(long) == 4, "32-bit build: long is 4 bytes");
static_assert(sizeof(void *) == 4, "32-bit build: pointers are 4 bytes");

/*
**	Layouts that are the same with every compiler (no enums, no bit fields).
*/
static_assert(sizeof(CommHeaderType) == 7, "CommHeaderType wire size (Watcom: 7)");
static_assert(sizeof(GlobalHeaderType) == 9, "GlobalHeaderType wire size (Watcom: 9)");
static_assert(sizeof(IPXAddressClass) == 10, "IPXAddressClass wire size (Watcom: 10)");
static_assert(sizeof(RemoteFileTransferType) == 200, "RemoteFileTransferType wire size (Watcom: 198)");
static_assert(MAX_IPX_PACKET_SIZE == 512, "game packet payload (Watcom: 520, 26 events)");

/*
**	The enum types stored in wire structs are int-sized with modern compilers (Watcom used
**	the smallest type that fits: 1 byte for these).
*/
static_assert(sizeof(EventClass::EventType) == 4, "EventType (Watcom: 1)");
static_assert(sizeof(NetCommandType) == 4, "NetCommandType (Watcom: 1)");
static_assert(sizeof(HousesType) == 4, "HousesType (Watcom: 1)");
static_assert(sizeof(PlayerColorType) == 4, "PlayerColorType (Watcom: 1)");

static_assert(offsetof(EventClass, Data) == 8, "EventClass header (Watcom: 5)");
static_assert(sizeof(EventClass) == 32, "EventClass wire size (Watcom: 20)");
static_assert(sizeof(GlobalPacketType) == 144, "GlobalPacketType wire size (Watcom: 139)");
static_assert(offsetof(GlobalPacketType, PlayerInfo.MinVersion) == 28, "version fields must never move (Watcom: 20)");
static_assert(offsetof(GlobalPacketType, PlayerInfo.MaxVersion) == 32, "version fields must never move (Watcom: 24)");

/*
**	Bit fields: GCC/Clang on Linux pack them System V style, Microsoft compilers (and MinGW,
**	which defaults to -mms-bitfields) give each bit field its declared type's size.
**	The _WIN32 numbers were measured with MinGW-w64 i686 (GCC 13, ms-bitfields); MSVC is
**	expected to agree. If the first MSVC CI run disagrees, MSVC's own numbers go here.
*/
#if defined(_WIN32)
static_assert(sizeof(SpecialClass) == 4, "SpecialClass wire size (Watcom: 4)");
static_assert(offsetof(GlobalPacketType, ScenarioInfo.Seed) == 71, "ScenarioInfo layout (Watcom: 69)");
static_assert(offsetof(GlobalPacketType, ScenarioInfo.Special) == 75, "ScenarioInfo layout (Watcom: 73)");
static_assert(sizeof(SerialPacketType) == 157, "SerialPacketType size");
#else
static_assert(sizeof(SpecialClass) == 2, "SpecialClass wire size (Watcom: 4)");
static_assert(offsetof(GlobalPacketType, ScenarioInfo.Seed) == 68, "ScenarioInfo layout (Watcom: 69)");
static_assert(offsetof(GlobalPacketType, ScenarioInfo.Special) == 72, "ScenarioInfo layout (Watcom: 73)");
static_assert(sizeof(SerialPacketType) == 152, "SerialPacketType size");
#endif

/*
**	Not a wire struct, but the assembly (and S07's C++ ports of it) reads GraphicViewPortClass
**	at the fixed offsets of WIN32LIB/INCLUDE/GBUFFER.INC (STRUC GraphicViewPort). Its layout
**	must not change until S07 is done. The members are protected, hence the derived probe.
*/
struct GraphicViewPortLayoutProbe : GraphicViewPortClass {
	static void Check(void) {
		static_assert(offsetof(GraphicViewPortLayoutProbe, Offset) == 0, "GVPOffset");
		static_assert(offsetof(GraphicViewPortLayoutProbe, Width) == 4, "GVPWidth");
		static_assert(offsetof(GraphicViewPortLayoutProbe, Height) == 8, "GVPHeight");
		static_assert(offsetof(GraphicViewPortLayoutProbe, XAdd) == 12, "GVPXAdd");
		static_assert(offsetof(GraphicViewPortLayoutProbe, XPos) == 16, "GVPXPos");
		static_assert(offsetof(GraphicViewPortLayoutProbe, YPos) == 20, "GVPYPos");
		static_assert(offsetof(GraphicViewPortLayoutProbe, Pitch) == 24, "GVPPitch");
		static_assert(offsetof(GraphicViewPortLayoutProbe, GraphicBuff) == 28, "GVPBuffPtr");
	}
};
static_assert(sizeof(GraphicViewPortClass) == 40, "GraphicViewPortClass size");

/*
**	S07-A's C++ ports read the class through a mirror struct (WIN32LIB/DRAWBUFF/GVPORT.H,
**	GVPLayout) because GBUFFER.H did not compile with GCC when they were written. The mirror
**	must stay identical to the real class.
*/
static_assert(sizeof(GVPLayout) == sizeof(GraphicViewPortClass), "GVPLayout size must match GraphicViewPortClass");
struct GVPLayoutProbe : GraphicViewPortClass {
	static void Check(void) {
		static_assert(offsetof(GVPLayoutProbe, Offset) == offsetof(GVPLayout, Offset), "GVPLayout.Offset");
		static_assert(offsetof(GVPLayoutProbe, Width) == offsetof(GVPLayout, Width), "GVPLayout.Width");
		static_assert(offsetof(GVPLayoutProbe, Height) == offsetof(GVPLayout, Height), "GVPLayout.Height");
		static_assert(offsetof(GVPLayoutProbe, XAdd) == offsetof(GVPLayout, XAdd), "GVPLayout.XAdd");
		static_assert(offsetof(GVPLayoutProbe, XPos) == offsetof(GVPLayout, XPos), "GVPLayout.XPos");
		static_assert(offsetof(GVPLayoutProbe, YPos) == offsetof(GVPLayout, YPos), "GVPLayout.YPos");
		static_assert(offsetof(GVPLayoutProbe, Pitch) == offsetof(GVPLayout, Pitch), "GVPLayout.Pitch");
		static_assert(offsetof(GVPLayoutProbe, GraphicBuff) == offsetof(GVPLayout, GraphicBuff), "GVPLayout.GraphicBuff");
		static_assert(offsetof(GVPLayoutProbe, IsDirectDraw) == offsetof(GVPLayout, IsDirectDraw), "GVPLayout.IsDirectDraw");
		static_assert(offsetof(GVPLayoutProbe, LockCount) == offsetof(GVPLayout, LockCount), "GVPLayout.LockCount");
	}
};

#endif	// 32-bit x86
