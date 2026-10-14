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
 * BITBLIT.CPP -- C++ port of BITBLIT.ASM (session S07-A)                  *
 *                                                                         *
 * Original: Julio R. Jerez, Feb 6, 1995.                                  *
 *                                                                         *
 * Functions:                                                              *
 *   Linear_Blit_To_Linear -- copy a rectangle between two viewports       *
 ***************************************************************************/

#include "GVPORT.H"
#include <stdint.h>
#include <string.h>

/***************************************************************************
 * LINEAR_BLIT_TO_LINEAR -- Copies a rectangle from one viewport to another*
 *                                                                         *
 * INPUT:  thisptr       source viewport                                   *
 *         dest          destination viewport                              *
 *         x_pixel/y_pixel   top-left of the rectangle in the source       *
 *         dest_x0/dest_y0   top-left of the rectangle in the destination  *
 *         pixel_width/height  size of the rectangle                       *
 *         trans         if bit 0 is set, colour 0 in the source is        *
 *                       transparent (not copied)                          *
 *                                                                         *
 * OUTPUT: The asm left EAX undefined; this returns 0 (FALSE). No caller   *
 *         uses the value.                                                 *
 *                                                                         *
 * Behaviour kept from the asm (Cohen-Sutherland clipping, see GVPORT.H):  *
 *   1. The source rectangle is clipped against the source viewport. This  *
 *      does NOT move the destination: if x_pixel < 0 is clipped to 0, the *
 *      pixels that were at x = 0 land at dest_x0, i.e. the image shifts.  *
 *   2. The destination rectangle (dest_x0 + clipped source size) is then  *
 *      clipped against the destination viewport; this does keep source   *
 *      and destination aligned.                                           *
 *   3. Overlap: when source and destination start at the same address     *
 *      nothing is copied. When the source starts at a lower address the   *
 *      copy runs backwards (last row first, right to left), otherwise     *
 *      forwards, so overlapping blits within one buffer behave like       *
 *      memmove. (The asm compared the two addresses as signed numbers;    *
 *      that only differs from this unsigned compare for a buffer that      *
 *      straddles address 0x80000000.)                                     *
 ***************************************************************************/
extern "C" int GVP_CDECL Linear_Blit_To_Linear(void *thisptr, void *dest, int x_pixel, int y_pixel, int dest_x0,
									int dest_y0, int pixel_width, int pixel_height, int trans)
{
	GVPLayout const *src_vp = (GVPLayout const *)thisptr;
	GVPLayout const *dst_vp = (GVPLayout const *)dest;

	/*
	** Clip the source rectangle against the source viewport.
	*/
	int x1_pixel = (int)((unsigned)x_pixel + (unsigned)pixel_width);
	int y1_pixel = (int)((unsigned)y_pixel + (unsigned)pixel_height);

	unsigned code0 = GVP_Outcode(x_pixel, y_pixel, src_vp->Width, src_vp->Height);
	unsigned code1 = GVP_Outcode(x1_pixel, y1_pixel, src_vp->Width, src_vp->Height);
	if (code0 & code1) return 0;
	if (code0 | code1) {
		if (code0 & 8) x_pixel = 0;
		if (code0 & 2) y_pixel = 0;
		if (code1 & 4) x1_pixel = src_vp->Width;
		if (code1 & 1) y1_pixel = src_vp->Height;
	}

	/*
	** Build the destination rectangle from the (clipped) source size and clip it against the
	** destination viewport, moving the source edges by the same amounts.
	*/
	int dest_x1 = dest_x0 - x_pixel + x1_pixel;
	int dest_y1 = dest_y0 - y_pixel + y1_pixel;

	code0 = GVP_Outcode(dest_x0, dest_y0, dst_vp->Width, dst_vp->Height);
	code1 = GVP_Outcode(dest_x1, dest_y1, dst_vp->Width, dst_vp->Height);
	if (code0 & code1) return 0;
	if (code0 | code1) {
		if (code0 & 8) {
			x_pixel -= dest_x0;
			dest_x0 = 0;
		}
		if (code0 & 2) {
			y_pixel -= dest_y0;
			dest_y0 = 0;
		}
		if (code1 & 4) {
			x1_pixel -= dest_x1 - dst_vp->Width;
			dest_x1 = dst_vp->Width;
		}
		if (code1 & 1) {
			y1_pixel -= dest_y1 - dst_vp->Height;
			dest_y1 = dst_vp->Height;
		}
	}

	/*
	** Work out the addresses and sizes.
	*/
	long const src_bpr = GVP_Bytes_Per_Row(src_vp);
	long const dst_bpr = GVP_Bytes_Per_Row(dst_vp);
	unsigned char *src = GVP_Pixel_Ptr(src_vp, x_pixel, y_pixel);
	unsigned char *dst = GVP_Pixel_Ptr(dst_vp, dest_x0, dest_y0);

	int const width = dest_x1 - dest_x0;
	if (width <= 0) return 0;
	int const height = dest_y1 - dest_y0;
	if (height <= 0) return 0;

	/*
	** The source pointer advances by its own row length minus the copied width; the asm took that
	** width from the source edges (x1_pixel - x_pixel), which always equals `width` here.
	*/
	long const src_skip = src_bpr - (x1_pixel - x_pixel);
	long const dst_skip = dst_bpr - width;

	if (src == dst) return 0;

	if ((uintptr_t)src > (uintptr_t)dst) {
		/*
		** Forward blit: top row first, left to right.
		*/
		if (trans & 1) {
			for (int row = 0; row < height; row++) {
				for (int i = 0; i < width; i++) {
					unsigned char pixel = *src++;
					if (pixel) *dst = pixel;
					dst++;
				}
				src += src_skip;
				dst += dst_skip;
			}
		} else {
			for (int row = 0; row < height; row++) {
				memmove(dst, src, (size_t)width);	// rows of one buffer may overlap
				src += width + src_skip;
				dst += width + dst_skip;
			}
		}
	} else {
		/*
		** Backward blit: bottom row first, right to left. Rows are src_bpr/dst_bpr apart.
		*/
		unsigned char *src_row = src + (ptrdiff_t)(height - 1) * src_bpr;
		unsigned char *dst_row = dst + (ptrdiff_t)(height - 1) * dst_bpr;
		for (int row = height - 1; row >= 0; row--) {
			if (trans & 1) {
				for (int i = width - 1; i >= 0; i--) {
					unsigned char pixel = src_row[i];
					if (pixel) dst_row[i] = pixel;
				}
			} else {
				memmove(dst_row, src_row, (size_t)width);
			}
			src_row -= src_bpr;
			dst_row -= dst_bpr;
		}
	}
	return 0;
}
