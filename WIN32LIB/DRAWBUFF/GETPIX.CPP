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
 * GETPIX.CPP -- C++ port of GETPIX.ASM (session S07-A)                    *
 *                                                                         *
 * Original: Phil Gorrow, June 7, 1994.                                    *
 *                                                                         *
 * Functions:                                                              *
 *   Buffer_Get_Pixel -- read one pixel of a viewport                      *
 ***************************************************************************/

#include "GVPORT.H"

/***************************************************************************
 * BUFFER_GET_PIXEL -- Gets a pixel from a graphic viewport                *
 *                                                                         *
 * Returns the pixel (0-255) at (x, y).                                    *
 *                                                                         *
 * Bounds are tested with UNSIGNED compares (`jae`), so negative           *
 * coordinates are out of bounds too.                                      *
 *                                                                         *
 * Out-of-bounds return value (kept from the asm): the routine exits with  *
 * whatever was in EAX, which is the rejected coordinate itself: `x` when  *
 * x is out of range, otherwise `y`. No caller relies on it.               *
 ***************************************************************************/
extern "C" int GVP_CDECL Buffer_Get_Pixel(void *thisptr, int x, int y)
{
	GVPLayout const *vp = (GVPLayout const *)thisptr;

	if ((unsigned)x >= (unsigned)vp->Width) return x;
	if ((unsigned)y >= (unsigned)vp->Height) return y;
	return *GVP_Pixel_Ptr(vp, x, y);
}
