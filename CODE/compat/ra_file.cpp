/*
**	Command & Conquer Red Alert(tm) - multiplayer port. GPLv3 or later, see LICENSE.md.
**	Portable low-level file I/O (session S10b, ADR-0022). See ra_file.h.
**
**	Built with the game's settings (1-byte struct packing), so every system header is
**	included under natural packing, and no STL container is used (libstdc++'s
**	precompiled instantiations would not match a packed layout).
*/

#if !defined(_MSC_VER)
#  pragma pack(push, 8)
#endif
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#if defined(_WIN32)
#  include <io.h>
#  include <sys/utime.h>
#else
#  include <dirent.h>
#  include <strings.h>
#  include <unistd.h>
#endif
#if !defined(_MSC_VER)
#  pragma pack(pop)
#endif

#include "ra_file.h"

#if defined(_WIN32)
#  define RA_OPEN		_open
#  define RA_CLOSE		_close
#  define RA_READ		_read
#  define RA_WRITE		_write
#  define RA_LSEEK		_lseek
#  define RA_FSTAT		_fstat
#  define RA_STAT_T		struct _stat
#  define RA_BINARY		_O_BINARY
#else
#  define RA_OPEN		open
#  define RA_CLOSE		close
#  define RA_READ		read
#  define RA_WRITE		write
#  define RA_LSEEK		lseek
#  define RA_FSTAT		fstat
#  define RA_STAT_T		struct stat
#  define RA_BINARY		0
#endif


/* ======================================================================== */
/* Path resolution                                                           */
/* ======================================================================== */

#if !defined(_WIN32)
/*
**	Find the entry of 'dir' that matches 'name' case-insensitively. Several entries can
**	match on a case-sensitive file system ("main.mix" and "MAIN.MIX"); take the one that
**	sorts first byte-wise, so the answer never depends on readdir order.
*/
static bool Find_Entry_Nocase(char const * dir, char const * name, char * found, size_t found_size)
{
	DIR * d = opendir(dir[0] ? dir : ".");
	if (d == NULL) return false;

	bool have = false;
	struct dirent * e;
	while ((e = readdir(d)) != NULL) {
		if (strcasecmp(e->d_name, name) != 0) continue;
		if (strlen(e->d_name) >= found_size) continue;
		if (!have || strcmp(e->d_name, found) < 0) {
			strcpy(found, e->d_name);
			have = true;
		}
	}
	closedir(d);
	return have;
}


static bool Exists(char const * path)
{
	struct stat st;
	return stat(path[0] ? path : ".", &st) == 0;
}
#endif


bool RA_Resolve_Path(char const * name, char * out, size_t out_size, bool must_exist)
{
	if (name == NULL || out == NULL || out_size == 0) return false;
	size_t len = strlen(name);
	if (len >= out_size) return false;

#if defined(_WIN32)
	/*
	**	Windows is already case-insensitive and accepts both separators: pass the name
	**	through, but honour must_exist like the POSIX branch does.
	*/
	if (must_exist) {
		struct stat st;
		if (stat(name, &st) != 0) return false;
	}
	memcpy(out, name, len + 1);
	return true;
#else
	/*
	**	DOS spelling to POSIX spelling: separators, and no drive letter.
	*/
	char path[1024];
	if (len >= sizeof(path)) return false;
	char const * src = name;
	if (len >= 2 && ((src[0] >= 'A' && src[0] <= 'Z') || (src[0] >= 'a' && src[0] <= 'z')) && src[1] == ':') {
		src += 2;
	}
	size_t n = 0;
	for (; *src; src++) path[n++] = (*src == '\\') ? '/' : *src;
	path[n] = '\0';

	/*
	**	Fast path: the name already exists exactly as spelled.
	*/
	if (Exists(path)) {
		if (n >= out_size) return false;
		memcpy(out, path, n + 1);
		return true;
	}

	/*
	**	Slow path: walk the components, matching each case-insensitively.
	*/
	char result[1024];
	size_t rlen = 0;
	result[0] = '\0';
	char const * p = path;
	if (*p == '/') {
		result[rlen++] = '/';
		result[rlen] = '\0';
		while (*p == '/') p++;
	}

	while (*p) {
		char const * end = strchr(p, '/');
		size_t clen = end ? (size_t)(end - p) : strlen(p);
		bool last = (end == NULL);
		char component[256];
		if (clen >= sizeof(component)) return false;
		memcpy(component, p, clen);
		component[clen] = '\0';

		/*
		**	Try the exact spelling first, then a case-insensitive match.
		*/
		char candidate[1024];
		if (rlen + clen + 2 >= sizeof(candidate)) return false;
		memcpy(candidate, result, rlen);
		memcpy(candidate + rlen, component, clen + 1);

		if (!Exists(candidate) && strcmp(component, ".") != 0 && strcmp(component, "..") != 0) {
			char found[256];
			if (Find_Entry_Nocase(result, component, found, sizeof(found))) {
				size_t flen = strlen(found);
				if (rlen + flen + 2 >= sizeof(candidate)) return false;
				memcpy(candidate + rlen, found, flen + 1);
			} else if (!(last && !must_exist)) {
				return false;
			}
		}

		rlen = strlen(candidate);
		memcpy(result, candidate, rlen + 1);
		if (end == NULL) break;

		/* Keep exactly one separator, and a trailing one if the name had it. */
		result[rlen++] = '/';
		result[rlen] = '\0';
		p = end;
		while (*p == '/') p++;
	}

	if (rlen >= out_size) return false;
	memcpy(out, result, rlen + 1);
	return true;
#endif
}


