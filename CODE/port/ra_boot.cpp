/*
**	Command & Conquer Red Alert(tm) - multiplayer port. GPLv3 or later, see LICENSE.md.
**	Start-up helpers: data folder, first-run configuration, data check (session S10c, ADR-0023).
**	See ra_boot.h.
**
**	Built with the game's settings (1-byte struct packing), so system headers are included
**	under natural packing, as in CODE/compat/ra_file.cpp.
*/

#if !defined(_MSC_VER)
#  pragma pack(push, 8)
#endif
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/stat.h>
#if defined(_WIN32)
#  include <direct.h>
#  include <windows.h>		// GetModuleFileNameA
#else
#  include <unistd.h>
#endif
#if !defined(_MSC_VER)
#  pragma pack(pop)
#endif

#include "ra_boot.h"
#include "../compat/ra_file.h"


/*
**	Small string helpers (no STL: see the packing note above).
*/
static bool Copy(char * out, size_t out_size, char const * src)
{
	size_t len = strlen(src);
	if (len >= out_size) return false;
	memcpy(out, src, len + 1);
	return true;
}

static bool Is_Separator(char c)
{
#if defined(_WIN32)
	return c == '/' || c == '\\';
#else
	return c == '/';
#endif
}

static void Strip_Trailing_Separators(char * path)
{
	size_t len = strlen(path);
	while (len > 1 && Is_Separator(path[len - 1])) {
#if defined(_WIN32)
		if (len == 3 && path[1] == ':') break;		// keep "C:\"
#endif
		path[--len] = '\0';
	}
}

static bool Same_Text(char const * a, char const * b)
{
#if defined(_WIN32)
	for (; *a && *b; a++, b++) {
		char ca = (char)tolower((unsigned char)*a);
		char cb = (char)tolower((unsigned char)*b);
		if (ca == '\\') ca = '/';
		if (cb == '\\') cb = '/';
		if (ca != cb) return false;
	}
	return *a == *b;
#else
	return strcmp(a, b) == 0;
#endif
}

static bool Option_Name_Is(char const * arg, char const * name, size_t len)
{
	for (size_t i = 0; i < len; i++) {
		if (arg[i] == '\0') return false;
		if (tolower((unsigned char)arg[i]) != name[i]) return false;
	}
	return true;
}


bool RA_Boot_Take_Data_Option(int * argc, char ** argv, char * out, size_t out_size, bool * missing_value)
{
	bool found = false;
	if (missing_value) *missing_value = false;
	if (argc == NULL || argv == NULL) return false;

	int write = 1;
	for (int read = 1; read < *argc; read++) {
		char const * arg = argv[read];
		char const * value = NULL;
		bool is_option = false;

		if (arg != NULL) {
			char const * p = arg;
			if (p[0] == '-' && p[1] == '-') p += 2;
			else if (p[0] == '-') p += 1;
			else p = NULL;

			if (p != NULL && Option_Name_Is(p, "data", 4)) {
				if (p[4] == '=') {
					is_option = true;
					value = p + 5;
				} else if (p[4] == '\0') {
					is_option = true;
					if (read + 1 < *argc) {
						value = argv[++read];
					}
				}
			}
		}

		if (!is_option) {
			argv[write++] = argv[read];
			continue;
		}

		found = true;
		if (value == NULL || value[0] == '\0') {
			if (missing_value) *missing_value = true;
			if (out && out_size) out[0] = '\0';
		} else if (out != NULL) {
			if (!Copy(out, out_size, value)) {
				if (missing_value) *missing_value = true;
				out[0] = '\0';
			} else if (missing_value) {
				*missing_value = false;
			}
		}
	}
	for (int i = write; i < *argc; i++) argv[i] = NULL;
	*argc = write;
	return found;
}


bool RA_Boot_Current_Dir(char * out, size_t out_size)
{
	if (out == NULL || out_size == 0) return false;
#if defined(_WIN32)
	if (_getcwd(out, (int)out_size) == NULL) return false;
#else
	if (getcwd(out, out_size) == NULL) return false;
#endif
	Strip_Trailing_Separators(out);
	return true;
}


bool RA_Boot_Is_Directory(char const * path)
{
	if (path == NULL || path[0] == '\0') return false;
	struct stat st;
	if (stat(path, &st) != 0) return false;
	return (st.st_mode & S_IFMT) == S_IFDIR;
}


static bool Is_Absolute(char const * path)
{
#if defined(_WIN32)
	if (Is_Separator(path[0])) return true;		// "\dir" or "\\server\share"
	return isalpha((unsigned char)path[0]) && path[1] == ':' && Is_Separator(path[2]);
#else
	return path[0] == '/';
#endif
}


