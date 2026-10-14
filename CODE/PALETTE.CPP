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
 *                    File Name : PALETTE.CPP                                                  *
 *                                                                                             *
 *   RECONSTRUCTED in session S06a (2026-10-14) from the uses of PaletteClass in the code.     *
 *   See docs/codebase/compat-notes.md ("Rewritten files") for what was inferred from where.  *
 *   None of this is simulation code: palettes only affect what is drawn.                     *
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 * Functions:                                                                                  *
 *   PaletteClass::PaletteClass -- Fills the palette with one color.                           *
 *   PaletteClass::operator == -- Compares two palettes.                                       *
 *   PaletteClass::Adjust -- Fades the whole palette toward black.                              *
 *   PaletteClass::Adjust -- Fades the whole palette toward another palette.                    *
 *   PaletteClass::Partial_Adjust -- Fades the flagged colors toward black.                     *
 *   PaletteClass::Partial_Adjust -- Fades the flagged colors toward another palette.           *
 *   PaletteClass::Set -- Makes this the displayed palette.                                    *
 *   PaletteClass::Closest_Color -- Finds the palette entry closest to a color.                *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#include	"function.h"


/*
**	The palette that is currently displayed. RGBClass::Set (RGB.CPP) writes single entries
**	of it, CONQUER.CPP reads it.
*/
PaletteClass PaletteClass::CurrentPalette;


/***********************************************************************************************
 * PaletteClass::PaletteClass -- Fills the palette with one color.                            *
 *                                                                                             *
 *    Used for the solid palettes in GLOBALS.CPP: BlackPalette(RGBClass(0,0,0)) and            *
 *    WhitePalette(RGBClass(255,255,255)).                                                     *
 *                                                                                             *
 * INPUT:   rgb   -- The color to give every entry.                                            *
 *=============================================================================================*/
PaletteClass::PaletteClass(RGBClass const & rgb)
{
	for (int index = 0; index < COLOR_COUNT; index++) {
		Palette[index] = rgb;
	}
}


/***********************************************************************************************
 * PaletteClass::operator == -- Compares two palettes.                                        *
 *                                                                                             *
 * OUTPUT:  Are all 256 color entries identical?                                               *
 *=============================================================================================*/
int PaletteClass::operator == (PaletteClass const & palette) const
{
	if (this == &palette) return(true);
	return(memcmp(&Palette[0], &palette.Palette[0], sizeof(Palette)) == 0);
}


/***********************************************************************************************
 * PaletteClass::Adjust -- Fades the whole palette toward black.                               *
 *                                                                                             *
 *    The one-argument form fades toward black: RGB.H keeps the original commented-out         *
 *    declaration "void Adjust(int ratio, RGBClass const & rgb = BlackColor);", and the code   *
 *    uses it to undo a white flash (Adjust(0x08, WhitePalette) then Adjust(0xFF)).            *
 *                                                                                             *
 * INPUT:   ratio -- How far to move toward black, 0..255 (0% .. ~100%).                       *
 *=============================================================================================*/
void PaletteClass::Adjust(int ratio)
{
	for (int index = 0; index < COLOR_COUNT; index++) {
		Palette[index].Adjust(ratio, BlackColor);
	}
}


/***********************************************************************************************
 * PaletteClass::Adjust -- Fades the whole palette toward another palette.                     *
 *                                                                                             *
 * INPUT:   ratio    -- How far to move toward the other palette, 0..255.                      *
 *          palette  -- The palette to move toward (entry by entry).                           *
 *=============================================================================================*/
void PaletteClass::Adjust(int ratio, PaletteClass const & palette)
{
	for (int index = 0; index < COLOR_COUNT; index++) {
		Palette[index].Adjust(ratio, palette[index]);
	}
}


/***********************************************************************************************
 * PaletteClass::Partial_Adjust -- Fades the flagged colors toward black.                      *
 *                                                                                             *
 *    EGOS.CPP (the credits slide show) fills a 256 byte table with 1 for the picture colors   *
 *    and 0 for the font colors, then fades only the picture ("The text colors never fade").   *
 *                                                                                             *
 * INPUT:   ratio -- How far to move toward black, 0..255.                                     *
 *          lut   -- 256 flags; only entries with a non-zero flag are changed.                 *
 *=============================================================================================*/
void PaletteClass::Partial_Adjust(int ratio, char * lut)
{
	for (int index = 0; index < COLOR_COUNT; index++) {
		if (lut[index]) {
			Palette[index].Adjust(ratio, BlackColor);
		}
	}
}


/***********************************************************************************************
 * PaletteClass::Partial_Adjust -- Fades the flagged colors toward another palette.            *
 *                                                                                             *
 * INPUT:   ratio    -- How far to move toward the other palette, 0..255.                      *
 *          palette  -- The palette to move toward (entry by entry).                           *
 *          lut      -- 256 flags; only entries with a non-zero flag are changed.              *
 *=============================================================================================*/
void PaletteClass::Partial_Adjust(int ratio, PaletteClass const & palette, char * lut)
{
	for (int index = 0; index < COLOR_COUNT; index++) {
		if (lut[index]) {
			Palette[index].Adjust(ratio, palette[index]);
		}
	}
}


/***********************************************************************************************
 * PaletteClass::Set -- Makes this the displayed palette.                                      *
 *                                                                                             *
 *    The Win32 build cannot program the VGA DAC directly, so this goes through the WIN32LIB   *
 *    palette code: Fade_Palette_To() steps the displayed palette toward this one over "time"  *
 *    ticks (with time 0 it sets it at once), calling "callback" while it waits. The callers   *
 *    pass FADE_PALETTE_FAST/MEDIUM/SLOW (DEFINES.H, timer ticks) and Call_Back.               *
 *                                                                                             *
 * INPUT:   time     -- Duration of the fade in ticks (0 = immediately).                       *
 *          callback -- Function to call while fading (may be NULL).                           *
 *=============================================================================================*/
void PaletteClass::Set(int time, void (* callback)(void)) const
{
	Fade_Palette_To((void *)&Palette[0], (unsigned int)time, callback);
	CurrentPalette = *this;
}


/***********************************************************************************************
 * PaletteClass::Closest_Color -- Finds the palette entry closest to a color.                  *
 *                                                                                             *
 *    Same search as the remap table builders in JSHELL.CPP and VORTEX.CPP: the smallest       *
 *    RGBClass::Difference wins, and on a tie the lowest index wins.                           *
 *                                                                                             *
 * INPUT:   rgb   -- The color to match.                                                       *
 *                                                                                             *
 * OUTPUT:  The index (0..255) of the closest palette entry.                                   *
 *=============================================================================================*/
int PaletteClass::Closest_Color(RGBClass const & rgb) const
{
	int best = -1;
	int bvalue = 0;
	for (int index = 0; index < COLOR_COUNT; index++) {
		int diff = Palette[index].Difference(rgb);
		if (best == -1 || diff < bvalue) {
			best = index;
			bvalue = diff;
		}
	}
	return(best);
}
