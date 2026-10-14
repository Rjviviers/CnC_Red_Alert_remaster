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
 * STAMP.CPP -- C++ port of STAMP.ASM (session S07-A)                      *
 *                                                                         *
 * Original: Joe L. Bostic, August 23, 1993.                               *
 *                                                                         *
 * Draws one icon ("stamp": a terrain template tile) from an icon set.     *
 * An icon set starts with an IControl_Type header (TILE.H; the asm's      *
 * STAMP.INC); the offsets in it are relative to the start of the set.     *
 *                                                                         *
 * Functions:                                                              *
 *   Init_Stamps -- cache the header fields of an icon set                 *
 *   Buffer_Draw_Stamp -- draw an icon, no clipping                        *
 *   Buffer_Draw_Stamp_Clip -- draw an icon clipped to a window            *
 ***************************************************************************/

#include "GVPORT.H"
#include "TILE.H"
#include <string.h>

/*
** Header fields of the last icon set seen (the asm's DATASEG). LastIconset, MapPtr, IconCount,
** IconSize and StampPtr were exported because STMPCACH.ASM read them.
*/
extern "C" {
	void const			*LastIconset = NULL;	// last icon set initialised
	unsigned char const	*StampPtr = NULL;		// icon image data
	unsigned char const	*MapPtr = NULL;			// logical -> physical icon map
	int					IconCount = 0;			// number of icons in the set
	int					IconSize = 0;			// bytes per icon (width * height)
}
static unsigned char const	*IsTrans = NULL;	// per-icon "has transparent pixels" flags
static int					IconWidth = 0;
static int					IconHeight = 0;

/***************************************************************************
 * INIT_STAMPS -- Records the layout of an icon set                        *
 *                                                                         *
 * Does nothing for NULL or for the set already recorded.                  *
 *                                                                         *
 * Note: MapPtr is always set to icondata + Map, even when the set has no  *
 * map (Map == 0); the draw routines therefore always translate the icon   *
 * number through it. RA's template sets always have a map.                *
 ***************************************************************************/
extern "C" void GVP_CDECL Init_Stamps(void const *icondata)
{
	if (icondata == NULL) return;
	if (LastIconset == icondata) return;
	LastIconset = icondata;

	IControl_Type const *header = (IControl_Type const *)icondata;
	unsigned char const *base = (unsigned char const *)icondata;

	IconCount = (unsigned short)header->Count;		// movzx: the fields are WORDs
	IconWidth = (unsigned short)header->Width;
	IconHeight = (unsigned short)header->Height;
	IconSize = IconWidth * IconHeight;
	MapPtr = base + header->Map;
	StampPtr = base + header->Icons;
	IsTrans = base + header->TransFlag;
}

/*
** Shared front end of both draw routines: set up the icon set and translate the icon number.
** Returns false if the icon is not in the set.
**
** Kept from the asm: the map lookup is `mov bl,[edi+ebx]`, which replaces only the LOW BYTE of the
** icon number. For the icon numbers RA uses (< 256) that is a plain map[icon] lookup.
*/
static bool Stamp_Lookup(void const *icondata, int &icon)
{
	Init_Stamps(icondata);
	unsigned number = (unsigned)icon;
	if (MapPtr != NULL) {
		number = (number & ~0xFFu) | MapPtr[number];
	}
	if (number >= (unsigned)IconCount) return false;		// unsigned compare (jae)
	icon = (int)number;
	return true;
}

/***************************************************************************
 * BUFFER_DRAW_STAMP -- Draws an icon at (x_pixel, y_pixel), no clipping   *
 *                                                                         *
 * Three ways to draw, as in the asm:                                      *
 *   - remap != NULL: every pixel goes through the 256-byte remap table,   *
 *     and pixels that are 0 AFTER the remap are skipped (so a remap can   *
 *     make colours invisible);                                            *
 *   - else, if the icon's IsTrans flag is set: pixels that are 0 are      *
 *     skipped;                                                            *
 *   - else a plain copy. The asm copied 4 rows and Width/4 DWORDs at a    *
 *     time: only (Height/4)*4 rows and (Width/4)*4 bytes per row are      *
 *     copied, and the source only advances by the bytes copied. RA's     *
 *     icons are 24x24, where this is the whole icon. (For Height < 4 the  *
 *     asm loop would wrap around and crash; here nothing is drawn.)       *
 ***************************************************************************/
