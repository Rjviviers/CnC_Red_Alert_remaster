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
 **      C O N F I D E N T I A L --- W E S T W O O D   S T U D I O S      **
 ***************************************************************************
 *                                                                         *
 *                 Project Name : Westwood 32 bit Audio Library            *
 *                                                                         *
 *                    File Name : AUDUNCMP.CPP                             *
 *                                                                         *
 *                   Programmer : Phil W. Gorrow                           *
 *                                                                         *
 *                   Start Date : March 14, 1995                           *
 *                                                                         *
 *                  Last Update : June 26, 1995   [PWG]                    *
 *                                                                         *
 *-------------------------------------------------------------------------*
 * Port (S09, 2026-10-14): C++ translation of AUDUNCMP.ASM (TASM), which   *
 * this file replaces. Behaviour is kept bit for bit, including the        *
 * quirks listed below. The asm header listed Decompress_Frame_Lock and   *
 * _Unlock, but the asm never defined them (their only caller is compiled  *
 * out), so they are not provided. See docs/codebase/audio.md.             *
 *-------------------------------------------------------------------------*
 * Functions:                                                              *
 *   Decompress_Frame -- Uncompresses a WW compressed audio frame          *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#include "soundint.h"

/*
** Westwood ADPCM ("SCOMP_WESTWOOD") frame format. The output is unsigned
** 8-bit PCM. Each code byte holds a 2-bit command in bits 6-7 and a 6-bit
** argument in bits 0-5:
**
**   CODE_2BIT    (0): argument+1 bytes follow; each holds four 2-bit deltas
**                     (bits 0-1 first) from _2bitdecode. Result clamped to 0..255.
**   CODE_4BIT    (1): argument+1 bytes follow; each holds two 4-bit deltas
**                     (low nibble first) from _4bitdecode. Result clamped to 0..255.
**   CODE_RAW     (2): if bit 5 is set, bits 0-4 are a signed 5-bit delta added
**                     to the previous sample WITHOUT clamping (8-bit wrap-around,
**                     as in the asm). Otherwise argument+1 raw sample bytes follow.
**   CODE_SILENCE (3): repeat the previous sample argument+1 times.
**
** The previous sample starts at 0x80 for every frame.
*/
#define	CODE_2BIT		0
#define	CODE_4BIT		1
#define	CODE_RAW		2
#define	CODE_SILENCE	3

static signed char const _2bitdecode[4] = {-2,-1,0,1};
static signed char const _4bitdecode[16] = {-9,-8,-6,-5,-4,-3,-2,-1,0,1,2,3,4,5,6,8};


/*
** Add a delta to an unsigned 8-bit sample and clamp to 0..255. This is what
** the asm's carry tests after 'add dl,[table]' do.
*/
static inline unsigned char Add_Clamped(unsigned char sample, int delta)
{
	int value = (int)sample + delta;
	if (value < 0) value = 0;
	if (value > 255) value = 255;
	return (unsigned char)value;
}


/***************************************************************************
 * DECOMPRESS_FRAME -- Uncompresses a WW compressed audio frame            *
 *                                                                         *
 * INPUT:	void * source - pointer to encoded audio data                  *
 *		void * dest   - pointer to decompression area                      *
 *		long size     - the maximum size of destination buffer             *
 *                                                                         *
 * OUTPUT:      long - the number of bytes we uncompressed                 *
 *              (Port note: like the asm, this is really the number of     *
 *              SOURCE bytes read. The only caller ignores it.)            *
 *                                                                         *
 * WARNINGS:  The asm wrote whole runs even when they ran past 'size'.     *
 *            The port reads the source exactly like the asm but never     *
 *            writes outside dest[0..size-1].                              *
 *                                                                         *
 * HISTORY:                                                                *
 *   03/14/1995 PWG : Created.                                             *
 *   10/14/2026 S09 : Ported from AUDUNCMP.ASM.                            *
 *=========================================================================*/
extern "C" long __cdecl Decompress_Frame(void * source, void * dest, long size)
{
	long incount = 0;				// Bytes read from source.

	/*
	**	Source, Dest and count must be valid.
	*/
	if (source == 0 || dest == 0 || size == 0) {
		return(incount);
	}

	unsigned char const *in = (unsigned char const *)source;
	unsigned char *out = (unsigned char *)dest;
	long const limit = size;		// Writes are bounded to dest[0..limit-1].
	long written = 0;				// Next output position.
	long count = size;			// Bytes still to fill (may go negative, as in the asm).
	unsigned char previous = 0x80;

	while (count > 0) {
		unsigned code = *in++;
		incount++;
		unsigned command = code >> 6;
		unsigned arg = code & 0x3F;

		if (command == CODE_RAW) {
			if (arg & 0x20) {

				/*
				**	The lower 5 bits are a signed delta. Sign extend it and add it to
				**	the stream. The asm uses an 8-bit add here, so the sample wraps.
				*/
				int delta = (int)(arg & 0x1F);
				if (delta & 0x10) delta -= 0x20;
				previous = (unsigned char)(previous + delta);
				if (written < limit) out[written] = previous;
				written++;
				count--;
			} else {

				/*
				**	The lower 5 bits hold a count of the number of raw
				**	samples that follow this code. Dump these samples to
				**	the output buffer.
				*/
				long run = (long)arg + 1;
				for (long i = 0; i < run; i++) {
					previous = in[i];
					if (written < limit) out[written] = previous;
					written++;
				}
				in += run;
				incount += run;
				count -= run;
			}
			continue;
		}

		long run = (long)arg + 1;		// Following codes use argument+1.

		if (command == CODE_4BIT) {

			/*
			**	A sequence of 4bit deltas follow. 'run' equals the
			**	number of nibble packed delta bytes to process.
			*/
			for (long i = 0; i < run; i++) {
				unsigned char deltas = *in++;
				incount++;
				previous = Add_Clamped(previous, _4bitdecode[deltas & 0x0F]);
				if (written < limit) out[written] = previous;
				written++;
				previous = Add_Clamped(previous, _4bitdecode[deltas >> 4]);
				if (written < limit) out[written] = previous;
				written++;
				count -= 2;
			}
		} else if (command == CODE_2BIT) {

			/*
			**	A sequence of 2bit deltas follow. 'run' equals the number of
			**	packed delta bytes to process.
			*/
			for (long i = 0; i < run; i++) {
				unsigned char deltas = *in++;
				incount++;
				for (int shift = 0; shift < 8; shift += 2) {
					previous = Add_Clamped(previous, _2bitdecode[(deltas >> shift) & 0x03]);
					if (written < limit) out[written] = previous;
					written++;
				}
				count -= 4;
			}
		} else {

			/*
			**	There is a run of zero deltas. Zero deltas merely duplicate
			**	the 'previous' sample the requested number of times.
			*/
			for (long i = 0; i < run; i++) {
				if (written < limit) out[written] = previous;
				written++;
			}
			count -= run;
		}
	}
	return(incount);
}

