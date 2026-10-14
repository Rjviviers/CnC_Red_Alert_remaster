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
 * CLEAR.CPP -- C++ port of CLEAR.ASM (session S07-A)                      *
 *                                                                         *
 * Original: Phil Gorrow, June 7, 1994; last update August 23, 1994 [SKB]. *
 *                                                                         *
 * Functions:                                                              *
 *   Buffer_Clear -- fill a whole viewport with one colour                 *
 ***************************************************************************/

#include "GVPORT.H"
#include <string.h>

/***************************************************************************
 * BUFFER_CLEAR -- Clears a graphic viewport                               *
 *                                                                         *
 * Fills Width bytes of every one of the Height rows; rows are             *
 * Width + XAdd + Pitch bytes apart. The asm's DWORD alignment was only a  *
 * speed trick and does not change the result.                             *
 *                                                                         *
 * Difference from the asm: with Height <= 0 the asm's `dec edx / jnz`     *
 * loop would wrap and run about 4 billion rows (a crash). Here nothing    *
 * is drawn. Width <= 0 draws nothing in both.                             *
 ***************************************************************************/
extern "C" void GVP_CDECL Buffer_Clear(void *thisptr, unsigned char color)
{
	GVPLayout const *vp = (GVPLayout const *)thisptr;
	unsigned char *dst = GVP_Base(vp);
	long const bpr = GVP_Bytes_Per_Row(vp);

	if (vp->Height <= 0 || vp->Width <= 0) return;

	for (int row = 0; row < vp->Height; row++) {
		memset(dst, color, (size_t)vp->Width);
		dst += bpr;
	}
}
