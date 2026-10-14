/*
**	Command & Conquer Red Alert(tm) - multiplayer port. GPLv3 or later, see LICENSE.md.
**	Compatibility shim (session S06a, ADR-0014). See docs/codebase/compat-notes.md.
**
**	<pldos32.h> (Phar Lap TNT DOS extender, DOS calls). Nothing calls it in the Win32
**	build; the header only has to exist.
*/
#ifndef RA_COMPAT_PLDOS32_H
#define RA_COMPAT_PLDOS32_H
#include <pharlap.h>
#endif
