//
//	Command & Conquer Red Alert(tm)
//	Copyright 2025 Electronic Arts Inc.
//
//	This program is free software: you can redistribute it and/or modify
//	it under the terms of the GNU General Public License as published by
//	the Free Software Foundation, either version 3 of the License, or
//	(at your option) any later version.
//
//	This program is distributed in the hope that it will be useful,
//	but WITHOUT ANY WARRANTY; without even the implied warranty of
//	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//	GNU General Public License for more details.
//
//	You should have received a copy of the GNU General Public License
//	along with this program.  If not, see <http://www.gnu.org/licenses/>.
//

/***************************************************************************
 **      C O N F I D E N T I A L --- W E S T W O O D   S T U D I O S      **
 ***************************************************************************
 *                                                                         *
 *                 Project Name : Westwood 32 bit Library                  *
 *                                                                         *
 *                    File Name : 2TXTPRNT.CPP (was 2TXTPRNT.ASM)          *
 *                                                                         *
 *                   Programmer : Phil W. Gorrow (asm original)            *
 *                                                                         *
 *                   Start Date : January 17, 1995                         *
 *                                                                         *
 *-------------------------------------------------------------------------*
 * Portable C++ port of the text printer (session S07-C, 2026-10-14).      *
 * Same symbols, signatures and output as the TASM original.               *
 *                                                                         *
 * Functions:                                                              *
 *   Buffer_Print -- prints a string into a viewport with the current font *
 *   Get_Font_Palette_Ptr -- returns the font colour translation table     *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

// This file deliberately does not include the Westwood headers (they still
// need windows.h/ddraw.h, see session S06). The prototypes below must match
// WIN32LIB/INCLUDE/DRAWBUFF.H and WIN32LIB/INCLUDE/FONT.H. Both targets use
// cdecl by default, so the headers' __cdecl is implied.

#include <stddef.h>
#include <stdint.h>

/*
** The first fields of GraphicViewPortClass (WIN32LIB/INCLUDE/GBUFFER.H), in
** declaration order. The asm read them through a STRUC at fixed offsets
** (Offset 0, Width 4, Height 8, XAdd 12, XPos 16, YPos 20, Pitch 24 on the
** 32-bit build); declaring the same member types in the same order gives the
** same layout as the real class under any packing.
*/
namespace {
struct TextViewPortLayout {
	long	Offset;		// address of the viewport's top-left pixel
	int		Width;
	int		Height;
	int		XAdd;
	int		XPos;
	int		YPos;
	long	Pitch;
};
}

static_assert(sizeof(void *) != 4 || offsetof(TextViewPortLayout, Pitch) == 24,
	"GraphicViewPortClass mirror no longer matches GBUFFER.INC");

/*
** Font file header offsets (see the asm comment):
**   UWORD FontLength;        0
**   BYTE  FontCompress;      2
**   BYTE  FontDataBlocks;    3
**   UWORD InfoBlockOffset;   4
**   UWORD OffsetBlockOffset; 6
**   UWORD WidthBlockOffset;  8
**   UWORD DataBlockOffset;  10
**   UWORD HeightOffset;     12
*/
enum {
	FONTINFOBLOCK		= 4,
	FONTOFFSETBLOCK		= 6,
	FONTWIDTHBLOCK		= 8,
	FONTDATABLOCK		= 10,
	FONTHEIGHTBLOCK		= 12,

	FONTINFOMAXHEIGHT	= 4,
	FONTINFOMAXWIDTH	= 5
};

