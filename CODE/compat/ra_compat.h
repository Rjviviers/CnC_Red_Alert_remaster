/*
**	Command & Conquer Red Alert(tm) - multiplayer port
**	Compiler compatibility pre-include (session S06a, ADR-0014).
**
**	This program is free software: you can redistribute it and/or modify
**	it under the terms of the GNU General Public License as published by
**	the Free Software Foundation, either version 3 of the License, or
**	(at your option) any later version.
**
**	This program is distributed in the hope that it will be useful,
**	but WITHOUT ANY WARRANTY; without even the implied warranty of
**	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
**	GNU General Public License for more details.
**
**	You should have received a copy of the GNU General Public License
**	along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

/*
**	ra_compat.h -- force-included in front of EVERY translation unit of every target
**	(game, win32lib, vqa32wp, vqm32wp, unit tests) by cmake/RABuildConfig.cmake
**	(GCC/Clang: -include, MSVC: /FI). It replaces what Watcom C++ 10.6 and its
**	runtime library supplied implicitly. Keep it small: only things that would
**	otherwise have to be repeated in many files. Every entry is explained in
**	docs/codebase/compat-notes.md.
*/

#ifndef RA_COMPAT_H
#define RA_COMPAT_H

/*
**	1. Native bool.
**	About 25 headers carry Watcom 10.6's workaround for the missing bool type:
**
**		#ifndef TRUE_FALSE_DEFINED
**		#define TRUE_FALSE_DEFINED
**		enum {false=0,true=1};
**		typedef int bool;
**		#endif
**
**	That is illegal in standard C++ (bool, true and false are keywords). Defining the
**	guard here turns every copy off and the code uses the built-in bool. NOTE: the
**	original bool was 4 bytes; ours is 1. See the determinism section of the S06a log.
*/
#define TRUE_FALSE_DEFINED 1

/*
**	2. Calling-convention keywords.
**	Watcom accepted "cdecl" without underscores. MSVC and MinGW know __cdecl/_cdecl.
**	On other platforms (Linux, 32-bit SysV) cdecl is the default convention, and the
**	Win32 conventions only appear on declarations of Win32 API functions that our
**	Win32 shim declares but never defines, so they are empty.
*/
#if defined(_MSC_VER) || defined(__MINGW32__)
#  ifndef cdecl
#    define cdecl __cdecl
#  endif
#else
#  ifndef __cdecl
#    define __cdecl
#  endif
#  ifndef _cdecl
#    define _cdecl
#  endif
#  ifndef cdecl
#    define cdecl
#  endif
#  ifndef __stdcall
#    define __stdcall
#  endif
#  ifndef _stdcall
#    define _stdcall
#  endif
#  ifndef __declspec
#    define __declspec(x)
#  endif
#endif

/*
**	3. Struct packing and the system headers (non-MSVC only).
**	The game is built with 1-byte packing everywhere (-fpack-struct=1, like Watcom
**	/zp1; ADR-0014). GCC applies that to the C/C++ runtime headers as well, which would
**	give their structs a different layout from the compiled runtime library they
**	describe. So the runtime headers the code uses are included here first, with the
**	natural packing restored; their include guards make every later #include of them a
**	no-op. (MSVC is different: the Windows SDK and UCRT headers push their own packing.)
*/
#if !defined(_MSC_VER) && defined(__cplusplus)
#  if defined(RA_PACK_STRUCTS)
#    pragma pack(push, 8)
#  endif
#  include <stddef.h>
#  include <stdarg.h>
#  include <stdio.h>
/*
**	glibc declares "long random(void)"; WIN32LIB/INCLUDE/MISC.H declares its own
**	extern "C" "unsigned long random(unsigned long)" (DETPROC.ASM). Two C functions
**	cannot share a name, so glibc's is declared under another name. The game never
**	calls the C library's random().
*/
#  if !defined(__MINGW32__)
#    define random ra_glibc_random
#  endif
#  include <stdlib.h>
#  if !defined(__MINGW32__)
#    undef random
#  endif
#  include <string.h>
#  include <ctype.h>
#  include <errno.h>
#  include <limits.h>
#  include <time.h>
#  include <math.h>
#  include <assert.h>
#  include <fcntl.h>
#  include <signal.h>
#  include <setjmp.h>
#  include <sys/types.h>
#  include <sys/stat.h>
#  include <new>
#  if !defined(__MINGW32__)
#    include <strings.h>
#    include <unistd.h>
#    include <dirent.h>
#    include <sys/time.h>
#  else
#    include <io.h>
#    include <direct.h>
#    include <malloc.h>
#  endif
/*
**	SDL3 (S08/S09) has the same problem: its headers assert their natural struct
**	layout (SDL_alignment_test) and fail under -fpack-struct=1. So when the SDL3 backend
**	is enabled, SDL is pre-included here too, under natural packing. MSVC (/Zp1) has the
**	same problem, because SDL_begin_code.h's packing push comes after that assert:
**	audio_sdl3.cpp therefore includes SDL inside its own #pragma pack(push, 8).
*/
#  if defined(RA_HAVE_SDL3) && defined(__has_include)
#    if __has_include(<SDL3/SDL.h>)
#      include <SDL3/SDL.h>
#    endif
#  endif
#  if defined(RA_PACK_STRUCTS)
#    pragma pack(pop)
#  endif
#endif

/*
**	glibc's <endian.h> (pulled in by <stdlib.h> above) defines BIG_ENDIAN and
**	LITTLE_ENDIAN as the constants 4321/1234 on every machine. The game uses
**	"#ifdef BIG_ENDIAN" to mean "this is a big-endian machine" (DEFINES.H, FIXED.H,
**	BASE64.CPP), so those names must not be defined on a little-endian host. glibc
**	itself only uses the __BIG_ENDIAN spellings. CODE/wire_layout_check.cpp has an
**	#error that catches a reappearance.
*/
#if !defined(_MSC_VER) && !defined(__MINGW32__)
#  undef BIG_ENDIAN
#  undef LITTLE_ENDIAN
#  undef PDP_ENDIAN
#  undef BYTE_ORDER
#endif

/*
**	Watcom's <stdio.h> defined the FILE flag bits _READ (1) and _WRITE (2), and
**	WIN32LIB/INCLUDE/WWFILE.H uses them as the file access rights READ/WRITE (COMPAT.H
**	gives READ and WRITE the same values). Every host.
*/
#ifndef _READ
#  define _READ		0x0001
#endif
#ifndef _WRITE
#  define _WRITE	0x0002
#endif

/*
**	4. Watcom/Microsoft C runtime extensions that the POSIX C library lacks
**	(strcmpi, strupr, itoa, _MAX_PATH, _makepath, ...). Linux and other non-Windows
**	hosts only; MSVC and MinGW have the real ones.
*/
#if !defined(_MSC_VER) && !defined(__MINGW32__) && defined(__cplusplus)
#  include "ra_crt_compat.h"
#endif

#endif /* RA_COMPAT_H */
