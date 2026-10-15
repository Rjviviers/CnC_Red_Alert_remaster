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

/* $Header: /counterstrike/STARTUP.CPP 6     3/15/97 7:18p Steve_tall $ */
/***********************************************************************************************
 ***              C O N F I D E N T I A L  ---  W E S T W O O D  S T U D I O S               ***
 ***********************************************************************************************
 *                                                                                             *
 *                 Project Name : Command & Conquer                                            *
 *                                                                                             *
 *                    File Name : STARTUP.CPP                                                  *
 *                                                                                             *
 *                   Programmer : Joe L. Bostic                                                *
 *                                                                                             *
 *                   Start Date : October 3, 1994                                              *
 *                                                                                             *
 *                  Last Update : September 30, 1996 [JLB]                                     *
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 * Functions:                                                                                  *
 *   Prog_End -- Cleans up library systems in prep for game exit.                              *
 *   main -- Initial startup routine (preps library systems).                                  *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#include	"function.h"
#include	<conio.h>
#include	<io.h>
#include	"platform.h"		// S10b: SDL platform layer (platform/platform.h)

#ifdef MCIMPEG // Denzil 6/15/98
#include "mcimovie.h"
#endif


bool Read_Private_Config_Struct(FileClass & file, NewConfigType * config);
void Print_Error_End_Exit(char * string);
void Print_Error_Exit(char * string);

//WinTimerClass * WinTimer;
HINSTANCE	ProgramInstance;		// S10b: always NULL (no WinMain instance any more)
bool InitDDraw(void);					// S10b: was declared in FUNCTION.H under MPEGMOVIE
extern void Install_Platform_Callbacks(void);	// WINSTUB.CPP (S10b)
extern void Game_Shutdown(bool shutdown_network);	// WINSTUB.CPP (S10b)
void Check_Use_Compressed_Shapes (void);
void Read_Setup_Options(RawFileClass *config_file);
bool VideoBackBufferAllowed = true;

const char* Game_Registry_Key();

#if (ENGLISH)
#define WINDOW_NAME		"Red Alert"
#endif

#if (FRENCH)
#define WINDOW_NAME		"Alerte Rouge"
#endif

#if (GERMAN)
#define WINDOW_NAME		"Alarmstufe Rot"
#endif



/***********************************************************************************************
 * main -- Initial startup routine (preps library systems).                                    *
 *                                                                                             *
 *    This is the routine that is first called when the program starts up. It basically        *
 *    handles the command line parsing and setting up library systems.                         *
 *                                                                                             *
 * INPUT:   argc  -- Number of command line arguments.                                         *
 *                                                                                             *
 *          argv  -- Pointer to array of command line argument strings.                        *
 *                                                                                             *
 * OUTPUT:  Returns with execution failure code (if any).                                      *
 *                                                                                             *
 * WARNINGS:   none                                                                            *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   03/20/1995 JLB : Created.                                                                 *
 *=============================================================================================*/
