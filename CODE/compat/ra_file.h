/*
**	Command & Conquer Red Alert(tm) - multiplayer port. GPLv3 or later, see LICENSE.md.
**	Portable low-level file I/O (session S10b, ADR-0022). See docs/codebase/compat-notes.md, "S10b notes".
**
**	RawFileClass (CODE/RAWFILE.CPP) used the Win32 file API (CreateFile, ReadFile,
**	SetFilePointer, ...). It now uses these functions on every host, so there is one
**	code path. They are thin wrappers over the C runtime's numbered file descriptors
**	(POSIX open/read/write/lseek; on Windows the CRT's _open/_read/_write/_lseek), which
**	behave like the Win32 handles did: unbuffered, positioned, readable and writable on
**	the same handle without the stdio "seek between read and write" rule.
**
**	File names: the game spells its data files in upper case ("REDALERT.MIX") and uses
**	DOS separators ("MOVIES\\X.MPG", "C:\\WESTWOOD\\..."). On case-sensitive file systems
**	RA_Resolve_Path turns such a name into the real path on disk:
**	  - '\\' becomes '/';
**	  - a leading drive letter ("C:") is dropped (there are no drives; the rest of the
**	    path is used as is, so "C:\\X" means "/X" and "D:X" means "X");
**	  - every path component that does not exist with the exact spelling is matched
**	    case-insensitively against the directory's entries (first match in name order,
**	    so the result does not depend on readdir order).
**	On Windows names are passed through unchanged (the OS is already case-insensitive
**	and accepts both separators); with must_exist, a missing name still returns false.
**
**	Handles are ints, -1 when closed (the value RawFileClass used in the DOS build).
*/

#ifndef RA_FILE_H
#define RA_FILE_H

#include <stddef.h>

/* Access rights, the same bits as WWFILE.H READ/WRITE. */
#define RA_FILE_READ	1
#define RA_FILE_WRITE	2

/*
**	Resolve a game file name into a host path (see above). With must_exist false the
**	last component may be missing (a file about to be created): it is kept as spelled,
**	unless an entry matching it case-insensitively already exists. Returns false if the
**	name cannot be resolved or does not fit in out_size.
*/
bool RA_Resolve_Path(char const * name, char * out, size_t out_size, bool must_exist);

/*
**	Open a file. RA_FILE_READ: existing file, read only. RA_FILE_WRITE: create or
**	truncate, write only. RA_FILE_READ|RA_FILE_WRITE: create or truncate, read/write
**	(the original used CREATE_ALWAYS for both). Returns the handle or -1 (errno set).
*/
int RA_File_Open(char const * name, int rights);
bool RA_File_Close(int handle);

/* Return the bytes transferred, or -1 on error (errno set). 0 from read = end of file. */
long RA_File_Read(int handle, void * buffer, long size);
long RA_File_Write(int handle, void const * buffer, long size);

/* whence is SEEK_SET/SEEK_CUR/SEEK_END. Returns the new position, or -1 on error. */
long RA_File_Seek(int handle, long pos, int whence);

/* Size of an open file in bytes, or -1 on error. */
long RA_File_Size(int handle);

/* Delete a file by name. */
bool RA_File_Delete(char const * name);

/* Does a regular file with this name exist (case-insensitively)? */
bool RA_File_Exists(char const * name);

/*
**	Last-write time as DOS date/time ((date << 16) | time), as FileTimeToDosDateTime
**	gave it from the file's UTC FILETIME (no time zone conversion). 0 on error.
*/
unsigned long RA_File_Get_Dos_Date_Time(int handle);
bool RA_File_Set_Dos_Date_Time(int handle, unsigned long datetime);

/* Conversions used by the two functions above (exposed for the unit test). */
unsigned long RA_Dos_Date_Time_From_Unix(long long seconds);
long long RA_Unix_From_Dos_Date_Time(unsigned long datetime);

#endif
