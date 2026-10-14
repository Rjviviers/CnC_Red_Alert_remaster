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
 **   C O N F I D E N T I A L --- W E S T W O O D   A S S O C I A T E S   **
 ***************************************************************************
 *                                                                         *
 *                 Project Name : Westwood 32 bit Library                  *
 *                                                                         *
 *                    File Name : WWMOUSE.CPP (was WWMOUSE.ASM)            *
 *                                                                         *
 *                   Programmer : Philip W. Gorrow (asm original)          *
 *                                                                         *
 *                   Start Date : December 12, 1995                        *
 *                                                                         *
 *-------------------------------------------------------------------------*
 * Portable C++ port of the software mouse cursor (session S07-C,          *
 * 2026-10-14). Called by WWMouseClass (WIN32LIB/KEYBOARD/MOUSE.CPP).      *
 *                                                                         *
 * Functions:                                                              *
 *   Mouse_Shadow_Buffer -- saves/restores the screen under the cursor     *
 *   Draw_Mouse -- draws the cursor with transparency and clipping         *
 *   ASM_Set_Mouse_Cursor -- unpacks a shape into the cursor buffer        *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

// Prototypes must match WIN32LIB/KEYBOARD/MOUSE.H, SHAPE.H and IFF.H
// (all extern "C", cdecl). The headers are not included because they still
// need windows.h (session S06).

#include <stddef.h>
#include <stdint.h>
#include <string.h>

namespace {

/*
** The first fields of WWMouseClass (WIN32LIB/KEYBOARD/MOUSE.H), in declaration
** order. The asm read them through STRUC MouseType (KEYBOARD/MOUSE.INC). The
** same member types in the same order give the same layout as the class under
** any packing. Do not reorder; the class layout must not change either.
*/
struct MouseLayout {
	char	*MouseCursor;		// cursor image, CursorWidth x CursorHeight bytes
	int		MouseXHot;
	int		MouseYHot;
	int		CursorWidth;
	int		CursorHeight;

	char	*MouseBuffer;
	int		MouseBuffX;
	int		MouseBuffY;
	int		MaxWidth;			// largest cursor the buffers can hold
	int		MaxHeight;

	int		MouseCXLeft;
	int		MouseCYUpper;
	int		MouseCXRight;
	int		MouseCYLower;
	char	MCFlags;
	char	MCCount;

	void	*Screen;			// GraphicViewPortClass *
	char	*PrevCursor;
};

/*
** The first fields of GraphicViewPortClass (GBUFFER.H / GBUFFER.INC).
*/
struct MouseViewPortLayout {
	long	Offset;
	int		Width;
	int		Height;
	int		XAdd;
	int		XPos;
	int		YPos;
	long	Pitch;
};

struct PackProbe { char c; int i; };

}	// namespace

// In the original configuration (32-bit, structs packed to 1 byte like
// Watcom /zp1) the mirrors must have exactly the MOUSE.INC/GBUFFER.INC offsets.
static_assert(sizeof(void *) != 4 || sizeof(PackProbe) != 5 ||
	(offsetof(MouseLayout, MaxWidth) == 32 && offsetof(MouseLayout, MaxHeight) == 36 &&
	 offsetof(MouseLayout, MCCount) == 57 && offsetof(MouseLayout, PrevCursor) == 62),
	"WWMouseClass mirror no longer matches KEYBOARD/MOUSE.INC");
static_assert(sizeof(void *) != 4 || offsetof(MouseViewPortLayout, Pitch) == 24,
	"GraphicViewPortClass mirror no longer matches GBUFFER.INC");

/*
** Shape header flags (SHAPE.INC).
*/
enum {
	MAKESHAPE_NORMAL	= 0,		// 256-color compressed shape
	MAKESHAPE_COMPACT	= 1,		// 16-color shape (built-in color table)
	MAKESHAPE_NOCOMP	= 2,		// non-wwcomped shape
	MAKESHAPE_VARIABLE	= 4			// not handled here (nor by the asm)
};

