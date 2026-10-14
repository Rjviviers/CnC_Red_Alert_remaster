/*
**	Command & Conquer Red Alert(tm) - multiplayer port. GPLv3 or later, see LICENSE.md.
**	<windowsx.h> for non-Windows hosts (session S06a, ADR-0014): the message-cracker
**	macros the code uses. See windows.h in this directory.
*/
#ifndef RA_COMPAT_WINDOWSX_H
#define RA_COMPAT_WINDOWSX_H
#include <windows.h>
#define GET_X_LPARAM(lp)	((int)(short)LOWORD(lp))
#define GET_Y_LPARAM(lp)	((int)(short)HIWORD(lp))
#endif
