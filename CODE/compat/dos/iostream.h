/*
**	Command & Conquer Red Alert(tm) - multiplayer port. GPLv3 or later, see LICENSE.md.
**	Compatibility shim (session S06a, ADR-0014). See docs/codebase/compat-notes.md.
**
**	<iostream.h>: the pre-standard iostream header. Maps to <iostream> and makes the
**	common names visible without std:: as they were in 1996.
*/
#ifndef RA_COMPAT_IOSTREAM_H
#define RA_COMPAT_IOSTREAM_H
#if defined(RA_PACK_STRUCTS) && !defined(_MSC_VER)
#pragma pack(push, 8)
#endif
#include <iostream>
#include <iomanip>
#if defined(RA_PACK_STRUCTS) && !defined(_MSC_VER)
#pragma pack(pop)
#endif
using std::ios;
using std::istream;
using std::ostream;
using std::cin;
using std::cout;
using std::cerr;
using std::endl;
using std::hex;
using std::dec;
using std::setw;
using std::setfill;
#endif