/* ======================================================================== */
/* Handles                                                                   */
/* ======================================================================== */

int RA_File_Open(char const * name, int rights)
{
	if (name == NULL) {
		errno = ENOENT;
		return -1;
	}

	int flags;
	switch (rights) {
		case RA_FILE_READ:
			flags = O_RDONLY;
			break;
		case RA_FILE_WRITE:
			flags = O_WRONLY | O_CREAT | O_TRUNC;
			break;
		case RA_FILE_READ | RA_FILE_WRITE:
			flags = O_RDWR | O_CREAT | O_TRUNC;
			break;
		default:
			errno = EINVAL;
			return -1;
	}

	char path[1024];
	if (!RA_Resolve_Path(name, path, sizeof(path), rights == RA_FILE_READ)) {
		errno = ENOENT;
		return -1;
	}

	/*
	**	Only regular files: CreateFile(OPEN_EXISTING) on a directory failed too.
	*/
	int handle;
#if defined(_WIN32)
	handle = RA_OPEN(path, flags | RA_BINARY, _S_IREAD | _S_IWRITE);
#else
	handle = RA_OPEN(path, flags | RA_BINARY, 0666);
#endif
	if (handle != -1) {
		RA_STAT_T st;
		if (RA_FSTAT(handle, &st) == 0 && (st.st_mode & S_IFMT) == S_IFDIR) {
			RA_CLOSE(handle);
			errno = EISDIR;
			return -1;
		}
	}
	return handle;
}


bool RA_File_Close(int handle)
{
	if (handle == -1) return false;
	return RA_CLOSE(handle) == 0;
}


long RA_File_Read(int handle, void * buffer, long size)
{
	if (handle == -1 || size < 0) {
		errno = EBADF;
		return -1;
	}
	long total = 0;
	char * dst = (char *)buffer;
	while (total < size) {
		long r = (long)RA_READ(handle, dst + total, (unsigned)(size - total));
		if (r < 0) {
			if (errno == EINTR) continue;
			return total > 0 ? total : -1;
		}
		if (r == 0) break;
		total += r;
	}
	return total;
}


long RA_File_Write(int handle, void const * buffer, long size)
{
	if (handle == -1 || size < 0) {
		errno = EBADF;
		return -1;
	}
	long total = 0;
	char const * src = (char const *)buffer;
	while (total < size) {
		long w = (long)RA_WRITE(handle, src + total, (unsigned)(size - total));
		if (w < 0) {
			if (errno == EINTR) continue;
			return -1;
		}
		if (w == 0) break;
		total += w;
	}
	return total;
}


long RA_File_Seek(int handle, long pos, int whence)
{
	if (handle == -1) {
		errno = EBADF;
		return -1;
	}
	return (long)RA_LSEEK(handle, pos, whence);
}


long RA_File_Size(int handle)
{
	RA_STAT_T st;
	if (handle == -1 || RA_FSTAT(handle, &st) != 0) return -1;
	return (long)st.st_size;
}


