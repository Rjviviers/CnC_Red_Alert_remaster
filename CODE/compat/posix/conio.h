/*
**	Command & Conquer Red Alert(tm) - multiplayer port. GPLv3 or later, see LICENSE.md.
**	Compatibility shim (session S06a, ADR-0014). See docs/codebase/compat-notes.md.
**	Non-Windows hosts only: this directory is searched after the system headers
**	(-idirafter), so a real header of the same name always wins.
**
**	<conio.h> (DOS console and port I/O). There is no DOS console: kbhit() reports no
**	key and getch() returns 0. Port I/O (inp/outp) cannot be done from user mode on any
**	modern OS (the Win32 build would have crashed too), so it does nothing.
*/
#ifndef RA_COMPAT_CONIO_H
#define RA_COMPAT_CONIO_H
#include <stdio.h>
inline int kbhit(void) {return 0;}
inline int getch(void) {return 0;}
inline int getche(void) {return 0;}
inline int _kbhit(void) {return 0;}
inline int _getch(void) {return 0;}
inline int cputs(char const * s) {return fputs(s, stdout);}
inline int inp(unsigned port) {(void)port; return 0;}
inline unsigned inpw(unsigned port) {(void)port; return 0;}
inline int outp(unsigned port, int value) {(void)port; return value;}
inline unsigned outpw(unsigned port, unsigned value) {(void)port; return value;}
#endif
