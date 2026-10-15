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
 *   Boot_Setup_Data -- First-run REDALERT.INI, data folder and data check (S10c).             *
 *   Boot_Data_Error -- Reports a data problem found during initialisation and exits (S10c).   *
 *   main -- Initial startup routine (preps library systems).                                  *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#include	"function.h"
#include	<conio.h>
#include	<io.h>
#include	"platform.h"		// S10b: SDL platform layer (platform/platform.h)
#include	"port/ra_boot.h"	// S10c: data folder, first-run INI, data check (ADR-0023)

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

/*
**	S10c: where the game looks for its data (see Boot_Setup_Data and docs/playing.md).
*/
static RABootSearchList BootSearch;
static bool Boot_Setup_Data(bool have_data_option, bool data_option_empty, char const * data_option,
							char const * start_dir, char const * game_dir);
void Boot_Data_Error(char const * detail);

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
	**	S10c: take the data folder option out of the command line before the game's own
	**	parser sees it (Parse_Command_Line upper-cases every argument, and its "-CD" test
	**	would match inside a path). A relative folder is relative to where the game was
	**	started, so remember that before changing directory.
	*/
	char start_dir[RA_BOOT_PATH_MAX] = "";
	RA_Boot_Current_Dir(start_dir, sizeof(start_dir));

	char data_option[RA_BOOT_PATH_MAX] = "";
	bool data_option_empty = false;
	bool have_data_option = RA_Boot_Take_Data_Option(&argc, argv, data_option, sizeof(data_option), &data_option_empty);

	/*
	**	Change directory to where the executable is located, as the original did, so
	**	REDALERT.INI and saved games live next to it. S10c: the folder comes from the OS
	**	(/proc/self/exe, GetModuleFileName), which also works when the game was started
	**	through PATH; argv[0] is only the fallback.
	*/
	char game_dir[RA_BOOT_PATH_MAX] = "";
	if (RA_Boot_Executable_Dir(argc > 0 ? argv[0] : NULL, game_dir, sizeof(game_dir))) {
		if (chdir(game_dir) != 0) {
			strcpy(game_dir, start_dir);
		}
	} else {
		strcpy(game_dir, start_dir);
	}

	if (Parse_Command_Line(argc, argv)) {

		/*
		**	S10c: create REDALERT.INI on the first run, find the data folder and check that
		**	the data files are there, before the window opens. On a problem the player gets a
		**	message that says what is missing and where the game looked, and the game exits.
		*/
		if (!Boot_Setup_Data(have_data_option, data_option_empty, data_option, start_dir, game_dir)) {
			Platform_Shutdown();
			return(EXIT_FAILURE);
		}

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
			**	S10c: [Intro] PlayIntro is no longer read or written. In the original it was
			**	missing (= yes) on the first run after installing, which set IsFromInstall: the
			**	intro movie played and the game went straight into the first Allied mission
			**	without showing the main menu. Movies are not played in this port (ADR-0005) and
			**	the main menu is where multiplayer starts, so every start goes to the menu.
			**	(The original's hidden "from install" command-line switch still sets the flag.)
			*/
			SlowPalette = ini.Get_Bool("Options", "SlowPalette", false);

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
			** window, so effectively forever under Windows 95). S10c: Boot_Setup_Data creates
			** the file, so this is only reached if it disappeared again in between.
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

/***********************************************************************************************
 * Boot_Setup_Data -- First-run REDALERT.INI, data folder and data check (S10c).               *
 *                                                                                             *
 *    Creates REDALERT.INI with documented defaults if there is none, works out the folders    *
 *    to search (data folder from -data= or [Data] Path=, then the game folder, then the       *
 *    folder the game was started from), registers them with CCFileClass, and checks that     *
 *    the files the game cannot start without are there. See docs/playing.md and ADR-0023.    *
 *                                                                                             *
 * INPUT:   have_data_option  -- -data was on the command line                                 *
 *          data_option_empty -- ... but without a folder                                      *
 *          data_option       -- the folder as typed                                           *
 *          start_dir         -- the current directory at start-up                             *
 *          game_dir          -- the executable's folder (now the current directory)           *
 *                                                                                             *
 * OUTPUT:  true to carry on; false after a message has been shown (the caller exits).         *
 *=============================================================================================*/
static void Boot_Report(char const * text)
{
	/*
	**	A message box for the player; the same text on stderr for logs. (When no box can be
	**	shown, headless or offscreen, the platform layer writes it to stderr itself.)
	*/
	fflush(stderr);
	if (Platform_Message_Box(WINDOW_NAME, text)) {
		fprintf(stderr, "%s: %s\n", WINDOW_NAME, text);
	}
	fflush(stderr);
}

static bool Boot_Setup_Data(bool have_data_option, bool data_option_empty, char const * data_option,
							char const * start_dir, char const * game_dir)
{
	char message[4096];

	/*
	**	First run: write a REDALERT.INI with defaults (RA_Boot_Default_Config).
	*/
	int created = RA_Boot_Ensure_Config(CONFIG_FILE_NAME);
	if (created < 0) {
		snprintf(message, sizeof(message),
			"Could not create %s in %s (%s).\n\nThe game keeps its settings next to the "
			"executable: make that folder writable, or copy the game to a folder you can write to.",
			CONFIG_FILE_NAME, game_dir, strerror(errno));
		Boot_Report(message);
		return(false);
	}
	if (created > 0) {
		fprintf(stderr, "Red Alert: created %s with default settings in %s\n", CONFIG_FILE_NAME, game_dir);
	}

	/*
	**	The data folder: the command line wins over the INI.
	*/
	char data_dir[RA_BOOT_PATH_MAX] = "";
	char const * data_label = "data folder";
	if (have_data_option) {
		if (data_option_empty) {
			Boot_Report("-data needs a folder, for example: -data=/path/to/redalert");
			return(false);
		}
		if (!RA_Boot_Make_Absolute(data_option, start_dir, data_dir, sizeof(data_dir))) {
			Boot_Report("The -data folder name is too long.");
			return(false);
		}
		data_label = "data folder, from -data";
	} else {
		RawFileClass cfile(CONFIG_FILE_NAME);
		INIClass ini;
		ini.Load(cfile);
		char path[RA_BOOT_PATH_MAX] = "";
		ini.Get_String("Data", "Path", "", path, sizeof(path));
		if (path[0] != '\0') {
			if (!RA_Boot_Make_Absolute(path, game_dir, data_dir, sizeof(data_dir))) {
				Boot_Report("The [Data] Path folder name in REDALERT.INI is too long.");
				return(false);
			}
			data_label = "data folder, from REDALERT.INI";
		}
	}

	if (data_dir[0] != '\0' && !RA_Boot_Is_Directory(data_dir)) {
		snprintf(message, sizeof(message),
			"The data folder does not exist:\n  %s\n(%s)\n\nPoint -data= or Path= in the [Data] section "
			"of REDALERT.INI at the folder that holds your Red Alert .MIX files. See docs/playing.md.",
			data_dir, data_label);
		Boot_Report(message);
		return(false);
	}

	RA_Boot_Search_Init(&BootSearch);
	RA_Boot_Search_Add(&BootSearch, data_dir, data_label);
	RA_Boot_Search_Add(&BootSearch, game_dir, "game folder");
	RA_Boot_Search_Add(&BootSearch, start_dir, "start-up folder");

	/*
	**	Hand the list to the file system. CDFileClass looks for MIX files in these folders
	**	in this order, and for other files in the current directory first (CDFILE.H).
	**	Set_Search_Drives takes the string apart with strtok, so it gets a copy.
	*/
	static char search[sizeof(BootSearch.Dir) + RA_BOOT_MAX_DIRS];
	if (RA_Boot_Search_String(&BootSearch, search, sizeof(search)) && search[0] != '\0') {
		CCFileClass::Set_Search_Drives(search);
	}

	for (int i = 0; i < BootSearch.Count; i++) {
		fprintf(stderr, "Red Alert: data search %d: %s (%s)%s\n", i + 1, BootSearch.Dir[i], BootSearch.Label[i],
			RA_Boot_Is_Directory(BootSearch.Dir[i]) ? "" : " - does not exist");
	}

	/*
	**	The files the game cannot start without.
	*/
	if (!RA_Boot_Check_Required(&BootSearch, message, sizeof(message))) {
		Boot_Report(message);
		return(false);
	}

	char found[RA_BOOT_PATH_MAX];
	for (int r = 0; r < RA_Boot_Required_Count; r++) {
		if (RA_Boot_Find(&BootSearch, RA_Boot_Required_Files[r].Name, found, sizeof(found)) >= 0) {
			fprintf(stderr, "Red Alert: found %s\n", found);
		}
	}
	fprintf(stderr, "Red Alert: Counterstrike (%s): %s\n", RA_BOOT_COUNTERSTRIKE_MIX,
		RA_Boot_Find(&BootSearch, RA_BOOT_COUNTERSTRIKE_MIX, NULL, 0) >= 0 ? "found" : "not found");
	fprintf(stderr, "Red Alert: Aftermath (%s): %s\n", RA_BOOT_AFTERMATH_MIX,
		RA_Boot_Find(&BootSearch, RA_BOOT_AFTERMATH_MIX, NULL, 0) >= 0 ? "found" : "not found");
	return(true);
}


/***********************************************************************************************
 * Boot_Data_Error -- Reports a data problem found during initialisation and exits (S10c).     *
 *                                                                                             *
 *    Called by the Init_* code (INIT.CPP) when a data file it cannot do without is missing    *
 *    or unreadable after the start-up check passed (for example a damaged REDALERT.MIX).      *
 *    Shows the problem and the folders searched, then exits through Emergency_Exit.           *
 *                                                                                             *
 * INPUT:   detail -- what is wrong, one or two sentences                                      *
 *                                                                                             *
 * OUTPUT:  does not return                                                                    *
 *=============================================================================================*/
void Boot_Data_Error(char const * detail)
{
	char message[4096];
	snprintf(message, sizeof(message), "%s\n\nFolders searched, in this order:\n", detail ? detail : "");
	for (int i = 0; i < BootSearch.Count; i++) {
		char line[RA_BOOT_PATH_MAX + 64];
		snprintf(line, sizeof(line), "  %d. %s (%s)\n", i + 1, BootSearch.Dir[i], BootSearch.Label[i]);
		strncat(message, line, sizeof(message) - strlen(message) - 1);
	}
	strncat(message, "\nYour data files may be incomplete or damaged: copy them again from your Red Alert "
		"installation. See docs/playing.md.", sizeof(message) - strlen(message) - 1);
	Boot_Report(message);
	Emergency_Exit(EXIT_FAILURE);
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






