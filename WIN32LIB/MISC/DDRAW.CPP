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
 *                 Project Name : Westwood Win32 Library                   *
 *                                                                         *
 *                    File Name : DDRAW.CPP                                *
 *                                                                         *
 *                   Programmer : Philip W. Gorrow                         *
 *                                                                         *
 *                   Start Date : October 10, 1995                         *
 *                                                                         *
 *                  Last Update : October 10, 1995   []                    *
 *                                                                         *
 *-------------------------------------------------------------------------*
 * Functions:                                                              *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

/*=========================================================================*/
/* The following PRIVATE functions are in this file:                       */
/*=========================================================================*/


/*= = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = =*/


#include "misc.h"
#include <gbuffer.h>
#include <palette.h>
#include "platform.h"

/*
** S08 (2026-10-14): DirectDraw is replaced by the platform layer (platform/platform.h,
** SDL3 underneath, ADR-0004). This file keeps every function and global the game and
** the rest of WIN32LIB use, with the same names and signatures, and forwards them:
**
**    Set_Video_Mode      -> Platform_Video_Init (window + renderer at the game size)
**    Reset_Video_Mode    -> Platform_Video_Shutdown
**    Set_DD_Palette      -> Platform_Set_Palette (6-bit values, << 2 like before)
**    Wait_Vert_Blank     -> Platform_Wait_Vertical_Blank (present, wait to 1/60 s)
**    video memory / caps -> none: every buffer is system memory (GBUFFER.CPP DD_Init)
**
** The DirectDraw object/palette/surface globals stay defined (always NULL) because
** headers declare them and CODE/CONQUER.CPP's MPEG movie path still names them;
** see docs/codebase/platform-layer.md, "Remaining integration for S10".
*/

LPDIRECTDRAW		DirectDrawObject=NULL;	// Always NULL now (S08)
LPDIRECTDRAW2	DirectDraw2Interface = NULL;  	// Always NULL now (S08)

HWND			MainWindow;					// Handle to programs main window (still set by
												// CODE/WINSTUB.CPP Create_Main_Window until S10)

PALETTEENTRY				PaletteEntries[256];		// Last palette set, 8-bit (kept for old users)
LPDIRECTDRAWPALETTE	PalettePtr = NULL;			// Always NULL now (S08)
BOOL						FirstPaletteSet=FALSE;	// Is this the first time 'Set_Palette' has been called?
LPDIRECTDRAWSURFACE	PaletteSurface=NULL;			// Always NULL now (S08)
SurfaceMonitorClass	AllSurfaces;				//List of all direct draw surfaces (always empty)
BOOL						CanVblankSync = TRUE;

BOOL						SystemToVideoBlits =FALSE;	// Does hardware support system mem to video mem blits?
BOOL						VideoToSystemBlits =FALSE;	// Does hardware support video mem to system mem blits?
BOOL						SystemToSystemBlits = FALSE; 	// Does hardware support system mem to system mem blits?
BOOL						OverlappedVideoBlits = TRUE;	// Software blits handle overlapping regions

/*
** Function to call if we detect focus loss
*/
extern	void (*Misc_Focus_Loss_Function)(void) = NULL;
extern	void (*Misc_Focus_Restore_Function)(void) = NULL;


/***********************************************************************************************
 * Process_DD_Result -- Does a message box based on the result of a DD command                 *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   09/27/1995 PWG : Created.                                                                 *
 *   2026-10-14 S08: there are no DirectDraw results any more; does nothing.                   *
 *=============================================================================================*/
void Process_DD_Result(HRESULT , int )
{
}


/***********************************************************************************************
 * Check_Overlapped_Blit_Capability -- See if video driver supports blitting overlapped regions*
 *                                                                                             *
 * HISTORY:                                                                                    *
 *    6/7/96 5:06PM ST : Created                                                               *
 *    2026-10-14 S08: software blits always handle overlap.                                    *
 *=============================================================================================*/
void Check_Overlapped_Blit_Capability(void)
{
	OverlappedVideoBlits = TRUE;
}


/***********************************************************************************************
 * Set_Video_Mode -- Initializes Direct Draw and sets the required Video Mode                  *
 *                                                                                             *
 * INPUT:    hwnd           - ignored (the platform layer owns its window)                    *
 *           w, h           - game resolution: 640x400, or 640x480 (Resolution=1 in the INI)  *
 *           bits_per_pixel - must be 8                                                        *
 *                                                                                             *
 * OUTPUT:   TRUE if the window could be opened                                                *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   09/26/1995 PWG : Created.                                                                 *
 *   2026-10-14 S08: opens (or resizes) the platform window instead of a display mode.        *
 *=============================================================================================*/
BOOL Set_Video_Mode(HWND , int w, int h, int bits_per_pixel)
{
	if (bits_per_pixel != 8) {
		return (FALSE);
	}

	PlatformVideoConfig config;
	Platform_Default_Video_Config(&config);
	config.Width = w;
	config.Height = h;
	if (!Platform_Video_Init(&config, "Red Alert")) {
		return (FALSE);
	}

	/*
	** The game draws its own cursor into the visible page (WWMouseClass).
	*/
	Platform_Show_System_Cursor(false);
	Check_Overlapped_Blit_Capability();
	return (TRUE);
}


/***********************************************************************************************
 * Reset_Video_Mode -- Resets video mode and deletes Direct Draw Object                        *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   09/26/1995 PWG : Created.                                                                 *
 *   2026-10-14 S08: closes the platform window.                                               *
 *=============================================================================================*/
