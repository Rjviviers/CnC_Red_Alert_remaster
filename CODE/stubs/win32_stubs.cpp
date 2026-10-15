/*
**	Command & Conquer Red Alert(tm) - multiplayer port. GPLv3 or later, see LICENSE.md.
**
**	STUBS (session S10b) for Win32 calls that need the Windows desktop or registry and
**	have no meaning on other hosts. See docs/codebase/stubs.md. Real portable
**	implementations of the other Win32 calls are in CODE/compat/win32_api.cpp.
**
**	Only for non-Windows hosts. On Windows the real API is used.
**
**	  Registry (RegOpenKeyEx, RegQueryValueEx, RegCloseKey)
**	      CONQUER.CPP Is_Counterstrike_Installed/Is_Aftermath_Installed and INIT.CPP
**	      Is_DVD_Installed read flags the installer wrote. There is no installer and no
**	      registry: every key is missing, so the expansions read as "not installed" and the
**	      DVD as absent. S10c replaces the expansion checks with MIX-file detection
**	      (ADR-0011); the DVD check can stay a "no".
**	  GetVolumeInformation
**	      CONQUER.CPP Get_CD_Index reads CD volume labels. There are no drives: fails with
**	      ERROR_PATH_NOT_FOUND, so no CD is ever "found". Stays (CD checks are dead code
**	      once S10c points the game at a data folder).
**	  SendMessage
**	      WSPROTO.CPP/WSPUDP.CPP post FD_WRITE notifications to MainWindow to kick the
**	      async socket writer. No window: does nothing. REPLACED IN S12 (new transport).
**	  ShowCursor
**	      INIT.CPP Init_Mouse hides the Windows cursor. The platform layer already hides
**	      the OS cursor over the window (Set_Video_Mode); does nothing.
*/

#if !defined(_WIN32)

#include <windows.h>

/* ---- Registry: every key is missing. ---- */

LONG WINAPI RegOpenKeyEx(HKEY, LPCSTR, DWORD, REGSAM, PHKEY result)
{
	if (result) *result = NULL;
	return ERROR_FILE_NOT_FOUND;
}

LONG WINAPI RegQueryValueEx(HKEY, LPCSTR, LPDWORD, LPDWORD, LPBYTE, LPDWORD)
{
	return ERROR_FILE_NOT_FOUND;
}

LONG WINAPI RegCloseKey(HKEY)
{
	return ERROR_SUCCESS;
}


/* ---- No drives, so no volume labels. ---- */

BOOL WINAPI GetVolumeInformation(LPCSTR, LPSTR name, DWORD name_size, LPDWORD, LPDWORD, LPDWORD, LPSTR, DWORD)
{
	if (name && name_size) name[0] = '\0';
	SetLastError(ERROR_PATH_NOT_FOUND);
	return FALSE;
}


/* ---- No Win32 window. ---- */

LRESULT WINAPI SendMessage(HWND, UINT, WPARAM, LPARAM)
{
	return 0;
}

int WINAPI ShowCursor(BOOL show)
{
	/* The display counter Windows would return: 0 shown, -1 hidden. */
	return show ? 0 : -1;
}

#endif /* !_WIN32 */
