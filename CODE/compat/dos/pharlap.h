/*
**	Command & Conquer Red Alert(tm) - multiplayer port. GPLv3 or later, see LICENSE.md.
**	Compatibility shim (session S06a, ADR-0014). See docs/codebase/compat-notes.md.
**
**	<pharlap.h> (Phar Lap TNT DOS extender). Only the types that the WINVQ headers name
**	in structure declarations (VESA mode info). Nothing calls the DOS extender.
**	REALPTR is a real-mode segment:offset pointer, 4 bytes.
*/
#ifndef RA_COMPAT_PHARLAP_H
#define RA_COMPAT_PHARLAP_H
typedef unsigned long REALPTR;
typedef unsigned short SEL;
typedef unsigned long ULONG;
typedef unsigned short USHORT;
typedef unsigned char UCHAR;
#endif
