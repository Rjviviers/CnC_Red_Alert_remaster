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
 * CLIPRECT.CPP -- C++ port of CLIPRECT.ASM (session S07-A)                *
 *                                                                         *
 * Original: Julio R Jerez, Mar 2, 1995.                                   *
 *                                                                         *
 * Functions:                                                              *
 *   Clip_Rect -- clip a rectangle against a window                        *
 *   Confine_Rect -- move a rectangle so that it lies inside a window      *
 *                                                                         *
 * Pure integer code: results are bit-exact with the asm. The game uses    *
 * them for the tactical view and radar positions (per-client view state,  *
 * not simulation state).                                                  *
 ***************************************************************************/

#if defined(_MSC_VER) || defined(__MINGW32__)
#define CLIPRECT_CDECL __cdecl
#else
#define CLIPRECT_CDECL
#endif

/* Same declarations as WIN32LIB/INCLUDE/MISC.H (which needs <windows.h>, so it is not included). */
extern "C" int CLIPRECT_CDECL Clip_Rect(int *x, int *y, int *dw, int *dh, int width, int height);
extern "C" int CLIPRECT_CDECL Confine_Rect(int *x, int *y, int dw, int dh, int width, int height);

/* Sign bit of a - b with 32-bit wrap-around, like the asm's sub + shld. */
static inline unsigned Sign_Of_Diff(int a, int b)
{
	return ((unsigned)a - (unsigned)b) >> 31;
}

/***************************************************************************
 * CLIP_RECT -- clip a given rectangle against a given window              *
 *                                                                         *
 * INPUT:  *x, *y, *dw, *dh   rectangle (updated in place when clipped)    *
 *         width, height      size of the clipping window at (0, 0)        *
 *                                                                         *
 * OUTPUT:  0  rectangle entirely inside, nothing changed                  *
 *         -1  rectangle entirely outside, or nothing left after clipping  *
 *          1  rectangle was clipped                                       *
 *                                                                         *
 * Kept from the asm:                                                      *
 *   - The right/bottom tests treat x + dw == width as inside (the edge    *
 *     test is "x > width", not "x >= width").                             *
 *   - A start past the right/bottom edge with the far corner inside (a    *
 *     negative dw/dh) is neither rejected nor changed, and returns 1.     *
 *   - When the right clip leaves no width it returns -1 at once, after    *
 *     storing the new *dw and without looking at the bottom edge.         *
 ***************************************************************************/
extern "C" int CLIPRECT_CDECL Clip_Rect(int *x, int *y, int *dw, int *dh, int width, int height)
{
	int x0 = *x;
	int y0 = *y;
	int x1 = (int)((unsigned)x0 + (unsigned)*dw);
	int y1 = (int)((unsigned)y0 + (unsigned)*dh);
	int wplus1 = (int)((unsigned)width + 1);
	int hplus1 = (int)((unsigned)height + 1);

	/* bit 3: x < 0, bit 2: x > width, bit 1: y < 0, bit 0: y > height */
	unsigned code0 = (((unsigned)x0 >> 31) << 3) | (Sign_Of_Diff(x0, wplus1) << 2) |
					 (((unsigned)y0 >> 31) << 1) | Sign_Of_Diff(y0, hplus1);
	unsigned code1 = (((unsigned)x1 >> 31) << 3) | (Sign_Of_Diff(x1, wplus1) << 2) |
					 (((unsigned)y1 >> 31) << 1) | Sign_Of_Diff(y1, hplus1);
	code0 ^= 5;
	code1 ^= 5;

	if (code0 & code1) return -1;
	if ((code0 | code1) == 0) return 0;

	if (code0 & 8) {				// left edge
		int old = *x;
		*x = 0;
		*dw += old;
	}
	if (code0 & 2) {				// top edge
		int old = *y;
		*y = 0;
		*dh += old;
	}
	if (code1 & 4) {				// right edge
		*dw = width - *x;
		if (*dw <= 0) return -1;
	}
	if (code1 & 1) {				// bottom edge
		*dh = height - *y;
		if (*dh <= 0) return -1;
	}
	return 1;
}

/***************************************************************************
 * CONFINE_RECT -- move a rectangle so that it lies inside a window        *
 *                                                                         *
 * INPUT:  *x, *y     position (updated in place)                          *
 *         dw, dh     size of the rectangle                                *
 *         width, height  size of the window at (0, 0)                     *
 *                                                                         *
 * OUTPUT: 0 if the position was already inside on both axes, 1 if either  *
 *         axis was "adjusted".                                            *
 *                                                                         *
 * Per axis: inside means x > 0 and x + dw <= width. Otherwise x <= 0      *
 * moves to 0, and x + dw > width moves to width - dw.                     *
 *                                                                         *
 * Kept from the asm: x == 0 counts as NOT inside (the test is "-x is      *
 * negative"), so a rectangle at the window's left/top edge reports 1      *
 * although its position does not change. CODE/DISPLAY.CPP:1206 stores    *
 * this result in `shifted`. As the original comment says, a rectangle     *
 * bigger than the window gives a meaningless result.                      *
 ***************************************************************************/
extern "C" int CLIPRECT_CDECL Confine_Rect(int *x, int *y, int dw, int dh, int width, int height)
{
	int result = 0;

	int neg = (int)(0u - (unsigned)*x);										// -x
	int over = (int)((unsigned)*x + (unsigned)dw - (unsigned)width - 1);	// x + dw - width - 1
	if ((neg & over) >= 0) {		// not (x > 0 and x + dw <= width)
		result = 1;
		if (neg >= 0) {
			*x = 0;
		} else {
			*x -= over + 1;
		}
	}

	neg = (int)(0u - (unsigned)*y);
	over = (int)((unsigned)*y + (unsigned)dh - (unsigned)height - 1);
	if ((neg & over) >= 0) {
		result = 1;
		if (neg >= 0) {
			*y = 0;
		} else {
			*y -= over + 1;
		}
	}
	return result;
}