/*
** S10b: WinMain became a portable main (docs/codebase/platform-layer.md, "How main works").
** Gone with it: the hand-made argv split, the "playing off the network" check, the
** _dos_getdrive/_dos_setdrive drive juggling, the Win32 window class and window
** (Create_Main_Window), the one-second Sleep that proved the timer ticks, and the
** PostMessage(WM_DESTROY) + ReadyToQuit wait loop at shutdown (Game_Shutdown in WINSTUB.CPP
** now does the WM_DESTROY work directly). The non-MPEGMOVIE copy of InitDDraw that used
** to sit inline here is gone too: InitDDraw below is the only video set-up.
*/
int main(int argc, char * argv[])
{
	/*
	** The platform layer (SDL) comes first: message boxes, the window and input need it.
	*/
	if (!Platform_Init()) {
		Platform_Message_Box(WINDOW_NAME, Platform_Last_Error());
		return(EXIT_FAILURE);
	}
	Install_Platform_Callbacks();

	if (Ram_Free(MEM_NORMAL) < 7000000) {
		Platform_Message_Box(TEXT_SHORT_TITLE, TEXT_NO_RAM);
		Platform_Shutdown();
		return(EXIT_FAILURE);
	}

	/*
	**	Change directory to where the executable is located, as the original did. Handle
	**	the case where there is no path attached to argv[0].
	*/
	if (argc > 0 && argv[0] != NULL) {
		char drive[_MAX_DRIVE];
		char path[_MAX_PATH];
		char dir[_MAX_DRIVE + _MAX_PATH];
		_splitpath(argv[0], drive, path, NULL, NULL);
		strcpy(dir, drive);
		strcat(dir, path);
		size_t len = strlen(dir);
		while (len > 1 && (dir[len-1] == '\\' || dir[len-1] == '/') && dir[len-2] != ':') {
			dir[--len] = '\0';
		}
		if (dir[0]) {
			chdir(dir);
		}
	}

	if (Parse_Command_Line(argc, argv)) {

		WindowsTimer = new WinTimerClass(60, FALSE);

		RawFileClass cfile(CONFIG_FILE_NAME);

		Keyboard = new KeyboardClass();

		/*
		** If there is loads of memory then use uncompressed shapes
		*/
		Check_Use_Compressed_Shapes();

		/*
		** If there is not enough disk space free, warn. (The original asked yes/no;
		** the platform message box only has OK, so the game carries on, as "yes" did.)
		*/
		if (Disk_Space_Available() < INIT_FREE_DISK_SPACE) {
			char	disk_space_message [512];
			sprintf (disk_space_message, TEXT_CRITICALLY_LOW, (INIT_FREE_DISK_SPACE) / (1024 * 1024));
			Platform_Message_Box(TEXT_SHORT_TITLE, disk_space_message);
		}

		if (cfile.Is_Available()) {

			Read_Private_Config_Struct(cfile, &NewConfig);

			/*
			** Set the options as requested by the ccsetup program
			*/
			Read_Setup_Options( &cfile );

			/*
			** MainWindow stays NULL: the platform layer owns the window (Set_Video_Mode).
			*/
			SoundOn = Audio_Init ( MainWindow , 16 , false , 11025*2 , 0 );

			if (!InitDDraw()) {
				Game_Shutdown(false);
				return (EXIT_FAILURE);
			}

			/*
			** WM_ACTIVATEAPP used to arrive when the window was created. The platform only
			** reports changes (the focus callback), so start "in focus"; otherwise the mouse
			** cursor would not draw until the window had lost and regained focus.
			*/
			GameInFocus = true;
			AllSurfaces.Set_Surface_Focus(TRUE);

			Options.Adjust_Variables_For_Resolution();

			/*
			** Install the memory error handler
			*/
			Memory_Error = &Memory_Error_Handler;

			WindowList[0][WINDOWWIDTH] 	= SeenBuff.Get_Width();
			WindowList[0][WINDOWHEIGHT]	= SeenBuff.Get_Height();
			WindowList[WINDOW_EDITOR][WINDOWWIDTH] = SeenBuff.Get_Width();
			WindowList[WINDOW_EDITOR][WINDOWHEIGHT]= SeenBuff.Get_Height();

			WWMouse = new WWMouseClass(&SeenBuff, 48, 48);
			MouseInstalled = TRUE;

			CDFileClass::Set_CD_Drive (CDList.Get_First_CD_Drive());


			/*
			** See if we should run the intro
			*/
			INIClass ini;
			ini.Load(cfile);

			/*
			**	Check for forced intro movie run disabling. If the conquer
			**	configuration file says "no", then don't run the intro.
			*/
			if (!Special.IsFromInstall) {
				Special.IsFromInstall = ini.Get_Bool("Intro", "PlayIntro", true);
			}
			SlowPalette = ini.Get_Bool("Options", "SlowPalette", false);

			/*
			** Regardless of whether we should run it or not, here we're
			** gonna change it to say "no" in the future.
			*/
			if (Special.IsFromInstall) {
				BreakoutAllowed = true;
//				BreakoutAllowed = false;
				ini.Put_Bool("Intro", "PlayIntro", false);
				ini.Save(cfile);
			}

			/*
			**	If the intro is being run for the first time, then don't
			**	allow breaking out of it with the <ESC> key.
			*/
			if (Special.IsFromInstall) {
				BreakoutAllowed = true;
//				BreakoutAllowed = false;
			}

			Memory_Error_Exit = Print_Error_End_Exit;

			Main_Game(argc, argv);

			VisiblePage.Clear();
			HiddenPage.Clear();
			Memory_Error_Exit = Print_Error_Exit;

			/*
			** Flag that this is a clean shutdown, then do what the WM_DESTROY handler did.
			*/
			ReadyToQuit = 1;
			Game_Shutdown(false);

			return (EXIT_SUCCESS);

		} else {
			/*
			** No REDALERT.INI. The original printed this and waited for a key (with no
			** window, so effectively forever under Windows 95).
			*/
			Platform_Message_Box(TEXT_SHORT_TITLE, TEXT_SETUP_FIRST);
		}

		if (WindowsTimer) {
		 	delete WindowsTimer;
		 	WindowsTimer = NULL;
		}

	}

	Platform_Shutdown();
	return(EXIT_SUCCESS);
}

