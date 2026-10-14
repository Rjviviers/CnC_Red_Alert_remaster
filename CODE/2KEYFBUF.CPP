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
 * 2KEYFBUF.CPP -- Buffer_Frame_To_Page, replacing 2KEYFBUF.ASM (S07-B).
 *
 * Keeps the signature the game calls (CODE/FUNCTION.H, used by CC_Draw_Shape
 * in CODE/CONQUER.CPP) and forwards to the portable renderer in SHAPEDRW.CPP,
 * which has no game dependencies so it can be unit tested on its own.
 * See docs/codebase/shape-renderer.md.
 ***************************************************************************/

#include "function.h"
#include "SHAPEDRW.H"

/***************************************************************************
 * Buffer_Frame_To_Page -- Copies a linear buffer to a virtual viewport    *
 *                                                                         *
 * INPUT:   x, y           - destination position in the viewport          *
 *          w, h           - size of the shape frame                       *
 *          Buffer         - frame (raw pixels, or a big-shape-buffer      *
 *                           header when UseBigShapeBuffer is set)         *
 *          view           - viewport to draw into                         *
 *          flags          - SHAPE_* flags, followed by their arguments:   *
 *                           GHOST: table; FADING: table, count;           *
 *                           PREDATOR: offset; PARTIAL: value              *
 *                                                                         *
 * OUTPUT:  0 (the asm returned whatever was in EAX; no caller uses it)    *
 ***************************************************************************/
extern "C" long __cdecl Buffer_Frame_To_Page(int x, int y, int w, int h, void *Buffer, GraphicViewPortClass &view, int flags, ...)
{
	ShapeViewType v;
	v.Offset = (unsigned char *)view.Get_Offset();
	v.Width = view.Get_Width();
	v.Height = view.Get_Height();
	v.XAdd = view.Get_XAdd();
	v.Pitch = view.Get_Pitch();

	va_list args;
	va_start(args, flags);
	long result = Buffer_Frame_To_View(x, y, w, h, Buffer, v, flags, args);
	va_end(args);
	return result;
}
