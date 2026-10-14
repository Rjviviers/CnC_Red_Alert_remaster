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
 *                 Project Name : Westwood 32 bit Library                  *
 *                                                                         *
 *                    File Name : GBUFFER.CPP                              *
 *                                                                         *
 *                   Programmer : Phil W. Gorrow                           *
 *                                                                         *
 *                   Start Date : May 3, 1994                              *
 *                                                                         *
 *                  Last Update : October 9, 1995   []                     *
 *                                                                         *
 *-------------------------------------------------------------------------*
 * Functions:                                                              *
 *   VVPC::VirtualViewPort -- Default constructor for a virtual viewport   *
 *   VVPC:~VirtualViewPortClass -- Destructor for a virtual viewport       *
 *   VVPC::Clear -- Clears a graphic page to correct color                 *
 *   VBC::VideoBufferClass -- Lowlevel constructor for video buffer class  *
 *   GVPC::Change -- Changes position and size of a Graphic View Port      *
 *   VVPC::Change -- Changes position and size of a Video View Port      	*
 *   Set_Logic_Page -- Sets LogicPage to new buffer                        *
 *   GBC::DD_Init -- Inits a direct draw surface for a GBC                 *
 *   GBC::Init -- Core function responsible for initing a GBC              *
 *   GBC::Lock -- Locks a Direct Draw Surface                              *
 *   GBC::Unlock -- Unlocks a direct draw surface                          *
 *   GBC::GraphicBufferClass -- Default constructor (requires explicit init)*
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#ifndef GBUFFER_H
#include "gbuffer.h"
#include "misc.h"
#endif
#include "platform.h"		// S08: the visible page is presented by the platform layer
#include <string.h>
#pragma inline

int		TotalLocks;
BOOL 	AllowHardwareBlitFills = TRUE;

// S08: the buffer created with GBC_VISIBLE (the old DirectDraw primary surface).
static GraphicBufferClass *VisibleFrameBuffer = NULL;


//int	CacheAllowed;

/*=========================================================================*/
/* The following PRIVATE functions are in this file:                       */
/*=========================================================================*/


/*= = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = =*/



/***************************************************************************
 * GVPC::GRAPHICVIEWPORTCLASS -- Constructor for basic view port class     *
 *                                                   m                      *
 * INPUT:		GraphicBufferClass * gbuffer	- buffer to attach to			*
 *					int x								- x offset into buffer			*
 *					int y								- y offset into buffer			*
 *					int w								- view port width in pixels   *
 *					int h   							- view port height in pixels	*
 *                                                                         *
 * OUTPUT:     Constructors may not have a return value							*
 *                                                                         *
 * HISTORY:                                                                *
 *   05/09/1994 PWG : Created.                                             *
 *=========================================================================*/
GraphicViewPortClass::GraphicViewPortClass(GraphicBufferClass *gbuffer, int x, int y, int w, int h) :
	LockCount(0),
	GraphicBuff(NULL)
{
	Attach(gbuffer, x, y, w, h);
}

/***************************************************************************
 * GVPC::GRAPHICVIEWPORTCLASS -- Default constructor for view port class   *
 *                                                                         *
 * INPUT:		none                                                        *
 *                                                                         *
 * OUTPUT:     none                                                        *
 *                                                                         *
 * HISTORY:                                                                *
 *   05/09/1994 PWG : Created.                                             *
 *=========================================================================*/
GraphicViewPortClass::GraphicViewPortClass(void)
{
}

/***************************************************************************
 * GVPC::~GRAPHICVIEWPORTCLASS -- Destructor for GraphicViewPortClass		*
 *                                                                         *
 * INPUT:		none                                                        *
 *                                                                         *
 * OUTPUT:     A destructor may not return a value.                        *
 *                                                                         *
 * HISTORY:                                                                *
 *   05/10/1994 PWG : Created.                                             *
 *=========================================================================*/
GraphicViewPortClass::~GraphicViewPortClass(void)
{
 	Offset			= 0;
	Width				= 0;										// Record width of Buffer
	Height			= 0;										// Record height of Buffer
	XAdd				= 0;										// Record XAdd of Buffer
	XPos				= 0;										// Record XPos of Buffer
	YPos				= 0;										// Record YPos of Buffer
	Pitch				= 0;										// Record width of Buffer
	IsDirectDraw	= FALSE;
	LockCount		= 0;
	GraphicBuff		= NULL;
}

