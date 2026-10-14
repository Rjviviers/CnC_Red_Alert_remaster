//
//	Command & Conquer Red Alert(tm)
//	Copyright 2025 Electronic Arts Inc.
//
//	This program is free software: you can redistribute it and/or modify
//	it under the terms of the GNU General Public License as published by
//	the Free Software Foundation, either version 3 of the License, or
//	(at your option) any later version.
//
//	This program is distributed in the hope that it will be useful,
//	but WITHOUT ANY WARRANTY; without even the implied warranty of
//	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//	GNU General Public License for more details.
//
//	You should have received a copy of the GNU General Public License
//	along with this program.  If not, see <http://www.gnu.org/licenses/>.
//

/***************************************************************************
 * SHAPEDRW.CPP -- portable C++ port of CODE/2KEYFBUF.ASM (session S07-B).
 *
 * Original: KEYFBUFF.ASM, David R. Dettmer (March 1995), line-header system
 * by ST (November 1995).
 *
 * The asm had 16 "BF_*" routines (whole rectangle, used when the shape is
 * clipped or has no line headers) and 19 "Single_Line_*" routines (one row,
 * picked per row through a jump table indexed by a line-header byte).
 * Here every one of them is the same row loop, Draw_Row<F>, instantiated
 * with a set of compile-time pixel-operation flags. The flags also encode
 * the asm's bugs that change pixels, so each instantiation draws exactly
 * what its asm routine drew. Line numbers in comments refer to
 * 2KEYFBUF.ASM as it was before it was deleted (git history).
 *
 * docs/codebase/shape-renderer.md explains the format and the variants.
 ***************************************************************************/

#include "SHAPEDRW.H"

#include <stddef.h>
#include <string.h>

