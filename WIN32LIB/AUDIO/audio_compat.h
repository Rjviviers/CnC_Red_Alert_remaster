/*
**	Command & Conquer Red Alert(tm)
**	Copyright 2025 Electronic Arts Inc.
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
** Session S09: lets the audio library (SOUNDIO.CPP, SOUNDINT.CPP,
** SOUNDLCK.CPP) and its tests include audio.h WITHOUT windows.h, dsound.h or
** the Watcom-era wwstd.h, so they build on every platform today.
**
** Include this BEFORE audio.h. Only the audio library's own files and tests
** include it; the game keeps getting these names from windows.h and wwstd.h.
**
** What audio.h needs from its includers:
**   BOOL, TRUE, FALSE   wwstd.h: "#define BOOL int" etc. (identical definitions below)
**   HWND    windows.h. Off Windows we declare it the way windows.h does
**           (DECLARE_HANDLE with STRICT), so Audio_Init has the same
**           signature as the game's call. S08's platform layer must declare
**           HWND the same way if it provides one (see docs/codebase/audio.md).
**   far, __cdecl   empty off Windows.
** audio.h starts with #include "wwstd.h"; WWSTD_H is predefined here so that
** include is skipped (the only thing audio.h used from it is BOOL).
*/
#ifndef AUDIO_COMPAT_H
#define AUDIO_COMPAT_H

#if defined(_MSC_VER) || defined(__MINGW32__)
#  ifndef WIN32_LEAN_AND_MEAN
#    define WIN32_LEAN_AND_MEAN
#  endif
#  ifndef NOMINMAX
#    define NOMINMAX
#  endif
#  include <windows.h>
#else
struct HWND__;
typedef struct HWND__ *HWND;
#endif

#ifndef WWSTD_H
#define WWSTD_H
#endif

#ifndef BOOL
#define BOOL int
#endif

#ifndef TRUE
#define TRUE		1
#endif

#ifndef FALSE
#define FALSE		0
#endif

#ifndef far
#define far
#endif

#if !defined(_MSC_VER) && !defined(__MINGW32__) && !defined(__CYGWIN__) && !defined(__cdecl)
#define __cdecl
#endif

#ifndef WW_ERROR
#define WW_ERROR		-1
#endif

#ifndef READ
#define READ			1
#endif

/*
** File functions the audio library uses. The game defines them in
** CODE/CCFILE.CPP (prototypes from WIN32LIB/INCLUDE/FILE.H); tests define
** their own. Only the game thread calls them.
*/
int __cdecl Open_File(char const *file_name, int mode);
void __cdecl Close_File(int handle);
long __cdecl Read_File(int handle, void *buf, unsigned long bytes);
int __cdecl Find_File(char const *file_name);
unsigned long __cdecl File_Size(int handle);

#endif