/***************************************************************************
 * GVPC::ATTACH -- Attaches a viewport to a buffer class                   *
 *                                                                         *
 * INPUT:		GraphicBufferClass *g_buff	- pointer to gbuff to attach to  *
 *					int x                     - x position to attach to			*
 *					int y 							- y position to attach to			*
 *					int w							- width of the view port			*
 *					int h							- height of the view port			*
 *                                                                         *
 * OUTPUT:     none                                                        *
 *                                                                         *
 * HISTORY:                                                                *
 *   05/10/1994 PWG : Created.                                             *
 *=========================================================================*/
void GraphicViewPortClass::Attach(GraphicBufferClass *gbuffer, int x, int y, int w, int h)
{
	/*======================================================================*/
	/* Can not attach a Graphic View Port if it is actually the physical		*/
	/*	   representation of a Graphic Buffer.											*/
	/*======================================================================*/
	if (this == Get_Graphic_Buffer())  {
		return;
	}

	/*======================================================================*/
	/* Verify that the x and y coordinates are valid and placed within the	*/
	/*		physical buffer.																	*/
	/*======================================================================*/
	if (x < 0) 										// you cannot place view port off
		x = 0;										//		the left edge of physical buf
	if (x >= gbuffer->Get_Width())			// you cannot place left edge off
		x = gbuffer->Get_Width() - 1;			//		the right edge of physical buf
	if (y < 0) 										// you cannot place view port off
		y = 0;										//		the top edge of physical buf
	if (y >= gbuffer->Get_Height()) 			// you cannot place view port off
		y = gbuffer->Get_Height() - 1;		//		bottom edge of physical buf

	/*======================================================================*/
	/* Adjust the width and height of necessary										*/
	/*======================================================================*/
	if (x + w > gbuffer->Get_Width()) 		// if the x plus width is larger
		w = gbuffer->Get_Width() - x;			//		than physical, fix width

	if (y + h > gbuffer->Get_Height()) 		// if the y plus height is larger
		h = gbuffer->Get_Height() - y;		//		than physical, fix height

	/*======================================================================*/
	/* Get a pointer to the top left edge of the buffer.							*/
	/*======================================================================*/
 	Offset 		= gbuffer->Get_Offset() + ((gbuffer->Get_Width()+gbuffer->Get_Pitch()) * y) + x;

	/*======================================================================*/
	/* Copy over all of the variables that we need to store.						*/
	/*======================================================================*/
 	XPos			= x;
 	YPos			= y;
 	XAdd			= gbuffer->Get_Width() - w;
 	Width			= w;
 	Height		= h;
	Pitch			= gbuffer->Get_Pitch();
 	GraphicBuff = gbuffer;
	IsDirectDraw= gbuffer->IsDirectDraw;
}


/***************************************************************************
 * GVPC::CHANGE -- Changes position and size of a Graphic View Port        *
 *                                                                         *
 * INPUT:   	int the new x pixel position of the graphic view port      *
 *					int the new y pixel position of the graphic view port		*
 *					int the new width of the viewport in pixels						*
 *					int the new height of the viewport in pixels					*
 *                                                                         *
 * OUTPUT:  	BOOL whether the Graphic View Port could be sucessfully     *
 *				      resized.																	*
 *                                                                         *
 * WARNINGS:   You may not resize a Graphic View Port which is derived 		*
 *						from a Graphic View Port Buffer, 								*
 *                                                                         *
 * HISTORY:                                                                *
 *   09/14/1994 SKB : Created.                                             *
 *=========================================================================*/
BOOL GraphicViewPortClass::Change(int x, int y, int w, int h)
{
	/*======================================================================*/
	/* Can not change a Graphic View Port if it is actually the physical		*/
	/*	   representation of a Graphic Buffer.											*/
	/*======================================================================*/
	if (this == Get_Graphic_Buffer())  {
		return(FALSE);
	}

	/*======================================================================*/
	/* Since there is no allocated information, just re-attach it to the		*/
	/*		existing graphic buffer as if we were creating the						*/
	/*		GraphicViewPort.																	*/
	/*======================================================================*/
	Attach(Get_Graphic_Buffer(), x, y, w, h);
	return(TRUE);
}


/***************************************************************************
 * GBC::DD_INIT -- Inits a direct draw surface for a GBC                   *
 *                                                                         *
 * INPUT:		none                                                        *
 *                                                                         *
 * OUTPUT:     none                                                        *
 *                                                                         *
 * HISTORY:                                                                *
 *   10/09/1995     : Created.                                             *
 *=========================================================================*/
