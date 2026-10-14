/*
**	Command & Conquer Red Alert(tm) - multiplayer port. GPLv3 or later, see LICENSE.md.
**	Compatibility shim (session S06a, ADR-0014). See docs/codebase/compat-notes.md.
**	Non-Windows hosts only: this directory is searched after the system headers
**	(-idirafter), so a real header of the same name always wins.
**
**	<process.h> (Microsoft/Watcom process control): getpid, exit, spawn*. Only the
**	POSIX equivalents of what the code calls.
*/
#ifndef RA_COMPAT_PROCESS_H
#define RA_COMPAT_PROCESS_H
#include <stdlib.h>
#include <unistd.h>
inline int _getpid(void) {return (int)getpid();}
#endif
