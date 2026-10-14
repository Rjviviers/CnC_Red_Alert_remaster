/*
**	Command & Conquer Red Alert(tm) - multiplayer port
**	Watcom/Microsoft C runtime extensions for non-Windows hosts (session S06a, ADR-0014).
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

/*
**	Included by ra_compat.h on hosts without the Microsoft/Watcom C runtime (Linux).
**	Provides the non-standard runtime functions and constants the 1996 code calls, with
**	the same semantics as the Watcom 10.6 / MSVC runtime. Only what the code uses.
**	Documented in docs/codebase/compat-notes.md.
*/

#ifndef RA_CRT_COMPAT_H
#define RA_CRT_COMPAT_H

#include <string.h>
#include <strings.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <unistd.h>
#include <limits.h>

/* Path length limits, with the values of the Microsoft runtime (stdlib.h). */
#ifndef _MAX_PATH
#define _MAX_PATH	260
#define _MAX_DRIVE	3
#define _MAX_DIR	256
#define _MAX_FNAME	256
#define _MAX_EXT	256
#endif

/* Case-insensitive comparisons. Watcom: strcmpi == stricmp. */
inline int stricmp(char const * a, char const * b) {return strcasecmp(a, b);}
inline int _stricmp(char const * a, char const * b) {return strcasecmp(a, b);}
inline int strcmpi(char const * a, char const * b) {return strcasecmp(a, b);}
inline int strnicmp(char const * a, char const * b, size_t n) {return strncasecmp(a, b, n);}
inline int _strnicmp(char const * a, char const * b, size_t n) {return strncasecmp(a, b, n);}
inline int strncmpi(char const * a, char const * b, size_t n) {return strncasecmp(a, b, n);}
inline int memicmp(void const * a, void const * b, size_t n)
{
	unsigned char const * pa = (unsigned char const *)a;
	unsigned char const * pb = (unsigned char const *)b;
	for (size_t i = 0; i < n; i++) {
		int ca = tolower(pa[i]);
		int cb = tolower(pb[i]);
		if (ca != cb) return ca - cb;
	}
	return 0;
}
inline int _memicmp(void const * a, void const * b, size_t n) {return memicmp(a, b, n);}

/* In-place string transformations; return their argument like the originals. */
inline char * strupr(char * s) {for (char * p = s; *p; p++) *p = (char)toupper((unsigned char)*p); return s;}
inline char * strlwr(char * s) {for (char * p = s; *p; p++) *p = (char)tolower((unsigned char)*p); return s;}
inline char * _strupr(char * s) {return strupr(s);}
inline char * _strlwr(char * s) {return strlwr(s);}
inline char * strrev(char * s)
{
	size_t len = strlen(s);
	for (size_t i = 0; i < len / 2; i++) {
		char t = s[i];
		s[i] = s[len - 1 - i];
		s[len - 1 - i] = t;
	}
	return s;
}
inline char * _strrev(char * s) {return strrev(s);}
inline char * strset(char * s, int c) {for (char * p = s; *p; p++) *p = (char)c; return s;}
inline char * strnset(char * s, int c, size_t n) {for (char * p = s; *p && n; p++, n--) *p = (char)c; return s;}

