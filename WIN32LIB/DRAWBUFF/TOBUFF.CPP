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
 * TOBUFF.CPP -- C++ port of TOBUFF.ASM (session S07-A)                    *
 *                                                                         *
 * Original: Phil W. Gorrow, June 8, 1994; last update Feb 10, 1995 [jrj]. *
 *                                                                         *
 * Functions:                                                              *
 *   Buffer_To_Buffer -- copy a viewport rectangle into a linear buffer    *
 ***************************************************************************/

#include "GVPORT.H"
#include <string.h>

/***************************************************************************
 * BUFFER_TO_BUFFER -- Copies a viewport rectangle to a linear buffer      *
 *                                                                         *
 * INPUT:  thisptr       source viewport                                   *
 *         x, y, w, h    rectangle in the viewport                         *
 *         buff          destination buffer; it is laid out as w bytes per *
 *                       row (the REQUESTED width, not the clipped one)    *
 *         size          size of buff in bytes                             *
 *                                                                         *
 * OUTPUT: The asm returned whatever was left in EAX; this returns 0.      *
 *         No caller uses the value.                                       *
 *                                                                         *
 * Behaviour kept from the asm:                                            *
 *   - The rectangle is clipped against the viewport. Pixels clipped off   *
 *     the left/top are skipped in the buffer too: the copy starts at      *
 *     buffer offset (clipped_top * w + clipped_left), so every pixel      *
 *     lands where it would have without clipping. Bytes for clipped-off   *
 *     pixels are left untouched.                                          *
 *   - The copy is refused if w * (clipped height) > size (signed          *
 *     compare). The left/top skip is not counted in that test.            *
 ***************************************************************************/
extern "C" long GVP_CDECL Buffer_To_Buffer(void *thisptr, int x, int y, int w, int h, void *buff, long size)
{
	GVPLayout const *vp = (GVPLayout const *)thisptr;

	int dest_x1 = 0;		// pixels clipped off the left edge (the asm reused these names)
	int dest_y1 = 0;		// rows clipped off the top edge
	int x1 = (int)((unsigned)x + (unsigned)w);
	int y1 = (int)((unsigned)y + (unsigned)h);

	unsigned code0 = GVP_Outcode(x, y, vp->Width, vp->Height);
	unsigned code1 = GVP_Outcode(x1, y1, vp->Width, vp->Height);
	if (code0 & code1) return 0;
	if (code0 | code1) {
		if (code0 & 8) {
			dest_x1 = -x;
			x = 0;
		}
		if (code0 & 2) {
			dest_y1 = -y;
			y = 0;
		}
		if (code1 & 4) x1 = vp->Width;
		if (code1 & 1) y1 = vp->Height;
	}

	unsigned char const *src = GVP_Pixel_Ptr(vp, x, y);
	long const src_bpr = GVP_Bytes_Per_Row(vp);
	unsigned char *dst = (unsigned char *)buff + ((ptrdiff_t)dest_y1 * w + dest_x1);

	int const rows = y1 - y;
	if (rows <= 0) return 0;
	int const width = x1 - x;
	if (width <= 0) return 0;

	if ((int)((unsigned)w * (unsigned)rows) > size) return 0;	// 32-bit imul, signed compare

	for (int row = 0; row < rows; row++) {
		memmove(dst, src, (size_t)width);
		src += src_bpr;
		dst += w;
	}
	return 0;
}
