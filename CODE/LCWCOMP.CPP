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

/* Ported from: $Header:   F:\projects\c&c0\vcs\code\lcwcomp.asv   5.0   11 Nov 1996 09:40:34   JOE_BOSTIC  $ */
/***************************************************************************
 **   C O N F I D E N T I A L --- W E S T W O O D   A S S O C I A T E S   **
 ***************************************************************************
 *                                                                         *
 *                 Project Name : Library routine                          *
 *                                                                         *
 *                    File Name : LCWCOMP.CPP (was COMPRESS.ASM)           *
 *                                                                         *
 *                   Programmer : Louis Castle                             *
 *                                                                         *
 *                  Last Update : 20 August, 1990   [CY]                   *
 *                                                                         *
 *-------------------------------------------------------------------------*
 * C++ port of CODE/LCWCOMP.ASM (session S07-D, 2026-10-14). The output is *
 * byte-for-byte the same as the asm's for every input of 2 or more bytes; *
 * tests/s07d_lcw_test.cpp checks that against the original instructions.  *
 * The format is described in docs/codebase/file-formats.md.               *
 *                                                                         *
 * Functions:                                                              *
 *   LCW_Comp -- Compress a block of data with LCW.                        *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#ifndef _MSC_VER
#ifndef __cdecl
#define __cdecl
#endif
#endif

#include	"LCW.H"


/***************************************************************************
 * LCW_Comp -- Compress a block of data with LCW.                          *
 *                                                                         *
 *    A greedy compressor: at each position it looks for the longest      *
 *    earlier match (the LAST one wins on a tie, which keeps short copies *
 *    possible) and writes the cheapest command for it:                   *
 *                                                                         *
 *      n=0xxxyyyy,yyyyyyyy      short copy: back y bytes, x+3 bytes       *
 *      n=10xxxxxx,n1..nx        literal: copy the next x bytes            *
 *      n=11xxxxxx,w1            medium copy: x+3 bytes from offset w1     *
 *      n=11111111,w1,w2         long copy: w1 bytes from offset w2        *
 *      n=11111110,w1,b1         run: w1 copies of byte b1                 *
 *      n=10000000               end of data                               *
 *                                                                         *
 * INPUT:   source -- data to compress                                     *
 *          dest   -- output buffer. The worst case is length + length/63 *
 *                    + 3 bytes (all literals).                            *
 *          length -- bytes to compress. Offsets and counts are written as *
 *                    16-bit words, so blocks over 64 KB do not round-trip *
 *                    (as in the original).                                *
 *                                                                         *
 * OUTPUT:  size of the compressed data in bytes.                          *
 *                                                                         *
 * WARNINGS:  Differences from the asm, only for inputs it did not handle: *
 *    - length 0 writes just the end code; length 1 writes one literal and *
 *      the end code. The asm read past the input for both and wrote junk. *
 *    - The asm compared the byte 64 ahead to spot runs, reading up to 64  *
 *      bytes past the end of the input. That read never changed the       *
 *      output (a run found that close to the end is always too short), so *
 *      this port simply skips the check there.                           *
 *                                                                         *
 * HISTORY:                                                                *
 *   08/20/1990 CY  : Created (asm).                                       *
 *   10/14/2026     : Ported to C++.                                       *
 *=========================================================================*/
extern "C" int __cdecl LCW_Comp(void const * source, void * dest, int length)
{
	unsigned char const * const first = (unsigned char const *) source;
	unsigned char * const out_start = (unsigned char *) dest;
	unsigned char * out = out_start;

	if (length <= 0) {
		*out++ = 0x80;
		return (int)(out - out_start);
	}

	unsigned char const * const end = first + length;
	unsigned char const * src = first;

	/*
	** The first byte is always a literal of length 1.
	*/
	unsigned char * lenoff = out;			// the literal command being extended
	bool inlen = true;					// true while the previous command is that literal
	*out++ = 0x81;
	*out++ = *src++;

	while (src < end) {
		unsigned char const * search = first;	// the asm's edi during the search
		unsigned long count = 1;				// best match length so far
		unsigned char const * matchoff = first;	// where the best match starts

		/*
		** Search loop. The asm re-enters it after every improved match and after a run;
		** the run check gives the same answer each time for the same position.
		*/
		for (;;) {
			unsigned char value = *src;

			/*
			** A run of 65 or more equal bytes becomes a 0xFE run command. The asm only looks
			** when the byte 64 ahead equals this one. When the run reaches the end of the
			** data, the asm's scan counts one byte less, so the last byte is never part of
			** a run.
			*/
			if (src + 64 < end && src[64] == value) {
				unsigned char const * p = src;
				while (p < end && *p == value) p++;
				if (p == end) p = end - 1;
				unsigned long run = (unsigned long)(p - src);
				if (run >= 65) {
					*out++ = 0xFE;
					*out++ = (unsigned char)(run & 0xFF);
					*out++ = (unsigned char)((run >> 8) & 0xFF);
					*out++ = value;
					src = p;
					inlen = false;
					continue;			// back to the search loop at the new position
				}
			}

			/*
			** Look for the longest earlier match, scanning forward from 'search'.
			*/
			bool improved = false;
			while (search < src) {
				/* repne scasb: find the next occurrence of the current byte. */
				unsigned char const * m = search;
				while (m < src && *m != value) m++;
				if (m == src) {
					search = src;
					break;
				}
				search = m + 1;

				/* Quick reject: a better match must also agree at position count-1. */
				if (src[count - 1] != m[count - 1]) continue;

				/* repe cmpsb: measure the match (it may overlap the current position). */
				unsigned long maxlen = (unsigned long)(end - src);
				unsigned long len = 0;
				while (len < maxlen && src[len] == m[len]) len++;

				if (len >= count) {			// '>=': a later match of equal length wins
					count = len;
					matchoff = m;
					improved = true;
					break;					// the asm jumps back to the search loop here
				}
			}
			if (!improved) break;
		}

		/*
		** Write the command for this position.
		*/
		if (count <= 2) {

			/* Literal byte: extend the current literal command, or start a new one. */
			if (!inlen || *lenoff == 0xBF) {
				lenoff = out;
				*out++ = 0x80;
			}
			(*lenoff)++;
			*out++ = *src++;
			inlen = true;

		} else {

			unsigned long distance = (unsigned long)(src - matchoff);
			unsigned long offset = (unsigned long)(matchoff - first);

			if (count <= 10 && distance <= 0x0FFF) {
				/* Short copy, relative to the current position. */
				*out++ = (unsigned char)(((count - 3) << 4) + (distance >> 8));
				*out++ = (unsigned char)(distance & 0xFF);
			} else if (count <= 64) {
				/* Medium copy, absolute offset. */
				*out++ = (unsigned char)(0xC0 | (count - 3));
				*out++ = (unsigned char)(offset & 0xFF);
				*out++ = (unsigned char)((offset >> 8) & 0xFF);
			} else {
				/* Long copy, absolute offset. */
				*out++ = 0xFF;
				*out++ = (unsigned char)(count & 0xFF);
				*out++ = (unsigned char)((count >> 8) & 0xFF);
				*out++ = (unsigned char)(offset & 0xFF);
				*out++ = (unsigned char)((offset >> 8) & 0xFF);
			}
			src += count;
			inlen = false;
		}
	}

	*out++ = 0x80;
	return (int)(out - out_start);
}
