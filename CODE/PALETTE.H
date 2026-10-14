/*
**	Command & Conquer Red Alert(tm) - multiplayer port.
**	Reconstruction of a file that is missing from the Electronic Arts source release
**	(session S06a). Licensed like the rest of the game code:
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

/***********************************************************************************************
 *                                                                                             *
 *                 Project Name : Command & Conquer                                            *
 *                                                                                             *
 *                    File Name : PALETTE.H                                                    *
 *                                                                                             *
 *   RECONSTRUCTED in session S06a (2026-10-14). The original file is missing from the        *
 *   source release and the project owner confirmed there is no other copy. This version is  *
 *   rebuilt from every use of PaletteClass in the code. What was inferred, and from where,  *
 *   is listed in docs/codebase/compat-notes.md ("Rewritten files").                          *
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 * Functions:                                                                                  *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

/*
**	The include guard is not PALETTE_H: WIN32LIB/INCLUDE/PALETTE.H (the C palette functions,
**	reached through <wwlib32.h>) already uses that name, and both headers are included by
**	FUNCTION.H.
*/
#ifndef PALETTECLASS_H
#define PALETTECLASS_H

#include	"rgb.h"


/*
**	A complete 256 color palette. The object is nothing but the 256 RGBClass entries, so it
**	is exactly 768 bytes in the VGA 6-bit-per-gun format of the .PAL files: the code reads
**	palette files straight into it (CCFileClass::Read(&CCPalette, sizeof(CCPalette))), casts
**	raw file data to it (DISPLAY.CPP) and copies it with memcpy/memmove. It must therefore
**	never get virtual functions or extra data members.
*/
class PaletteClass
{
	public:
		enum {
			COLOR_COUNT=256
		};

		PaletteClass(void) {};
		PaletteClass(RGBClass const & rgb);

		/*
		**	Access to the individual color entries.
		*/
		RGBClass & operator[] (int index) {return(Palette[index]);};
		RGBClass const & operator[] (int index) const {return(Palette[index]);};

		/*
		**	The raw 768 byte palette, for the WIN32LIB palette functions and memcpy().
		*/
		operator unsigned char * (void) {return((unsigned char *)&Palette[0]);};
		operator unsigned char const * (void) const {return((unsigned char const *)&Palette[0]);};

		int operator == (PaletteClass const & palette) const;
		int operator != (PaletteClass const & palette) const {return(!(*this == palette));};

		/*
		**	Fade every color (or only the colors flagged in a lookup table) toward black or
		**	toward the matching color of another palette. "ratio" is 0..255 (0..100%).
		*/
		void Adjust(int ratio);
		void Adjust(int ratio, PaletteClass const & palette);
		void Partial_Adjust(int ratio, char * lut);
		void Partial_Adjust(int ratio, PaletteClass const & palette, char * lut);

		/*
		**	Make this the displayed palette, fading to it over "time" ticks (60 per second)
		**	while calling "callback" repeatedly.
		*/
		void Set(int time = 0, void (* callback)(void) = 0) const;

		/*
		**	The index of the palette entry that is closest to the specified color.
		*/
		int Closest_Color(RGBClass const & rgb) const;

		/*
		**	A copy of the palette that is currently displayed.
		*/
		static PaletteClass CurrentPalette;

	protected:
		RGBClass Palette[COLOR_COUNT];
};

#if __cplusplus >= 201103L
static_assert(sizeof(PaletteClass) == 768, "PaletteClass must be the raw 768 byte VGA palette");
#endif

#endif
