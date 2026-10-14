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
 * SCALE.CPP -- C++ port of SCALE.ASM (session S07-A)                      *
 *                                                                         *
 * Original: Phil W. Gorrow, June 16, 1994; last update June 21, 1994.     *
 *                                                                         *
 * Functions:                                                              *
 *   Linear_Scale_To_Linear -- scaled copy between two viewports           *
 ***************************************************************************/

#include "GVPORT.H"

namespace {

/*
** imul + idiv: 64-bit product, quotient truncated toward zero. Returns false where the asm's idiv
** would have raised a divide fault (division by zero or a quotient that does not fit in 32 bits).
*/
bool Mul_Div(int a, int b, int c, int &result)
{
	if (c == 0) return false;
	long long q = ((long long)a * (long long)b) / (long long)c;
	if (q < -2147483647LL - 1 || q > 2147483647LL) return false;
	result = (int)q;
	return true;
}

}

/***************************************************************************
 * LINEAR_SCALE_TO_LINEAR -- Scales a viewport rectangle into another one  *
 *                                                                         *
 * INPUT:  thisptr, dest   source and destination viewports                *
 *         src_x, src_y, src_width, src_height   source rectangle          *
 *         dst_x, dst_y, dst_width, dst_height   destination rectangle     *
 *         trans           non-zero: source colour 0 is transparent        *
 *         remap           optional 256-byte table applied to the pixels   *
 *                                                                         *
 * OUTPUT: The asm preserved EAX (`USES eax`), so the value was undefined; *
 *         this returns 0. No caller uses the value.                       *
 *                                                                         *
 * Behaviour kept from the asm:                                            *
 *   - Nothing happens if any of the four sizes is 0.                      *
 *   - Clipping: the source rectangle is clipped against the source        *
 *     viewport and the matching destination edge is recomputed as        *
 *     dst + (edge - src) * dst_size / src_size (truncating division).     *
 *     Then the destination is clipped against the destination viewport   *
 *     and the source edge recomputed the same way.                        *
 *   - Stepping: per destination pixel the source advances by             *
 *     src_width / dst_width in 16.16 fixed point; the fraction starts at  *
 *     0 on every row. Per destination row the source advances by         *
 *     src_height / dst_height rows plus a Bresenham-style remainder       *
 *     whose accumulator starts at -dst_height.                            *
 *   - With trans, transparency is tested on the SOURCE pixel, before the *
 *     remap table is applied (so a colour remapped to 0 is still drawn).  *
 *   - The asm divided src_height (zero-extended into EDX:EAX) by          *
 *     dst_height and (src_width << 16) likewise by dst_width. A negative  *
 *     size would make that divide fault; here the call returns without   *
 *     drawing in that case.                                               *
 ***************************************************************************/
