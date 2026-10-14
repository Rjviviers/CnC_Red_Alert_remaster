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
 * DRAWLINE.CPP -- C++ port of DRAWLINE.ASM (session S07-A)                *
 *                                                                         *
 * Original: Phil W. Gorrow, June 16, 1994; clipping fix August 30, 1994   *
 * [IML].                                                                  *
 *                                                                         *
 * Functions:                                                              *
 *   Buffer_Draw_Line -- clipped Bresenham line                            *
 ***************************************************************************/

#include "GVPORT.H"
#include <string.h>

namespace {

/*
** Clipping state. (ax, ay) and (bx, by) are the asm's EAX/EBX and ECX/EDX: the clip loop swaps them
** on every pass, and each clip routine moves the (ax, ay) end onto one window edge.
*/
struct LineClip {
	int min_x, max_x, min_y, max_y;		// inclusive clip window
	int ax, ay, bx, by;

	/* ??set_bits: 1 = above, 2 = below, 4 = left, 8 = right. */
	unsigned Code(int x, int y) const
	{
		unsigned code = 0;
		if (y < min_y) code |= 1;
		if (y > max_y) code |= 2;
		if (x < min_x) code |= 4;
		if (x > max_x) code |= 8;
		return code;
	}

	void Swap()
	{
		int t = ax; ax = bx; bx = t;
		t = ay; ay = by; by = t;
	}

	/*
	** The asm divided EDX:EAX by a 32-bit value with imul/idiv: a 64-bit product, truncated toward
	** zero. (A quotient that does not fit in 32 bits would have faulted; with viewport-sized
	** coordinates it cannot happen.)
	*/
	static int Mul_Div(int a, int b, int c)
	{
		return (int)(((long long)a * (long long)b) / (long long)c);
	}

	/* ??clip_vert: xa' = xa + (edge - ya) * (xb - xa) / (yb - ya); ya' = edge. */
	void Clip_Vert(int edge)
	{
		ax = ax + Mul_Div(edge - ay, bx - ax, by - ay);
		ay = edge;
	}

	/* ??clip_horiz: ya' = ya + (edge - xa) * (yb - ya) / (xb - xa); xa' = edge. */
	void Clip_Horiz(int edge)
	{
		ay = ay + Mul_Div(edge - ax, by - ay, bx - ax);
		ax = edge;
	}

	/*
	** ??clip_tbl. Returns true ("carry set") when it moved the (ax, ay) end. For corner regions the
	** asm picks one edge: up+left -> left, down+left -> down, up+right -> up, down+right -> right.
	** The down/right variants negate the coordinates first so they can share the up/left formulas;
	** with truncating division that gives exactly the same numbers, so it is written directly here.
	*/
	bool Clip_End(unsigned code)
	{
		switch (code) {
			case 1:  case 9:  Clip_Vert(min_y);  return true;	// ??a_up
			case 2:  case 6:  Clip_Vert(max_y);  return true;	// ??a_dwn
			case 4:  case 5:  Clip_Horiz(min_x); return true;	// ??a_lft
			case 8:  case 10: Clip_Horiz(max_x); return true;	// ??a_rgt
			default: return false;								// ??nada
		}
	}
};

}

/***************************************************************************
 * BUFFER_DRAW_LINE -- Draws a clipped line in a graphic viewport          *
 *                                                                         *
 * Draws from (sx, sy) to (dx, dy), both ends inclusive, in colour         *
 * `color`. The line is clipped against the viewport (0..Width-1,          *
 * 0..Height-1) with the asm's iterative Cohen-Sutherland loop, then drawn *
 * with its Bresenham variant:                                             *
 *   - horizontal and vertical lines are special cases;                    *
 *   - other lines are always drawn from the upper end downwards, so the   *
 *     pixels do not depend on the order of the end points;                *
 *   - the error accumulator starts at (major length) / 2 and the minor    *
 *     coordinate steps when it goes below zero.                           *
 ***************************************************************************/
extern "C" void GVP_CDECL Buffer_Draw_Line(void *thisptr, int sx, int sy, int dx, int dy, unsigned char color)
{
	GVPLayout const *vp = (GVPLayout const *)thisptr;
	long const bpr = GVP_Bytes_Per_Row(vp);

	LineClip c;
	c.min_x = 0;
	c.min_y = 0;
	c.max_x = vp->Width - 1;			// tested inclusively
	c.max_y = vp->Height - 1;
	c.ax = sx;
	c.ay = sy;
	c.bx = dx;
	c.by = dy;

	/*
	** Push the line into bounds, unless both ends are already inside.
	*/
	if (c.Code(c.ax, c.ay) | c.Code(c.bx, c.by)) {
		for (;;) {
			unsigned code_a = c.Code(c.ax, c.ay);
			c.Swap();
			unsigned code_b = c.Code(c.ax, c.ay);
			if ((code_a | code_b) == 0) break;		// ??on_screen
			if (code_a & code_b) return;			// ??off_screen
			if (c.Clip_End(code_b)) continue;		// clipped the end now in (ax, ay)
			c.Swap();
			c.Clip_End(code_a);
		}
	}

	int x0 = c.ax, y0 = c.ay, x1 = c.bx;
	int ylen = c.by - c.ay;

	/*
	** Horizontal line.
	*/
	if (ylen == 0) {
		if (x0 >= x1) { int t = x0; x0 = x1; x1 = t; }
		memset(GVP_Pixel_Ptr(vp, x0, y0), color, (size_t)(x1 - x0 + 1));
		return;
	}

	/*
	** Make the line go downwards: start at the upper end.
	*/
	if (ylen < 0) {
		ylen = -ylen;
		int t = x0; x0 = x1; x1 = t;
		y0 = y0 - ylen;
	}

	unsigned char *dst = GVP_Pixel_Ptr(vp, x0, y0);
	int xstep = 1;
	int xlen = x1 - x0;

	/*
	** Vertical line.
	*/
	if (xlen == 0) {
		for (int count = ylen + 1; count > 0; count--) {
			*dst = color;
			dst += bpr;
		}
		return;
	}

	if (xlen < 0) {
		xlen = -xlen;
		xstep = -1;
	}

	if (xlen >= ylen) {
		/*
		** Mostly horizontal: one pixel per column, step down when the accumulator underflows.
		*/
		int const greater = xlen, lesser = ylen;
		int accum = (int)((unsigned)greater >> 1);
		for (int count = greater; ; ) {
			*dst = color;
			if (--count < 0) break;
			dst += xstep;
			accum -= lesser;
			if (accum < 0) {
				accum += greater;
				dst += bpr;
			}
		}
	} else {
		/*
		** Mostly vertical: one pixel per row, step sideways when the accumulator underflows.
		*/
		int const greater = ylen, lesser = xlen;
		int accum = (int)((unsigned)greater >> 1);
		for (int count = greater; ; ) {
			*dst = color;
			if (--count < 0) break;
			dst += bpr;
			accum -= lesser;
			if (accum < 0) {
				accum += greater;
				dst += xstep;
			}
		}
	}
}