extern "C" {

// Shape library (WIN32LIB/SHAPE/GETSHAPE.CPP, SHAPE.ASM) and LCW decoder.
int Get_Shape_Uncomp_Size(void const *shape);
int Get_Shape_Width(void const *shape);
int Get_Shape_Original_Height(void const *shape);
unsigned long LCW_Uncompress(void *source, void *dest, unsigned long length);
extern char *_ShapeBuffer;		// the asm used the alias label ShapeBuffer (same storage)

void Mouse_Shadow_Buffer(void *thisptr, void *srcdst, void *buffer, int x, int y, int hotx, int hoty, int store);
void Draw_Mouse(void *thisptr, void *srcdst, int x, int y);
void *ASM_Set_Mouse_Cursor(void *thisptr, int hotspotx, int hotspoty, void *cursor);

}	// extern "C"


/*
** Outcode of a point, exactly as the asm built it with SHLD/XOR:
**   8 = x < 0, 4 = x > width, 2 = y < 0, 1 = y > height.
** "x > width" is computed as "x - width - 1 is not negative" in 32-bit
** wrapping arithmetic, so an end coordinate equal to the width is inside.
*/
static unsigned Clip_Code(int x, int y, int width, int height)
{
	unsigned code = 0;
	if (x < 0) code |= 8;
	if ((int32_t)((uint32_t)x - (uint32_t)width - 1u) >= 0) code |= 4;
	if (y < 0) code |= 2;
	if ((int32_t)((uint32_t)y - (uint32_t)height - 1u) >= 0) code |= 1;
	return code;
}


/*
** The clipping and addressing shared by Mouse_Shadow_Buffer and Draw_Mouse.
** (x0, y0) is the top-left of the cursor rectangle on screen, `image` the
** start of the cursor-sized buffer (stride = CursorWidth).
** Returns false if nothing is visible.
*/
namespace {
struct CursorSpan {
	unsigned char	*screen;		// first screen byte to touch
	unsigned char	*image;			// matching byte in the cursor-sized buffer
	int				width;			// visible columns
	int				height;			// visible rows
	int				screen_skip;	// screen pitch - width
	int				image_skip;		// CursorWidth - width
};
}

static bool Clip_Cursor(MouseViewPortLayout const *vp, int x0, int y0, int cursor_width, int cursor_height,
						unsigned char *image, CursorSpan &span)
{
	int x1 = x0 + cursor_width;
	int y1 = y0 + cursor_height;
	int buffx0 = 0;
	ptrdiff_t buffy0 = 0;

	unsigned start_code = Clip_Code(x0, y0, vp->Width, vp->Height);
	unsigned end_code = Clip_Code(x1, y1, vp->Width, vp->Height);
	if (start_code & end_code) {
		return false;					// entirely off one side
	}
	if (start_code | end_code) {
		if (start_code & 8) {			// off the left
			buffx0 = -x0;
			x0 = 0;
		}
		if (start_code & 2) {			// off the top
			buffy0 += (ptrdiff_t)(-y0) * cursor_width;
			y0 = 0;
		}
		if (end_code & 4) {				// off the right
			x1 = vp->Width;
		}
		if (end_code & 1) {				// off the bottom
			y1 = vp->Height;
		}
	}

	int pitch = vp->Width + vp->XAdd + (int)vp->Pitch;
	span.screen = (unsigned char *)(intptr_t)vp->Offset + (ptrdiff_t)y0 * pitch + x0;
	span.image = image + buffy0 + buffx0;

	span.width = x1 - x0;
	if (span.width <= 0) return false;
	span.height = y1 - y0;
	if (span.height <= 0) return false;

	span.screen_skip = pitch - span.width;
	span.image_skip = cursor_width - span.width;
	return true;
}


