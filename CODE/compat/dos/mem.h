/*
**	Command & Conquer Red Alert(tm) - multiplayer port. GPLv3 or later, see LICENSE.md.
**	Compatibility shim (session S06a, ADR-0014). See docs/codebase/compat-notes.md.
**
**	<mem.h> (Watcom/Borland): the memory functions of <string.h>, plus Borland's
**	movmem/setmem (note movmem's source-first argument order).
*/
#ifndef RA_COMPAT_MEM_H
#define RA_COMPAT_MEM_H
#include <string.h>
inline void movmem(void const * src, void * dest, unsigned length) {memmove(dest, src, length);}
inline void setmem(void * dest, unsigned length, char value) {memset(dest, value, length);}
#endif