/*
**	Integer to string in any base 2..36, like the Microsoft runtime: negative values are
**	only written with a minus sign in base 10 (other bases print the two's complement).
*/
inline char * _ra_ultoa(unsigned long value, char * buf, int radix, bool negative)
{
	char tmp[33];
	int n = 0;
	if (radix < 2 || radix > 36) {buf[0] = '\0'; return buf;}
	do {
		int d = (int)(value % (unsigned long)radix);
		tmp[n++] = (char)(d < 10 ? '0' + d : 'a' + d - 10);
		value /= (unsigned long)radix;
	} while (value != 0);
	char * p = buf;
	if (negative) *p++ = '-';
	while (n) *p++ = tmp[--n];
	*p = '\0';
	return buf;
}
inline char * ultoa(unsigned long value, char * buf, int radix) {return _ra_ultoa(value, buf, radix, false);}
inline char * ltoa(long value, char * buf, int radix)
{
	if (radix == 10 && value < 0) return _ra_ultoa(0UL - (unsigned long)value, buf, radix, true);
	return _ra_ultoa((unsigned long)value, buf, radix, false);
}
inline char * itoa(int value, char * buf, int radix)
{
	if (radix == 10 && value < 0) return _ra_ultoa(0UL - (unsigned long)(long)value, buf, radix, true);
	return _ra_ultoa((unsigned long)(unsigned int)value, buf, radix, false);
}
inline char * _itoa(int value, char * buf, int radix) {return itoa(value, buf, radix);}
inline char * _ltoa(long value, char * buf, int radix) {return ltoa(value, buf, radix);}
inline char * _ultoa(unsigned long value, char * buf, int radix) {return ultoa(value, buf, radix);}

/* 32-bit rotates (Microsoft runtime: stdlib.h). */
inline unsigned long _lrotl(unsigned long value, int shift)
{
	unsigned int v = (unsigned int)value;
	shift &= 31;
	return shift ? (unsigned long)((v << shift) | (v >> (32 - shift))) : (unsigned long)v;
}
inline unsigned long _lrotr(unsigned long value, int shift)
{
	unsigned int v = (unsigned int)value;
	shift &= 31;
	return shift ? (unsigned long)((v >> shift) | (v << (32 - shift))) : (unsigned long)v;
}

/*
**	_splitpath / _makepath. There are no drive letters on POSIX: the drive part is
**	always empty, and both '/' and '\\' separate directories (the data files use '\\').
*/
inline void _splitpath(char const * path, char * drive, char * dir, char * fname, char * ext)
{
	if (drive) drive[0] = '\0';
	char const * p = path;
	if (p[0] != '\0' && p[1] == ':') {
		if (drive) {drive[0] = p[0]; drive[1] = ':'; drive[2] = '\0';}
		p += 2;
	}
	char const * slash = NULL;
	for (char const * q = p; *q; q++) if (*q == '/' || *q == '\\') slash = q;
	char const * base = slash ? slash + 1 : p;
	if (dir) {
		size_t n = (size_t)(base - p);
		if (n >= _MAX_DIR) n = _MAX_DIR - 1;
		memcpy(dir, p, n);
		dir[n] = '\0';
	}
	char const * dot = strrchr(base, '.');
	if (!dot) dot = base + strlen(base);
	if (fname) {
		size_t n = (size_t)(dot - base);
		if (n >= _MAX_FNAME) n = _MAX_FNAME - 1;
		memcpy(fname, base, n);
		fname[n] = '\0';
	}
	if (ext) {
		strncpy(ext, dot, _MAX_EXT - 1);
		ext[_MAX_EXT - 1] = '\0';
	}
}

inline void _makepath(char * path, char const * drive, char const * dir, char const * fname, char const * ext)
{
	path[0] = '\0';
	if (drive && *drive) {
		path[0] = drive[0];
		path[1] = ':';
		path[2] = '\0';
	}
	if (dir && *dir) {
		strcat(path, dir);
		char last = dir[strlen(dir) - 1];
		if (last != '/' && last != '\\') strcat(path, "/");
	}
	if (fname) strcat(path, fname);
	if (ext && *ext) {
		if (*ext != '.') strcat(path, ".");
		strcat(path, ext);
	}
}

inline char * _fullpath(char * absolute, char const * relative, size_t max)
{
	char * r = realpath(relative, NULL);
	if (!r) return NULL;
	if (!absolute) return r;
	if (strlen(r) >= max) {free(r); return NULL;}
	strcpy(absolute, r);
	free(r);
	return absolute;
}

#endif /* RA_CRT_COMPAT_H */