/***************************************************************************
 * MOUSE_SHADOW_BUFFER -- Handles storing and restoring the mouse buffer   *
 *                                                                         *
 * INPUT:	MouseClass *		- pointer to mouse class data                  *
 *		GraphicBufferClass *	- screen to draw the mouse on                  *
 *		void *buffer		- background buffer (CursorWidth stride)           *
 *		int x			- x position to store the mouse at                     *
 *		int y			- y position to store the mouse at                     *
 *		int hotx, hoty		- hot spot to subtract from x, y                   *
 *		int store		- 1: screen -> buffer; anything else: buffer ->        *
 *				  screen (the asm compares with exactly 1)                     *
 *                                                                         *
 * The buffer is laid out with the CURRENT CursorWidth as its stride, and  *
 * clipped parts keep their place in it, so a later restore with the same  *
 * x/y/hot spot puts back exactly what was saved.                          *
 *                                                                         *
 * HISTORY:                                                                *
 *   10/27/1994 PWG : Created (asm).                                       *
 *=========================================================================*/
void Mouse_Shadow_Buffer(void *thisptr, void *srcdst, void *buffer, int x, int y, int hotx, int hoty, int store)
{
	MouseLayout const *mouse = (MouseLayout const *)thisptr;
	MouseViewPortLayout const *vp = (MouseViewPortLayout const *)srcdst;

	CursorSpan span;
	if (!Clip_Cursor(vp, x - hotx, y - hoty, mouse->CursorWidth, mouse->CursorHeight,
					 (unsigned char *)buffer, span)) {
		return;
	}

	unsigned char *screen = span.screen;
	unsigned char *image = span.image;
	for (int row = 0; row < span.height; row++) {
		if (store == 1) {
			memcpy(image, screen, (size_t)span.width);
		} else {
			memcpy(screen, image, (size_t)span.width);
		}
		screen += span.width + span.screen_skip;
		image += span.width + span.image_skip;
	}
}


/***************************************************************************
 * DRAW_MOUSE -- Handles drawing the mouse cursor                          *
 *                                                                         *
 * INPUT:	MouseClass *		- pointer to mouse class data                  *
 *		GraphicBufferClass *	- screen to draw the mouse on                  *
 *		int x			- x position of the mouse (hot spot is subtracted)     *
 *		int y			- y position of the mouse (hot spot is subtracted)     *
 *                                                                         *
 * Colour 0 in the cursor image is transparent.                            *
 *                                                                         *
 * Kept from the asm: the visible width and height are counted in 8-bit    *
 * registers (AH and CL), so a visible size of exactly 256 behaves as 256  *
 * but larger sizes wrap. Cursors are far smaller (MaxWidth/MaxHeight).    *
 *                                                                         *
 * HISTORY:                                                                *
 *   10/27/1994 PWG : Created (asm).                                       *
 *=========================================================================*/
void Draw_Mouse(void *thisptr, void *srcdst, int mousex, int mousey)
{
	MouseLayout const *mouse = (MouseLayout const *)thisptr;
	MouseViewPortLayout const *vp = (MouseViewPortLayout const *)srcdst;

	CursorSpan span;
	if (!Clip_Cursor(vp, mousex - mouse->MouseXHot, mousey - mouse->MouseYHot,
					 mouse->CursorWidth, mouse->CursorHeight,
					 (unsigned char *)mouse->MouseCursor, span)) {
		return;
	}

	unsigned char *screen = span.screen;
	unsigned char const *image = span.image;
	unsigned char rows = (unsigned char)span.height;
	do {
		unsigned char cols = (unsigned char)span.width;
		do {
			unsigned char pixel = *image++;
			if (pixel != 0) {
				*screen = pixel;
			}
			screen++;
		} while (--cols != 0);
		image += span.image_skip;
		screen += span.screen_skip;
	} while (--rows != 0);
}


