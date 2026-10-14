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
 * TOPAGE.CPP -- C++ port of TOPAGE.ASM (session S07-A)                    *
 *                                                                         *
 * Original: Phil W. Gorrow, June 8, 1994; last update June 15, 1994.      *
 *                                                                         *
 * Functions:                                                              *
 *   Buffer_To_Page -- copy a linear buffer into a viewport rectangle      *
 ***************************************************************************/

#include "GVPORT.H"
#include <string.h>

/***************************************************************************
 * BUFFER_TO_PAGE -- Copies a linear buffer to a viewport                  *
 *                                                                         *
 * INPUT:  x, y, w, h    destination rectangle in the viewport             *
 *         Buffer        source, laid out as w bytes per row               *
 *         view          destination viewport                              *
 *                                                                         *
 * OUTPUT: The asm preserved EAX (`USES eax`), so the caller saw its own   *
 *         garbage; this returns 0. No caller uses the value.              *
 *                                                                         *
 * Behaviour kept from the asm:                                            *
 *   - Nothing happens when Buffer is NULL.                                *
 *   - The rectangle is clipped against the viewport; source pixels that   *
 *     were clipped off the left/top are skipped (the copy starts at       *
 *     buffer offset clipped_top * w + clipped_left).                      *
 *   - Unlike Buffer_To_Buffer there is no buffer size check, and colour 0 *
 *     is copied (the transparent variant was assembled out: TRANSP = 0).  *
 ***************************************************************************/
extern "C" long GVP_CDECL Buffer_To_Page(int x, int y, int w, int h, void *Buffer, void *view)
{
	if (Buffer == NULL) return 0;

	GVPLayout const *vp = (GVPLayout const *)view;

	int scr_x = 0;
	int scr_y = 0;
	int x1 = (int)((unsigned)x + (unsigned)w);
	int y1 = (int)((unsigned)y + (unsigned)h);

	unsigned code0 = GVP_Outcode(x, y, vp->Width, vp->Height);
	unsigned code1 = GVP_Outcode(x1, y1, vp->Width, vp->Height);
	if (code0 & code1) return 0;
	if (code0 | code1) {
		if (code0 & 8) {
			scr_x = -x;
			x = 0;
		}
		if (code0 & 2) {
			scr_y = -y;
			y = 0;
		}
		if (code1 & 4) x1 = vp->Width;
		if (code1 & 1) y1 = vp->Height;
	}

	unsigned char *dst = GVP_Pixel_Ptr(vp, x, y);
	long const dst_bpr = GVP_Bytes_Per_Row(vp);
	unsigned char const *src = (unsigned char const *)Buffer + ((ptrdiff_t)scr_y * w + scr_x);

	int const rows = y1 - y;
	if (rows <= 0) return 0;
	int const width = x1 - x;
	if (width <= 0) return 0;

	for (int row = 0; row < rows; row++) {
		memmove(dst, src, (size_t)width);
		src += w;
		dst += dst_bpr;
	}
	return 0;
}