namespace {

typedef unsigned int u32;		// the asm's 32-bit registers
typedef unsigned char u8;

/*
** Shape flags tested for the line-header system (asm lines 965-966, 1273-1274).
*/
const u32 HEADER_FLAG_MASK = BF_SHAPE_TRANS | BF_SHAPE_FADING | BF_SHAPE_PREDATOR | BF_SHAPE_GHOST;

/*
** jflags bits that select the BF_* routine (asm lines 64-70).
*/
enum {
	FLAG_TRANS = 1,
	FLAG_GHOST = 2,
	FLAG_FADING = 4,
	FLAG_PREDATOR = 8,
	FLAG_MASK = 15
};

const u32 PRED_MASK = 0x0E;		// asm line 110

/*****************************************************************************
** Predator effect state. These were global variables in the asm's data
** segment (lines 159-178) and they keep their values between calls, as before.
*/
const unsigned short BFPredTable[8] = {1, 3, 2, 5, 2, 3, 4, 1};				// line 178
const short BFPredNegBase[8] = {-1, -3, -2, -5, -2, -4, -3, -1};			// line 159
unsigned short BFPredNegTable[8];	// line 161: base + row length, set per call (lines 1386-1398)
u32 BFPredOffset;			// byte offset into BFPredTable; 0xFFFFFF00+n selects BFPredNegTable
u32 BFPartialPred;			// 0x100 = every pixel is a predator pixel
u32 BFPartialCount;

/*****************************************************************************
** The line-header jump table (NewShapeJumpTable, asm lines 251-875),
** transcribed literally: row = (TRANS|GHOST|FADING|PREDATOR) of the call's flags
** in BLIT_* bit order, column = line-header byte & BLIT_ALL. One letter per routine:
**
**   C Short/Long_Single_Line_Copy   K Single_Line_Skip
**   T Single_Line_Trans
**   G Single_Line_Ghost             g Single_Line_Ghost_Trans
**   F Single_Line_Fading            f Single_Line_Fading_Trans
**   S Single_Line_Single_Fade       s Single_Line_Single_Fade_Trans
**   H Single_Line_Ghost_Fading      h Single_Line_Ghost_Fading_Trans
**   P Single_Line_Predator          p Single_Line_Predator_Trans
**   Q Single_Line_Predator_Ghost    q Single_Line_Predator_Ghost_Trans
**   R Single_Line_Predator_Fading   r Single_Line_Predator_Fading_Trans
**   U Single_Line_Predator_Ghost_Fading   u ..._Trans
**
** Row 15 is AllFlagsJumpTable. The table is not const: like the asm
** (lines 1347-1353) every call with SHAPE_FADING rewrites columns 4 and 5 of
** the row it uses ("CriticalFadeRedirections").
*/
char LineJumpTable[16][33] = {
	"CCCCCCCCCCCCCCCCKKKKKKKKKKKKKKKK",	//  0 lines 257-289
	"CTCTCTCTCTCTCTCTKKKKKKKKKKKKKKKK",	//  1 lines 295-326
	"CCGGCCGGCCGGCCGGKKKKKKKKKKKKKKKK",	//  2 lines 333-364
	"CTGgCTGgCTGgCTGgKKKKKKKKKKKKKKKK",	//  3 lines 371-402
	"CCCCSSFFCCCCFFFFKKKKKKKKKKKKKKKK",	//  4 lines 412-443
	"CTCTSsFfCTCTFfFfKKKKKKKKKKKKKKKK",	//  5 lines 450-481
	"CCGGSSHHCCGGFFHHKKKKKKKKKKKKKKKK",	//  6 lines 488-519
	"CTGgSsHhCTGgFfHhKKKKKKKKKKKKKKKK",	//  7 lines 527-558
	"CCCCCCCCPPPPPPPPKKKKKKKKKKKKKKKK",	//  8 lines 569-600
	"CTCTCTCTPpPpPpPpKKKKKKKKKKKKKKKK",	//  9 lines 606-637
	"CCGGCCGGPPQQPPQQKKKKKKKKKKKKKKKK",	// 10 lines 644-675
	"CTGgCTGgPpQqPpQqKKKKKKKKKKKKKKKK",	// 11 lines 682-713
	"CCCCSSFFPPPPRRRRKKKKKKKKKKKKKKKK",	// 12 lines 723-754
	"CTCTSsFfPpPpRrRrKKKKKKKKKKKKKKKK",	// 13 lines 761-792
	"CCGGSSHHPPQQRRUUKKKKKKKKKKKKKKKK",	// 14 lines 799-830
	"CTGgSsHhPpQqRrUuKKKKKKKKKKKKKKKK"	// 15 lines 843-874 (AllFlagsJumpTable)
};
const int ALL_FLAGS_TABLE = 15;

/*****************************************************************************
** Per-call values (the asm's LOCALs).
*/
struct DrawContext {
	u8 const *IsTranslucent;	// ghost table: 256 bytes, 0xFF = not translucent
	u8 const *Translucent;		// IsTranslucent + 256: one 256-byte row per translucent colour
	u8 const *FadingTable;
	u32 FadingNum;			// 0-63; 0 means 2^32 iterations (dec ecx / jnz)
	u32 EaxHigh;			// bits 8-31 of EAX, see PX_GHOST_WIDE
};

/*
** Run a colour through the fading table FadingNum times (asm "fade_loop":
** mov al,[ebx+eax] / dec ecx / jnz). A count of 0 is 2^32 iterations in the
** asm; that is computed exactly here through the cycle the value falls into.
*/
u8 Fade(DrawContext const &c, u32 v)
{
	u32 n = c.FadingNum;
	if (n != 0) {
		while (n--) v = c.FadingTable[v];
		return (u8)v;
	}

	/*
	** 2^32 steps: walk until a colour repeats, then skip whole cycles.
	*/
	unsigned long long remaining = 1ULL << 32;
	int seen[256];
	for (int i = 0; i < 256; i++) seen[i] = -1;
	int step = 0;
	while (remaining > 0) {
		if (seen[v] >= 0) {
			remaining %= (unsigned long long)(step - seen[v]);
			for (int i = 0; i < 256; i++) seen[i] = -1;	// only the tail is left
			while (remaining--) v = c.FadingTable[v];
			return (u8)v;
		}
		seen[v] = step++;
		v = c.FadingTable[v];
		remaining--;
	}
	return (u8)v;
}

/*
** One predator step for a drawn pixel ("get_pred" in every predator routine).
** Returns true when this pixel is a predator pixel; offset then receives the
** distance to the destination pixel that is copied.
*/
template<bool AndBug>
inline bool Predator_Step(u32 &offset)
{
	u32 sum = BFPartialCount + BFPartialPred;	// mov ebx,[BFPartialCount] / add ebx,[BFPartialPred]
	if ((sum & 0xFF00) == 0) {			// or bh,bh / jnz get_pred
		BFPartialCount = sum;
		return false;
	}
	BFPartialCount = sum & ~0xFF00u;		// xor bh,bh

	u32 old = BFPredOffset;
	u32 low = old & 0xFF;
	if (AndBug) {
		low = low & 2;				// "and [BYTE BFPredOffset],2" (lines 2429, 2484)
	} else {
		low = (low + 2) & 0xFF;			// add [BYTE BFPredOffset],2
	}
	low &= PRED_MASK;				// and [BYTE BFPredOffset],PRED_MASK
	BFPredOffset = (old & ~0xFFu) | low;

	/*
	** movzx eax,[WORD BFPredTable+eax]: offsets 0xFFFFFF00+n read the
	** computed table 256 bytes in front of BFPredTable.
	*/
	u32 index = (old & PRED_MASK) >> 1;
	if ((old & 0xFFFFFF00u) == 0xFFFFFF00u) {
		offset = BFPredNegTable[index];
	} else {
		offset = BFPredTable[index];
	}
	return true;
}

/*****************************************************************************
** Pixel-operation flags. One combination = one asm routine.
*/
enum {
	PX_TRANS        = 0x001,	// colour 0 leaves the destination alone
	PX_GHOST        = 0x002,	// translucency through the ghost table
	PX_FADE         = 0x004,	// remap FadingNum times through the fading table
	PX_PRED         = 0x008,	// predator: some pixels copy a nearby destination pixel
	PX_PRED_SKIP    = 0x010,	// non-predator pixels are not drawn at all
	PX_PRED_AND_BUG = 0x020,	// predator offset advanced with AND instead of ADD
	PX_GHOST_WIDE   = 0x040,	// ghost lookup indexed with all of EAX, not just AL
	PX_FADE_ZERO    = 0x080,	// fade starts from colour 0, not from the pixel
	PX_FADE_ONCE    = 0x100,	// one table lookup whatever FadingNum is
	PX_COPY         = 0x200,	// plain copy (memcpy)
	PX_SKIP         = 0x400	// draw nothing (Single_Line_Skip)
};

template<unsigned F>
void Draw_Row(DrawContext &c, u8 const *src, u8 *dst, int count)
{
	if (F & PX_SKIP) return;
	if (F & PX_COPY) {
		memcpy(dst, src, (size_t)count);
		return;
	}

	for (int i = 0; i < count; i++, src++, dst++) {
		u32 v = *src;
		if ((F & PX_TRANS) && v == 0) continue;

		if (F & PX_PRED) {
			u32 offset;
			if (Predator_Step<(F & PX_PRED_AND_BUG) != 0>(offset)) {
				v = dst[offset];			// mov al,[edi+eax]
				if (F & PX_GHOST_WIDE) c.EaxHigh = offset & 0xFF00;
			} else if (F & PX_PRED_SKIP) {
				continue;
			}
		}

		if (F & PX_GHOST) {
			u32 index = (F & PX_GHOST_WIDE) ? (c.EaxHigh | v) : v;
			u8 g = c.IsTranslucent[index];		// mov bh,[ebx+eax]
			if (g != 0xFF) {
				v = c.Translucent[g * 256u + *dst];	// ((g*256) + dest colour)
				if (F & PX_GHOST_WIDE) c.EaxHigh = 0;	// xor eax,eax
			}
		}

		if (F & PX_FADE) {
			if (F & PX_FADE_ZERO) v = 0;
			v = (F & PX_FADE_ONCE) ? c.FadingTable[v] : Fade(c, v);
		}

		*dst = (u8)v;
	}
}

typedef void (*RowFunc)(DrawContext &c, u8 const *src, u8 *dst, int count);

/*
** BufferFrameTable (asm lines 793-844), indexed by jflags.
** Used for clipped shapes and shapes without line headers.
*/
RowFunc const BufferFrameTable[16] = {
	Draw_Row<PX_COPY>,					// BF_Copy                         1567
	Draw_Row<PX_TRANS>,					// BF_Trans                        2664
	Draw_Row<PX_GHOST>,					// BF_Ghost                        2715
	Draw_Row<PX_GHOST | PX_TRANS>,				// BF_Ghost_Trans                  2783
	Draw_Row<PX_FADE>,					// BF_Fading                       2857
	Draw_Row<PX_FADE | PX_TRANS>,				// BF_Fading_Trans                 2924
	Draw_Row<PX_GHOST | PX_FADE>,				// BF_Ghost_Fading                 2997
	Draw_Row<PX_GHOST | PX_FADE | PX_TRANS>,		// BF_Ghost_Fading_Trans           3082
	Draw_Row<PX_PRED | PX_PRED_SKIP>,			// BF_Predator                     3173
	Draw_Row<PX_PRED | PX_TRANS>,				// BF_Predator_Trans: non-predator pixels ARE drawn (3297-3298)
	Draw_Row<PX_PRED | PX_GHOST>,				// BF_Predator_Ghost               3336
	Draw_Row<PX_PRED | PX_GHOST | PX_TRANS>,		// BF_Predator_Ghost_Trans         3426
	Draw_Row<PX_PRED | PX_FADE>,				// BF_Predator_Fading              3522
	Draw_Row<PX_PRED | PX_FADE | PX_TRANS>,			// BF_Predator_Fading_Trans        3611
	Draw_Row<PX_PRED | PX_GHOST | PX_FADE>,			// BF_Predator_Ghost_Fading        3706
	Draw_Row<PX_PRED | PX_GHOST | PX_FADE | PX_TRANS>	// BF_Predator_Ghost_Fading_Trans  3813
};

/*
** The Single_Line_* routine for a jump-table letter.
*/
RowFunc Line_Routine(char letter)
{
	switch (letter) {
		case 'C': return Draw_Row<PX_COPY>;							// 1697, 1732
		case 'K': return Draw_Row<PX_SKIP>;							// 1750
		case 'T': return Draw_Row<PX_TRANS>;							// 1619
		case 'G': return Draw_Row<PX_GHOST>;							// 1765
		case 'g': return Draw_Row<PX_GHOST | PX_TRANS>;						// 1798
		case 'F': return Draw_Row<PX_FADE>;							// 1912
		case 'f': return Draw_Row<PX_FADE | PX_TRANS>;						// 1947
		case 'S': return Draw_Row<PX_FADE | PX_FADE_ONCE>;					// 1989
		case 's': return Draw_Row<PX_FADE | PX_FADE_ONCE | PX_TRANS>;				// 2014
		case 'H': return Draw_Row<PX_GHOST | PX_FADE>;						// 2063
		case 'h': return Draw_Row<PX_GHOST | PX_FADE | PX_TRANS>;				// 2107
		case 'P': return Draw_Row<PX_PRED | PX_PRED_SKIP>;					// 2204
		case 'p': return Draw_Row<PX_PRED | PX_PRED_SKIP | PX_TRANS>;				// 2249
		case 'Q': return Draw_Row<PX_PRED | PX_GHOST | PX_GHOST_WIDE>;				// 2294
		case 'q': return Draw_Row<PX_PRED | PX_GHOST | PX_GHOST_WIDE | PX_TRANS>;		// 2351
		case 'R': return Draw_Row<PX_PRED | PX_PRED_AND_BUG | PX_FADE>;				// 2411
		case 'r': return Draw_Row<PX_PRED | PX_PRED_AND_BUG | PX_FADE | PX_TRANS>;		// 2464
		case 'U': return Draw_Row<PX_PRED | PX_GHOST | PX_FADE | PX_FADE_ZERO>;			// 2520
		case 'u': return Draw_Row<PX_PRED | PX_GHOST | PX_FADE | PX_FADE_ZERO | PX_TRANS>;	// 2589
	}
	return Draw_Row<PX_SKIP>;	// not reachable: the table only holds the letters above
}

/*
** Clip-code bit: the sign bit of a 32-bit value, as "shld reg,src,1" read it.
*/
inline u32 Sign(u32 v)
{
	return v >> 31;
}

} // namespace


