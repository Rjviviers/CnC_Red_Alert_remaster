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
 * PUTPIX.CPP -- C++ port of PUTPIX.ASM (session S07-A)                    *
 *                                                                         *
 * Original: Phil Gorrow, June 8, 1994.                                    *
 *                                                                         *
 * Functions:                                                              *
 *   Buffer_Put_Pixel -- write one pixel of a viewport                     *
 ***************************************************************************/

#include "GVPORT.H"

/***************************************************************************
 * BUFFER_PUT_PIXEL -- Puts a pixel on a graphic viewport                  *
 *                                                                         *
 * Writes `color` at (x, y). Out-of-range coordinates (including negative  *
 * ones: the asm compares unsigned) are silently ignored.                  *
 ***************************************************************************/
extern "C" void GVP_CDECL Buffer_Put_Pixel(void *thisptr, int x, int y, unsigned char color)
{
	GVPLayout const *vp = (GVPLayout const *)thisptr;

	if ((unsigned)x >= (unsigned)vp->Width) return;
	if ((unsigned)y >= (unsigned)vp->Height) return;
	*GVP_Pixel_Ptr(vp, x, y) = color;
}