void GraphicBufferClass::DD_Init(GBC_Enum flags)
{
	/*
	** S08: DirectDraw is gone (ADR-0004). A buffer that asked for video memory
	** (GBC_VIDEOMEM) or to be the visible page (GBC_VISIBLE) is now ordinary system
	** memory, exactly like a GBC_NONE buffer, so all drawing is software and every
	** Lock() succeeds. The GBC_VISIBLE buffer is handed to the platform layer, which
	** presents it (through the palette, scaled) from Platform_Pump_Events.
	** IsDirectDraw stays FALSE, so the DirectDraw-only paths in GBUFFER.H
	** (hardware blits and fills, the video-memory icon cache) are never taken.
	** Class layout is unchanged (S07-A's asm reads it through GBUFFER.INC).
	*/
	if (!Size) Size = Width * Height;
	Buffer		= new BYTE[Size];
	memset(Buffer, 0, Size);
	Allocated	= TRUE;				// BufferClass frees it
	Offset		= (long)Buffer;
	IsDirectDraw	= FALSE;
	LockCount	= 0;
	VideoSurfacePtr = NULL;
	memset(&VideoSurfaceDescription, 0, sizeof(VideoSurfaceDescription));

	if (flags & GBC_VISIBLE) {
		VisibleFrameBuffer = this;
		Platform_Set_Frame_Buffer((unsigned char *)Buffer, Width, Height, Width);
	}
}


void GraphicBufferClass::Attach_DD_Surface (GraphicBufferClass * )
{
	/* S08: no DirectDraw flipping chain; nothing to attach. */
}


/***************************************************************************
 * GBC::INIT -- Core function responsible for initing a GBC                *
 *                                                                         *
 * INPUT:		int 		- the width in pixels of the GraphicBufferClass    *
 *					int		- the heigh in pixels of the GraphicBufferClass		*
 *					void *	- pointer to user supplied buffer (system will		*
 *								  allocate space if buffer is NULL)						*
 *					long		- size of the user provided buffer						*
 *					GBC_Enum	- flags if this is defined as a direct draw			*
 *	                       surface														*
 *                                                                         *
 * OUTPUT:		none                                                        *
 *                                                                         *
 * HISTORY:                                                                *
 *   10/09/1995     : Created.                                             *
 *=========================================================================*/
void GraphicBufferClass::Init(int w, int h, void *buffer, long size, GBC_Enum flags)
{
	Size			= size;									// find size of physical buffer
	Width			= w;										// Record width of Buffer
	Height		= h;										// Record height of Buffer

	//
	// If the surface we are creating is a direct draw object then
	//   we need to do a direct draw init.  Otherwise we will do
	//   a normal alloc.
	//
	if (flags & (GBC_VIDEOMEM | GBC_VISIBLE)) {
		DD_Init(flags);
	} else {
		if (buffer) {										// if buffer is specified
			Buffer		= (BYTE *)buffer;				//		point to it and mark
			Allocated	= FALSE;							//		it as user allocated
		} else {
			if (!Size) Size = w*h;
			Buffer		= new BYTE[Size];				// otherwise allocate it and
			Allocated	= TRUE;							//		mark it system alloced
		}
		Offset			= (long)Buffer;				// Get offset to the buffer
		IsDirectDraw	= FALSE;
	}

	Pitch			= 0;										// Record width of Buffer
	XAdd			= 0;										// Record XAdd of Buffer
	XPos			= 0;										// Record XPos of Buffer
	YPos			= 0;										// Record YPos of Buffer
	GraphicBuff	= this;									// Get a pointer to our self
}


/***********************************************************************************************
 * GBC::Un_Init -- releases the video surface belonging to this gbuffer                        *
 *                                                                                             *
 *                                                                                             *
 *                                                                                             *
 * INPUT:    Nothing                                                                           *
 *                                                                                             *
 * OUTPUT:   Nothing                                                                           *
 *                                                                                             *
 * WARNINGS: None                                                                              *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *    6/6/96 12:44PM ST : Created                                                              *
 *=============================================================================================*/

void GraphicBufferClass::Un_Init (void)
{
	/*
	** S08: no DirectDraw surface to release. If this buffer is the one the
	** platform layer shows, stop showing it before its memory goes away
	** (BufferClass frees the memory itself).
	*/
	if (VisibleFrameBuffer == this) {
		Platform_Set_Frame_Buffer(NULL, 0, 0, 0);
		VisibleFrameBuffer = NULL;
	}
	VideoSurfacePtr = NULL;
}