extern "C" void GVP_CDECL Buffer_Draw_Stamp(void const *thisptr, void const *icondata, int icon, int x_pixel,
								int y_pixel, void const *remap)
{
	if (icondata == NULL) return;
	if (!Stamp_Lookup(icondata, icon)) return;

	GVPLayout const *vp = (GVPLayout const *)thisptr;
	long const bpr = GVP_Bytes_Per_Row(vp);
	long const modulo = bpr - IconWidth;
	unsigned char *dst = GVP_Pixel_Ptr(vp, x_pixel, y_pixel);
	unsigned char const *src = StampPtr + (size_t)((unsigned)icon * (unsigned)IconSize);

	if (remap != NULL) {
		unsigned char const *table = (unsigned char const *)remap;
		for (int row = 0; row < IconHeight; row++) {
			for (int col = 0; col < IconWidth; col++) {
				unsigned char pixel = table[*src++];
				if (pixel) *dst = pixel;
				dst++;
			}
			dst += modulo;
		}
		return;
	}

	if (IsTrans[icon] != 0) {
		for (int row = 0; row < IconHeight; row++) {
			for (int col = 0; col < IconWidth; col++) {
				unsigned char pixel = *src++;
				if (pixel) *dst = pixel;
				dst++;
			}
			dst += modulo;
		}
		return;
	}

	int const rows = (IconHeight >> 2) * 4;
	size_t const bytes = (size_t)(IconWidth >> 2) * 4;
	for (int row = 0; row < rows; row++) {
		memcpy(dst, src, bytes);
		src += bytes;
		dst += bytes + modulo;
	}
}

/***************************************************************************
 * BUFFER_DRAW_STAMP_CLIP -- Draws an icon clipped to a window             *
 *                                                                         *
 * min_x, min_y, max_x, max_y are really the window's x, y, width and      *
 * height in viewport pixels (the GBUFFER.H wrapper passes WindowList      *
 * entries). x_pixel/y_pixel are relative to the window. The icon is       *
 * clipped to the window only, not to the viewport.                        *
 *                                                                         *
 * Same three draw modes as Buffer_Draw_Stamp, but the plain copy copies   *
 * the full clipped width and height.                                      *
 ***************************************************************************/
extern "C" void GVP_CDECL Buffer_Draw_Stamp_Clip(void const *thisptr, void const *icondata, int icon, int x_pixel,
								int y_pixel, void const *remap, int min_x, int min_y, int max_x, int max_y)
{
	if (icondata == NULL) return;
	if (!Stamp_Lookup(icondata, icon)) return;

	int height = IconHeight;
	int iwidth = IconWidth;
	unsigned char const *src = StampPtr + (size_t)((unsigned)icon * (unsigned)IconSize);

	/*
	** Turn the window size into right/bottom limits and the position into viewport coordinates.
	*/
	max_x += min_x;
	x_pixel += min_x;
	max_y += min_y;
	y_pixel += min_y;

	/*
	** Reject icons entirely outside the window.
	*/
	if (x_pixel >= max_x) return;
	if (y_pixel >= max_y) return;
	if (y_pixel + IconHeight <= min_y) return;
	if (x_pixel + IconWidth <= min_x) return;

	/*
	** Clip. `skip` is how many source bytes to skip at the end of each row.
	*/
	if (x_pixel < min_x) {
		int cut = min_x - x_pixel;
		src += cut;
		iwidth -= cut;
		x_pixel = min_x;
	}
	int skip = IconWidth - iwidth;
	if (x_pixel + iwidth > max_x) {
		int old = iwidth;
		iwidth = max_x - x_pixel;
		skip += old - iwidth;
	}
	if (min_y > y_pixel) {
		int cut = min_y - y_pixel;
		height -= cut;
		src += (size_t)((unsigned)cut * (unsigned)IconWidth);
		y_pixel = min_y;
	}
	if (y_pixel + height > max_y) {
		height = max_y - y_pixel;
	}

	GVPLayout const *vp = (GVPLayout const *)thisptr;
	long const modulo = GVP_Bytes_Per_Row(vp) - iwidth;
	unsigned char *dst = GVP_Pixel_Ptr(vp, x_pixel, y_pixel);

	if (remap != NULL) {
		unsigned char const *table = (unsigned char const *)remap;
		for (int row = 0; row < height; row++) {
			for (int col = 0; col < iwidth; col++) {
				unsigned char pixel = table[*src++];
				if (pixel) *dst = pixel;
				dst++;
			}
			dst += modulo;
			src += skip;
		}
		return;
	}

	if (IsTrans[icon] != 0) {
		for (int row = 0; row < height; row++) {
			for (int col = 0; col < iwidth; col++) {
				unsigned char pixel = *src++;
				if (pixel) *dst = pixel;
				dst++;
			}
			dst += modulo;
			src += skip;
		}
		return;
	}

	for (int row = 0; row < height; row++) {
		memcpy(dst, src, (size_t)iwidth);
		src += iwidth + skip;
		dst += iwidth + modulo;
	}
}