/* Initialize DirectDraw and surfaces */
bool InitDDraw(void)
	{
	BOOL video_success = FALSE;

	/* Set 640x400 video mode. If its not available then try for 640x480 */
	if (ScreenHeight == 400)
		{
		if (Set_Video_Mode(MainWindow, ScreenWidth, ScreenHeight, 8))
			{
			video_success = TRUE;
			}
		else
			{
			if (Set_Video_Mode(MainWindow, ScreenWidth, 480, 8))
				{
				video_success = TRUE;
				ScreenHeight = 480;
				}
			}
		}
	else
		{
		if (Set_Video_Mode(MainWindow, ScreenWidth, ScreenHeight, 8))
			{
			video_success = TRUE;
			}
		}

	if (!video_success)
		{
		/* S10b: platform message box; the caller (main) cleans up through Game_Shutdown,
		 * which deletes WindowsTimer (deleting it here as well would free it twice). */
		Platform_Message_Box(TEXT_SHORT_TITLE, TEXT_VIDEO_ERROR);

		return false;
		}

	if (ScreenWidth == 320)
		{
		VisiblePage.Init(ScreenWidth, ScreenHeight, NULL, 0, (GBC_Enum)0);
		ModeXBuff.Init(ScreenWidth, ScreenHeight, NULL, 0, (GBC_Enum)(GBC_VISIBLE|GBC_VIDEOMEM));
		}
	else
		{
		VisiblePage.Init(ScreenWidth, ScreenHeight, NULL, 0, (GBC_Enum)(GBC_VISIBLE|GBC_VIDEOMEM));

		/* S08: the "did we really get a video memory page" check (DirectDraw GetCaps) is
		 * gone: the visible page is system memory presented by the platform layer, and
		 * Get_Free_Video_Memory() returns 0, so the hidden page below is system memory too. */

		/* If we have enough left then put the hidpage in video memory unless...
		 *
		 * If there is no blitter then we will get better performance with a system
		 * memory hidpage
		 *
		 * Use a system memory page if the user has specified it via the ccsetup program.
		 */
		/* S10b: the hidden page is always system memory. Get_Free_Video_Memory() is 0 and
		 * there are no DirectDraw surfaces, so the old GBC_VIDEOMEM branch (GetCaps check,
		 * Attach_DD_Surface) could never run and is removed. VideoBackBufferAllowed and the
		 * capability queries are no longer consulted. */
		HiddenPage.Init(ScreenWidth, ScreenHeight, NULL, 0, (GBC_Enum)0);
		}

	ScreenHeight = 400;

	if (VisiblePage.Get_Height() == 480)
		{
		SeenBuff.Attach(&VisiblePage, 0, 40, 640, 400);
		HidPage.Attach(&HiddenPage, 0, 40, 640, 400);
		}
	else
		{
		SeenBuff.Attach(&VisiblePage, 0, 0, 640, 400);
		HidPage.Attach(&HiddenPage, 0, 0, 640, 400);
		}

	return true;
	}


/***********************************************************************************************
 * Prog_End -- Cleans up library systems in prep for game exit.                                *
 *                                                                                             *
 *    This routine should be called before the game terminates. It handles cleaning up         *
 *    library systems so that a graceful return to the host operating system is achieved.      *
 *                                                                                             *
 * INPUT:   none                                                                               *
 *                                                                                             *
 * OUTPUT:  none                                                                               *
 *                                                                                             *
 * WARNINGS:   none                                                                            *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   03/20/1995 JLB : Created.                                                                 *
 *=============================================================================================*/