/***************************************************************************
 * Setup_Shape_Header -- create the line header bytes for a shape          *
 *                                                                         *
 * Port of asm lines 941-1082. Scans every row of the frame and stores one  *
 * byte per row describing which effects that row needs.                   *
 ***************************************************************************/
void Setup_Shape_Header(int pixel_width, int pixel_height, void const *src, void *headers,
	int flags, void const * /*translucent*/, void const *is_translucent)
{
	u8 const *pixels = (u8 const *)src;
	u8 const *is_trans = (u8 const *)is_translucent;
	ShapeFrameHeaderType *header = (ShapeFrameHeaderType *)headers;

	header->draw_flags = (u32)flags & HEADER_FLAG_MASK;
	u8 *line = (u8 *)headers + sizeof(ShapeFrameHeaderType);

	for (int row = 0; row < pixel_height; row++) {
		u32 seen = 0;			// bl
		int trans_count = 0;

		for (int col = 0; col < pixel_width; col++) {
			u8 pixel = *pixels++;
			if (pixel == 0 && (flags & BF_SHAPE_TRANS)) {
				seen |= BLIT_TRANSPARENT;
				trans_count++;
				continue;
			}
			if (flags & BF_SHAPE_PREDATOR) seen |= BLIT_PREDATOR;
			if ((flags & BF_SHAPE_GHOST) && is_trans[pixel] != 0xFF) seen |= BLIT_GHOST;
			if (flags & BF_SHAPE_FADING) seen |= BLIT_FADING;
		}

		u32 type = 0;			// bh
		if (seen & BLIT_TRANSPARENT) {
			type = BLIT_TRANSPARENT;
			if (trans_count == pixel_width) {
				*line++ = BLIT_SKIP;	// whole row transparent: nothing to draw
				continue;
			}
		}
		type |= seen & (BLIT_PREDATOR | BLIT_GHOST | BLIT_FADING);
		*line++ = (u8)type;
	}
}


