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
 **   C O N F I D E N T I A L --- W E S T W O O D   A S S O C I A T E S   **
 ***************************************************************************
 *                                                                         *
 *                 Project Name : Westwood Library                         *
 *                                                                         *
 *                    File Name : PAL.CPP (was PAL.ASM)                    *
 *                                                                         *
 *                   Programmer : Joe L. Bostic (asm original)             *
 *                                                                         *
 *                   Start Date : May 30, 1992                             *
 *                                                                         *
 *-------------------------------------------------------------------------*
 * Portable C++ port (session S07-C, 2026-10-14). Only the Win95 branch of *
 * the asm (IF 1) is ported; the VGA DAC port I/O versions (NOT_FOR_WIN95  *
 * and the ELSE branch) were dead and are not carried over.                *
 *                                                                         *
 * Functions:                                                              *
 *   Set_Palette_Range -- records and sets a new 256-colour palette        *
 *   Bump_Color -- moves one colour of a palette one step toward another   *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

// Prototypes must match WIN32LIB/INCLUDE/PALETTE.H and MISC.H (extern "C",
// cdecl). The headers are not included because they still need windows.h
// (session S06).

#include <string.h>

/*
** 768 bytes of 255: the asm's "DB 768 DUP(255)". Written out so the array is
** constant-initialised (ready before any static constructor runs).
*/
#define PAL_FF16	255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255
#define PAL_FF256	PAL_FF16,PAL_FF16,PAL_FF16,PAL_FF16,PAL_FF16,PAL_FF16,PAL_FF16,PAL_FF16, \
					PAL_FF16,PAL_FF16,PAL_FF16,PAL_FF16,PAL_FF16,PAL_FF16,PAL_FF16,PAL_FF16

extern "C" {

/*
** Copy of the palette last passed to Set_Palette_Range (the asm comment called
** it the "current values of DAC regs"). Read by WIN32LIB/PALETTE/PALETTE.CPP,
** WIN32LIB/MISC/DDRAW.CPP and the game.
**
** The asm also exported PaletteTable (1024 bytes, only used by the dead VGA
** branch) and kept a private VertBlank word; nothing references them, so they
** are gone.
*/
unsigned char CurrentPalette[768] = { PAL_FF256, PAL_FF256, PAL_FF256 };

/*
** Hands the palette to the display layer. The asm called the Watcom
** register-convention symbol Set_DD_Palette_ (EAX = palette); in C++ this is
** an ordinary call. Defined in WIN32LIB/MISC/DDRAW.CPP (to be replaced by the
** SDL layer, session S08).
*/
void Set_DD_Palette(void *palette);

void Set_Palette_Range(void *palette);
int Bump_Color(void *palette, int changable, int target);		// BOOL in PALETTE.H

}	// extern "C"

#undef PAL_FF256
#undef PAL_FF16


/***************************************************************************
 * SET_PALETTE_RANGE -- Sets a palette range to the new pal                *
 *                                                                         *
 * INPUT:   palette - 768 bytes (256 RGB triplets)                         *
 *                                                                         *
 * OUTPUT:  none                                                           *
 *                                                                         *
 * WARNINGS: Like the asm, there is no NULL check, and the whole palette   *
 *           is always copied and set (no "only changed entries" logic,    *
 *           no vertical-blank wait).                                      *
 *                                                                         *
 * HISTORY:                                                                *
 *   03/07/1995 PWG : Created (asm).                                       *
 *=========================================================================*/
void Set_Palette_Range(void *palette)
{
	memcpy(CurrentPalette, palette, 768);
	Set_DD_Palette(palette);
}


/***************************************************************************
 * Bump_Color -- adjusts specified color in specified palette              *
 *                                                                         *
 * INPUT:                                                                  *
 *	VOID *palette	- palette to modify                                    *
 *	WORD changable	- color # to change                                    *
 *	WORD target	- color to bend toward                                     *
 *                                                                         *
 * OUTPUT:  TRUE (1) if any gun changed, FALSE (0) if the colours already  *
 *          matched.                                                       *
 *                                                                         *
 * Each gun of `changable` moves one step (+1 or -1) toward the same gun   *
 * of `target`. Only the low 16 bits of the colour numbers are used (the   *
 * asm declared them WORD).                                                *
 *                                                                         *
 * HISTORY:                                                                *
 *   04/27/1994 BR : Converted to 32-bit.                                  *
 *=========================================================================*/
int Bump_Color(void *palette, int changable, int target)
{
	unsigned char *pal = (unsigned char *)palette;
	unsigned char *change = pal + 3 * (unsigned)(unsigned short)changable;
	unsigned char const *desired = pal + 3 * (unsigned)(unsigned short)target;

	int changed = 0;
	for (int gun = 0; gun < 3; gun++) {
		unsigned char want = desired[gun];
		unsigned char have = change[gun];
		if (want != have) {
			changed = 1;
			if (want > have) {
				change[gun] = (unsigned char)(have + 1);
			} else {
				change[gun] = (unsigned char)(have - 1);
			}
		}
	}
	return changed;
}
