/*
**	Command & Conquer Red Alert(tm) - multiplayer port. GPLv3 or later, see LICENSE.md.
**	Compatibility shim (session S06a, ADR-0014). See docs/codebase/compat-notes.md.
**	Non-Windows hosts only: this directory is searched after the system headers
**	(-idirafter), so a real header of the same name always wins.
**
**	<share.h>: file sharing modes for sopen(). POSIX has no mandatory sharing modes;
**	the constants exist so the code compiles, and sopen() ignores them.
*/
#ifndef RA_COMPAT_SHARE_H
#define RA_COMPAT_SHARE_H
#include <io.h>
#define SH_COMPAT	0x00
#define SH_DENYRW	0x10
#define SH_DENYWR	0x20
#define SH_DENYRD	0x30
#define SH_DENYNO	0x40
#define _SH_COMPAT	SH_COMPAT
#define _SH_DENYRW	SH_DENYRW
#define _SH_DENYWR	SH_DENYWR
#define _SH_DENYRD	SH_DENYRD
#define _SH_DENYNO	SH_DENYNO
inline int sopen(char const * name, int flags, int share, int mode = 0666) {(void)share; return open(name, flags, mode);}
inline int _sopen(char const * name, int flags, int share, int mode = 0666) {return sopen(name, flags, share, mode);}
#endif