extern "C" {

extern void const *FontPtr;		// WIN32LIB/FONT/LOADFONT.CPP
extern int FontXSpacing;
extern int FontYSpacing;

/*
** Colour translation table for 4-bit font pixels. A font byte holds two
** pixels: the low nibble indexes ColorXlat[0..15] directly; the high nibble is
** used WITHOUT shifting, so it indexes ColorXlat[0x00, 0x10, ..., 0xF0]. That
** is why entry 16*k also holds colour k. Set_Font_Palette_Range
** (WIN32LIB/FONT/SETFPAL.CPP) writes both places; Buffer_Print overwrites
** [0] (background), [1] and [16] (foreground) on every call.
** The asm table was 241 bytes (highest index used is 0xF0); the 15 bytes added
** here to round it to 256 are never read.
*/
unsigned char ColorXlat[256] = {
	0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0A,0x0B,0x0C,0x0D,0x0E,0x0F,
	0x01,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0x02,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0x03,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0x04,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0x05,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0x06,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0x07,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0x08,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0x09,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0x0A,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0x0B,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0x0C,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0x0D,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0x0E,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0x0F
};

long Buffer_Print(void *thisptr, const char *str, int x, int y, int fcolor, int bcolor);
void *Get_Font_Palette_Ptr(void);

}	// extern "C"


static inline unsigned Read_UWord(unsigned char const *p)
{
	return (unsigned)p[0] | ((unsigned)p[1] << 8);		// font files are little-endian
}


/***************************************************************************
 * BUFFER_PRINT -- prints a string into a viewport with the current font   *
 *                                                                         *
 * INPUT:   thisptr - GraphicViewPortClass * to draw into                  *
 *          str     - NUL-terminated string. '\n' moves to the left edge   *
 *                    of the viewport, '\r' back to the starting x.        *
 *          x, y    - viewport-relative position of the top-left corner    *
 *          fcolor  - colour for font pixel value 1                        *
 *          bcolor  - colour for font pixel value 0 (0 = transparent)      *
 *                                                                         *
 * OUTPUT:  The address where the next character would be drawn, or 0 if   *
 *          the text ran off the bottom of the viewport (or no font set).  *
 *                                                                         *
 * Behaviour kept from the asm on purpose (do not "fix" without a reason): *
 *  - There is no left/right clipping. A character that does not fit       *
 *    (x + width + FontXSpacing > viewport width) forces a line feed that  *
 *    returns to the starting x, like '\r'.                                *
 *  - A character whose data height is 0 skips the bottom-blank fill, even *
 *    when the background is opaque (the top blank is still filled).       *
 *  - Width, top-blank, height and bottom-blank counters are 8-bit, as in  *
 *    the asm (bottom blank = maxheight - top - height, modulo 256).       *
 *  - Pixel colour 0 after translation is never written (transparent).     *
 *                                                                         *
 * HISTORY:                                                                *
 *   01/17/1995 PWG : Created (asm).                                       *
 *   10/14/2026 S07-C : Ported to C++.                                     *
 *=========================================================================*/
