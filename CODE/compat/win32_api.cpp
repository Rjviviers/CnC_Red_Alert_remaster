/*
**	Command & Conquer Red Alert(tm) - multiplayer port. GPLv3 or later, see LICENSE.md.
**	Portable implementations of the Win32 calls the game still makes (session S10b,
**	ADR-0022). See docs/codebase/stubs.md ("Win32 API on non-Windows hosts").
**
**	Only compiled into something on non-Windows hosts: on Windows the real API is used.
**	These are not general Win32 emulations. Each one implements exactly what its callers
**	need, and says who they are:
**
**	  FindFirstFile/FindNextFile/FindClose  SESSION.CPP: multiplayer map packets (*.PKT, *.MPR)
**	  CreateFile/CloseHandle (read only)    CONQUER.CPP Get_CD_Index (never reached: see
**	                                        GetVolumeInformation in CODE/stubs/win32_stubs.cpp)
**	  GetLastError/SetLastError             CONQUER.CPP, WSPROTO.CPP (error codes for messages)
**	  GetSystemTime                         INIT.CPP, MENUS.CPP: seed bits for CryptRandom
**	  GlobalMemoryStatus                    2KEYFRAM.CPP Check_Use_Compressed_Shapes
**	  GetDriveType                          WIN32LIB/PLAYCD/GETCD.CPP: CD-ROM drive scan
**	  OutputDebugString                     WSPROTO.CPP, WSPUDP.CPP: debugger messages
**
**	Calls that only make sense with a Windows desktop or registry are stubs instead and
**	live in CODE/stubs/win32_stubs.cpp.
*/

#if !defined(_WIN32)

#pragma pack(push, 8)
#include <errno.h>
#include <fnmatch.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <time.h>
#include <dirent.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/time.h>
#pragma pack(pop)

#include <windows.h>
#include "ra_file.h"

/* ======================================================================== */
/* Last error                                                                */
/* ======================================================================== */

static DWORD LastError = ERROR_SUCCESS;

DWORD WINAPI GetLastError(void)
{
	return LastError;
}

void WINAPI SetLastError(DWORD error)
{
	LastError = error;
}

static DWORD Error_From_Errno(int e)
{
	switch (e) {
		case 0:			return ERROR_SUCCESS;
		case ENOENT:	return ERROR_FILE_NOT_FOUND;
		case ENOTDIR:	return ERROR_PATH_NOT_FOUND;
		case EACCES:
		case EPERM:
		case EISDIR:	return ERROR_ACCESS_DENIED;
		case EBADF:		return ERROR_INVALID_HANDLE;
		case ENOMEM:	return ERROR_NOT_ENOUGH_MEMORY;
		case EROFS:		return ERROR_WRITE_PROTECT;
		case ENOSPC:	return ERROR_DISK_FULL;
		case EEXIST:	return ERROR_ALREADY_EXISTS;
		default:		return ERROR_ACCESS_DENIED;
	}
}


/* ======================================================================== */
/* Files                                                                     */
/* ======================================================================== */

/* A file handle is the descriptor; INVALID_HANDLE_VALUE is -1 like a closed descriptor. */
static HANDLE Handle_From_Fd(int fd)
{
	return (HANDLE)(intptr_t)fd;
}

HANDLE WINAPI CreateFile(LPCSTR name, DWORD access, DWORD, LPSECURITY_ATTRIBUTES, DWORD disposition, DWORD, HANDLE)
{
	/*
	**	Only what the game asks for: open an existing file for reading.
	*/
	if (access != GENERIC_READ || disposition != OPEN_EXISTING) {
		LastError = ERROR_ACCESS_DENIED;
		return INVALID_HANDLE_VALUE;
	}
	int fd = RA_File_Open(name, RA_FILE_READ);
	if (fd == -1) {
		LastError = Error_From_Errno(errno);
		return INVALID_HANDLE_VALUE;
	}
	return Handle_From_Fd(fd);
}

BOOL WINAPI CloseHandle(HANDLE handle)
{
	if (handle == INVALID_HANDLE_VALUE || handle == NULL) {
		LastError = ERROR_INVALID_HANDLE;
		return FALSE;
	}
	return RA_File_Close((int)(intptr_t)handle) ? TRUE : FALSE;
}