/***************************************************************************
 * Buffer_Frame_To_View -- draws a shape frame into a viewport              *
 *                                                                         *
 * Port of Buffer_Frame_To_Page (asm lines 1135-1556). See SHAPEDRW.H.     *
 ***************************************************************************/
long Buffer_Frame_To_View(int x_pixel, int y_pixel, int pixel_width, int pixel_height, void *src_ptr,
	ShapeViewType const &view, int flags, va_list args)
{
	if (src_ptr == 0) return 0;								// 1181

	u8 const *src = (u8 const *)src_ptr;
	ShapeFrameHeaderType *header = 0;
	bool use_old_draw = true;

	if (!UseOldShapeDraw && UseBigShapeBuffer) {						// 1184-1210
		/*
		** Deliberate difference: the asm loops forever (Setup_Shape_Header) or
		** draws 2^32 rows here. Nothing sensible can be drawn, so stop.
		*/
		if (pixel_width <= 0 || pixel_height <= 0) return 0;

		header = (ShapeFrameHeaderType *)src_ptr;
		char *start = header->shape_buffer ? TheaterShapeBufferStart : BigShapeBufferStart;
		src = (u8 const *)(start + (size_t)header->shape_data);
		use_old_draw = false;
	}

	/*
	** Optional arguments (1218-1418).
	*/
	DrawContext c;
	c.IsTranslucent = 0;
	c.Translucent = 0;
	c.FadingTable = 0;
	c.FadingNum = 0;
	c.EaxHigh = 0;
	u32 jflags = 0;

	if (flags & BF_SHAPE_CENTER) {								// 1226-1234 (sar)
		x_pixel -= pixel_width >> 1;
		y_pixel -= pixel_height >> 1;
	}

	if (flags & BF_SHAPE_TRANS) jflags |= FLAG_TRANS;					// 1237-1240

	if (flags & BF_SHAPE_GHOST) {								// 1246-1254
		c.IsTranslucent = va_arg(args, u8 const *);
		c.Translucent = c.IsTranslucent + 256;
		jflags |= FLAG_GHOST;
	}

	/*
	** Pick the line-header jump table; build the line headers the first time
	** a frame is drawn (1263-1324).
	*/
	int table;
	{
		u32 blit = 0;										// 1295-1317
		if (flags & BF_SHAPE_PREDATOR) blit |= BLIT_PREDATOR;
		if (flags & BF_SHAPE_FADING) blit |= BLIT_FADING;
		if (flags & BF_SHAPE_TRANS) blit |= BLIT_TRANSPARENT;
		if (flags & BF_SHAPE_GHOST) blit |= BLIT_GHOST;
		table = (int)blit;
	}
	if (!use_old_draw) {
		if (header->draw_flags == 0xFFFFFFFFu) {
			Setup_Shape_Header(pixel_width, pixel_height, src, header, flags,
				c.Translucent, c.IsTranslucent);					// 1281-1290
			table = ALL_FLAGS_TABLE;							// 1291
		} else if (((u32)flags & HEADER_FLAG_MASK) != header->draw_flags) {
			use_old_draw = true;								// 1277-1278
		}
	}

	if (flags & BF_SHAPE_FADING) {								// 1330-1353
		c.FadingTable = va_arg(args, u8 const *);
		c.FadingNum = (u32)va_arg(args, int) & 0x3F;
		jflags |= FLAG_FADING;
		/*
		** A fade count of 0 clears SHAPE_FADING in the asm (1342), but nothing
		** tests that bit afterwards, so it changes nothing.
		*/
		if (c.FadingNum == 1) {
			LineJumpTable[table][4] = 'S';
			LineJumpTable[table][5] = 's';
		} else {
			LineJumpTable[table][4] = 'F';
			LineJumpTable[table][5] = 'f';
		}
	}

	if (flags & BF_SHAPE_PREDATOR) {							// 1361-1398
		u32 value = (u32)va_arg(args, int) << 1;
		jflags |= FLAG_PREDATOR;
		if ((int)value >= 0) {
			BFPredOffset = value & PRED_MASK;
		} else {
			BFPredOffset = 0xFFFFFF00u | ((0u - value) & PRED_MASK);
		}
		BFPartialCount = 0;
		BFPartialPred = 0x100;
		for (int i = 7; i >= 0; i--) {
			BFPredNegTable[i] = (unsigned short)((unsigned short)BFPredNegBase[i]
				+ view.Width + view.XAdd + view.Pitch);
		}
	}

	if (flags & BF_SHAPE_PARTIAL) {								// 1404-1410
		BFPartialPred = (u32)va_arg(args, int) & 0xFF;
	}

	RowFunc buffer_frame_rout = BufferFrameTable[jflags & FLAG_MASK];			// 1413-1418

	/*
	** Clip the destination rectangle against the viewport (1422-1492).
	** Bit 3 = left of 0, bit 2 = right of Width, bit 1 = above 0,
	** bit 0 = below Height; cl for the top-left corner, dl for bottom-right.
	*/
	int scr_x = 0;
	int scr_y = 0;
	int x1_pixel = x_pixel + pixel_width;
	int y1_pixel = y_pixel + pixel_height;
	u32 w1 = (u32)view.Width + 1;
	u32 h1 = (u32)view.Height + 1;

	u32 cl = (Sign((u32)x_pixel) << 3) | (Sign((u32)x_pixel - w1) << 2)
		| (Sign((u32)y_pixel) << 1) | Sign((u32)y_pixel - h1);
	u32 dl = (Sign((u32)x1_pixel) << 3) | (Sign((u32)x1_pixel - w1) << 2)
		| (Sign((u32)y1_pixel) << 1) | Sign((u32)y1_pixel - h1);
	cl ^= 5;
	dl ^= 5;
	if (cl & dl) return 0;									// fully outside

	if (cl | dl) {
		use_old_draw = true;								// 1462: headers can't clip
		if (cl & 8) {
			scr_x = -x_pixel;
			x_pixel = 0;
		}
		if (cl & 2) {
			scr_y = -y_pixel;
			y_pixel = 0;
		}
		if (dl & 4) x1_pixel = view.Width;
		if (dl & 1) y1_pixel = view.Height;
	}

	/*
	** do_blit (1494-1519).
	*/
	int stride = view.XAdd + view.Pitch + view.Width;
	u8 *dest = view.Offset + (ptrdiff_t)(int)((u32)y_pixel * (u32)stride) + x_pixel;
	int dest_adjust_width = stride + x_pixel - x1_pixel;
	int scr_adjust_width = pixel_width - x1_pixel + x_pixel;
	src += (ptrdiff_t)(int)((u32)scr_y * (u32)pixel_width) + scr_x;

	if (!use_old_draw) {
		/*
		** Line-header system (1528-1538 and the next_line macro, 1091-1107):
		** one routine per row, chosen by that row's header byte.
		*/
		u8 const *line = (u8 const *)header + sizeof(ShapeFrameHeaderType);
		for (int row = 0; row < pixel_height; row++) {
			c.EaxHigh = 0;	// see docs: the asm had a jump-table address here
			Line_Routine(LineJumpTable[table][line[row] & BLIT_ALL])(c, src, dest, pixel_width);
			src += pixel_width;
			dest += pixel_width + dest_adjust_width;
		}
		return 0;
	}

	/*
	** Old system: one BF_* routine for the whole (clipped) rectangle (1541-1551).
	*/
	int rows = y1_pixel - y_pixel;
	if (rows <= 0) return 0;
	int cols = x1_pixel - x_pixel;
	if (cols <= 0) return 0;

	for (int row = 0; row < rows; row++) {
		buffer_frame_rout(c, src, dest, cols);
		src += cols + scr_adjust_width;
		dest += cols + dest_adjust_width;
	}
	return 0;
}
