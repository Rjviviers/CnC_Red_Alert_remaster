/*
**	Command & Conquer Red Alert(tm) - multiplayer port. GPLv3 or later, see LICENSE.md.
**	Compatibility shim (session S06a, ADR-0014). See docs/codebase/compat-notes.md.
**	Non-Windows hosts only: this directory is searched after the system headers
**	(-idirafter), so a real header of the same name always wins.
**
**	<new.h>: the pre-standard name of <new>. Watcom's _set_new_handler is mapped to
**	std::set_new_handler.
*/
#ifndef RA_COMPAT_NEW_H
#define RA_COMPAT_NEW_H
#include <new>
using std::set_new_handler;
using std::new_handler;
#endif
