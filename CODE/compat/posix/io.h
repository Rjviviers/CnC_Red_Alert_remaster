/*
**	Command & Conquer Red Alert(tm) - multiplayer port. GPLv3 or later, see LICENSE.md.
**	Compatibility shim (session S06a, ADR-0014). See docs/codebase/compat-notes.md.
**	Non-Windows hosts only: this directory is searched after the system headers
**	(-idirafter), so a real header of the same name always wins.
**
**	<io.h> (Microsoft/Watcom low-level file I/O) on top of POSIX.
*/
#ifndef RA_COMPAT_IO_H
#define RA_COMPAT_IO_H
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#ifndef O_BINARY
#define O_BINARY	0
#endif
#ifndef O_TEXT
#define O_TEXT		0
#endif
#ifndef _O_BINARY
#define _O_BINARY	O_BINARY
#define _O_TEXT		O_TEXT
#define _O_RDONLY	O_RDONLY
#define _O_WRONLY	O_WRONLY
#define _O_RDWR		O_RDWR
#define _O_CREAT	O_CREAT
#define _O_TRUNC	O_TRUNC
#define _O_EXCL		O_EXCL
#define _O_APPEND	O_APPEND
#endif
#ifndef S_IREAD
#define S_IREAD		S_IRUSR
#define S_IWRITE	S_IWUSR
#endif
inline long tell(int fd) {return (long)lseek(fd, 0, SEEK_CUR);}
inline long filelength(int fd)
{
	struct stat st;
	if (fstat(fd, &st) != 0) return -1L;
	return (long)st.st_size;
}
inline int eof(int fd)
{
	long len = filelength(fd);
	if (len < 0) return -1;
	return tell(fd) >= len ? 1 : 0;
}
inline int chsize(int fd, long size) {return ftruncate(fd, (off_t)size);}
inline int _open(char const * name, int flags, int mode = 0666) {return open(name, flags, mode);}
inline int _close(int fd) {return close(fd);}
inline int _read(int fd, void * buf, unsigned n) {return (int)read(fd, buf, n);}
inline int _write(int fd, void const * buf, unsigned n) {return (int)write(fd, buf, n);}
inline long _lseek(int fd, long offset, int whence) {return (long)lseek(fd, (off_t)offset, whence);}
inline long _tell(int fd) {return tell(fd);}
inline long _filelength(int fd) {return filelength(fd);}
inline int _access(char const * name, int mode) {return access(name, mode);}
inline int _unlink(char const * name) {return unlink(name);}
inline int _chsize(int fd, long size) {return chsize(fd, size);}
inline int _eof(int fd) {return eof(fd);}
#endif
