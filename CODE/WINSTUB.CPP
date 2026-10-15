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

/* $Header: /CounterStrike/WINSTUB.CPP 3     3/13/97 2:06p Steve_tall $ */
/***********************************************************************************************
 ***              C O N F I D E N T I A L  ---  W E S T W O O D  S T U D I O S               ***
 ***********************************************************************************************
 *                                                                                             *
 *                 Project Name : Command & Conquer                                            *
 *                                                                                             *
 *                    File Name : WINSTUB.CPP                                                  *
 *                                                                                             *
 *                   Programmer : Steve Tall                                                   *
 *                                                                                             *
 *                   Start Date : 10/04/95                                                     *
 *                                                                                             *
 *                  Last Update : October 4th 1995 [ST]                                        *
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 * Overview:                                                                                   *
 *   This file contains stubs for undefined externals when linked under Watcom for Win 95      *
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 *                                                                                             *
 * Functions:                                                                                  *
 *   Assert_Failure -- display the line and source file where a failed assert occurred         *
 *   Check_For_Focus_Loss -- check for the end of the focus loss                               *
 *   Game_Shutdown -- what WM_DESTROY did (S10b; replaces Create_Main_Window/Windows_Procedure)*
 *   Install_Platform_Callbacks -- focus callback and quit hook (S10b)                         *
 *   Focus_Loss -- this function is called when a library function detects focus loss          *
 *   Memory_Error_Handler -- Handle a possibly fatal failure to allocate memory                *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#include	"function.h"

#include "WSProto.h"
#include "platform.h"		// S10b: SDL platform layer (focus, quit, event pump)

void output(short,short)
{}


// S10b: CCFocusMessage (a private window message) is gone; see Got_Focus.
static void Got_Focus(void);
extern	void VQA_PauseAudio(void);
extern	void VQA_ResumeAudio(void);

//#include "WolDebug.h"


/***********************************************************************************************
 * Focus_Loss -- this function is called when a library function detects focus loss            *
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
 *    2/1/96 2:10PM ST : Created                                                               *
 *=============================================================================================*/

void Focus_Loss(void)
{
	Theme.Suspend();
	Stop_Primary_Sound_Buffer();
	if (WWMouse) WWMouse->Clear_Cursor_Clip();
}

void Focus_Restore(void)
{
	Restore_Cached_Icons();
	Map.Flag_To_Redraw(true);
	Start_Primary_Sound_Buffer(TRUE);
	if (WWMouse) WWMouse->Set_Cursor_Clip();
	VisiblePage.Clear();
	HiddenPage.Clear();
}



/***********************************************************************************************
 * Check_For_Focus_Loss -- check for the end of the focus loss                                 *
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
 *    2/2/96 10:49AM ST : Created                                                              *
 *=============================================================================================*/

void Check_For_Focus_Loss(void)
{
	static BOOL	focus_last_time = 1;

	/*
	** S10b: the two PeekMessage/GetMessage/DispatchMessage loops became platform pumps.
	** GameInFocus is updated by the platform focus callback (On_Focus_Change below).
	** While out of focus this pumps once and sleeps a little, then returns: callers that
	** wait for focus loop around it (Main_Loop calls it every frame).
	*/
	if ( !GameInFocus ) {
		Focus_Loss();
		Platform_Pump_Events();
		Platform_Sleep(10);
	}

	if (!focus_last_time && GameInFocus) {

		VQA_PauseAudio();
		CountDownTimerClass cd;
		cd.Set(60*1);

		do {
			Platform_Pump_Events();
			Platform_Sleep(1);
		} while (cd.Time());
		VQA_ResumeAudio();
		Got_Focus();			// was PostMessage (MainWindow, CCFocusMessage, 0, 0)
//		AllSurfaces.Restore_Surfaces();
//		VisiblePage.Clear();
//		HiddenPage.Clear();
//		Map.Flag_To_Redraw(true);
	}

	focus_last_time = GameInFocus;

}



extern bool InMovie;

/***********************************************************************************************
 * S10b: Windows_Procedure is gone. What its messages did now happens here:                   *
 *                                                                                             *
 *   CCFocusMessage          -> Got_Focus (called by Check_For_Focus_Loss)                     *
 *   WM_ACTIVATEAPP          -> On_Focus_Change (platform focus callback)                      *
 *   WM_DESTROY              -> Game_Shutdown (called directly at exit)                        *
 *   SC_CLOSE (ignored)      -> Quit_Poll_Hook: closing the window now exits the game          *
 *   SC_SCREENSAVE (blocked) -> SDL disables the screen saver while the window is open         *
 *   keyboard/mouse messages -> the platform input queue (KEY.CPP Fill_Buffer_From_System)    *
 *   Winsock async messages  -> nothing until S12's polled transport (PacketTransport)         *
 *=============================================================================================*/
