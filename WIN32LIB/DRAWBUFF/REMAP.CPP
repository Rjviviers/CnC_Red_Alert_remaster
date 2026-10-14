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
 * REMAP.CPP -- C++ port of REMAP.ASM (session S07-A)                      *
 *                                                                         *
 * Original: Phil W. Gorrow, July 1, 1994.                                 *
 *                                                                         *
 * Functions:                                                              *
 *   Buffer_Remap -- pass a clipped rectangle through a 256-byte table     *
 ***************************************************************************/

#include "GVPORT.H"

/***************************************************************************
 * BUFFER_REMAP -- Remaps a rectangle of a graphic viewport                *
 *                                                                         *
 * Every pixel p in the rectangle (x0, y0, width, height) becomes          *
 * remap[p]. Nothing happens when `remap` is NULL.                         *
 *                                                                         *
 * Clipping (Cohen-Sutherland outcodes, see GVP_Outcode) as in the asm:    *
 *   - rejected when both corners are off the same side;                   *
 *   - x0/y0 below 0 are moved to 0, x1/y1 past Width/Height are moved to  *
 *     Width/Height (x1 = x0 + width is exclusive);                        *
 *   - anything with no width or height left draws nothing.                *
 ***************************************************************************/
extern "C" void GVP_CDECL Buffer_Remap(void *thisptr, int sx, int sy, int width, int height, void *remap)
{
	if (remap == NULL) return;

	GVPLayout const *vp = (GVPLayout const *)thisptr;
	unsigned char const *table = (unsigned char const *)remap;

	int x0 = sx;
	int y0 = sy;
	int x1 = (int)((unsigned)sx + (unsigned)width);
	int y1 = (int)((unsigned)sy + (unsigned)height);

	unsigned code0 = GVP_Outcode(x0, y0, vp->Width, vp->Height);
	unsigned code1 = GVP_Outcode(x1, y1, vp->Width, vp->Height);

	if (code0 & code1) return;						// trivially rejected
	if (code0 | code1) {
		if (code0 & 8) x0 = 0;						// left of the viewport
		if (code0 & 2) y0 = 0;						// above the viewport
		if (code1 & 4) x1 = vp->Width;				// past the right edge
		if (code1 & 1) y1 = vp->Height;				// past the bottom edge
	}

	int const w = x1 - x0;
	if (w <= 0) return;
	int rows = y1 - y0;
	if (rows <= 0) return;

	long const bpr = GVP_Bytes_Per_Row(vp);
	unsigned char *dst = GVP_Pixel_Ptr(vp, x0, y0);

	while (rows--) {
		for (int i = 0; i < w; i++) {
			dst[i] = table[dst[i]];
		}
		dst += bpr;
	}
}