extern "C" int GVP_CDECL Linear_Scale_To_Linear(void *thisptr, void *dest, int src_x, int src_y, int dst_x, int dst_y,
									int src_width, int src_height, int dst_width, int dst_height, int trans, char *remap)
{
	GVPLayout const *src_vp = (GVPLayout const *)thisptr;
	GVPLayout const *dst_vp = (GVPLayout const *)dest;

	if (dst_width == 0 || dst_height == 0 || src_width == 0 || src_height == 0) return 0;

	int src_x0 = src_x;
	int src_y0 = src_y;
	int src_x1 = (int)((unsigned)src_x + (unsigned)src_width);
	int src_y1 = (int)((unsigned)src_y + (unsigned)src_height);
	int dst_x0 = dst_x;
	int dst_y0 = dst_y;
	int dst_x1 = (int)((unsigned)dst_x + (unsigned)dst_width);
	int dst_y1 = (int)((unsigned)dst_y + (unsigned)dst_height);
	int t;

	/*
	** Clip the source rectangle against the source viewport.
	*/
	unsigned code0 = GVP_Outcode(src_x0, src_y0, src_vp->Width, src_vp->Height);
	unsigned code1 = GVP_Outcode(src_x1, src_y1, src_vp->Width, src_vp->Height);
	if (code0 & code1) return 0;
	if (code0 | code1) {
		if (code0 & 8) {
			src_x0 = 0;
			if (!Mul_Div(0 - src_x, dst_width, src_width, t)) return 0;
			dst_x0 = t + dst_x;
		}
		if (code0 & 2) {
			src_y0 = 0;
			if (!Mul_Div(0 - src_y, dst_height, src_height, t)) return 0;
			dst_y0 = t + dst_y;
		}
		if (code1 & 4) {
			src_x1 = src_vp->Width;
			if (!Mul_Div(src_vp->Width - src_x, dst_width, src_width, t)) return 0;
			dst_x1 = t + dst_x;
		}
		if (code1 & 1) {
			src_y1 = src_vp->Height;
			if (!Mul_Div(src_vp->Height - src_y, dst_height, src_height, t)) return 0;
			dst_y1 = t + dst_y;
		}
	}

	/*
	** Clip the destination rectangle against the destination viewport. (src_x1/src_y1 are updated
	** like in the asm, although nothing reads them afterwards.)
	*/
	code0 = GVP_Outcode(dst_x0, dst_y0, dst_vp->Width, dst_vp->Height);
	code1 = GVP_Outcode(dst_x1, dst_y1, dst_vp->Width, dst_vp->Height);
	if (code0 & code1) return 0;
	if (code0 | code1) {
		if (code0 & 8) {
			dst_x0 = 0;
			if (!Mul_Div(0 - dst_x, src_width, dst_width, t)) return 0;
			src_x0 = t + src_x;
		}
		if (code0 & 2) {
			dst_y0 = 0;
			if (!Mul_Div(0 - dst_y, src_height, dst_height, t)) return 0;
			src_y0 = t + src_y;
		}
		if (code1 & 4) {
			dst_x1 = dst_vp->Width;
			if (!Mul_Div(dst_vp->Width - dst_x, src_width, dst_width, t)) return 0;
			src_x1 = t + src_x;
		}
		if (code1 & 1) {
			dst_y1 = dst_vp->Height;
			if (!Mul_Div(dst_vp->Height - dst_y, src_height, dst_height, t)) return 0;
			src_y1 = t + src_y;
		}
	}
	(void)src_x1;
	(void)src_y1;

	/*
	** Start addresses.
	*/
	long const src_win_width = GVP_Bytes_Per_Row(src_vp);
	long const dst_win_width = GVP_Bytes_Per_Row(dst_vp);
	unsigned char const *src_row = GVP_Pixel_Ptr(src_vp, src_x0, src_y0);
	unsigned char *dst_row = GVP_Pixel_Ptr(dst_vp, dst_x0, dst_y0);

	/*
	** Vertical step: whole source rows per destination row, plus a remainder.
	** `mov eax,[src_height] / xor edx,edx / idiv [dst_height]`: the dividend is src_height
	** zero-extended to 64 bits.
	*/
	long long const ydividend = (long long)(unsigned)src_height;
	long long const yq = ydividend / dst_height;
	if (yq < -2147483647LL - 1 || yq > 2147483647LL) return 0;		// the asm would fault
	long const dy_intr = (long)(int)((unsigned)(int)yq * (unsigned)src_win_width);
	int const dy_frac = (int)(ydividend % dst_height);
	int dy_acc = -dst_height;

	/*
	** Horizontal step in 16.16 fixed point: (src_width << 16) / dst_width, again zero-extended.
	** The integer part is added to the source pointer, the fraction (kept in the top 16 bits of a
	** 32-bit accumulator) adds one more source pixel whenever it carries out.
	*/
	long long const xdividend = (long long)(unsigned)((unsigned)src_width << 16);
	long long const xq = xdividend / dst_width;
	if (xq < -2147483647LL - 1 || xq > 2147483647LL) return 0;		// the asm would fault
	unsigned const xstep = (unsigned)(int)xq;
	unsigned const dx_intr = xstep >> 16;
	unsigned const dx_frac = xstep << 16;

	int const rows = dst_y1 - dst_y0;
	if (rows <= 0) return 0;
	int const width = dst_x1 - dst_x0;
	if (width <= 0) return 0;

	unsigned char const *table = (unsigned char const *)remap;

	for (int row = 0; row < rows; row++) {
		unsigned char const *src = src_row;
		unsigned char *dst = dst_row;
		unsigned frac = 0;

		for (int i = 0; i < width; i++) {
			unsigned char pixel = *src;
			if (trans) {
				if (pixel != 0) {
					*dst = table ? table[pixel] : pixel;
				}
			} else {
				*dst = table ? table[pixel] : pixel;
			}
			unsigned before = frac;
			frac += dx_frac;
			src += dx_intr + (frac < before ? 1 : 0);		// add ecx,eax / adc esi,edx
			dst++;
		}

		dst_row += dst_win_width;
		src_row += dy_intr;
		int acc = dy_acc + dy_frac;
		if (acc > 0) {
			src_row += src_win_width;
			acc -= dst_height;
		}
		dy_acc = acc;
	}
	return 0;
}
