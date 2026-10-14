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
 *                    File Name : MEM_COPY.CPP (was MEM_COPY.ASM)          *
 *                                                                         *
 *                   Programmer : Scott Bowen                              *
 *                                                                         *
 *                  Last Update : September 8, 1994	[IML]              *
 *                                                                         *
 *-------------------------------------------------------------------------*
 * C++ port of WIN32LIB/MEM/MEM_COPY.ASM (session S07-D, 2026-10-14).      *
 * The asm was an overlap-safe copy, i.e. memmove with the arguments in a  *
 * different order. Largest_Mem_Block, also listed for that file, was      *
 * inside "IF 0" in the asm and never existed, so it is not provided.      *
 *                                                                         *
 * Functions:                                                              *
 *   Mem_Copy -- Copies from one pointer to another.                       *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#ifndef _MSC_VER
#ifndef __cdecl
#define __cdecl
#endif
#endif

#include <string.h>

extern "C" void __cdecl Mem_Copy(void const *source, void *dest, unsigned long bytes_to_copy);


/***************************************************************************
 * MEM_COPY -- Copies from one pointer to another.                         *
 *                                                                         *
 *    Copies bytes_to_copy bytes from source to dest. The areas may        *
 *    overlap. Like the asm, it does nothing when source == dest or either *
 *    pointer is NULL.                                                     *
 *                                                                         *
 * INPUT:   source, dest, bytes_to_copy (note: source comes FIRST).        *
 *                                                                         *
 * HISTORY:                                                                *
 *   04/18/1994 SKB : Created.                                             *
 *=========================================================================*/
extern "C" void __cdecl Mem_Copy(void const *source, void *dest, unsigned long bytes_to_copy)
{
	if (source == dest || source == 0 || dest == 0) return;
	memmove(dest, source, bytes_to_copy);
}
