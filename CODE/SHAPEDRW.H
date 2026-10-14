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
 * SHAPEDRW.H -- portable shape renderer (port of CODE/2KEYFBUF.ASM, S07-B).
 *
 * This header has no game dependencies, so the renderer can be unit tested
 * on its own (tests/s07b_*.cpp). The game entry point with the original
 * signature, Buffer_Frame_To_Page(..., GraphicViewPortClass &view, ...),
 * lives in CODE/2KEYFBUF.CPP and forwards to Buffer_Frame_To_View().
 *
 * See docs/codebase/shape-renderer.md for the shape format and every
 * drawing variant.
 ***************************************************************************/

#ifndef SHAPEDRW_H
#define SHAPEDRW_H

#include <stdarg.h>

/*
** Shape flag bits understood by Buffer_Frame_To_Page (2KEYFBUF.ASM lines 73-90).
** They have the same values as ShapeFlags_Type in WIN32LIB/INCLUDE/SHAPE.H;
** SHAPE_TRANS (0x40) only exists in the asm.
*/
enum {
	BF_SHAPE_CENTER   = 0x0020,		// x/y are the centre of the shape
	BF_SHAPE_TRANS    = 0x0040,		// colour 0 is transparent
	BF_SHAPE_FADING   = 0x0100,		// args: void *fade_table, int fade_count
	BF_SHAPE_PREDATOR = 0x0200,		// args: int predator_offset
	BF_SHAPE_GHOST    = 0x1000,		// args: void *ghost_table (is_translucent[256] + translucent[n][256])
	BF_SHAPE_PARTIAL  = 0x4000		// args: int partial_predator_value (0-255)
};

/*
** The fields of GraphicViewPortClass the renderer reads (the asm read them
** through GBUFFER.INC: GVPOffset, GVPWidth, GVPHeight, GVPXAdd, GVPPitch).
** One destination row is Width + XAdd + Pitch bytes long.
*/
struct ShapeViewType {
	unsigned char *Offset;		// first pixel of the viewport
	int Width;
	int Height;
	int XAdd;
	int Pitch;
};

/*
** Header the big shape buffer puts in front of each uncompressed frame
** (CODE/2KEYFRAM.CPP, typedef ShapeHeaderType). It is followed by one
** line-header byte per row (see Setup_Shape_Header). The layout must stay
** identical to the struct in 2KEYFRAM.CPP: Build_Frame writes it, the renderer
** reads it. With 32-bit pointers it is the 12-byte header the asm expected.
*/
struct ShapeFrameHeaderType {
	unsigned draw_flags;		// 0xFFFFFFFF = line headers not built yet
	char *shape_data;		// offset of the pixels from the buffer start
	int shape_buffer;		// 1 = TheaterShapeBufferStart, 0 = BigShapeBufferStart
};

/*
** Line-header bits (2KEYFBUF.ASM lines 209-214).
*/
enum {
	BLIT_TRANSPARENT = 1,
	BLIT_GHOST       = 2,
	BLIT_FADING      = 4,
	BLIT_PREDATOR    = 8,
	BLIT_SKIP        = 16,
	BLIT_ALL         = 31
};

/*
** Globals owned by the game (CODE/CONQUER.CPP and CODE/2KEYFRAM.CPP).
*/
extern "C" {
	extern bool UseOldShapeDraw;
	extern int UseBigShapeBuffer;			// BOOL in 2KEYFRAM.CPP
	extern char *BigShapeBufferStart;
	extern char *TheaterShapeBufferStart;
}

/*
** Buffer_Frame_To_Page with the viewport passed as plain fields and the
** optional arguments as a va_list. The optional arguments are read in this
** order, each only when its flag is set: SHAPE_GHOST (void *), SHAPE_FADING
** (void *, int), SHAPE_PREDATOR (int), SHAPE_PARTIAL (int).
** Always returns 0 (the asm returned the caller's EAX unchanged).
*/
long Buffer_Frame_To_View(int x, int y, int pixel_width, int pixel_height, void *src,
	ShapeViewType const &view, int flags, va_list args);

/*
** Builds the line headers of a big-shape-buffer frame (asm Setup_Shape_Header).
** headers points at the ShapeFrameHeaderType; the line bytes follow it.
*/
void Setup_Shape_Header(int pixel_width, int pixel_height, void const *src, void *headers,
	int flags, void const *translucent, void const *is_translucent);

#endif