/***************************************************************************
 * GBC::GRAPHICBUFFERCLASS -- Default constructor (requires explicit init) *
 *                                                                         *
 * INPUT:		none                                                        *
 *                                                                         *
 * OUTPUT:     none                                                        *
 *                                                                         *
 * HISTORY:                                                                *
 *   10/09/1995     : Created.                                             *
 *=========================================================================*/
GraphicBufferClass::GraphicBufferClass(void)
{
	GraphicBuff			= this; 							// Get a pointer to our self
	VideoSurfacePtr	= NULL;
	memset(&VideoSurfaceDescription, 0, sizeof(VideoSurfaceDescription));
}


/***************************************************************************
 * GBC::GRAPHICBUFFERCLASS -- Constructor for fixed size buffers           *
 *                                                                         *
 * INPUT:		long size		- size of the buffer to create					*
 *					int w			- width of buffer in pixels (default = 320)  *
 *					int h			- height of buffer in pixels (default = 200) *
 *					void *buffer	- a pointer to the buffer if any (optional)	*
 *                                                                         *
 * OUTPUT:                                                                 *
 *                                                                         *
 * WARNINGS:                                                               *
 *                                                                         *
 * HISTORY:                                                                *
 *   05/13/1994 PWG : Created.                                             *
 *=========================================================================*/
GraphicBufferClass::GraphicBufferClass(int w, int h, void *buffer, long size)
{
	Init(w, h, buffer, size, GBC_NONE);
}
/*=========================================================================*
 * GBC::GRAPHICBUFFERCLASS -- inline constructor for GraphicBufferClass		*
 *                                                                         *
 * INPUT:		int w			- width of buffer in pixels (default = 320)  *
 *					int h			- height of buffer in pixels (default = 200) *
 *					void *buffer	- a pointer to the buffer if any (optional)	*
 *                                                                         *
 * OUTPUT:     none                                                        *
 *                                                                         *
 * HISTORY:                                                                *
 *   05/03/1994 PWG : Created.                                             *
 *=========================================================================*/
GraphicBufferClass::GraphicBufferClass(int w, int h, void *buffer)
{
	Init(w, h, buffer, w * h, GBC_NONE);
}

/*====================================================================================*
 * GBC::GRAPHICBUFFERCLASS -- contructor for GraphicsBufferClass with special flags   *
 *                                                                                    *
 * INPUT:		int w			- width of buffer in pixels (default = 320)           *
 *					int h			- height of buffer in pixels (default = 200)      *
 *					void *buffer	- unused                                	      *
 *               unsigned flags - flags for creation of special buffer types          *
 *                                GBC_VISIBLE - buffer is a visible screen surface    *
 *                                GBC_VIDEOMEM - buffer resides in video memory       *
 *                                                                                    *
 * OUTPUT:     none                                                                   *
 *                                                                                    *
 * HISTORY:                                                                           *
 *   09-21-95 04:19pm ST : Created                                                    *
 *====================================================================================*/
GraphicBufferClass::GraphicBufferClass(int w, int h, GBC_Enum flags)
{
	Init(w, h, NULL, w * h, flags);
}

/*=========================================================================*
 * GBC::~GRAPHICBUFFERCLASS -- Destructor for the graphic buffer class     *
 *                                                                         *
 *	INPUT:		none																			*
 *                                                                         *
 * OUTPUT:     none                                                        *
 *                                                                         *
 * HISTORY:                                                                *
 *   05/03/1994 PWG : Created.                                             *
 *=========================================================================*/
GraphicBufferClass::~GraphicBufferClass()
{

//
// Release the direct draw surface if it exists
//
	Un_Init();
}



/***************************************************************************
 * SET_LOGIC_PAGE -- Sets LogicPage to new buffer                          *
 *                                                                         *
 * INPUT:		GraphicBufferClass * the buffer we are going to set         *
 *                                                                         *
 * OUTPUT:     GraphicBufferClass * the previous buffer type					*
 *                                                                         *
 * WARNINGS:                                                               *
 *                                                                         *
 * HISTORY:                                                                *
 *   02/23/1995 PWG : Created.                                             *
 *=========================================================================*/
GraphicViewPortClass *Set_Logic_Page(GraphicViewPortClass *ptr)
{
	GraphicViewPortClass *old = LogicPage;
	LogicPage					= ptr;
	return(old);
}