bool RA_Boot_Make_Absolute(char const * path, char const * base, char * out, size_t out_size)
{
	if (path == NULL || out == NULL || out_size == 0 || path[0] == '\0') return false;

	char work[RA_BOOT_PATH_MAX];
	if (Is_Absolute(path) || base == NULL || base[0] == '\0') {
		if (!Copy(work, sizeof(work), path)) return false;
	} else {
		/* "./x" and "x" both mean base/x; leave ".." to the OS. */
		while (path[0] == '.' && Is_Separator(path[1])) {
			path += 2;
			while (Is_Separator(path[0])) path++;
		}
		size_t blen = strlen(base);
		size_t plen = strlen(path);
		if (blen + 1 + plen >= sizeof(work)) return false;
		memcpy(work, base, blen);
		size_t n = blen;
		if (n > 0 && !Is_Separator(work[n - 1])) work[n++] = '/';
		memcpy(work + n, path, plen + 1);
		if (plen == 0 || (plen == 1 && path[0] == '.')) {
			work[n] = '\0';
		}
	}
	Strip_Trailing_Separators(work);
	return Copy(out, out_size, work);
}


bool RA_Boot_Executable_Dir(char const * argv0, char * out, size_t out_size)
{
	if (out == NULL || out_size == 0) return false;
	char path[RA_BOOT_PATH_MAX];
	path[0] = '\0';

#if defined(_WIN32)
	DWORD n = GetModuleFileNameA(NULL, path, (DWORD)sizeof(path));
	if (n == 0 || n >= sizeof(path)) path[0] = '\0';
#elif defined(__linux__)
	ssize_t n = readlink("/proc/self/exe", path, sizeof(path) - 1);
	if (n > 0) {
		path[n] = '\0';
	} else {
		path[0] = '\0';
	}
#endif

	if (path[0] == '\0') {
		/*
		**	Fallback: the directory part of argv[0], made absolute against the current
		**	directory. With no directory part (started through PATH) there is no answer.
		*/
		if (argv0 == NULL) return false;
		char cwd[RA_BOOT_PATH_MAX];
		if (!RA_Boot_Current_Dir(cwd, sizeof(cwd))) return false;
		if (!RA_Boot_Make_Absolute(argv0, cwd, path, sizeof(path))) return false;
		bool has_dir = false;
		for (char const * p = argv0; *p; p++) {
			if (Is_Separator(*p)) has_dir = true;
		}
		if (!has_dir) return false;
	}

	/* Cut the file name. */
	size_t len = strlen(path);
	while (len > 0 && !Is_Separator(path[len - 1])) len--;
	if (len == 0) return false;
	path[len] = '\0';
	Strip_Trailing_Separators(path);
	return Copy(out, out_size, path);
}


void RA_Boot_Search_Init(RABootSearchList * list)
{
	if (list == NULL) return;
	memset(list, 0, sizeof(*list));
}


bool RA_Boot_Search_Add(RABootSearchList * list, char const * dir, char const * label)
{
	if (list == NULL || dir == NULL || dir[0] == '\0') return false;
	for (int i = 0; i < list->Count; i++) {
		if (Same_Text(list->Dir[i], dir)) return false;
	}
	if (list->Count >= RA_BOOT_MAX_DIRS) return false;
	if (!Copy(list->Dir[list->Count], RA_BOOT_PATH_MAX, dir)) return false;
	Strip_Trailing_Separators(list->Dir[list->Count]);
	list->Label[list->Count] = label ? label : "";
	list->Count++;
	return true;
}


int RA_Boot_Find(RABootSearchList const * list, char const * name, char * found, size_t found_size)
{
	if (list == NULL || name == NULL) return -1;
	for (int i = 0; i < list->Count; i++) {
		char joined[RA_BOOT_PATH_MAX];
		size_t dlen = strlen(list->Dir[i]);
		size_t nlen = strlen(name);
		if (dlen + 1 + nlen >= sizeof(joined)) continue;
		memcpy(joined, list->Dir[i], dlen);
		joined[dlen] = '/';
		memcpy(joined + dlen + 1, name, nlen + 1);

		char real[RA_BOOT_PATH_MAX];
		if (RA_Resolve_Path(joined, real, sizeof(real), true) && !RA_Boot_Is_Directory(real)) {
			if (found != NULL && found_size > 0) {
				if (!Copy(found, found_size, real)) found[0] = '\0';
			}
			return i;
		}
	}
	return -1;
}


bool RA_Boot_Search_String(RABootSearchList const * list, char * out, size_t out_size)
{
	if (list == NULL || out == NULL || out_size == 0) return false;
	out[0] = '\0';
	size_t used = 0;
	for (int i = 0; i < list->Count; i++) {
		size_t len = strlen(list->Dir[i]);
		if (used + (used ? 1 : 0) + len >= out_size) return false;
		if (used) out[used++] = ';';
		memcpy(out + used, list->Dir[i], len + 1);
		used += len;
	}
	return true;
}


