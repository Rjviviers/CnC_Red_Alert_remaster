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
 * GVPORT.H -- C++ replacement for GBUFFER.INC (session S07-A)             *
 *                                                                         *
 * The DRAWBUFF assembly routines received a GraphicViewPortClass pointer  *
 * as a plain `void *this_object` and read its first eight fields through  *
 * the GBUFFER.INC structure. Their C++ ports do the same through          *
 * GVPLayout below, which mirrors GraphicViewPortClass (GBUFFER.H) member  *
 * by member: same names, same types, same order.                          *
 *                                                                         *
 * Why not include GBUFFER.H directly? It pulls in <ddraw.h>, <windows.h>  *
 * and DOS headers through WWSTD.H, which do not compile on GCC yet (S06,  *
 * S08). The ports only need the data layout, so they depend on this       *
 * header alone. The GBUFFER.H inline wrappers (Clear, Blit, Fill_Rect...) *
 * keep calling the same extern "C" entry points with `this`.              *
 *                                                                         *
 * RULE: if anyone changes the data members of GraphicViewPortClass, this  *
 * struct must change with them. Once GBUFFER.H compiles, add             *
 *   static_assert(sizeof(GraphicViewPortClass) == sizeof(GVPLayout), "") *
 * in a file that includes both (see assembly-inventory.md, S07-A notes).  *
 *                                                                         *
 * The prototypes below are the ones in DRAWBUFF.H, written with plain C   *
 * types (BOOL = int, VOID = void, LONG = long) so that both declarations  *
 * are compatible when a translation unit sees both headers.               *
 ***************************************************************************/

#ifndef GVPORT_H
#define GVPORT_H

#include <stddef.h>

/*
** The original entry points are cdecl. GCC on 32-bit x86 uses cdecl by default and does not know
** the __cdecl keyword (MinGW predefines it), so only spell it out where the compiler understands it.
*/
#if defined(_MSC_VER) || defined(__MINGW32__)
#define GVP_CDECL __cdecl
#else
#define GVP_CDECL
#endif

/*
** Mirror of the data members of GraphicViewPortClass (WIN32LIB/INCLUDE/GBUFFER.H). The first eight
** are the GBUFFER.INC fields GVPOffset ... GVPBuffPtr, which are all the asm ever read.
*/
struct GVPLayout {
	long	Offset;			// address of the viewport's top-left pixel (GVPOffset)
	int		Width;			// width in pixels (GVPWidth)
	int		Height;			// height in pixels (GVPHeight)
	int		XAdd;			// bytes from the end of one row to the start of the next (GVPXAdd)
	int		XPos;			// x offset inside the owning GraphicBufferClass (GVPXPos)
	int		YPos;			// y offset inside the owning GraphicBufferClass (GVPYPos)
	long	Pitch;			// extra row pitch of a DirectDraw surface (GVPPitch)
	void	*GraphicBuff;	// owning GraphicBufferClass * (GVPBuffPtr)
	int		IsDirectDraw;	// BOOL; not used by the ports, kept so sizeof matches the class
	int		LockCount;		// not used by the ports, kept so sizeof matches the class
};

#if defined(__cplusplus) && __cplusplus >= 201103L
/* GBUFFER.INC offsets, as the asm used them (32-bit build, ADR-0003). */
static_assert(sizeof(void *) != 4 || sizeof(long) != 4 || (
	offsetof(GVPLayout, Offset) == 0  && offsetof(GVPLayout, Width) == 4 &&
	offsetof(GVPLayout, Height) == 8  && offsetof(GVPLayout, XAdd) == 12 &&
	offsetof(GVPLayout, XPos) == 16   && offsetof(GVPLayout, YPos) == 20 &&
	offsetof(GVPLayout, Pitch) == 24  && offsetof(GVPLayout, GraphicBuff) == 28),
	"GVPLayout must match GBUFFER.INC");
#endif

/*
** Helpers shared by the ports.
*/
inline unsigned char *GVP_Base(GVPLayout const *vp)
{
	return (unsigned char *)(size_t)vp->Offset;
}

