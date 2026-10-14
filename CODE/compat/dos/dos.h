/*
**	Command & Conquer Red Alert(tm) - multiplayer port. GPLv3 or later, see LICENSE.md.
**	Compatibility shim for Watcom's <dos.h> (session S06a, ADR-0014).
**	See docs/codebase/compat-notes.md.
**
**	Neither MSVC nor glibc has a usable <dos.h>, so this one is used on every compiler
**	(it is on the normal include path and shadows MinGW's small dos.h too). It provides
**	the parts of the Watcom 10.6 <dos.h> the code still uses:
**	  - _dos_findfirst/_dos_findnext/struct find_t: real implementations (save-game and
**	    expansion MIX scanning need them at run time);
**	  - _dos_getdrive/_dos_getdiskfree/struct diskfree_t: real implementations (free disk
**	    space check before saving);
**	  - union REGS/struct SREGS/segread/FP_SEG/FP_OFF: real-mode interrupt types; only
**	    referenced by DOS code that is never executed in the Win32 build.
*/

#ifndef RA_COMPAT_DOS_H
#define RA_COMPAT_DOS_H

#include <stddef.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

#if defined(_WIN32) && (defined(_MSC_VER) || defined(__MINGW32__))
#  include <io.h>
#  include <direct.h>
#  define RA_DOS_WINDOWS_CRT 1
#else
#  include <dirent.h>
#  include <fnmatch.h>
#  include <sys/stat.h>
#  include <sys/statvfs.h>
#endif

/* File attributes (Watcom values). */
#define _A_NORMAL	0x00
#define _A_RDONLY	0x01
#define _A_HIDDEN	0x02
#define _A_SYSTEM	0x04
#define _A_VOLID	0x08
#define _A_SUBDIR	0x10
#define _A_ARCH		0x20

/*
**	Watcom's struct find_t. The 21 reserved bytes hold the search state in DOS; here
**	they hold a pointer to our own state (freed when the search ends).
*/
struct find_t {
	char			reserved[21];
	char			attrib;
	unsigned short	wr_time;
	unsigned short	wr_date;
	unsigned long	size;
	char			name[13];
};

namespace ra_dos_detail {

/* DOS packed date/time of a time_t (local time). */
inline void Pack_Time(time_t t, unsigned short & dtime, unsigned short & ddate)
{
	struct tm * lt = localtime(&t);
	if (!lt) {dtime = 0; ddate = 0; return;}
	dtime = (unsigned short)((lt->tm_hour << 11) | (lt->tm_min << 5) | (lt->tm_sec / 2));
	ddate = (unsigned short)(((lt->tm_year - 80) << 9) | ((lt->tm_mon + 1) << 5) | lt->tm_mday);
}

struct FindState {
#ifdef RA_DOS_WINDOWS_CRT
	intptr_t Handle;
#else
	DIR * Dir;
	char Directory[260];
	char Pattern[260];
#endif
	unsigned Attributes;
};

inline FindState * State_Of(struct find_t * f)
{
	FindState * s;
	memcpy(&s, f->reserved, sizeof(s));
	return s;
}

inline void Set_State(struct find_t * f, FindState * s)
{
	memcpy(f->reserved, &s, sizeof(s));
}

inline void Copy_Name(struct find_t * f, char const * name)
{
	strncpy(f->name, name, sizeof(f->name) - 1);
	f->name[sizeof(f->name) - 1] = '\0';
}

#ifdef RA_DOS_WINDOWS_CRT
inline bool Accept(struct _finddata_t const & d, unsigned attributes)
{
	/* DOS semantics: normal files always match, hidden/system/subdir only if asked. */
	unsigned special = d.attrib & (_A_HIDDEN | _A_SYSTEM | _A_SUBDIR);
	return (special & ~attributes) == 0;
}

inline void Fill(struct find_t * f, struct _finddata_t const & d)
{
	f->attrib = (char)d.attrib;
	Pack_Time(d.time_write, f->wr_time, f->wr_date);
	f->size = (unsigned long)d.size;
	Copy_Name(f, d.name);
}
#else
inline int Next(struct find_t * f)
{
	FindState * s = State_Of(f);
	if (!s || !s->Dir) return 1;
	struct dirent * e;
	while ((e = readdir(s->Dir)) != NULL) {
		/* DOS file names are case insensitive. */
		if (fnmatch(s->Pattern, e->d_name, FNM_CASEFOLD) != 0) continue;
		char full[600];
		strcpy(full, s->Directory);
		strcat(full, e->d_name);
		struct stat st;
		if (stat(full, &st) != 0) continue;
		bool is_dir = S_ISDIR(st.st_mode);
		if (is_dir && !(s->Attributes & _A_SUBDIR)) continue;
		f->attrib = (char)(is_dir ? _A_SUBDIR : _A_ARCH);
		Pack_Time(st.st_mtime, f->wr_time, f->wr_date);
		f->size = (unsigned long)st.st_size;
		Copy_Name(f, e->d_name);
		return 0;
	}
	return 1;
}
#endif

inline void Finish(struct find_t * f)
{
	FindState * s = State_Of(f);
	if (!s) return;
#ifdef RA_DOS_WINDOWS_CRT
	if (s->Handle != -1) _findclose(s->Handle);
#else
	if (s->Dir) closedir(s->Dir);
#endif
	delete s;
	Set_State(f, NULL);
}

} // namespace ra_dos_detail