/*
**	FindFirstFile: every entry of the directory whose name matches the pattern
**	case-insensitively (as on Windows), returned in case-insensitive name order. NTFS
**	returns names in that order too; readdir order is arbitrary, and the result decides
**	the order of the multiplayer map list.
*/
struct FindState {
	char Directory[1024];
	char ** Names;
	int Count;
	int Next;
};

static int Compare_Names(void const * a, void const * b)
{
	char const * x = *(char const * const *)a;
	char const * y = *(char const * const *)b;
	int c = strcasecmp(x, y);
	return c ? c : strcmp(x, y);
}

static void Free_Find(FindState * s)
{
	if (!s) return;
	for (int i = 0; i < s->Count; i++) free(s->Names[i]);
	free(s->Names);
	free(s);
}

static unsigned long long Filetime_From_Unix(time_t t)
{
	return ((unsigned long long)(long long)t + 11644473600ULL) * 10000000ULL;
}

static bool Fill_Find_Data(FindState * s, LPWIN32_FIND_DATA data)
{
	while (s->Next < s->Count) {
		char const * name = s->Names[s->Next++];
		char full[2048];
		snprintf(full, sizeof(full), "%s%s", s->Directory, name);
		struct stat st;
		if (stat(full, &st) != 0) continue;

		memset(data, 0, sizeof(*data));
		data->dwFileAttributes = S_ISDIR(st.st_mode) ? FILE_ATTRIBUTE_DIRECTORY : FILE_ATTRIBUTE_ARCHIVE;
		unsigned long long ft = Filetime_From_Unix(st.st_mtime);
		data->ftLastWriteTime.dwLowDateTime = (DWORD)ft;
		data->ftLastWriteTime.dwHighDateTime = (DWORD)(ft >> 32);
		data->ftLastAccessTime = data->ftLastWriteTime;
		data->ftCreationTime = data->ftLastWriteTime;
		unsigned long long size = (unsigned long long)st.st_size;
		data->nFileSizeLow = (DWORD)size;
		data->nFileSizeHigh = (DWORD)(size >> 32);
		strncpy(data->cFileName, name, sizeof(data->cFileName) - 1);
		/* No 8.3 alias: callers fall back to cFileName when this is empty. */
		data->cAlternateFileName[0] = '\0';
		return true;
	}
	return false;
}

HANDLE WINAPI FindFirstFile(LPCSTR pattern, LPWIN32_FIND_DATA data)
{
	if (pattern == NULL || data == NULL) {
		LastError = ERROR_FILE_NOT_FOUND;
		return INVALID_HANDLE_VALUE;
	}

	/*
	**	Split "dir\pattern" (either separator) into a directory and a file pattern.
	*/
	char const * slash = NULL;
	for (char const * p = pattern; *p; p++) if (*p == '/' || *p == '\\') slash = p;
	char dirname[1024] = "";
	char const * filepattern = pattern;
	if (slash) {
		size_t n = (size_t)(slash - pattern) + 1;
		if (n >= sizeof(dirname)) {
			LastError = ERROR_PATH_NOT_FOUND;
			return INVALID_HANDLE_VALUE;
		}
		memcpy(dirname, pattern, n);
		dirname[n] = '\0';
		filepattern = slash + 1;
	}

	FindState * s = (FindState *)calloc(1, sizeof(FindState));
	if (!s) {
		LastError = ERROR_NOT_ENOUGH_MEMORY;
		return INVALID_HANDLE_VALUE;
	}
	if (dirname[0]) {
		if (!RA_Resolve_Path(dirname, s->Directory, sizeof(s->Directory) - 1, true)) {
			Free_Find(s);
			LastError = ERROR_PATH_NOT_FOUND;
			return INVALID_HANDLE_VALUE;
		}
		size_t n = strlen(s->Directory);
		if (n == 0 || s->Directory[n-1] != '/') strcat(s->Directory, "/");
	}

	DIR * d = opendir(s->Directory[0] ? s->Directory : ".");
	if (!d) {
		Free_Find(s);
		LastError = ERROR_PATH_NOT_FOUND;
		return INVALID_HANDLE_VALUE;
	}
	int capacity = 0;
	struct dirent * e;
	while ((e = readdir(d)) != NULL) {
		if (fnmatch(filepattern, e->d_name, FNM_CASEFOLD) != 0) continue;
		if (s->Count == capacity) {
			capacity = capacity ? capacity * 2 : 16;
			char ** grown = (char **)realloc(s->Names, sizeof(char *) * (size_t)capacity);
			if (!grown) break;
			s->Names = grown;
		}
		s->Names[s->Count] = strdup(e->d_name);
		if (s->Names[s->Count]) s->Count++;
	}
	closedir(d);
	if (s->Count > 1) qsort(s->Names, (size_t)s->Count, sizeof(char *), Compare_Names);

	if (!Fill_Find_Data(s, data)) {
		Free_Find(s);
		LastError = ERROR_FILE_NOT_FOUND;
		return INVALID_HANDLE_VALUE;
	}
	return (HANDLE)s;
}