void __cdecl Prog_End(void)
{
	Sound_End();
	if (WWMouse) {
		delete WWMouse;
		WWMouse = NULL;
	}
	if (WindowsTimer) {
		 delete WindowsTimer;
		 WindowsTimer = NULL;
	}
}


void Print_Error_End_Exit(char * string)
{
	Prog_End();
	printf( "%s\n", string );
	exit(1);
}


void Print_Error_Exit(char * string)
{
	printf( "%s\n", string );
	exit(1);
}







/***********************************************************************************************
 * Emergency_Exit -- Function to call when we want to exit unexpectedly.                       *
 *                   Use this function instead of exit(n) so everything is properly cleaned up.*
 *                                                                                             *
 *                                                                                             *
 * INPUT:    Code to return to the OS                                                          *
 *                                                                                             *
 * OUTPUT:   Nothing                                                                           *
 *                                                                                             *
 * WARNINGS: None                                                                              *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *    3/13/97 1:32AM ST : Created                                                              *
 *=============================================================================================*/
void Emergency_Exit ( int code )
{


	/*
	** Clear out the video buffers so we dont glitch when we lose focus
	*/
	VisiblePage.Clear();
	HiddenPage.Clear();
	BlackPalette.Set();
	Memory_Error_Exit = Print_Error_Exit;

	/*
	** Flag that this is an emergency shut down - not a clean shutdown but
	** not killed with Ctrl-Alt-Del either.
	*/
	ReadyToQuit = 3;

	/*
	** S10b: was PostMessage(MainWindow, WM_DESTROY) and a wait for the message handler.
	** Do the WM_DESTROY work directly (ReadyToQuit 3 also shut the network down).
	*/
	Game_Shutdown(true);



	exit ( code );
}












/***********************************************************************************************
 * Read_Setup_Options -- Read stuff in from the INI file that we need to know sooner           *
 *                                                                                             *
 *                                                                                             *
 *                                                                                             *
 * INPUT:    Ptr to config file class                                                          *
 *                                                                                             *
 * OUTPUT:   Nothing                                                                           *
 *                                                                                             *
 * WARNINGS: None                                                                              *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *    6/7/96 4:09PM ST : Created                                                               *
 *   09/30/1996 JLB : Uses INI class.                                                          *
 *=============================================================================================*/
void Read_Setup_Options( RawFileClass *config_file )
{
	if (config_file->Is_Available()) {

		INIClass ini;

		ini.Load(*config_file);

		/*
		** Read in the boolean options
		*/
		VideoBackBufferAllowed = ini.Get_Bool("Options", "VideoBackBuffer", true);
		AllowHardwareBlitFills = ini.Get_Bool("Options", "HardwareFills", true);
		ScreenHeight = ini.Get_Bool("Options", "Resolution", false) ? 480 : 400;

		/*
		** See if an alternative socket number has been specified
		*/
		int socket = ini.Get_Int("Options", "Socket", 0);
		if (socket >0 ) {
			socket += 0x4000;
			if (socket >= 0x4000 && socket < 0x8000) {
				Ipx.Set_Socket (socket);
			}
		}

		/*
		** See if a destination network has been specified
		*/
		char netbuf [512];
		memset(netbuf, 0, sizeof(netbuf));
		char * netptr = netbuf;
		bool found = ini.Get_String("Options", "DestNet", NULL, netbuf, sizeof(netbuf));

		if (found && netptr != NULL && strlen(netbuf)) {
			NetNumType net;
			NetNodeType node;

			/*
			** Scan the string, pulling off each address piece
			*/
			int i = 0;
			char * p = strtok(netbuf,".");
			int x;
			while (p != NULL) {
				sscanf(p,"%x",&x);			// convert from hex string to int
				if (i < 4) {
					net[i] = (char)x;			// fill NetNum
				} else {
					node[i-4] = (char)x;		// fill NetNode
				}
				i++;
				p = strtok(NULL,".");
			}

			/*
			** If all the address components were successfully read, fill in the
			** BridgeNet with a broadcast address to the network across the bridge.
			*/
			if (i >= 4) {
				Session.IsBridge = 1;
				memset(node, 0xff, 6);
				Session.BridgeNet = IPXAddressClass(net, node);
			}
		}
	}
}