bool RA_File_Delete(char const * name)
{
	char path[1024];
	if (!RA_Resolve_Path(name, path, sizeof(path), true)) {
		errno = ENOENT;
		return false;
	}
	return remove(path) == 0;
}


bool RA_File_Exists(char const * name)
{
	char path[1024];
	if (!RA_Resolve_Path(name, path, sizeof(path), true)) return false;
	struct stat st;
	return stat(path, &st) == 0 && (st.st_mode & S_IFMT) == S_IFREG;
}


/* ======================================================================== */
/* DOS date/time                                                             */
/* ======================================================================== */

/* Days from 1970-01-01 to the given civil date (proleptic Gregorian). */
static long long Days_From_Civil(int y, int m, int d)
{
	y -= m <= 2;
	long long era = (y >= 0 ? y : y - 399) / 400;
	unsigned yoe = (unsigned)(y - era * 400);
	unsigned doy = (unsigned)((153 * (m + (m > 2 ? -3 : 9)) + 2) / 5 + d - 1);
	unsigned doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;
	return era * 146097 + (long long)doe - 719468;
}


unsigned long RA_Dos_Date_Time_From_Unix(long long seconds)
{
	/* DOS dates cover 1980-01-01 .. 2107-12-31. */
	long long const min = Days_From_Civil(1980, 1, 1) * 86400LL;
	if (seconds < min) seconds = min;

	long long days = seconds / 86400;
	long long rem = seconds % 86400;

	/* Civil date from days (inverse of Days_From_Civil). */
	long long z = days + 719468;
	long long era = (z >= 0 ? z : z - 146096) / 146097;
	unsigned doe = (unsigned)(z - era * 146097);
	unsigned yoe = (doe - doe / 1460 + doe / 36524 - doe / 146096) / 365;
	long long y = (long long)yoe + era * 400;
	unsigned doy = doe - (365 * yoe + yoe / 4 - yoe / 100);
	unsigned mp = (5 * doy + 2) / 153;
	unsigned d = doy - (153 * mp + 2) / 5 + 1;
	unsigned m = mp < 10 ? mp + 3 : mp - 9;
	y += (m <= 2);

	if (y > 2107) return 0;

	unsigned hour = (unsigned)(rem / 3600);
	unsigned minute = (unsigned)((rem / 60) % 60);
	unsigned second = (unsigned)(rem % 60);

	unsigned long dosdate = ((unsigned long)(y - 1980) << 9) | (m << 5) | d;
	unsigned long dostime = (hour << 11) | (minute << 5) | (second / 2);
	return (dosdate << 16) | dostime;
}


long long RA_Unix_From_Dos_Date_Time(unsigned long datetime)
{
	unsigned dosdate = (unsigned)((datetime >> 16) & 0xFFFF);
	unsigned dostime = (unsigned)(datetime & 0xFFFF);
	int year = 1980 + (int)(dosdate >> 9);
	int month = (int)((dosdate >> 5) & 0x0F);
	int day = (int)(dosdate & 0x1F);
	int hour = (int)(dostime >> 11);
	int minute = (int)((dostime >> 5) & 0x3F);
	int second = (int)(dostime & 0x1F) * 2;
	if (month < 1 || month > 12 || day < 1 || hour > 23 || minute > 59 || second > 59) return -1;
	return Days_From_Civil(year, month, day) * 86400LL + hour * 3600 + minute * 60 + second;
}


unsigned long RA_File_Get_Dos_Date_Time(int handle)
{
	RA_STAT_T st;
	if (handle == -1 || RA_FSTAT(handle, &st) != 0) return 0;
	return RA_Dos_Date_Time_From_Unix((long long)st.st_mtime);
}


bool RA_File_Set_Dos_Date_Time(int handle, unsigned long datetime)
{
	if (handle == -1) return false;
	long long t = RA_Unix_From_Dos_Date_Time(datetime);
	if (t < 0) return false;
#if defined(_WIN32)
	struct _utimbuf times;
	times.actime = (time_t)t;
	times.modtime = (time_t)t;
	return _futime(handle, &times) == 0;
#else
	struct timespec times[2];
	times[0].tv_sec = (time_t)t;
	times[0].tv_nsec = 0;
	times[1] = times[0];
	return futimens(handle, times) == 0;
#endif
}
