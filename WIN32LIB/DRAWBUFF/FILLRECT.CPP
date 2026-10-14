/*
**	Command & Conquer Red Alert(tm)
**	Copyright 2025 Electronic Arts Inc.
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

/***************************************************************************
 * FILLRECT.CPP -- C++ port of FILLRECT.ASM (session S07-A)                *
 *                                                                         *
 * Original: Phil Gorrow, June 7, 1994.                                    *
 *                                                                         *
 * Functions:                                                              *
 *   Buffer_Fill_Rect -- fill a clipped rectangle of a viewport            *
 ***************************************************************************/

#include "GVPORT.H"
#include <string.h>

/***************************************************************************
 * BUFFER_FILL_RECT -- Fills a rectangular region of a graphic viewport    *
 *                                                                         *
 * (x1, y1) and (x2, y2) are opposite corners, both INCLUSIVE, in either   *
 * order. Behaviour kept from the asm:                                     *
 *   - The coordinates were declared `ARG x1_pixel:WORD` and loaded with   *
 *     movsx, so only their low 16 bits count (sign-extended).             *
 *   - The corners are sorted, then clipped to the viewport. A rectangle   *
 *     that starts right of / below the viewport (x1 >= Width) or that has *
 *     no pixels left after clipping draws nothing.                        *
 ***************************************************************************/
extern "C" void GVP_CDECL Buffer_Fill_Rect(void *thisptr, int sx, int sy, int dx, int dy, unsigned char color)
{
	GVPLayout const *vp = (GVPLayout const *)thisptr;
	int const vpwidth = vp->Width;
	int const vpheight = vp->Height;

	int x1 = (short)sx;
	int y1 = (short)sy;
	int x2 = (short)dx;
	int y2 = (short)dy;

	/*
	** Convert the corners to a top-left position plus width and height.
	** (The asm swaps when x1 >= x2; swapping equal values changes nothing.)
	*/
	if (x1 >= x2) { int t = x1; x1 = x2; x2 = t; }
	int width = x2 - x1 + 1;
	if (y1 >= y2) { int t = y1; y1 = y2; y2 = t; }
	int height = y2 - y1 + 1;

	/*
	** Clip the left/top edge: `cmp eax,VPwidth / jge out / jb ok` -- a start at or past the right edge
	** (signed) rejects, a start inside the viewport (unsigned below) is fine, and a negative start is
	** moved to 0 with the width reduced to match.
	*/
	if (x1 >= vpwidth) return;
	if (!((unsigned)x1 < (unsigned)vpwidth)) {
		width += x1;
		x1 = 0;
	}
	if (y1 >= vpheight) return;
	if (!((unsigned)y1 < (unsigned)vpheight)) {
		height += y1;
		y1 = 0;
	}

	/*
	** Clip the right/bottom edge.
	*/
	int over = vpwidth - x1 - width;
	if (over < 0) width += over;
	over = vpheight - y1 - height;
	if (over < 0) height += over;

	/*
	** Last checks: zero size, or a "size" that is negative (it compares above the viewport size as
	** an unsigned number), means nothing is visible.
	*/
	if (width == 0 || height == 0) return;
	if ((unsigned)width > (unsigned)vpwidth) return;
	if ((unsigned)height > (unsigned)vpheight) return;

	unsigned char *dst = GVP_Pixel_Ptr(vp, x1, y1);
	long const bpr = GVP_Bytes_Per_Row(vp);

	/*
	** The asm merged all rows into a single run when the viewport had no row padding; filling row by
	** row writes exactly the same bytes.
	*/
	for (int row = 0; row < height; row++) {
		memset(dst, color, (size_t)width);
		dst += bpr;
	}
}