/* Bytes from one row to the next: every routine uses Width + XAdd + Pitch. */
inline long GVP_Bytes_Per_Row(GVPLayout const *vp)
{
	return (long)vp->Width + vp->XAdd + vp->Pitch;
}

/*
** Address of pixel (x, y). The asm computed this with a 32-bit unsigned multiply and wrapping adds;
** for any pixel that lies inside the buffer the signed arithmetic used here gives the same address,
** and it also stays correct on 64-bit hosts.
*/
inline unsigned char *GVP_Pixel_Ptr(GVPLayout const *vp, int x, int y)
{
	return GVP_Base(vp) + (ptrdiff_t)y * GVP_Bytes_Per_Row(vp) + x;
}

/*
** Sign bit of (a - b) computed with 32-bit wrap-around, exactly like the asm's `sub` + `shld`.
** Used to build the Cohen-Sutherland codes of BITBLIT, REMAP, SCALE, TOBUFF, TOPAGE and CLIPRECT.
*/
inline unsigned GVP_Sign_Of_Diff(int a, int b)
{
	return ((unsigned)a - (unsigned)b) >> 31;
}

/*
** The rectangle outcode used by all of those routines. For a coordinate pair (x, y) against a
** window of size (w, h) it returns, from bit 3 down to bit 0:
**   bit 3: x < 0          bit 2: x > w
**   bit 1: y < 0          bit 0: y > h
** (The asm computed "x <= w" as the sign of x - (w + 1) and then flipped bits 2 and 0 with xor 5.)
*/
inline unsigned GVP_Outcode(int x, int y, int w, int h)
{
	unsigned code = 0;
	code = (code << 1) | ((unsigned)x >> 31);
	code = (code << 1) | GVP_Sign_Of_Diff(x, (int)((unsigned)w + 1));
	code = (code << 1) | ((unsigned)y >> 31);
	code = (code << 1) | GVP_Sign_Of_Diff(y, (int)((unsigned)h + 1));
	return code ^ 5;
}

/*
** Entry points. Same names, argument lists and C linkage as DRAWBUFF.H, so the GBUFFER.H wrappers
** and the game call them unchanged.
*/
extern "C" {
	void	GVP_CDECL Buffer_Put_Pixel(void *thisptr, int x, int y, unsigned char color);
	int		GVP_CDECL Buffer_Get_Pixel(void *thisptr, int x, int y);
	void	GVP_CDECL Buffer_Clear(void *thisptr, unsigned char color);
	long	GVP_CDECL Buffer_To_Buffer(void *thisptr, int x, int y, int w, int h, void *buff, long size);
	long	GVP_CDECL Buffer_To_Page(int x, int y, int w, int h, void *Buffer, void *view);
	int		GVP_CDECL Linear_Blit_To_Linear(void *thisptr, void *dest, int x_pixel, int y_pixel, int dx_pixel,
										int dy_pixel, int pixel_width, int pixel_height, int trans);
	int		GVP_CDECL Linear_Scale_To_Linear(void *, void *, int, int, int, int, int, int, int, int, int, char *);
	void	GVP_CDECL Buffer_Draw_Line(void *thisptr, int sx, int sy, int dx, int dy, unsigned char color);
	void	GVP_CDECL Buffer_Fill_Rect(void *thisptr, int sx, int sy, int dx, int dy, unsigned char color);
	void	GVP_CDECL Buffer_Remap(void *thisptr, int sx, int sy, int width, int height, void *remap);
	void	GVP_CDECL Buffer_Draw_Stamp(void const *thisptr, void const *icondata, int icon, int x_pixel, int y_pixel,
										void const *remap);
	void	GVP_CDECL Buffer_Draw_Stamp_Clip(void const *thisptr, void const *icondata, int icon, int x_pixel,
										int y_pixel, void const *remap, int, int, int, int);

	/* STAMP.ASM also exported these (STMPCACH used them). */
	void	GVP_CDECL Init_Stamps(void const *icondata);
	extern void const	*LastIconset;
	extern unsigned char const	*MapPtr;
	extern int	IconCount;
	extern int	IconSize;
	extern unsigned char const	*StampPtr;
}

#endif // GVPORT_H