/* Returns 0 when a file was found (like Watcom), non-zero otherwise. */
inline unsigned _dos_findnext(struct find_t * f)
{
#ifdef RA_DOS_WINDOWS_CRT
	ra_dos_detail::FindState * s = ra_dos_detail::State_Of(f);
	if (!s) return 1;
	struct _finddata_t d;
	while (_findnext(s->Handle, &d) == 0) {
		if (ra_dos_detail::Accept(d, s->Attributes)) {
			ra_dos_detail::Fill(f, d);
			return 0;
		}
	}
	ra_dos_detail::Finish(f);
	return 1;
#else
	if (ra_dos_detail::Next(f) == 0) return 0;
	ra_dos_detail::Finish(f);
	return 1;
#endif
}

inline unsigned _dos_findfirst(char const * path, unsigned attributes, struct find_t * f)
{
	memset(f, 0, sizeof(*f));
	ra_dos_detail::FindState * s = new ra_dos_detail::FindState;
	s->Attributes = attributes;
	ra_dos_detail::Set_State(f, s);
#ifdef RA_DOS_WINDOWS_CRT
	struct _finddata_t d;
	s->Handle = _findfirst(path, &d);
	if (s->Handle == -1) {ra_dos_detail::Finish(f); return 1;}
	if (ra_dos_detail::Accept(d, attributes)) {
		ra_dos_detail::Fill(f, d);
		return 0;
	}
	return _dos_findnext(f);
#else
	/* Split "dir/pattern" (either slash) into directory and pattern. */
	char const * slash = NULL;
	for (char const * p = path; *p; p++) if (*p == '/' || *p == '\\') slash = p;
	if (slash) {
		size_t n = (size_t)(slash - path) + 1;
		if (n >= sizeof(s->Directory)) n = sizeof(s->Directory) - 1;
		memcpy(s->Directory, path, n);
		s->Directory[n] = '\0';
		for (char * p = s->Directory; *p; p++) if (*p == '\\') *p = '/';
	} else {
		s->Directory[0] = '\0';
	}
	strncpy(s->Pattern, slash ? slash + 1 : path, sizeof(s->Pattern) - 1);
	s->Pattern[sizeof(s->Pattern) - 1] = '\0';
	s->Dir = opendir(s->Directory[0] ? s->Directory : ".");
	return _dos_findnext(f);
#endif
}

/* Drive numbers: 1 = A:, 3 = C:. POSIX has no drives; the current directory is "C:". */
inline void _dos_getdrive(unsigned * drive)
{
#ifdef RA_DOS_WINDOWS_CRT
	*drive = (unsigned)_getdrive();
#else
	*drive = 3;
#endif
}

inline void _dos_setdrive(unsigned drive, unsigned * total)
{
#ifdef RA_DOS_WINDOWS_CRT
	_chdrive((int)drive);
#else
	(void)drive;
#endif
	if (total) *total = 26;
}

#ifdef RA_DOS_WINDOWS_CRT
/* The Microsoft runtime's <direct.h> has struct _diskfree_t (and #defines diskfree_t). */
#ifndef diskfree_t
#define diskfree_t _diskfree_t
#endif
#else
struct diskfree_t {
	unsigned total_clusters;
	unsigned avail_clusters;
	unsigned sectors_per_cluster;
	unsigned bytes_per_sector;
};
#endif

/*
**	Free space of a drive (0 = current). Values are scaled so that the product the
**	game computes (clusters * sectors * bytes) stays below 2 GB, as on a FAT disk.
*/
inline unsigned _dos_getdiskfree(unsigned drive, struct diskfree_t * d)
{
	unsigned long long avail = 0, total = 0;
#ifdef RA_DOS_WINDOWS_CRT
	struct _diskfree_t w;
	if (_getdiskfree(drive, &w) != 0) return 1;	/* fills a copy, then rescaled below */
	unsigned long long cluster = (unsigned long long)w.sectors_per_cluster * w.bytes_per_sector;
	avail = cluster * w.avail_clusters;
	total = cluster * w.total_clusters;
#else
	(void)drive;
	struct statvfs v;
	if (statvfs(".", &v) != 0) return 1;
	avail = (unsigned long long)v.f_bavail * v.f_frsize;
	total = (unsigned long long)v.f_blocks * v.f_frsize;
#endif
	unsigned long long const cap = 0x7FFF0000ULL;
	if (avail > cap) avail = cap;
	if (total > cap) total = cap;
	d->bytes_per_sector = 512;
	d->sectors_per_cluster = 128;		/* 64 KB clusters */
	d->avail_clusters = (unsigned)(avail / 65536ULL);
	d->total_clusters = (unsigned)(total / 65536ULL);
	return 0;
}

/*
**	Real-mode interrupt interface. Only referenced by DOS code paths; never executed.
*/
struct DWORDREGS {unsigned int eax, ebx, ecx, edx, esi, edi, cflag;};
struct WORDREGS {unsigned short ax, _1, bx, _2, cx, _3, dx, _4, si, _5, di, _6; unsigned int cflag;};
struct BYTEREGS {unsigned char al, ah; unsigned short _1; unsigned char bl, bh; unsigned short _2;
                 unsigned char cl, ch; unsigned short _3; unsigned char dl, dh; unsigned short _4;};
union REGS {struct DWORDREGS x; struct DWORDREGS w32; struct WORDREGS w; struct BYTEREGS h;};
struct SREGS {unsigned short es, cs, ss, ds, fs, gs;};
inline void segread(struct SREGS * s) {memset(s, 0, sizeof(*s));}
#ifndef FP_OFF
#define FP_OFF(p)	((unsigned)(size_t)(p))
#define FP_SEG(p)	((unsigned)0)
#endif

#endif /* RA_COMPAT_DOS_H */