/***************************************************************************
 * SET_LOGIC_PAGE -- Sets LogicPage to new buffer                          *
 *                                                                         *
 * INPUT:		GraphicBufferClass & the buffer we are going to set         *
 *                                                                         *
 * OUTPUT:     GraphicBufferClass * the previous buffer type					*
 *                                                                         *
 * WARNINGS:                                                               *
 *                                                                         *
 * HISTORY:                                                                *
 *   02/23/1995 PWG : Created.                                             *
 *=========================================================================*/
GraphicViewPortClass *Set_Logic_Page(GraphicViewPortClass &ptr)
{
	GraphicViewPortClass *old = LogicPage;
	LogicPage					= &ptr;
	return(old);
}


/***************************************************************************
 * GBC::LOCK -- Locks a Direct Draw Surface                                *
 *                                                                         *
 * INPUT:		none                                                        *
 *                                                                         *
 * OUTPUT:     none                                                        *
 *                                                                         *
 * HISTORY:                                                                *
 *   10/09/1995     : Created.                                             *
 *   10/09/1995     : Code stolen from Steve Tall                          *
 *=========================================================================*/
BOOL GraphicBufferClass::Lock(void)
{
	/*
	** S08: every buffer is plain system memory now (see DD_Init), so a lock is
	** always successful and there is nothing to lock. The original DirectDraw
	** path (lock the surface, restore it if lost, block the mouse thread) is gone;
	** see docs/codebase/platform-layer.md.
	*/
	return(TRUE);
}


/***************************************************************************
 * GBC::UNLOCK -- Unlocks a buffer                                         *
 *                                                                         *
 * INPUT:		none                                                        *
 *                                                                         *
 * OUTPUT:     none                                                        *
 *                                                                         *
 * HISTORY:                                                                *
 *   10/09/1995     : Created.                                             *
 *   10/09/1995     : Code stolen from Steve Tall                          *
 *   2026-10-14 S08: system memory only, nothing to unlock.                *
 *=========================================================================*/
BOOL GraphicBufferClass::Unlock(void)
{
	return(TRUE);
}


/***********************************************************************************************
 * GVPC::DD_Linear_Blit_To_Linear -- blit using the hardware blitter                           *
 *                                                                                             *
 *                                                                                             *
 *                                                                                             *
 * INPUT:    destination vvpc                                                                  *
 *           x coord to blit from                                                              *
 *           y coord to blit from                                                              *
 *           x coord to blit to                                                                *
 *           y coord to blit to                                                                *
 *           width to blit                                                                     *
 *           height to blit                                                                    *
 *                                                                                             *
 * OUTPUT:   DD_OK if successful                                                               *
 *                                                                                             *
 * WARNINGS: None                                                                              *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   09-22-95 11:05am ST : Created                                                             *
 *=============================================================================================*/

HRESULT GraphicViewPortClass::DD_Linear_Blit_To_Linear (
								  GraphicViewPortClass &dest
								, int source_x
								, int source_y
								, int dest_x
								, int dest_y
								, int width
								, int height
								, BOOL mask )

{
	/*
	** S08: there is no hardware blitter any more. Buffers are never IsDirectDraw,
	** so GBUFFER.H no longer calls this; it is kept as a software copy so any
	** remaining caller still gets the right pixels. Coordinates are buffer-relative
	** (the callers add XPos/YPos), and `mask` skips colour 0 like the DirectDraw
	** source colour key did.
	*/
	GraphicBufferClass *src_buff = GraphicBuff;
	GraphicBufferClass *dst_buff = dest.GraphicBuff;
	if (!src_buff || !dst_buff || width <= 0 || height <= 0) return (HRESULT)-1;

	int src_stride = src_buff->Get_Width() + src_buff->Get_XAdd() + src_buff->Get_Pitch();
	int dst_stride = dst_buff->Get_Width() + dst_buff->Get_XAdd() + dst_buff->Get_Pitch();
	unsigned char *src = (unsigned char *)src_buff->Get_Offset() + source_y * src_stride + source_x;
	unsigned char *dst = (unsigned char *)dst_buff->Get_Offset() + dest_y * dst_stride + dest_x;

	/* Same buffer, destination further down: copy bottom-up so rows are not overwritten. */
	bool backwards = (src_buff == dst_buff && dst > src);
	for (int i = 0; i < height; i++) {
		int row = backwards ? (height - 1 - i) : i;
		unsigned char *s = src + row * src_stride;
		unsigned char *d = dst + row * dst_stride;
		if (mask) {
			for (int x = 0; x < width; x++) {
				if (s[x]) d[x] = s[x];
			}
		} else {
			memmove(d, s, width);
		}
	}
	return (HRESULT)0;		// DD_OK
}




