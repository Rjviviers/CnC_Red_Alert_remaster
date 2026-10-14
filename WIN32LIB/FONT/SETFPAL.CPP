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
 **     C O N F I D E N T I A L --- W E S T W O O D   S T U D I O S       **
 ***************************************************************************
 *                 Project Name : 32 bit library Text Print                *
 *                    File Name : SETFPAL.CPP (was SETFPAL.ASM)            *
 *                   Programmer : Scott K. Bowen (asm original)            *
 *                   Start Date : July 2, 1994                             *
 *-------------------------------------------------------------------------*
 * Portable C++ port (session S07-C, 2026-10-14).                          *
 *                                                                         *
 * Functions:                                                              *
 *   Set_Font_Palette_Range -- sets colours in the font translation table  *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

// Prototype must match WIN32LIB/INCLUDE/FONT.H (extern "C", cdecl).
// The header is not included because it still needs windows.h (session S06).

extern "C" {

// Defined in CODE/2TXTPRNT.CPP, like the asm, where SETFPAL used the table
// that 2TXTPRNT.ASM defined. See the comment there for the table layout.
extern unsigned char ColorXlat[];

void Set_Font_Palette_Range(void const *palette, int start_idx, int end_idx);

}


/***********************************************************
 * SET_FONT_PALETTE_RANGE
 *
 * VOID Set_Font_Palette_Range(VOID *palette, WORD start, WORD end);
 *
 * Copies colours start..end of `palette` (one byte per colour) into the
 * text colour translation table: entry i goes to ColorXlat[i] (low-nibble
 * lookup) and ColorXlat[i * 16] (high-nibble lookup).
 *
 * Bounds checking: start and end are reduced to 0-15 with "& 15" (not
 * clamped), and nothing happens if end < start after that. The palette is
 * read from its first byte, not from palette[start].
 ***********************************************************/
void Set_Font_Palette_Range(void const *palette, int start_idx, int end_idx)
{
	unsigned char const *src = (unsigned char const *)palette;

	int start = start_idx & 0x0F;
	int end = end_idx & 0x0F;
	if (end < start) {
		return;
	}

	for (int index = start; index <= end; index++) {
		unsigned char color = *src++;
		ColorXlat[index] = color;			// low-nibble entry
		ColorXlat[index * 16] = color;		// high-nibble entry
	}
}
