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

/* $Header: /CounterStrike/LCW.CPP 1     3/03/97 10:25a Joe_bostic $ */
/***************************************************************************
 **    C O N F I D E N T I A L --- W E S T W O O D   S T U D I O S        **
 ***************************************************************************
 *                                                                         *
 *               Project Name : WESTWOOD LIBRARY (PSX)                     *
 *                                                                         *
 *                 File Name : LCWUNCMP.CPP                                *
 *                                                                         *
 *                Programmer : Ian M. Leslie                               *
 *                                                                         *
 *                Start Date : May 17, 1995                                *
 *                                                                         *
 *               Last Update : May 17, 1995    [IML]                       *
 *                                                                         *
 *-------------------------------------------------------------------------*
 * Functions:                                                              *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */


/***************************************************************************
 * LCW_Uncomp -- Decompress an LCW encoded data block.                     *
 *                                                                         *
 * Uncompress data to the following codes in the format b = byte, w = word *
 * n = byte code pulled from compressed data.                              *
 *                                                                         *
 *   Command code, n        |Description                                   *
 * ------------------------------------------------------------------------*
 * n=0xxxyyyy,yyyyyyyy      |short copy back y bytes and run x+3 from dest *
 * n=10xxxxxx,n1,n2,...,nx+1|med length copy the next x+1 bytes from source*
 * n=11xxxxxx,w1            |med copy from dest x+3 bytes from offset w1   *
 * n=11111111,w1,w2         |long copy from dest w1 bytes from offset w2   *
 * n=11111110,w1,b1         |long run of byte b1 for w1 bytes              *
 * n=10000000               |end of data reached                           *
 *                                                                         *
 *                                                                         *
 * INPUT:                                                                  *
 *      void * source ptr                                                  *
 *      void * destination ptr                                             *
 *      unsigned long length of uncompressed data                          *
 *                                                                         *
 *                                                                         *
 * OUTPUT:                                                                 *
 *     unsigned long # of destination bytes written                        *
 *                                                                         *
 * WARNINGS:                                                               *
 *     3rd argument is dummy. It exists to provide cross-platform          *
 *      compatibility. Note therefore that this implementation does not    *
 *      check for corrupt source data by testing the uncompressed length.  *
 *                                                                         *
 * HISTORY:                                                                *
 *    03/20/1995 IML : Created.                                            *
 *=========================================================================*/

/*
**	Port notes (session S07-D, 2026-10-14)
**
**	This file is now the ONLY LCW decoder in the build. It provides two entry points:
**
**	  LCW_Uncomp      - the game's C++ decoder (CODE/LCWSTRAW.CPP, CODE/LCWPIPE.CPP). It ignores the
**	                    length argument and decodes until the 0x80 end code, as before.
**	  LCW_Uncompress  - the library's C entry point (WIN32LIB/INCLUDE/IFF.H, WINVQ COMPRESS.H). It used
**	                    to be WIN32LIB/IFF/LCWUNCMP.ASM, which is deleted. Like that asm, it never writes
**	                    more than 'length' bytes: every command is clipped to the bytes left, and decoding
**	                    stops when the output is full, even without an end code.
**
**	Both share Decode() below. Two changes from the 1995 C code, neither of which changes the bytes
**	written for any stream the original handled correctly:
**	  - The long run (0xFE) is a plain byte fill. The original filled with aligned 8-byte stores that
**	    could write up to 4 bytes past the end of the run (later overwritten, or past the buffer),
**	    and broke completely for a run shorter than the 1-4 bytes it wrote before its first aligned store. It also
**	    cast a pointer to 'unsigned', which does not compile for 64-bit targets.
**	  - The bounded mode for LCW_Uncompress, which the C code never had (the asm did).
**	The format itself is described in docs/codebase/file-formats.md. Tests: tests/s07d_lcw_test.cpp.
*/

#ifndef _MSC_VER
#ifndef __cdecl
#define __cdecl
#endif
#endif

#include	"LCW.H"

static unsigned long Decode(void const * source, void * dest, unsigned long length, bool bounded)
{
	unsigned char const * source_ptr = (unsigned char const *) source;
	unsigned char * const start = (unsigned char *) dest;
	unsigned char * dest_ptr = start;
	unsigned long left = length;		// bytes the output may still take (bounded mode only)

	for (;;) {
		if (bounded) {
			left = length - (unsigned long)(dest_ptr - start);
			if (left == 0) break;
		}

		unsigned op_code = *source_ptr++;
		unsigned long count;
		unsigned char const * copy_ptr;

		if (!(op_code & 0x80)) {

			/* 0cccpppp pppppppp: copy c+3 bytes from p bytes back in the output. */
			count = (op_code >> 4) + 3;
			copy_ptr = dest_ptr - (*source_ptr++ + ((op_code & 0x0f) << 8));

		} else if (!(op_code & 0x40)) {

			/* 10000000: end of data. */
			if (op_code == 0x80) break;

			/* 10cccccc: copy the next c bytes of the source. */
			count = op_code & 0x3f;
			if (bounded && count > left) count = left;
			while (count--) *dest_ptr++ = *source_ptr++;
			continue;

		} else if (op_code == 0xfe) {

			/* 11111110 w1 b: write byte b, w1 times. */
			count = source_ptr[0] + ((unsigned) source_ptr[1] << 8);
			unsigned char data = source_ptr[2];
			source_ptr += 3;
			if (bounded && count > left) count = left;
			while (count--) *dest_ptr++ = data;
			continue;

		} else {

			if (op_code == 0xff) {
				/* 11111111 w1 w2: copy w1 bytes from offset w2 of the output. */
				count = source_ptr[0] + ((unsigned) source_ptr[1] << 8);
				source_ptr += 2;
			} else {
				/* 11cccccc w1: copy c+3 bytes from offset w1 of the output. */
				count = (op_code & 0x3f) + 3;
			}
			copy_ptr = start + source_ptr[0] + ((unsigned) source_ptr[1] << 8);
			source_ptr += 2;
		}

		/* Copy from the output, one byte at a time: overlapping copies repeat a pattern. */
		if (bounded && count > left) count = left;
		while (count--) *dest_ptr++ = *copy_ptr++;
	}

	return (unsigned long)(dest_ptr - start);
}


int LCW_Uncomp(void const * source, void * dest, unsigned long )
{
	return (int) Decode(source, dest, 0, false);
}


/***************************************************************************
 * LCW_Uncompress -- C entry point of the old WIN32LIB/IFF/LCWUNCMP.ASM.   *
 *                                                                         *
 *    Decodes at most 'length' bytes (see the port notes above).           *
 *                                                                         *
 * OUTPUT:  number of bytes written to dest.                               *
 *=========================================================================*/
extern "C" unsigned long __cdecl LCW_Uncompress(void * source, void * dest, unsigned long length)
{
	return Decode(source, dest, length, true);
}
