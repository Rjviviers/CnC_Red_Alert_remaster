/*
**	Command & Conquer Red Alert(tm) - multiplayer port. GPLv3 or later, see LICENSE.md.
**	Compatibility shim (session S06a, ADR-0014). See docs/codebase/compat-notes.md.
**	Non-Windows hosts only: this directory is searched after the system headers
**	(-idirafter), so a real header of the same name always wins.
**
**	<direct.h> (Microsoft/Watcom directory functions). There are no drive letters on
**	POSIX; drive functions report drive C: (3) and changing drives does nothing.
*/
#ifndef RA_COMPAT_DIRECT_H
#define RA_COMPAT_DIRECT_H
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
inline int mkdir(char const * path) {return mkdir(path, 0777);}
inline int _mkdir(char const * path) {return mkdir(path, 0777);}
inline int _rmdir(char const * path) {return rmdir(path);}
inline int _chdir(char const * path) {return chdir(path);}
inline char * _getcwd(char * buf, int size) {return getcwd(buf, (size_t)size);}
inline char * _getdcwd(int drive, char * buf, int size) {(void)drive; return getcwd(buf, (size_t)size);}
inline int _getdrive(void) {return 3;}
inline int _chdrive(int drive) {(void)drive; return 0;}
#endif
