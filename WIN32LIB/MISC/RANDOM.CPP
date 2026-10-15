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
 *                 Project Name : Library routine                          *
 *                                                                         *
 *                    File Name : RANDOM.CPP (was RANDOM.ASM)              *
 *                                                                         *
 *                   Programmer : Christopher Yates                        *
 *                                                                         *
 *                  Last Update : 20 August, 1990   [CY]                   *
 *                                                                         *
 *-------------------------------------------------------------------------*
 * Replacement for WIN32LIB/MISC/RANDOM.ASM (session S07-D, 2026-10-14).   *
 *                                                                         *
 * Only the seed variable RandNumb is kept: CODE/INIT.CPP still writes the *
 * game seed into it. The asm functions Random() and Get_Random_Mask() are *
 * NOT ported, on purpose (ADR-0010): the game never calls them, and the   *
 * lockstep simulation must only ever use the game's own C++ generators    *
 * (RandomClass in CODE/RANDOM.CPP, Sim_Random/Sim_IRandom in RAND.CPP).   *
 * Nothing reads RandNumb any more.                                        *
 *                                                                         *
 * The old Random() sequence is documented (and pinned against the         *
 * original instructions) in tests/s07d_misc_test.cpp, in case old data    *
 * ever needs it.                                                          *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

/*
** C linkage, like the asm's export and the declaration in CODE/INIT.CPP (inside extern "C").
** With GCC/Clang C and C++ linkage give the same variable symbol; with MSVC they do not, and the
** C++ definition left INIT.CPP's _RandNumb unresolved (CI run 40). Initial value as in the asm's DATASEG.
*/
extern "C" {
long RandNumb = 0x12349876L;
}