static void Got_Focus(void)
{
	Start_Primary_Sound_Buffer(TRUE);
	if (!InMovie) {
		Theme.Stop();
		Theme.Queue_Song(THEME_PICK_ANOTHER);
	}
}


/*
** WM_ACTIVATEAPP: the platform layer calls this when the window gains or loses focus.
*/
static void On_Focus_Change(bool in_focus)
{
	GameInFocus = in_focus;
	if (!GameInFocus) Focus_Loss();
	AllSurfaces.Set_Surface_Focus (GameInFocus);
	AllSurfaces.Restore_Surfaces();
}


/*
** The window was closed (close button, Alt+F4, the desktop shutting down). The original
** ignored SC_CLOSE; the port exits cleanly instead, the same way Emergency_Exit does. Runs
** from the platform pump, which every input check reaches (Keyboard->Check()).
*/
static void Quit_Poll_Hook(void *)
{
	static bool quitting = false;
	if (quitting || !Platform_Quit_Requested()) return;
	quitting = true;
	Emergency_Exit(EXIT_SUCCESS);
}


/***********************************************************************************************
 * Install_Platform_Callbacks -- hook the game into the platform layer (S10b)                 *
 *                                                                                             *
 *    Called by main right after Platform_Init. Replaces the window class registration and    *
 *    the focus-loss function pointers that Create_Main_Window set up.                        *
 *=============================================================================================*/
void Install_Platform_Callbacks(void)
{
	Platform_Set_Focus_Callback(On_Focus_Change);
	Platform_Add_Poll_Hook(Quit_Poll_Hook, NULL);

	Audio_Focus_Loss_Function = &Focus_Loss;
	Misc_Focus_Loss_Function = &Focus_Loss;
	Misc_Focus_Restore_Function = &Focus_Restore;
	Gbuffer_Focus_Loss_Function = &Focus_Loss;
}


/***********************************************************************************************
 * Game_Shutdown -- what the WM_DESTROY handler did, called directly (S10b)                   *
 *                                                                                             *
 *    The original posted WM_DESTROY to its own window and waited for the message loop to     *
 *    run this. shutdown_network corresponds to ReadyToQuit 0/3 (forced or emergency exit).   *
 *    Safe to call more than once.                                                             *
 *=============================================================================================*/
void Game_Shutdown(bool shutdown_network)
{
	static bool done = false;
	if (done) return;
	done = true;

	Prog_End();
	Invalidate_Cached_Icons();
	VisiblePage.Un_Init();
	HiddenPage.Un_Init();
	AllSurfaces.Release();
	if (!InDebugger) Reset_Video_Mode();
	if (shutdown_network) {
		Shutdown_Network();
	}
	ReadyToQuit = 2;
	Platform_Shutdown();
}
HANDLE DebugFile = INVALID_HANDLE_VALUE;

/***********************************************************************************************
 * WWDebugString -- sends a string to the debugger and echos it to disk                        *
 *                                                                                             *
 *                                                                                             *
 *                                                                                             *
 * INPUT:    string                                                                            *
 *                                                                                             *
 * OUTPUT:   Nothing                                                                           *
 *                                                                                             *
 * WARNINGS: None                                                                              *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *    10/28/96 12:48PM ST : Created                                                              *
 *=============================================================================================*/
