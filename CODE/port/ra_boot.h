/*
**	Command & Conquer Red Alert(tm) - multiplayer port. GPLv3 or later, see LICENSE.md.
**	Start-up helpers: data folder, first-run configuration, data check (session S10c, ADR-0023).
**
**	Everything here is independent of the game's headers (no FUNCTION.H), so it can be unit
**	tested (tests/s10c_boot_test.cpp) and called from main() before anything else is set up.
**	CODE/STARTUP.CPP wires it together; docs/playing.md describes what the player sees.
**
**	Folders are searched in this order (the "search list"):
**	  1. the data folder: -data=<folder> on the command line, else [Data] Path= in REDALERT.INI;
**	  2. the game folder: the folder that holds the executable (main() changes to it, as the
**	     original did, so REDALERT.INI and saved games live there);
**	  3. the start-up folder: the current directory when the game was started.
**	Duplicates are dropped. File names are matched case-insensitively (ADR-0022).
*/

#ifndef RA_BOOT_H
#define RA_BOOT_H

#include <stddef.h>

#define RA_BOOT_PATH_MAX	1024
#define RA_BOOT_MAX_DIRS	3

/*
**	Remove every data folder option from the command line and return the last value:
**	"-data=<folder>", "--data=<folder>", "-data <folder>" or "--data <folder>" (the option name
**	is case-insensitive, the folder is kept exactly as typed). argc/argv are compacted in place,
**	so the game's own parser (which upper-cases arguments) never sees the folder.
**	Returns true if the option was given; *missing_value is set if it was given without a folder.
*/
bool RA_Boot_Take_Data_Option(int * argc, char ** argv, char * out, size_t out_size, bool * missing_value);

/*
**	The folder that holds the running executable (no trailing separator). Uses the OS
**	(/proc/self/exe, GetModuleFileName) and falls back to the directory part of argv0.
**	Returns false if neither gives a folder.
*/
bool RA_Boot_Executable_Dir(char const * argv0, char * out, size_t out_size);

/* The current directory. */
bool RA_Boot_Current_Dir(char * out, size_t out_size);

/*
**	Make path absolute relative to base (if it is not already) and drop a trailing separator.
**	A path that starts with '~' is not expanded (the shell does that).
*/
bool RA_Boot_Make_Absolute(char const * path, char const * base, char * out, size_t out_size);

/* Is this an existing directory? */
bool RA_Boot_Is_Directory(char const * path);

/*
**	The search list. Add folders in priority order; empty and duplicate folders are ignored.
**	Each entry carries a label for messages ("data folder", "game folder", ...).
*/
struct RABootSearchList {
	int Count;
	char Dir[RA_BOOT_MAX_DIRS][RA_BOOT_PATH_MAX];
	char const * Label[RA_BOOT_MAX_DIRS];
};
void RA_Boot_Search_Init(RABootSearchList * list);
bool RA_Boot_Search_Add(RABootSearchList * list, char const * dir, char const * label);

/*
**	Find a file in the search list, case-insensitively. Returns the index of the folder it is
**	in (and its real path in found, if not NULL), or -1.
*/
int RA_Boot_Find(RABootSearchList const * list, char const * name, char * found, size_t found_size);

/*
**	The search list as one ';'-separated string for CDFileClass::Set_Search_Drives.
**	Returns false if it does not fit.
*/
bool RA_Boot_Search_String(RABootSearchList const * list, char * out, size_t out_size);

/*
**	The data files the game cannot start without (see docs/playing.md, "Files the game needs").
**	Each requirement is met by its file, or by its alternative (the same content as loose files).
*/
struct RABootRequirement {
	char const * Name;			// e.g. "REDALERT.MIX"
	char const * Alternative;	// e.g. "LOCAL.MIX" (or NULL)
	char const * What;			// one-line description for the message
};
extern RABootRequirement const RA_Boot_Required_Files[];
extern int const RA_Boot_Required_Count;

/*
**	Check the required files. Returns true if all are present. If not, writes the message
**	shown to the player into report: the folders searched (in order, with "does not exist"
**	where that is the case), the files that are missing, and what to do.
*/
bool RA_Boot_Check_Required(RABootSearchList const * list, char * report, size_t report_size);

/*
**	Expansions (ADR-0011), detected from their MIX files (names from CODE/INIT.CPP
**	Init_Bootstrap_Mixfiles and CODE/EXPAND.CPP): Counterstrike = EXPAND.MIX,
**	Aftermath = EXPAND2.MIX.
*/
#define RA_BOOT_COUNTERSTRIKE_MIX	"EXPAND.MIX"
#define RA_BOOT_AFTERMATH_MIX		"EXPAND2.MIX"

/*
**	REDALERT.INI for a first run (every key is documented in docs/playing.md).
**	RA_Boot_Ensure_Config writes it if filename does not exist (case-insensitively).
**	Returns 1 if it was created, 0 if it already existed, -1 on error (errno set).
*/
extern char const RA_Boot_Default_Config[];
int RA_Boot_Ensure_Config(char const * filename);

#endif