long Buffer_Print(void *thisptr, const char *str, int x_pixel, int y_pixel, int fcolor, int bcolor)
{
	TextViewPortLayout const *vp = (TextViewPortLayout const *)thisptr;
	unsigned char const *string = (unsigned char const *)str;

	/*
	** Where to draw.
	*/
	int vpheight = vp->Height;
	int vpwidth = vp->Width;
	int bufferwidth = vp->Width + vp->XAdd + (int)vp->Pitch;		// bytes per line

	unsigned char *curline = (unsigned char *)(intptr_t)vp->Offset + (ptrdiff_t)bufferwidth * y_pixel;
	unsigned char *startdraw = curline + x_pixel;
	int original_x = x_pixel;

	/*
	** Block pointers. The asm also gave up on a NULL font.
	*/
	unsigned char const *font = (unsigned char const *)FontPtr;
	if (font == NULL) {
		return 0;
	}
	unsigned char const *offsetblock = font + Read_UWord(font + FONTOFFSETBLOCK);
	unsigned char const *infoblock = font + Read_UWord(font + FONTINFOBLOCK);
	unsigned char const *widthblock = font + Read_UWord(font + FONTWIDTHBLOCK);
	unsigned char const *heightblock = font + Read_UWord(font + FONTHEIGHTBLOCK);

	/*
	** Make sure the tallest character fits on this line. From here on y_pixel
	** is the bottom of the current text line.
	*/
	unsigned char maxheight = infoblock[FONTINFOMAXHEIGHT];
	if (maxheight + y_pixel > vpheight) {
		return 0;
	}
	y_pixel += maxheight;

	/*
	** Foreground and background colours.
	*/
	ColorXlat[1] = (unsigned char)fcolor;
	ColorXlat[16] = (unsigned char)fcolor;
	ColorXlat[0] = (unsigned char)bcolor;

	for (;;) {
		unsigned chr = *string;
		if (chr == 0) {
			break;
		}
		unsigned char *edi = startdraw;
		string++;

		unsigned char feed_char;			// BL in the asm: 10, 13 or 0 (forced)
		if (chr == 10 || chr == 13) {
			feed_char = (unsigned char)chr;
		} else {
			unsigned char width = widthblock[chr];
			int next_x = x_pixel + width + FontXSpacing;
			startdraw += width + FontXSpacing;

			if (next_x <= vpwidth) {
				x_pixel = next_x;
				int nextdraw = bufferwidth - width;

				unsigned char const *data = font + Read_UWord(offsetblock + chr * 2);
				unsigned char topblank = heightblock[chr * 2];
				unsigned char charheight = heightblock[chr * 2 + 1];
				unsigned char bottomblank = (unsigned char)(maxheight - (unsigned char)(charheight + topblank));

				/*
				** Blank rows above the character: skipped if the background is
				** transparent, filled with the background colour otherwise.
				*/
				if (topblank != 0) {
					unsigned char bg = ColorXlat[0];
					if (bg == 0) {
						edi += (ptrdiff_t)topblank * bufferwidth;
					} else {
						unsigned char rows = topblank;
						do {
							unsigned char cols = width;
							do {
								*edi++ = bg;
							} while (--cols != 0);
							edi += nextdraw;
						} while (--rows != 0);
					}
				}

				/*
				** The character itself, two pixels per byte, low nibble first.
				** A row with an odd width leaves the last high nibble unused.
				*/
				if (charheight == 0) {
					continue;			// note: no bottom-blank fill (asm behaviour)
				}
				unsigned char rows = charheight;
				do {
					unsigned char cols = width;
					for (;;) {
						unsigned char b = *data++;
						unsigned char c = ColorXlat[b & 0x0F];
						if (c != 0) *edi = c;
						edi++;
						if (--cols == 0) break;

						c = ColorXlat[b & 0xF0];		// high nibble, unshifted
						if (c != 0) *edi = c;
						edi++;
						if (--cols == 0) break;
					}
					edi += nextdraw;
				} while (--rows != 0);

				/*
				** Blank rows below the character, only if the background is opaque.
				*/
				if (bottomblank != 0) {
					unsigned char bg = ColorXlat[0];
					if (bg != 0) {
						unsigned char brows = bottomblank;
						do {
							unsigned char cols = width;
							do {
								*edi++ = bg;
							} while (--cols != 0);
							edi += nextdraw;
						} while (--brows != 0);
					}
				}
				continue;
			}

			/*
			** Does not fit: back up so this character is printed again on the
			** next line, and do a line feed that keeps the starting x.
			*/
			string--;
			feed_char = 0;
		}

		/*
		** Line feed. '\n' goes to the left edge of the viewport; '\r' and a
		** forced line feed go back to the starting x.
		*/
		int step = maxheight + FontYSpacing;
		if (y_pixel + step > vpheight) {
			return 0;
		}
		curline += (ptrdiff_t)bufferwidth * step;
		y_pixel += step;
		x_pixel = (feed_char == 10) ? 0 : original_x;
		startdraw = curline + x_pixel;
	}

	return (long)(intptr_t)startdraw;
}


/***************************************************************************
 * GET_FONT_PALETTE_PTR -- Returns a pointer to the 256 byte font palette  *
 *                                                                         *
 * PROTO:   void *Get_Font_Palette_Ptr(void);                              *
 *                                                                         *
 * HISTORY:                                                                *
 *   08/18/1995 PWG : Created (asm).                                       *
 *=========================================================================*/
void *Get_Font_Palette_Ptr(void)
{
	return ColorXlat;
}
