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
 **   C O N F I D E N T I A L --- W E S T W O O D   A S S O C I A T E S   **
 ***************************************************************************
 *                                                                         *
 *                 Project Name : WSA Support routines                     *
 *                                                                         *
 *                    File Name : XORDELTA.CPP (was XORDELTA.ASM)          *
 *                                                                         *
 *                   Programmer : Scott K. Bowen                           *
 *                                                                         *
 *                  Last Update : May 23, 1994   [SKB]                     *
 *                                                                         *
 *-------------------------------------------------------------------------*
 * C++ port of WIN32LIB/WSA/XORDELTA.ASM (session S07-D, 2026-10-14).      *
 * Same results as the asm for every delta stream the asm could decode;    *
 * tests/s07d_xordelta_test.cpp checks that against the original           *
 * instructions. The format is described in docs/codebase/file-formats.md. *
 *                                                                         *
 * Functions:                                                              *
 *   Apply_XOR_Delta -- Apply XOR delta data to a linear buffer.           *
 *   Apply_XOR_Delta_To_Page_Or_Viewport -- Apply it to a rectangle of a   *
 *                                          wider buffer, XOR or copy.     *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#ifndef _MSC_VER
#ifndef __cdecl
#define __cdecl
#endif
#endif

extern "C" {
unsigned int __cdecl Apply_XOR_Delta(char *target, char *delta);
void __cdecl Apply_XOR_Delta_To_Page_Or_Viewport(void *target, void *delta, int width, int nextrow, int copy);
}

/*
** Values of the 'copy' argument. They must match the ones in WSA.CPP.
*/
#define DO_XOR		0
#define DO_COPY		1

namespace {

/*
** One decoded command of the delta stream.
**
**   00000000 c v          SHORTRUN   c times the byte v (c = 1..255)
**   0ccccccc d1..dc       SHORTDUMP  the next c bytes (c = 1..127)
**   1sssssss              SHORTSKIP  skip s pixels (s = 1..127)
**   10000000 w            word w, little endian:
**     w = 0                 STOP
**     w = 0sss... (1..7FFF) LONGSKIP  skip w pixels
**     w = 10cc...           LONGDUMP  the next c bytes (c = w & 0x3FFF)
**     w = 11cc... v         LONGRUN   c times the byte v (c = w & 0x3FFF)
**
** A count of 0 in a run or dump makes the asm loop 2^32 times (it decrements before testing);
** Westwood's encoder never writes one. This port writes nothing for it.
*/
enum OpType { OP_STOP, OP_SKIP, OP_DUMP, OP_RUN };

struct Op {
	OpType type;
	unsigned count;
	unsigned char value;		// for OP_RUN
};

inline Op Next_Op(unsigned char const * & delta)
{
	Op op;
	op.value = 0;
	unsigned code = *delta++;

	if (code == 0) {
		op.type = OP_RUN;
		op.count = *delta++;
		op.value = *delta++;
	} else if (code < 0x80) {
		op.type = OP_DUMP;
		op.count = code;
	} else if (code != 0x80) {
		op.type = OP_SKIP;
		op.count = code - 0x80;
	} else {
		unsigned word = delta[0] | (delta[1] << 8);
		delta += 2;
		if (word == 0) {
			op.type = OP_STOP;
			op.count = 0;
		} else if (word < 0x8000) {
			op.type = OP_SKIP;
			op.count = word;
		} else if (word & 0x4000) {
			op.type = OP_RUN;
			op.count = word & 0x3FFF;
			op.value = *delta++;
		} else {
			op.type = OP_DUMP;
			op.count = word & 0x3FFF;
		}
	}
	return op;
}

}


/***************************************************************************
 * APPLY_XOR_DELTA -- Apply XOR delta data to a linear buffer.             *
 *                                                                         *
 * INPUT:  char *target - destination buffer.                              *
 *         char *delta - xor data to be delta uncompress.                  *
 *                                                                         *
 * OUTPUT: 0 (the asm returned eax, which is always 0 at the stop code).   *
 *                                                                         *
 * HISTORY:                                                                *
 *   05/23/1994 SKB : Created.                                             *
 *=========================================================================*/
extern "C" unsigned int __cdecl Apply_XOR_Delta(char *target, char *delta)
{
	unsigned char * dst = (unsigned char *) target;
	unsigned char const * src = (unsigned char const *) delta;

	for (;;) {
		Op op = Next_Op(src);
		switch (op.type) {
			case OP_STOP:
				return 0;

			case OP_SKIP:
				dst += op.count;
				break;

			case OP_DUMP:
				while (op.count--) *dst++ ^= *src++;
				break;

			case OP_RUN:
				while (op.count--) *dst++ ^= op.value;
				break;
		}
	}
}


/***************************************************************************
 * APPLY_XOR_DELTA_TO_PAGE_OR_VIEWPORT -- Apply a delta to a rectangle.    *
 *                                                                         *
 *    The delta describes a 'width' pixels wide image, row after row. The  *
 *    target is the top left corner of that image inside a bigger buffer.  *
 *                                                                         *
 * INPUT:  void *target - top left corner of the image in the buffer.      *
 *         void *delta  - the delta data.                                  *
 *         int width    - width of the image in pixels (must be > 0).      *
 *         int nextrow  - distance in bytes from the start of one row to   *
 *                        the start of the next (the buffer's pitch). The  *
 *                        asm's comment calls it "page width - anim width" *
 *                        but the code adds it to the start of the row,    *
 *                        and WSA.CPP passes the full buffer width.        *
 *         int copy     - DO_COPY stores the bytes, DO_XOR xors them.      *
 *                                                                         *
 * HISTORY:                                                                *
 *   05/23/1994 SKB : Created.                                             *
 *=========================================================================*/
extern "C" void __cdecl Apply_XOR_Delta_To_Page_Or_Viewport(void *target, void *delta, int width, int nextrow, int copy)
{
	if (width <= 0) return;		// the asm would loop forever on the first skip

	unsigned char * dst = (unsigned char *) target;
	unsigned char const * src = (unsigned char const *) delta;
	unsigned const w = (unsigned) width;
	unsigned col = 0;						// column within the current row
	bool const xor_mode = (copy == DO_XOR);

	for (;;) {
		Op op = Next_Op(src);

		if (op.type == OP_STOP) return;

		if (op.type == OP_SKIP) {
			dst -= col;
			col += op.count;
			while (col >= w) {
				col -= w;
				dst += nextrow;
			}
			dst += col;
			continue;
		}

		while (op.count--) {
			unsigned char value = (op.type == OP_RUN) ? op.value : *src++;
			if (xor_mode) {
				*dst ^= value;
			} else {
				*dst = value;
			}
			dst++;
			if (++col == w) {
				dst -= w;
				col = 0;
				dst += nextrow;
			}
		}
	}
}
