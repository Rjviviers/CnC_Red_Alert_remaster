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
 *                 Project Name : LIBRARY                                  *
 *                                                                         *
 *                    File Name : REVERSE.CPP (was REVERSE.ASM)            *
 *                                                                         *
 *                   Programmer : Christopher Yates                        *
 *                                                                         *
 *                  Last Update : April 20, 1994   [SKB]                   *
 *                                                                         *
 *-------------------------------------------------------------------------*
 * C++ port of WIN32LIB/MISC/REVERSE.ASM (session S07-D, 2026-10-14).      *
 * Byte swapping for the big-endian IFF format. Only the low 32 bits of a  *
 * 'long' are used, so the results are the same where long is 64 bits.    *
 *                                                                         *
 * Functions:                                                              *
 *   Reverse_Long  -- 0x11223344 -> 0x44332211                             *
 *   Reverse_Short -- 0x1122 -> 0x2211                                     *
 *   Swap_Long     -- 0x11223344 -> 0x33441122 (swaps the 16-bit halves)   *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#ifndef _MSC_VER
#ifndef __cdecl
#define __cdecl
#endif
#endif

extern "C" {
long __cdecl Reverse_Long(long number);
short __cdecl Reverse_Short(short number);
long __cdecl Swap_Long(long number);
}


extern "C" long __cdecl Reverse_Long(long number)
{
	unsigned long n = (unsigned long) number & 0xFFFFFFFFUL;
	n = ((n & 0x000000FFUL) << 24) | ((n & 0x0000FF00UL) << 8) | ((n & 0x00FF0000UL) >> 8) | ((n >> 24) & 0xFFUL);
	return (long)(int)(unsigned int) n;		// a signed 32-bit value, as eax was
}


extern "C" short __cdecl Reverse_Short(short number)
{
	unsigned int n = (unsigned short) number;
	return (short)(unsigned short)(((n & 0xFFu) << 8) | (n >> 8));
}


extern "C" long __cdecl Swap_Long(long number)
{
	unsigned long n = (unsigned long) number & 0xFFFFFFFFUL;
	n = ((n & 0xFFFFUL) << 16) | (n >> 16);
	return (long)(int)(unsigned int) n;
}