void Reset_Video_Mode(void)
{
	Platform_Video_Shutdown();
}


/***********************************************************************************************
 * Get_Free_Video_Memory -- returns amount of free video memory                                *
 *                                                                                             *
 * OUTPUT:   0: there is no video memory; the game then keeps its hidden page in system memory*
 *                                                                                             *
 * HISTORY:                                                                                    *
 *    11/29/95 12:52PM ST : Created                                                            *
 *    2026-10-14 S08: always 0.                                                                *
 *=============================================================================================*/
unsigned int Get_Free_Video_Memory(void)
{
	return (0);
}


/***********************************************************************************************
 * Get_Video_Hardware_Caps -- returns bitmask of direct draw video hardware support            *
 *                                                                                             *
 * OUTPUT:   VIDEO_NO_HARDWARE_ASSIST: every drawing operation is done in software            *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *    1/12/96 9:14AM ST : Created                                                              *
 *    2026-10-14 S08: no hardware assistance.                                                  *
 *=============================================================================================*/
unsigned Get_Video_Hardware_Capabilities(void)
{
	return (VIDEO_NO_HARDWARE_ASSIST);
}


/***********************************************************************************************
 * Wait_Vert_Blank -- Waits for the start (leading edge) of a vertical blank                   *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   2026-10-14 S08: present the frame and wait for the next 1/60 s boundary.                  *
 *=============================================================================================*/
extern int ScreenWidth;

void Wait_Vert_Blank(void)
{
	if (CanVblankSync) {
		Platform_Wait_Vertical_Blank();
	}
}


/***********************************************************************************************
 * Set_DD_Palette -- set up the palette                                                        *
 *                                                                                             *
 * INPUT:    palette: 768 bytes, 256 RGB triplets of 6-bit (0..63) VGA values                 *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   10/11/1995 PWG : Created.                                                                 *
 *   2026-10-14 S08: hands the palette to the platform layer (same << 2 expansion).            *
 *=============================================================================================*/
void Set_DD_Palette ( void *palette )
{
	/*
	** Trap null ptr
	*/
	if (!palette) return;

	unsigned char *palette_get = (unsigned char *)palette;
	for (int k = 0; k < 256; k++) {
		PaletteEntries[k].peRed   = (unsigned char)(palette_get[k*3+0] << 2);
		PaletteEntries[k].peGreen = (unsigned char)(palette_get[k*3+1] << 2);
		PaletteEntries[k].peBlue  = (unsigned char)(palette_get[k*3+2] << 2);
	}
	Platform_Set_Palette(palette_get, 6);
	FirstPaletteSet = TRUE;
}


/***********************************************************************************************
 * Wait_Blit -- waits for the DirectDraw blitter to become idle                                *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   07-25-95 03:53pm ST : Created                                                             *
 *   2026-10-14 S08: blits are synchronous software copies; nothing to wait for.               *
 *=============================================================================================*/
void Wait_Blit (void)
{
}


/***********************************************************************************************
 * SMC::SurfaceMonitorClass -- constructor for surface monitor class                           *
 *                                                                                             *
 *    S08: no DirectDraw surfaces exist any more, so the list stays empty. The class is kept  *
 *    because the game calls Set_Surface_Focus/Restore_Surfaces when it gains focus and reads *
 *    SurfacesRestored; Restore_Surfaces still runs the focus-restore callback and resets the *
 *    palette, which is what the game relies on.                                               *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   11/3/95 3:23PM ST : Created                                                               *
 *=============================================================================================*/
SurfaceMonitorClass::SurfaceMonitorClass(void)
{
	for (int i=0 ; i<MAX_SURFACES ; i++)
	{
		Surface[i]=NULL;
	}
	InFocus=FALSE;
	SurfacesRestored=FALSE;
}


void SurfaceMonitorClass::Add_DD_Surface (LPDIRECTDRAWSURFACE new_surface)
{
	if ( new_surface && !Got_Surface_Already (new_surface) ){
		for (int i=0 ; i<MAX_SURFACES ; i++)
		{
			if ( Surface[i]==NULL ){
				Surface[i]=new_surface;
				return;
			}
		}
	}
}


void SurfaceMonitorClass::Remove_DD_Surface (LPDIRECTDRAWSURFACE old_surface)
{
	for (int i=0 ; i<MAX_SURFACES ; i++)
	{
		if ( Surface[i]==old_surface ){
			Surface[i]=NULL;
			return;
		}
	}
}


BOOL SurfaceMonitorClass::Got_Surface_Already (LPDIRECTDRAWSURFACE test_surface)
{
	for (int i=0 ; i<MAX_SURFACES ; i++)
	{
		if ( Surface[i]==test_surface ){
			return(TRUE);
		}
	}
	return (FALSE);
}


void SurfaceMonitorClass::Restore_Surfaces (void)
{
	if (InFocus){
		/*
		** Nothing to restore (system memory is never lost). Notify the program as
		** before so it can clear the pages and redraw, then restore the palette.
		*/
		if (Misc_Focus_Restore_Function){
			Misc_Focus_Restore_Function();
		}
		SurfacesRestored = TRUE;
		Set_DD_Palette (CurrentPalette);
	}
}


void SurfaceMonitorClass::Set_Surface_Focus ( BOOL in_focus )
{
	InFocus=in_focus;
}


void SurfaceMonitorClass::Release(void)
{
	for (int i=0 ; i<MAX_SURFACES ; i++)
	{
		Surface[i] = 0;
	}
}