/*
** Un-RLEs an uncompressed (MAKESHAPE_NOCOMP) shape's pixel data: a non-zero
** byte is a pixel (remapped through the 16-byte table for compact shapes), a
** zero byte is followed by a count of transparent (0) pixels.
** `count` is width * height. As in the asm the loop runs until the running
** count is exactly 0, so malformed data can overrun the destination.
*/
static void Unpack_Cursor(unsigned char *dest, unsigned char const *source, unsigned char const *remap, int32_t count)
{
	for (;;) {
		unsigned char pixel = *source++;
		if (pixel != 0) {
			*dest++ = (remap != NULL) ? remap[pixel] : pixel;
			if (--count == 0) break;
		} else {
			unsigned char zeros = *source++;
			count -= zeros;
			memset(dest, 0, zeros);
			dest += zeros;
			if (count == 0) break;
		}
	}
}


/***************************************************************************
 * SET_MOUSE_CURSOR -- Sets the shape to be used as the mouse.             *
 *                                                                         *
 *    This will inform the system of the shape desired as the mouse        *
 *    cursor.                                                              *
 *                                                                         *
 * INPUT:    xhotspot    -- Offset of click point into shape.              *
 *                                                                         *
 *           yhotspot    -- Offset of click point into shape.              *
 *                                                                         *
 *           cursor      -- Shape to use as the new mouse shape.           *
 *                                                                         *
 * OUTPUT:   Returns the PREVIOUS value of PrevCursor and stores `cursor`  *
 *           in PrevCursor. This happens even when the shape is too big    *
 *           (wider than MaxWidth or taller than MaxHeight), in which case *
 *           nothing else changes.                                         *
 *                                                                         *
 * A compressed shape is first staged in _ShapeBuffer: the type word (with *
 * MAKESHAPE_NOCOMP added), the rest of the 10-byte header, the 16-byte    *
 * colour table for compact shapes, then the LCW-decoded data.             *
 *                                                                         *
 * HISTORY:                                                                *
 *   02/13/1992 JLB : Created (asm).                                       *
 *=========================================================================*/
void *ASM_Set_Mouse_Cursor(void *thisptr, int xhotspot, int yhotspot, void *cursor)
{
	MouseLayout *mouse = (MouseLayout *)thisptr;

	int datasize = Get_Shape_Uncomp_Size(cursor);
	int swidth = Get_Shape_Width(cursor);
	if (swidth <= mouse->MaxWidth) {
		int sheight = Get_Shape_Original_Height(cursor);
		if (sheight <= mouse->MaxHeight) {
			unsigned char const *shape = (unsigned char const *)cursor;
			unsigned stype = (unsigned)shape[0] | ((unsigned)shape[1] << 8);

			if (!(stype & MAKESHAPE_NOCOMP)) {
				/*
				** Compressed: stage an uncompressed copy in the shape buffer.
				*/
				unsigned char *staged = (unsigned char *)_ShapeBuffer;
				stype |= MAKESHAPE_NOCOMP;
				staged[0] = (unsigned char)stype;
				staged[1] = (unsigned char)(stype >> 8);

				size_t header = (stype & MAKESHAPE_COMPACT) ? 8 + 16 : 8;	// rest of header (+ colour table)
				memcpy(staged + 2, shape + 2, header);
				LCW_Uncompress((void *)(shape + 2 + header), staged + 2 + header, (unsigned long)datasize);
				shape = staged;
			}

			unsigned char const *remap = NULL;
			unsigned char const *data = shape + 10;
			if (stype & MAKESHAPE_COMPACT) {
				remap = shape + 10;
				data = shape + 10 + 16;
			}
			Unpack_Cursor((unsigned char *)mouse->MouseCursor, data, remap,
						  (int32_t)((uint32_t)swidth * (uint32_t)sheight));

			mouse->MouseXHot = xhotspot;
			mouse->MouseYHot = yhotspot;
			mouse->CursorHeight = sheight;
			mouse->CursorWidth = swidth;
		}
	}

	char *previous = mouse->PrevCursor;
	mouse->PrevCursor = (char *)cursor;
	return previous;
}