/*
**	What the game needs before it can show anything (CODE/INIT.CPP: Init_Bootstrap_Mixfiles,
**	Init_Fonts, Init_Secondary_Mixfiles, Init_Bulk_Data). REDALERT.MIX holds LOCAL.MIX,
**	HIRES.MIX and NCHIRES.MIX; MAIN.MIX holds CONQUER.MIX, GENERAL.MIX, SOUNDS.MIX and the
**	rest. The game also accepts those inner files as loose files, hence the alternatives.
*/
RABootRequirement const RA_Boot_Required_Files[] = {
	{"REDALERT.MIX", "LOCAL.MIX", "fonts, palettes, interface art (holds LOCAL.MIX, HIRES.MIX, NCHIRES.MIX)"},
	{"MAIN.MIX", "CONQUER.MIX", "game art, sounds and music (holds CONQUER.MIX, GENERAL.MIX, SOUNDS.MIX, ...)"},
};
int const RA_Boot_Required_Count = (int)(sizeof(RA_Boot_Required_Files) / sizeof(RA_Boot_Required_Files[0]));


/* Append to a bounded string; never overruns, always terminates. */
static void Append(char * out, size_t out_size, char const * text)
{
	size_t used = strlen(out);
	if (used + 1 >= out_size) return;
	size_t room = out_size - used - 1;
	size_t len = strlen(text);
	if (len > room) len = room;
	memcpy(out + used, text, len);
	out[used + len] = '\0';
}


bool RA_Boot_Check_Required(RABootSearchList const * list, char * report, size_t report_size)
{
	bool missing[16];
	bool all = true;
	for (int r = 0; r < RA_Boot_Required_Count && r < 16; r++) {
		RABootRequirement const & req = RA_Boot_Required_Files[r];
		bool ok = RA_Boot_Find(list, req.Name, NULL, 0) >= 0;
		if (!ok && req.Alternative != NULL) {
			ok = RA_Boot_Find(list, req.Alternative, NULL, 0) >= 0;
		}
		missing[r] = !ok;
		if (!ok) all = false;
	}
	if (all) {
		if (report && report_size) report[0] = '\0';
		return true;
	}
	if (report == NULL || report_size == 0) return false;

	report[0] = '\0';
	Append(report, report_size, "The Red Alert game data was not found.\n\nMissing:\n");
	for (int r = 0; r < RA_Boot_Required_Count && r < 16; r++) {
		if (!missing[r]) continue;
		Append(report, report_size, "  ");
		Append(report, report_size, RA_Boot_Required_Files[r].Name);
		Append(report, report_size, " - ");
		Append(report, report_size, RA_Boot_Required_Files[r].What);
		Append(report, report_size, "\n");
	}
	Append(report, report_size, "\nFolders searched, in this order:\n");
	if (list == NULL || list->Count == 0) {
		Append(report, report_size, "  (none)\n");
	} else {
		for (int i = 0; i < list->Count; i++) {
			char line[RA_BOOT_PATH_MAX + 64];
			snprintf(line, sizeof(line), "  %d. %s (%s)%s\n", i + 1, list->Dir[i], list->Label[i],
				RA_Boot_Is_Directory(list->Dir[i]) ? "" : " - does not exist");
			Append(report, report_size, line);
		}
	}
	Append(report, report_size,
		"\nCopy the .MIX files from your own Red Alert installation (for example the C&C Ultimate "
		"Collection, or the 2008 freeware release) into one of these folders, or start the game "
		"with -data=<folder>, or set Path= in the [Data] section of REDALERT.INI.\n"
		"Upper/lower case of the file names does not matter. See docs/playing.md.");
	return false;
}


/*
**	The first-run REDALERT.INI. Keep this in step with docs/playing.md ("REDALERT.INI").
**	The game rewrites the file when it saves its options, which drops these comments.
*/
char const RA_Boot_Default_Config[] =
	"; REDALERT.INI - created by Red Alert on its first run.\n"
	"; Every key is described in docs/playing.md. Delete this file to get the defaults back.\n"
	"\n"
	"[Data]\n"
	"; Folder that holds your Red Alert data files (REDALERT.MIX, MAIN.MIX, ...).\n"
	"; Empty: look in the game's folder, then in the folder the game was started from.\n"
	"; A relative path is relative to the game's folder. -data=<folder> overrides this.\n"
	"Path=\n"
	"\n"
	"[Options]\n"
	"; 0 = 640x400 (the original default), 1 = 640x480 (the picture is centred).\n"
	"Resolution=0\n";


int RA_Boot_Ensure_Config(char const * filename)
{
	if (filename == NULL) {
		errno = EINVAL;
		return -1;
	}
	if (RA_File_Exists(filename)) return 0;

	int handle = RA_File_Open(filename, RA_FILE_WRITE);
	if (handle == -1) return -1;
	long len = (long)strlen(RA_Boot_Default_Config);
	long wrote = RA_File_Write(handle, RA_Boot_Default_Config, len);
	int saved = errno;
	RA_File_Close(handle);
	if (wrote != len) {
		RA_File_Delete(filename);
		errno = saved ? saved : EIO;
		return -1;
	}
	return 1;
}
