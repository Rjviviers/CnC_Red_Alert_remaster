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
 * SHAPE.CPP -- the shape scratch buffer, replacing SHAPE.ASM (S07-B).
 *
 * Original: SHAPE.ASM, Phil W. Gorrow (1992-1994). It only defined the
 * shape buffer globals and Set_Shape_Buffer. Declarations are in
 * WIN32LIB/INCLUDE/SHAPE.H (not included here so this file builds on its own).
 *
 * The asm also exported ShapeBuffer and ShapeBufferSize as aliases of the
 * same two variables (TASM "label"). C++ can't alias a variable, and the only
 * user left is WIN32LIB/KEYBOARD/WWMOUSE.ASM (being ported), which must use
 * _ShapeBuffer. CODE/COMPAT.H already maps ShapeBufferSize to _ShapeBufferSize.
 ***************************************************************************/

extern "C" {

char *_ShapeBuffer = 0;
long _ShapeBufferSize = 0;

/***************************************************************************
 * SET_SHAPE_BUFFER -- Sets the shape buffer to the given pointer          *
 *                                                                         *
 * INPUT:   buffer - pointer to the shape buffer                           *
 *          size   - size of the buffer which has been passed in           *
 ***************************************************************************/
void Set_Shape_Buffer(void const *buffer, int size)
{
	_ShapeBufferSize = size;
	_ShapeBuffer = (char *)buffer;
}

}
