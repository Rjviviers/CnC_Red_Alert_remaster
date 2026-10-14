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
 **   C O N F I D E N T I A L --- W E S T W O O D    S T U D I O S        **
 ***************************************************************************
 *                                                                         *
 *                 Project Name : Westwood Library                         *
 *                                                                         *
 *                    File Name : FADING.CPP (was FADING.ASM)              *
 *                                                                         *
 *                   Programmer : Joe L. Bostic (asm original)             *
 *                                                                         *
 *                   Start Date : August 20, 1993                          *
 *                                                                         *
 *-------------------------------------------------------------------------*
 * Portable C++ port (session S07-C, 2026-10-14). Output is byte-identical *
 * to the asm, including its 8/16-bit arithmetic and tie-breaking.         *
 *                                                                         *
 * Functions:                                                              *
 *   Build_Fading_Table -- builds a 256-byte "fade toward a colour" remap  *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

// Prototype must match WIN32LIB/INCLUDE/MISC.H (extern "C", cdecl).
// The header is not included because it still needs windows.h (session S06).

#include <stdint.h>

extern "C" void *Build_Fading_Table(void const *palette, void const *dest, long int color, long int frac);


/*
** One colour gun of the "ideal" colour: new = orig - ((orig - target) * frac).
** The asm does this in 8/16-bit registers and the exact rounding matters:
**   AL = orig - target          (8-bit, wraps)
**   AX = (int8)AL * (int8)DL    (IMUL, DL = frac / 2, so 0..127)
**   AX <<= 1                    (16-bit)
**   result = orig - AH          (8-bit, wraps)
*/
static inline unsigned char Fade_Gun(unsigned char orig, unsigned char target, int8_t half_frac)
{
	int8_t diff = (int8_t)(unsigned char)(orig - target);
	uint16_t ax = (uint16_t)(int16_t)(diff * half_frac);
	ax = (uint16_t)(ax << 1);
	return (unsigned char)(orig - (unsigned char)(ax >> 8));
}


/*
** Squared difference of one gun, as the asm computes it: an 8-bit wrapping
** subtraction, then a signed 8x8 IMUL. Always 0..16384.
*/
static inline uint32_t Gun_Distance(unsigned char a, unsigned char b)
{
	int8_t d = (int8_t)(unsigned char)(a - b);
	return (uint32_t)(uint16_t)(int16_t)(d * d);
}


/***********************************************************
 * BUILD_FADING_TABLE
 *
 * void *Build_Fading_Table(void *palette, void *dest, long int color, long int frac);
 *
 * This routine will create the fading effect table used to coerce colors
 * from toward a common value.  This table is used when Fading_Effect is
 * active.
 *
 * palette: 256 RGB triplets (6-bit VGA values in practice).
 * dest:    256-byte remap table to fill.
 * color:   palette index to fade toward.
 * frac:    0..255 (larger values, including negative ones read as unsigned,
 *          become 255); 255 is "all the way".
 *
 * Returns dest. If palette or dest is NULL, nothing is written.
 *
 * Behaviour kept from the asm:
 *  - dest[0] is always 0 (transparent black never remaps).
 *  - Colour 0 is never chosen as a match, and a colour never maps to itself
 *    (so the table can be applied repeatedly).
 *  - The search stops at the first exact match; otherwise the closest colour
 *    wins and on a tie the HIGHER index wins (the asm compares with "<=").
 *
 * Bounds Checking: None
 ***********************************************************/
void *Build_Fading_Table(void const *palette, void const *dest, long int color, long int frac)
{
	if (palette == 0 || dest == 0) {
		return (void *)dest;
	}

	/*
	** Fractions above 255 become 255 (unsigned compare, as in the asm).
	*/
	if ((uint32_t)frac >= 0x100) {
		frac = 0xFF;
	}
	int8_t half_frac = (int8_t)((uint32_t)frac >> 1);

	unsigned char const *pal = (unsigned char const *)palette;
	unsigned char *out = (unsigned char *)dest;

	/*
	** Record the target gun values.
	*/
	unsigned char const *target = pal + 3 * color;
	unsigned char targetred = target[0];
	unsigned char targetgreen = target[1];
	unsigned char targetblue = target[2];

	/*
	** Transparent black never gets remapped.
	*/
	out[0] = 0;

	for (unsigned index = 1; index <= 255; index++) {
		unsigned char const *orig = pal + 3 * index;
		unsigned char idealred = Fade_Gun(orig[0], targetred, half_frac);
		unsigned char idealgreen = Fade_Gun(orig[1], targetgreen, half_frac);
		unsigned char idealblue = Fade_Gun(orig[2], targetblue, half_frac);

		/*
		** Sweep through the palette (colours 1..255, never the colour being
		** remapped) for the closest match.
		*/
		unsigned char matchcolor = (unsigned char)color;	// default (only kept if nothing matched)
		uint32_t matchvalue = 0xFFFFFFFFu;

		for (unsigned candidate = 1; candidate <= 255; candidate++) {
			if (candidate == index) {
				continue;
			}
			unsigned char const *c = pal + 3 * candidate;
			uint32_t distance = Gun_Distance(c[0], idealred)
				+ Gun_Distance(c[1], idealgreen)
				+ Gun_Distance(c[2], idealblue);

			if (distance == 0) {
				matchcolor = (unsigned char)candidate;		// perfect match: stop early
				break;
			}
			if (distance <= matchvalue) {
				matchvalue = distance;
				matchcolor = (unsigned char)candidate;
			}
		}

		out[index] = matchcolor;
	}

	return (void *)dest;
}