void WWDebugString (char *string)
{
#if (0)
	char outstr[256];

	sprintf (outstr, "%s", string);

	DWORD	actual;
	if (DebugFile == INVALID_HANDLE_VALUE){
		DebugFile = CreateFile("debug.txt", GENERIC_WRITE, 0,
												NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	}else{
		DebugFile = CreateFile("debug.txt", GENERIC_WRITE, 0,
												NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	}

	if (DebugFile != INVALID_HANDLE_VALUE){
		SetFilePointer (DebugFile, 0, NULL, FILE_END);
		WriteFile(DebugFile, outstr, strlen(outstr)+1, &actual, NULL);
		CloseHandle (DebugFile);
	}

	OutputDebugString (string);
#else	//(0)

	string = string;
//	debugprint( string );

#endif	//(0)

}









/*
** S10b: Create_Main_Window, Window_Dialog_Box (DialogBox + message loop) and
** Colour_Debug (a 320-wide palette debug helper nothing called) are removed. The
** platform layer owns the window (WIN32LIB/MISC/DDRAW.CPP Set_Video_Mode).
*/


BOOL Any_Locked (void)
{
	if (SeenBuff.Get_LockCount() ||
		HidPage.Get_LockCount()) {
		return (TRUE);
	} else {
		return(FALSE);
	}
}








//
// Miscellaneous stubs. Mainly for multi player stuff
//
//
//

//IPXAddressClass::IPXAddressClass(void) {
//	int i;
//	i++;
//}
//int IPXManagerClass::Num_Connections(void) { return (0); }
//int IPXManagerClass::Connection_ID( int ) { return (0); }
//IPXAddressClass * IPXManagerClass::Connection_Address( int ) { return ((IPXAddressClass*)0); }
//char * IPXManagerClass::Connection_Name( int ) { return ((char*)0); }
//int IPXAddressClass::Is_Broadcast() { return (0); }
//int IPXManagerClass::Send_Global_Message( void *, int, int, IPXAddressClass * ) { return (0); }
//int IPXManagerClass::Service() { return (0); }
//int IPXManagerClass::Get_Global_Message( void  *, int  *, IPXAddressClass  *, short unsigned  * ) { return (0); }
//int IPXAddressClass::operator ==( IPXAddressClass  & ) { return (0); }
//IPXManagerClass::IPXManagerClass( int, int, int, int, short unsigned, short unsigned ) {}
//IPXManagerClass::~IPXManagerClass() {
//	int i;
//	i++;
//	}
//int  IPXManagerClass::Delete_Connection( int ) { return (0); }
//IPXAddressClass::IPXAddressClass( char unsigned  *, char unsigned  * ) {}
//void  IPXManagerClass::Set_Socket( short unsigned ) {}
//int  IPXManagerClass::Is_IPX() { return (0); }
//int  IPXManagerClass::Init() { return (0); }
//void  IPXAddressClass::Get_Address( char unsigned  *, char unsigned  * ) {}
//void  IPXManagerClass::Set_Bridge( char unsigned  * ) {}
//int  IPXManagerClass::Global_Num_Send() { return (0); }
//void  IPXManagerClass::Set_Timing( long unsigned, long unsigned, long unsigned ) {}
//unsigned long IPXManagerClass::Global_Response_Time() { return (0); }
//int  IPXManagerClass::Create_Connection( int, char  *, IPXAddressClass  * ) { return (0); }
//int  IPXAddressClass::operator !=( IPXAddressClass  & ) { return (0); }
//int  IPXManagerClass::Send_Private_Message( void  *, int, int, int ) { return (0); }
//int  IPXManagerClass::Get_Private_Message( void  *, int  *, int  * ) { return (0); }
//int  IPXManagerClass::Connection_Index( int ) { return (0); }
//void  IPXManagerClass::Reset_Response_Time() {}
//long unsigned  IPXManagerClass::Response_Time() { return (0); }
//int  IPXManagerClass::Private_Num_Send( int ) { return (0); }

//_VQAHandle  *  VQA_Alloc(void) { return ((_VQAHandle *)0); }
//void  VQA_Init( _VQAHandle  *, long ( *)()) {}
//long  VQA_Open( _VQAHandle  *, char const  *, _VQAConfig  * ) { return (0); }
//void  VQA_Free( _VQAHandle  * ) {}
//void  VQA_Close( _VQAHandle  * ) {}
//long  VQA_Play( _VQAHandle  *, long ) { return (0); }

//void VQA_Init(VQAHandle *, long(*)(VQAHandle *vqa, long action,	void *buffer, long nbytes)) {}

//long VQA_Open(VQAHandle *, char const *, VQAConfig *)
//{
//	return (0);
//}

//void VQA_Close(VQAHandle *) {}

//long VQA_Play(VQAHandle *, long)
//{
//	return (0);
//}


unsigned char	*VQPalette;
long				VQNumBytes;
unsigned long	VQSlowpal;
bool				VQPaletteChange = false;


extern "C"{
	void __cdecl SetPalette(unsigned char *palette, long numbytes, unsigned long slowpal);
}



void Flag_To_Set_Palette(unsigned char *palette, long numbytes, unsigned long slowpal)
{
	VQPalette = palette;
	VQNumBytes = numbytes;
	VQSlowpal = slowpal;
	VQPaletteChange = true;
}



void Check_VQ_Palette_Set(void)
{
	if (VQPaletteChange) {
		SetPalette(VQPalette, VQNumBytes, VQSlowpal);
		VQPaletteChange = false;
	}
}





void __cdecl SetPalette(unsigned char *palette, long, unsigned long)
{
	for (int i=0 ; i<256*3 ; i++) {
		*(palette+i)&=63;
	}
	Increase_Palette_Luminance(palette , 15 , 15 , 15 ,63);

	if (PalettesRead) {
		memcpy (&PaletteInterpolationTable[0][0] , InterpolatedPalettes[PaletteCounter++] , 65536);
	}
	Set_Palette(palette);
}


#ifndef NDEBUG
/***********************************************************************************************
 * Assert_Failure -- display the line and source file where a failed assert occurred           *
 *                                                                                             *
 *                                                                                             *
 * INPUT:    line number in source file                                                        *
 *           name of source file                                                               *
 *                                                                                             *
 * OUTPUT:   Nothing                                                                           *
 *                                                                                             *
 * WARNINGS: None                                                                              *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *    4/17/96 9:58AM ST : Created                                                              *
 *=============================================================================================*/

void Assert_Failure (char *expression, int line, char *file)
{
	char	assertbuf[256];
	char   timebuff[512];
	sprintf (assertbuf, "assert '%s' failed at line %d in module %s.\n", expression, line, file);

	if (!MonoClass::Is_Enabled()) MonoClass::Enable();

	Mono_Clear_Screen();
	Mono_Printf("%s", assertbuf);

	WWDebugString(assertbuf);
	fputs(assertbuf, stderr);		// S10c: visible in a terminal / CI log
	fflush(stderr);

	/*
	** S10b: GetLocalTime + mmioOpen/mmioWrite became time/localtime + fopen. (The original
	** mmioOpen without MMIO_CREATE/append rewrote ASSERT.TXT from the start each time.)
	*/
	time_t now = time(NULL);
	struct tm * lt = localtime(&now);
	sprintf (timebuff, "%02d/%02d/%04d %02d:%02d:%02d - %s", lt ? lt->tm_mon + 1 : 0, lt ? lt->tm_mday : 0,
																		lt ? lt->tm_year + 1900 : 0,
																		lt ? lt->tm_hour : 0, lt ? lt->tm_min : 0, lt ? lt->tm_sec : 0,
																		assertbuf);

	FILE * handle = fopen("ASSERT.TXT", "wb");
	if (handle) {
		fwrite(timebuff, 1, strlen(timebuff), handle);
		fclose(handle);
	}

	/*
	**	S10c: the game's message box draws with the game font; before the fonts are loaded
	**	(an assert during start-up) use the platform's message box instead of crashing.
	*/
	if (Font8Ptr != NULL) {
		WWMessageBox().Process(assertbuf);
	} else {
		Platform_Message_Box("Red Alert", assertbuf);
	}
//	WWMessageBox().Process("Red Alert demo timed out - Aborting");
	//Get_Key();

	Game_Shutdown(false);		// S10b: was Prog_End, Invalidate_Cached_Icons, PostQuitMessage
	exit(0);					// S10b: was ExitProcess(0)
}
#endif







/***********************************************************************************************
 * Memory_Error_Handler -- Handle a possibly fatal failure to allocate memory                  *
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
 *    5/22/96 3:57PM ST : Created                                                              *
 *=============================================================================================*/
void Memory_Error_Handler(void)
{
	VisiblePage.Clear();
	CCPalette.Set();
	while (Get_Mouse_State()) {Show_Mouse();};
	WWMessageBox().Process(TEXT_MEMORY_ERROR, TEXT_ABORT);

	ReadyToQuit = 1;

	/*
	** S10b: was PostMessage(MainWindow, WM_DESTROY) + a wait for the message handler, then
	** ExitProcess(0).
	*/
	Game_Shutdown(false);
	exit(0);
}








GraphicBufferClass* Read_PCX_File(char* name, char* Palette, void *Buff, long Size);
void Load_Title_Screen(char *name, GraphicViewPortClass *video_page, unsigned char *palette)
{

	GraphicBufferClass *load_buffer;


	load_buffer = Read_PCX_File (name, (char*)palette, NULL, 0);

	if (load_buffer) {
		load_buffer->Blit(*video_page);
		delete load_buffer;
	}
}



#include "filepcx.h"

/***************************************************************************
 * READ_PCX_FILE -- read a pcx file into a Graphic Buffer                  *
 *                                                                         *
 *	GraphicBufferClass* Read_PCX_File (char* name, char* palette ,void *Buff, long size );	*
 *  																								*
 *                                                                         *
 * INPUT: name is a NULL terminated string of the format [xxxx.pcx]        *
 *        palette is optional, if palette != NULL the the color palette of *
 *					 the pcx file will be place in the memory block pointed	   *
 *               by palette.																*
 *			 Buff is optional, if Buff == NULL a new memory Buffer		 		*
 *					 will be allocated, otherwise the file will be placed 		*
 *					 at location pointed by Buffer;										*
 *			Size is the size in bytes of the memory block pointed by Buff		*
 *				  is also optional;															*                                                                         *
 * OUTPUT: on success a pointer to a GraphicBufferClass containing the     *
 *         pcx file, NULL otherwise.                                       *
 *																									*
 * WARNINGS:                                                               *
 *         Appears to be a comment-free zone                               *
 *                                                                         *
 * HISTORY:                                                                *
 *   05/03/1995 JRJ : Created.                                             *
 *   04/30/1996 ST : Tidied up and modified to use CCFileClass             *
 *=========================================================================*/

#define	POOL_SIZE 2048
#define	READ_CHAR()  *file_ptr++ ; \
							 if ( file_ptr	>= & pool [ POOL_SIZE ]	) { \
								 file_handle.Read (pool , POOL_SIZE ); \
								 file_ptr = pool ; \
							 }


GraphicBufferClass* Read_PCX_File(char* name, char* palette, void *Buff, long Size)
{
	unsigned					i, j;
	unsigned					rle;
	unsigned					color;
	unsigned					scan_pos;
	char						*file_ptr;
	int						width;
	int						height;
	char						*buffer;
	PCX_HEADER				header;
	RGB						*pal;
	char						pool [POOL_SIZE];
	GraphicBufferClass	*pic;

	CCFileClass file_handle(name);

	if (!file_handle.Is_Available()) return (NULL);

	file_handle.Open(READ);

	file_handle.Read (&header, sizeof (PCX_HEADER));

	if (header.id != 10 &&  header.version != 5 && header.pixelsize != 8 ) return NULL ;

	width = header.width - header.x + 1;
	height = header.height - header.y + 1;

	if (Buff) {
    	buffer = (char *)Buff;
    	i = Size / width;
    	height = MIN (i - 1, height);
    	pic = new GraphicBufferClass(width, height, buffer, Size);
    	if ( !(pic && pic->Get_Buffer()) ) return NULL ;
	} else {
    	pic = new GraphicBufferClass(width, height, NULL, width*(height+4));
    	if ( !(pic && pic->Get_Buffer()) ) return NULL ;
	}

	buffer = (char *) pic->Get_Buffer();
	file_ptr = pool ;
	file_handle.Read (pool , POOL_SIZE);

	if ( header.byte_per_line != width ) {

		for ( scan_pos = j = 0 ; j < height ; j ++, scan_pos += width ) {
			for ( i = 0 ; i < width ; ) {
				rle = READ_CHAR ();
				if ( rle > 192 ) {
					rle -= 192 ;
					color =	READ_CHAR (); ;
					memset ( buffer + scan_pos + i , color , rle );
					i += rle;
				} else {
					*(buffer+scan_pos + i++ ) = (char)rle;
				}
			}
      	}

		if ( i == width ) rle = READ_CHAR ();
		if ( rle > 192 )  rle = READ_CHAR ();

	} else {

		for ( i = 0 ; i < width * height ; ) {
  			rle = READ_CHAR ();
  			rle &= 0xff;
  			if ( rle > 192 ) {
        		rle -= 192 ;
        		color = READ_CHAR ();
  				memset ( buffer + i , color , rle );
        		i += rle ;
     		} else {
				*(buffer + i++) = (char)rle;
			}
		}
	}

	if ( palette ) {
		file_handle.Seek (- (256 * sizeof ( RGB )) , SEEK_END );
		file_handle.Read (palette , 256L * sizeof ( RGB ));

		pal = ( RGB * ) palette;
		for (i = 0 ; i < 256 ; i ++) {
			pal ->red	>>= 2;
			pal ->green	>>= 2;
			pal ->blue	>>= 2;
			pal ++ ;
		}
	}

	file_handle.Close();
	return pic;
}
