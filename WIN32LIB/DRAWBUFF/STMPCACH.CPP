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
 * STMPCACH.CPP -- replaces STMPCACH.ASM (session S07-A, ADR-0016)         *
 *                                                                         *
 * Original: routines by "ST", November 1995 (the file header was copied   *
 * from STAMP.ASM and credits Joe L. Bostic).                              *
 *                                                                         *
 * STMPCACH.ASM was the lookup half of the DirectDraw video-memory icon    *
 * cache (ICONCACH.CPP is the other half): it copied terrain icons into    *
 * video-memory surfaces so the hardware blitter could draw them. The SDL  *
 * port has no video-memory surfaces, so the cache is removed (ADR-0016).  *
 *                                                                         *
 * This file keeps the two entry points that remaining code still calls,   *
 * so the library links until S08 deletes the rest of the cache:           *
 *   - Is_Icon_Cached always answers "not cached" (-1). The Draw_Stamp     *
 *     wrapper in GBUFFER.H then draws with Buffer_Draw_Stamp_Clip, which  *
 *     is what the original did for every icon it could not cache.        *
 *   - Cache_Copy_Icon is ported as is (a 24x24 copy into a surface with   *
 *     the given pitch); only ICONCACH.CPP calls it.                       *
 * Is_Stamp_Registered, IconData and IconEntry were only used inside       *
 * STMPCACH.ASM and are not kept.                                          *
 *                                                                         *
 * Once S08 has removed ICONCACH.CPP/.H and the cache branch of            *
 * GraphicViewPortClass::Draw_Stamp, delete this file.                     *
 ***************************************************************************/

#include "GVPORT.H"
#include <string.h>

/*
** Same declarations as ICONCACH.H. (Watcom register convention in the original; plain C linkage now.)
*/
extern "C" void Cache_Copy_Icon(void const *icon_ptr, void *surface, int pitch);
extern "C" int Is_Icon_Cached(void const *icon_data, int icon);

enum {
	STMPCACH_ICON_WIDTH = 24,
	STMPCACH_ICON_HEIGHT = 24
};

/***************************************************************************
 * CACHE_COPY_ICON -- copy a 24x24 icon to a surface                       *
 *                                                                         *
 * INPUT:  icon_ptr  24*24 bytes of icon data                              *
 *         surface   top-left of the destination                           *
 *         pitch     bytes from one destination row to the next            *
 ***************************************************************************/
extern "C" void Cache_Copy_Icon(void const *icon_ptr, void *surface, int pitch)
{
	unsigned char const *src = (unsigned char const *)icon_ptr;
	unsigned char *dst = (unsigned char *)surface;

	for (int row = 0; row < STMPCACH_ICON_HEIGHT; row++) {
		memcpy(dst, src, STMPCACH_ICON_WIDTH);
		src += STMPCACH_ICON_WIDTH;
		dst += pitch;
	}
}

/***************************************************************************
 * IS_ICON_CACHED -- has an icon been cached in video memory?              *
 *                                                                         *
 * OUTPUT: always -1 ("not cached"): the video-memory cache is removed.    *
 ***************************************************************************/
extern "C" int Is_Icon_Cached(void const *icon_data, int icon)
{
	(void)icon_data;
	(void)icon;
	return -1;
}