BOOL WINAPI FindNextFile(HANDLE handle, LPWIN32_FIND_DATA data)
{
	if (handle == INVALID_HANDLE_VALUE || handle == NULL || data == NULL) {
		LastError = ERROR_INVALID_HANDLE;
		return FALSE;
	}
	if (!Fill_Find_Data((FindState *)handle, data)) {
		LastError = 18L;		// ERROR_NO_MORE_FILES
		return FALSE;
	}
	return TRUE;
}

BOOL WINAPI FindClose(HANDLE handle)
{
	if (handle == INVALID_HANDLE_VALUE || handle == NULL) return FALSE;
	Free_Find((FindState *)handle);
	return TRUE;
}


/*
**	GetDriveType: there are no drive letters, so no drive is a CD-ROM. GetCDClass then
**	finds no CD drives, which is what the port wants: the data comes from the folder the
**	player points the game at (S10c).
*/
UINT WINAPI GetDriveType(LPCSTR)
{
	return DRIVE_NO_ROOT_DIR;
}


/* ======================================================================== */
/* Time, memory, debug output                                                */
/* ======================================================================== */

void WINAPI GetSystemTime(LPSYSTEMTIME st)
{
	if (!st) return;
	struct timeval tv;
	gettimeofday(&tv, NULL);
	time_t secs = tv.tv_sec;
	struct tm utc;
	gmtime_r(&secs, &utc);
	st->wYear = (WORD)(utc.tm_year + 1900);
	st->wMonth = (WORD)(utc.tm_mon + 1);
	st->wDayOfWeek = (WORD)utc.tm_wday;
	st->wDay = (WORD)utc.tm_mday;
	st->wHour = (WORD)utc.tm_hour;
	st->wMinute = (WORD)utc.tm_min;
	st->wSecond = (WORD)utc.tm_sec;
	st->wMilliseconds = (WORD)(tv.tv_usec / 1000);
}


/*
**	GlobalMemoryStatus: the only caller compares dwTotalPhys with 16 MB. Report the
**	physical memory, capped below 2 GB like Windows did for this (pre-Ex) call.
*/
void WINAPI GlobalMemoryStatus(LPMEMORYSTATUS ms)
{
	if (!ms) return;
	unsigned long long const cap = 0x7FFFFFFFULL;
	long pages = sysconf(_SC_PHYS_PAGES);
	long avail = sysconf(_SC_AVPHYS_PAGES);
	long pagesize = sysconf(_SC_PAGESIZE);
	unsigned long long total = (pages > 0 && pagesize > 0) ? (unsigned long long)pages * (unsigned long long)pagesize : cap;
	unsigned long long free_bytes = (avail > 0 && pagesize > 0) ? (unsigned long long)avail * (unsigned long long)pagesize : total;
	if (total > cap) total = cap;
	if (free_bytes > cap) free_bytes = cap;

	ms->dwLength = sizeof(*ms);
	ms->dwMemoryLoad = total ? (DWORD)(100 - (free_bytes * 100) / total) : 0;
	ms->dwTotalPhys = (SIZE_T)total;
	ms->dwAvailPhys = (SIZE_T)free_bytes;
	ms->dwTotalPageFile = (SIZE_T)total;
	ms->dwAvailPageFile = (SIZE_T)free_bytes;
	ms->dwTotalVirtual = (SIZE_T)cap;
	ms->dwAvailVirtual = (SIZE_T)cap;
}


/* Messages for the debugger go to stderr when RA_DEBUG_OUTPUT is set in the environment. */
void WINAPI OutputDebugString(LPCSTR text)
{
	static int enabled = -1;
	if (enabled == -1) enabled = getenv("RA_DEBUG_OUTPUT") != NULL;
	if (enabled && text) fputs(text, stderr);
}

#endif /* !_WIN32 */
