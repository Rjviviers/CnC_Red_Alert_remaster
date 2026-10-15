/*
**	Command & Conquer Red Alert(tm) - multiplayer port. GPLv3 or later, see LICENSE.md.
**	Minimal Win32 declarations for non-Windows hosts (session S06a, ADR-0014).
**	See docs/codebase/compat-notes.md.
**
**	The 1996 code includes <windows.h> from almost every file. Until session S08 replaces
**	the Win32 code with the SDL platform layer, Linux builds use this header so that the
**	game code COMPILES. It declares only the types, constants and functions that the code
**	names, with the sizes and values of the real 32-bit Windows SDK. Functions are
**	declared, never defined: anything still calling them at link time (S10) must be
**	replaced or removed by S08/S09/S12, which is the point.
**
**	This directory is only on the include path on non-Windows hosts (-idirafter), so it
**	never shadows the real Windows SDK. Add declarations here in the same style when a
**	file needs one; keep values identical to the SDK.
*/

#ifndef RA_COMPAT_WINDOWS_H
#define RA_COMPAT_WINDOWS_H

#if defined(_MSC_VER) || defined(__MINGW32__)
#error "CODE/compat/win32 must not be used with a real Windows SDK"
#endif

#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <stdarg.h>

/*
**	Basic types. 32-bit Windows is ILP32: long is 4 bytes. The game is built for 32-bit
**	x86 (ADR-0003), where Linux long is 4 bytes too, so the SDK's own definitions are used
**	(DWORD is unsigned long, as in the SDK; code mixes the two freely).
*/
#define far
#define near
#define FAR
#define NEAR
#define PASCAL
#define WINAPI
#define APIENTRY
#define CALLBACK
#define WINAPIV
#define CONST const
#define VOID void
#ifndef IN
#define IN
#define OUT
#define OPTIONAL
#endif

#ifndef BOOL
typedef int BOOL;		/* WWSTD.H may already have #defined BOOL as int */
#endif
typedef unsigned char BYTE;
typedef unsigned short WORD;
typedef unsigned long DWORD;
typedef unsigned int UINT;
typedef int INT;
typedef long LONG;
typedef unsigned long ULONG;
typedef short SHORT;
typedef unsigned short USHORT;
typedef char CHAR;
typedef unsigned char UCHAR;
typedef wchar_t WCHAR;
typedef float FLOAT;
typedef BYTE BOOLEAN;
typedef long long LONGLONG;
typedef unsigned long long ULONGLONG;
typedef long long __int64;
typedef DWORD * LPDWORD;
typedef DWORD * PDWORD;
typedef WORD * LPWORD;
typedef BYTE * LPBYTE;
typedef BYTE * PBYTE;
typedef BOOL * LPBOOL;
typedef LONG * LPLONG;
typedef LONG * PLONG;
typedef int * LPINT;
typedef UINT * PUINT;
typedef void * LPVOID;
typedef void * PVOID;
typedef void const * LPCVOID;
typedef char * LPSTR;
typedef char * PSTR;
typedef char * PCHAR;
typedef char const * LPCSTR;
typedef char const * PCSTR;
typedef char TCHAR;
typedef char * LPTSTR;
typedef char const * LPCTSTR;
typedef WCHAR * LPWSTR;
typedef WCHAR const * LPCWSTR;
typedef intptr_t INT_PTR;
typedef uintptr_t UINT_PTR;
typedef intptr_t LONG_PTR;
typedef uintptr_t ULONG_PTR;
typedef ULONG_PTR DWORD_PTR;
typedef ULONG_PTR SIZE_T;
typedef UINT_PTR WPARAM;
typedef LONG_PTR LPARAM;
typedef LONG_PTR LRESULT;
typedef LONG HRESULT;
typedef WORD ATOM;
typedef DWORD COLORREF;
typedef DWORD * LPCOLORREF;

#ifndef TEXT
#define TEXT(s) s
#define _T(s) s
#endif

#define DECLARE_HANDLE(name) struct name##__ {int unused;}; typedef struct name##__ * name
typedef void * HANDLE;
typedef HANDLE * LPHANDLE;
typedef HANDLE * PHANDLE;
DECLARE_HANDLE(HWND);
DECLARE_HANDLE(HINSTANCE);
DECLARE_HANDLE(HDC);
DECLARE_HANDLE(HGLRC);
DECLARE_HANDLE(HMENU);
DECLARE_HANDLE(HICON);
DECLARE_HANDLE(HKEY);
DECLARE_HANDLE(HPALETTE);
DECLARE_HANDLE(HBITMAP);
DECLARE_HANDLE(HBRUSH);
DECLARE_HANDLE(HFONT);
DECLARE_HANDLE(HPEN);
DECLARE_HANDLE(HRGN);
DECLARE_HANDLE(HMONITOR);
DECLARE_HANDLE(HTASK);
DECLARE_HANDLE(HMMIO);
DECLARE_HANDLE(HWAVEOUT);
DECLARE_HANDLE(HMIDIOUT);
typedef HICON HCURSOR;
typedef HINSTANCE HMODULE;
typedef HANDLE HGLOBAL;
typedef HANDLE HLOCAL;
typedef HANDLE HGDIOBJ;
typedef HKEY * PHKEY;
typedef HANDLE HFILE;
typedef DWORD LCID;
typedef DWORD ACCESS_MASK;
typedef ACCESS_MASK REGSAM;

typedef LRESULT (CALLBACK * WNDPROC)(HWND, UINT, WPARAM, LPARAM);
typedef INT_PTR (CALLBACK * DLGPROC)(HWND, UINT, WPARAM, LPARAM);
typedef INT_PTR (WINAPI * FARPROC)();
typedef void (CALLBACK * TIMERPROC)(HWND, UINT, UINT_PTR, DWORD);
typedef DWORD (WINAPI * LPTHREAD_START_ROUTINE)(LPVOID);

#ifndef FALSE
#define FALSE 0
#endif
#ifndef TRUE
#define TRUE 1
#endif
#ifndef NULL
#define NULL 0
#endif

#define MAX_PATH 260
#define INFINITE 0xFFFFFFFF
#define INVALID_HANDLE_VALUE ((HANDLE)(LONG_PTR)-1)
#define HFILE_ERROR ((HFILE)-1)

#define MAKEWORD(a, b)	((WORD)(((BYTE)((DWORD_PTR)(a) & 0xff)) | ((WORD)((BYTE)((DWORD_PTR)(b) & 0xff))) << 8))
#define MAKELONG(a, b)	((LONG)(((WORD)((DWORD_PTR)(a) & 0xffff)) | ((DWORD)((WORD)((DWORD_PTR)(b) & 0xffff))) << 16))
#define LOWORD(l)		((WORD)((DWORD_PTR)(l) & 0xffff))
#define HIWORD(l)		((WORD)((DWORD_PTR)(l) >> 16))
#define LOBYTE(w)		((BYTE)((DWORD_PTR)(w) & 0xff))
#define HIBYTE(w)		((BYTE)((DWORD_PTR)(w) >> 8))
#define MAKELPARAM(l, h)	((LPARAM)(DWORD)MAKELONG(l, h))
#define MAKEWPARAM(l, h)	((WPARAM)(DWORD)MAKELONG(l, h))
#define MAKELRESULT(l, h)	((LRESULT)(DWORD)MAKELONG(l, h))
#define RGB(r,g,b)		((COLORREF)(((BYTE)(r)|((WORD)((BYTE)(g))<<8))|(((DWORD)(BYTE)(b))<<16)))
#define MAKEINTRESOURCE(i) ((LPSTR)((ULONG_PTR)((WORD)(i))))

#ifndef NOMINMAX
#ifndef max
#define max(a,b)	(((a) > (b)) ? (a) : (b))
#endif
#ifndef min
#define min(a,b)	(((a) < (b)) ? (a) : (b))
#endif
#endif

/* HRESULT helpers. */
#define S_OK			((HRESULT)0L)
#define S_FALSE			((HRESULT)1L)
#define E_FAIL			((HRESULT)0x80004005L)
#define E_NOINTERFACE	((HRESULT)0x80004002L)
#define E_OUTOFMEMORY	((HRESULT)0x8007000EL)
#define E_INVALIDARG	((HRESULT)0x80070057L)
#define E_POINTER		((HRESULT)0x80004003L)
#define SUCCEEDED(hr)	(((HRESULT)(hr)) >= 0)
#define FAILED(hr)		(((HRESULT)(hr)) < 0)
#define MAKE_HRESULT(sev,fac,code) ((HRESULT)(((unsigned long)(sev)<<31) | ((unsigned long)(fac)<<16) | ((unsigned long)(code))))

/*
**	Structures (all have the SDK layout; the SDK uses natural alignment, so these are
**	wrapped in pack(push, 8) to stay independent of the game's -fpack-struct=1).
*/
#pragma pack(push, 8)

typedef struct tagPOINT {LONG x; LONG y;} POINT, *PPOINT, *LPPOINT;
typedef struct tagSIZE {LONG cx; LONG cy;} SIZE, *PSIZE, *LPSIZE;
typedef struct tagRECT {LONG left; LONG top; LONG right; LONG bottom;} RECT, *PRECT, *LPRECT;
typedef RECT const * LPCRECT;

typedef struct tagMSG {
	HWND hwnd;
	UINT message;
	WPARAM wParam;
	LPARAM lParam;
	DWORD time;
	POINT pt;
} MSG, *PMSG, *LPMSG;

typedef struct tagWNDCLASSA {
	UINT style;
	WNDPROC lpfnWndProc;
	int cbClsExtra;
	int cbWndExtra;
	HINSTANCE hInstance;
	HICON hIcon;
	HCURSOR hCursor;
	HBRUSH hbrBackground;
	LPCSTR lpszMenuName;
	LPCSTR lpszClassName;
} WNDCLASS, WNDCLASSA, *PWNDCLASS, *LPWNDCLASS;

typedef struct tagPAINTSTRUCT {
	HDC hdc;
	BOOL fErase;
	RECT rcPaint;
	BOOL fRestore;
	BOOL fIncUpdate;
	BYTE rgbReserved[32];
} PAINTSTRUCT, *LPPAINTSTRUCT;

typedef struct tagPALETTEENTRY {BYTE peRed; BYTE peGreen; BYTE peBlue; BYTE peFlags;} PALETTEENTRY, *PPALETTEENTRY, *LPPALETTEENTRY;
typedef struct tagLOGPALETTE {WORD palVersion; WORD palNumEntries; PALETTEENTRY palPalEntry[1];} LOGPALETTE, *LPLOGPALETTE;
typedef struct tagRGBQUAD {BYTE rgbBlue; BYTE rgbGreen; BYTE rgbRed; BYTE rgbReserved;} RGBQUAD, *LPRGBQUAD;

typedef struct tagBITMAPINFOHEADER {
	DWORD biSize;
	LONG biWidth;
	LONG biHeight;
	WORD biPlanes;
	WORD biBitCount;
	DWORD biCompression;
	DWORD biSizeImage;
	LONG biXPelsPerMeter;
	LONG biYPelsPerMeter;
	DWORD biClrUsed;
	DWORD biClrImportant;
} BITMAPINFOHEADER, *LPBITMAPINFOHEADER, *PBITMAPINFOHEADER;
typedef struct tagBITMAPINFO {BITMAPINFOHEADER bmiHeader; RGBQUAD bmiColors[1];} BITMAPINFO, *LPBITMAPINFO;

typedef struct _FILETIME {DWORD dwLowDateTime; DWORD dwHighDateTime;} FILETIME, *PFILETIME, *LPFILETIME;
typedef struct _SYSTEMTIME {
	WORD wYear; WORD wMonth; WORD wDayOfWeek; WORD wDay;
	WORD wHour; WORD wMinute; WORD wSecond; WORD wMilliseconds;
} SYSTEMTIME, *PSYSTEMTIME, *LPSYSTEMTIME;

typedef union _LARGE_INTEGER {
	struct {DWORD LowPart; LONG HighPart;} u;
	struct {DWORD LowPart; LONG HighPart;};
	LONGLONG QuadPart;
} LARGE_INTEGER, *PLARGE_INTEGER;

typedef struct _SECURITY_ATTRIBUTES {DWORD nLength; LPVOID lpSecurityDescriptor; BOOL bInheritHandle;} SECURITY_ATTRIBUTES, *LPSECURITY_ATTRIBUTES;

typedef struct _OVERLAPPED {
	ULONG_PTR Internal;
	ULONG_PTR InternalHigh;
	DWORD Offset;
	DWORD OffsetHigh;
	HANDLE hEvent;
} OVERLAPPED, *LPOVERLAPPED;

typedef struct _WIN32_FIND_DATAA {
	DWORD dwFileAttributes;
	FILETIME ftCreationTime;
	FILETIME ftLastAccessTime;
	FILETIME ftLastWriteTime;
	DWORD nFileSizeHigh;
	DWORD nFileSizeLow;
	DWORD dwReserved0;
	DWORD dwReserved1;
	CHAR cFileName[MAX_PATH];
	CHAR cAlternateFileName[14];
} WIN32_FIND_DATA, WIN32_FIND_DATAA, *LPWIN32_FIND_DATA;

typedef struct _MEMORYSTATUS {
	DWORD dwLength;
	DWORD dwMemoryLoad;
	SIZE_T dwTotalPhys;
	SIZE_T dwAvailPhys;
	SIZE_T dwTotalPageFile;
	SIZE_T dwAvailPageFile;
	SIZE_T dwTotalVirtual;
	SIZE_T dwAvailVirtual;
} MEMORYSTATUS, *LPMEMORYSTATUS;

typedef struct _OSVERSIONINFOA {
	DWORD dwOSVersionInfoSize;
	DWORD dwMajorVersion;
	DWORD dwMinorVersion;
	DWORD dwBuildNumber;
	DWORD dwPlatformId;
	CHAR szCSDVersion[128];
} OSVERSIONINFO, OSVERSIONINFOA, *LPOSVERSIONINFO;

typedef struct _SYSTEM_INFO {
	union {DWORD dwOemId; struct {WORD wProcessorArchitecture; WORD wReserved;};};
	DWORD dwPageSize;
	LPVOID lpMinimumApplicationAddress;
	LPVOID lpMaximumApplicationAddress;
	DWORD_PTR dwActiveProcessorMask;
	DWORD dwNumberOfProcessors;
	DWORD dwProcessorType;
	DWORD dwAllocationGranularity;
	WORD wProcessorLevel;
	WORD wProcessorRevision;
} SYSTEM_INFO, *LPSYSTEM_INFO;

typedef struct _CRITICAL_SECTION {
	void * DebugInfo;
	LONG LockCount;
	LONG RecursionCount;
	HANDLE OwningThread;
	HANDLE LockSemaphore;
	ULONG_PTR SpinCount;
} CRITICAL_SECTION, *LPCRITICAL_SECTION;

typedef struct _GUID {
	unsigned long Data1;
	unsigned short Data2;
	unsigned short Data3;
	unsigned char Data4[8];
} GUID, IID, CLSID, *LPGUID, *LPIID, *LPCLSID;
#ifdef __cplusplus
#define REFGUID const GUID &
#define REFIID const IID &
#define REFCLSID const CLSID &
#else
#define REFGUID const GUID *
#define REFIID const IID *
#define REFCLSID const CLSID *
#endif
#define DEFINE_GUID(name, l, w1, w2, b1, b2, b3, b4, b5, b6, b7, b8) extern const GUID name

typedef struct _COORD {SHORT X; SHORT Y;} COORD;

#pragma pack(pop)

/*
**	COM. Interfaces are declared with DECLARE_INTERFACE_ the way the SDK does in C++.
*/
#define interface struct
#define STDMETHODCALLTYPE
#define STDMETHOD(method)		virtual HRESULT STDMETHODCALLTYPE method
#define STDMETHOD_(type,method)	virtual type STDMETHODCALLTYPE method
#define STDMETHODIMP			HRESULT STDMETHODCALLTYPE
#define STDMETHODIMP_(type)		type STDMETHODCALLTYPE
#define PURE					= 0
#define THIS_
#define THIS					void
#define DECLARE_INTERFACE(iface)			interface iface
#define DECLARE_INTERFACE_(iface, baseiface)	interface iface : public baseiface

DECLARE_INTERFACE(IUnknown)
{
	STDMETHOD(QueryInterface)(THIS_ REFIID riid, LPVOID * ppvObj) PURE;
	STDMETHOD_(ULONG,AddRef)(THIS) PURE;
	STDMETHOD_(ULONG,Release)(THIS) PURE;
};
typedef IUnknown * LPUNKNOWN;

/*
**	Window messages, styles and other constants (SDK values).
*/
#define WM_NULL				0x0000
#define WM_CREATE			0x0001
#define WM_DESTROY			0x0002
#define WM_MOVE				0x0003
#define WM_SIZE				0x0005
#define WM_ACTIVATE			0x0006
#define WM_SETFOCUS			0x0007
#define WM_KILLFOCUS		0x0008
#define WM_ENABLE			0x000A
#define WM_PAINT			0x000F
#define WM_CLOSE			0x0010
#define WM_QUIT				0x0012
#define WM_ERASEBKGND		0x0014
#define WM_SHOWWINDOW		0x0018
#define WM_ACTIVATEAPP		0x001C
#define WM_SETCURSOR		0x0020
#define WM_MOUSEACTIVATE	0x0021
#define WM_GETMINMAXINFO	0x0024
#define WM_WINDOWPOSCHANGING 0x0046
#define WM_WINDOWPOSCHANGED	0x0047
#define WM_NCCREATE			0x0081
#define WM_NCDESTROY		0x0082
#define WM_NCHITTEST		0x0084
#define WM_NCPAINT			0x0085
#define WM_NCACTIVATE		0x0086
#define WM_KEYFIRST			0x0100
#define WM_KEYDOWN			0x0100
#define WM_KEYUP			0x0101
#define WM_CHAR				0x0102
#define WM_DEADCHAR			0x0103
#define WM_SYSKEYDOWN		0x0104
#define WM_SYSKEYUP			0x0105
#define WM_SYSCHAR			0x0106
#define WM_KEYLAST			0x0108
#define WM_INITDIALOG		0x0110
#define WM_COMMAND			0x0111
#define WM_SYSCOMMAND		0x0112
#define WM_TIMER			0x0113
#define WM_MOUSEFIRST		0x0200
#define WM_MOUSEMOVE		0x0200
#define WM_LBUTTONDOWN		0x0201
#define WM_LBUTTONUP		0x0202
#define WM_LBUTTONDBLCLK	0x0203
#define WM_RBUTTONDOWN		0x0204
#define WM_RBUTTONUP		0x0205
#define WM_RBUTTONDBLCLK	0x0206
#define WM_MBUTTONDOWN		0x0207
#define WM_MBUTTONUP		0x0208
#define WM_MBUTTONDBLCLK	0x0209
#define WM_MOUSEWHEEL		0x020A
#define WM_MOUSELAST		0x020A
#define WM_USER				0x0400

#define SC_SCREENSAVE		0xF140
#define SC_MONITORPOWER		0xF170
#define SC_KEYMENU			0xF100
#define SC_CLOSE			0xF060

#define WA_INACTIVE			0
#define WA_ACTIVE			1
#define WA_CLICKACTIVE		2

#define MK_LBUTTON			0x0001
#define MK_RBUTTON			0x0002
#define MK_SHIFT			0x0004
#define MK_CONTROL			0x0008
#define MK_MBUTTON			0x0010

#define CS_VREDRAW			0x0001
#define CS_HREDRAW			0x0002
#define CS_DBLCLKS			0x0008
#define CS_OWNDC			0x0020

#define WS_OVERLAPPED		0x00000000L
#define WS_POPUP			0x80000000L
#define WS_CHILD			0x40000000L
#define WS_MINIMIZE			0x20000000L
#define WS_VISIBLE			0x10000000L
#define WS_DISABLED			0x08000000L
#define WS_CLIPSIBLINGS		0x04000000L
#define WS_CLIPCHILDREN		0x02000000L
#define WS_MAXIMIZE			0x01000000L
#define WS_CAPTION			0x00C00000L
#define WS_BORDER			0x00800000L
#define WS_DLGFRAME			0x00400000L
#define WS_VSCROLL			0x00200000L
#define WS_HSCROLL			0x00100000L
#define WS_SYSMENU			0x00080000L
#define WS_THICKFRAME		0x00040000L
#define WS_MINIMIZEBOX		0x00020000L
#define WS_MAXIMIZEBOX		0x00010000L
#define WS_OVERLAPPEDWINDOW	(WS_OVERLAPPED|WS_CAPTION|WS_SYSMENU|WS_THICKFRAME|WS_MINIMIZEBOX|WS_MAXIMIZEBOX)
#define WS_EX_TOPMOST		0x00000008L
#define WS_EX_APPWINDOW		0x00040000L

#define CW_USEDEFAULT		((int)0x80000000)

#define SW_HIDE				0
#define SW_SHOWNORMAL		1
#define SW_NORMAL			1
#define SW_SHOWMINIMIZED	2
#define SW_SHOWMAXIMIZED	3
#define SW_MAXIMIZE			3
#define SW_SHOWNOACTIVATE	4
#define SW_SHOW				5
#define SW_MINIMIZE			6
#define SW_SHOWMINNOACTIVE	7
#define SW_SHOWNA			8
#define SW_RESTORE			9
#define SW_SHOWDEFAULT		10

#define PM_NOREMOVE			0x0000
#define PM_REMOVE			0x0001
#define PM_NOYIELD			0x0002

#define MB_OK				0x00000000L
#define MB_OKCANCEL			0x00000001L
#define MB_ABORTRETRYIGNORE	0x00000002L
#define MB_YESNOCANCEL		0x00000003L
#define MB_YESNO			0x00000004L
#define MB_RETRYCANCEL		0x00000005L
#define MB_ICONHAND			0x00000010L
#define MB_ICONQUESTION		0x00000020L
#define MB_ICONEXCLAMATION	0x00000030L
#define MB_ICONASTERISK		0x00000040L
#define MB_ICONWARNING		MB_ICONEXCLAMATION
#define MB_ICONERROR		MB_ICONHAND
#define MB_ICONINFORMATION	MB_ICONASTERISK
#define MB_ICONSTOP			MB_ICONHAND
#define MB_DEFBUTTON1		0x00000000L
#define MB_DEFBUTTON2		0x00000100L
#define MB_APPLMODAL		0x00000000L
#define MB_SYSTEMMODAL		0x00001000L
#define MB_TASKMODAL		0x00002000L
#define MB_SETFOREGROUND	0x00010000L
#define MB_TOPMOST			0x00040000L

#define IDOK				1
#define IDCANCEL			2
#define IDABORT				3
#define IDRETRY				4
#define IDIGNORE			5
#define IDYES				6
#define IDNO				7

#define IDC_ARROW			MAKEINTRESOURCE(32512)
#define IDC_WAIT			MAKEINTRESOURCE(32514)
#define IDI_APPLICATION		MAKEINTRESOURCE(32512)

#define GWL_WNDPROC			(-4)
#define GWL_STYLE			(-16)
#define GWL_EXSTYLE			(-20)
#define GWL_USERDATA		(-21)

#define SWP_NOSIZE			0x0001
#define SWP_NOMOVE			0x0002
#define SWP_NOZORDER		0x0004
#define SWP_NOACTIVATE		0x0010
#define SWP_SHOWWINDOW		0x0040
#define HWND_TOP			((HWND)0)
#define HWND_TOPMOST		((HWND)-1)
#define HWND_NOTOPMOST		((HWND)-2)
#define HWND_BROADCAST		((HWND)0xffff)

#define SM_CXSCREEN			0
#define SM_CYSCREEN			1

#define COLOR_WINDOW		5
#define BLACK_BRUSH			4
#define NULL_BRUSH			5
#define WHITE_BRUSH			0
#define BLACK_PEN			7
#define WHITE_PEN			6
#define TRANSPARENT			1
#define OPAQUE				2
#define SRCCOPY				(DWORD)0x00CC0020

#define PC_RESERVED			0x01
#define PC_EXPLICIT			0x02
#define PC_NOCOLLAPSE		0x04
#define DIB_RGB_COLORS		0
#define DIB_PAL_COLORS		1
#define BI_RGB				0L

#define VER_PLATFORM_WIN32s			0
#define VER_PLATFORM_WIN32_WINDOWS	1
#define VER_PLATFORM_WIN32_NT		2

#define GMEM_FIXED			0x0000
#define GMEM_MOVEABLE		0x0002
#define GMEM_ZEROINIT		0x0040
#define GMEM_SHARE			0x2000
#define GMEM_DDESHARE		0x2000
#define GPTR				(GMEM_FIXED | GMEM_ZEROINIT)
#define GHND				(GMEM_MOVEABLE | GMEM_ZEROINIT)
#define LMEM_FIXED			0x0000
#define LMEM_ZEROINIT		0x0040
#define LPTR				(LMEM_FIXED | LMEM_ZEROINIT)

#define MEM_COMMIT			0x00001000
#define MEM_RESERVE			0x00002000
#define MEM_DECOMMIT		0x00004000
#define MEM_RELEASE			0x00008000
#define PAGE_NOACCESS		0x01
#define PAGE_READONLY		0x02
#define PAGE_READWRITE		0x04
#define PAGE_EXECUTE_READWRITE 0x40

#define GENERIC_READ		0x80000000L
#define GENERIC_WRITE		0x40000000L
#define FILE_SHARE_READ		0x00000001
#define FILE_SHARE_WRITE	0x00000002
#define CREATE_NEW			1
#define CREATE_ALWAYS		2
#define OPEN_EXISTING		3
#define OPEN_ALWAYS			4
#define TRUNCATE_EXISTING	5
#define FILE_ATTRIBUTE_READONLY		0x00000001
#define FILE_ATTRIBUTE_HIDDEN		0x00000002
#define FILE_ATTRIBUTE_SYSTEM		0x00000004
#define FILE_ATTRIBUTE_DIRECTORY	0x00000010
#define FILE_ATTRIBUTE_ARCHIVE		0x00000020
#define FILE_ATTRIBUTE_NORMAL		0x00000080
#define FILE_FLAG_WRITE_THROUGH		0x80000000
#define FILE_FLAG_OVERLAPPED		0x40000000
#define FILE_FLAG_NO_BUFFERING		0x20000000
#define FILE_FLAG_RANDOM_ACCESS		0x10000000
#define FILE_FLAG_SEQUENTIAL_SCAN	0x08000000
#define FILE_BEGIN			0
#define FILE_CURRENT		1
#define FILE_END			2
#define INVALID_FILE_SIZE	((DWORD)0xFFFFFFFF)
#define INVALID_SET_FILE_POINTER ((DWORD)-1)

#define DRIVE_UNKNOWN		0
#define DRIVE_NO_ROOT_DIR	1
#define DRIVE_REMOVABLE		2
#define DRIVE_FIXED			3
#define DRIVE_REMOTE		4
#define DRIVE_CDROM			5
#define DRIVE_RAMDISK		6

#define ERROR_SUCCESS			0L
#define NO_ERROR				0L
#define ERROR_FILE_NOT_FOUND	2L
#define ERROR_PATH_NOT_FOUND	3L
#define ERROR_ACCESS_DENIED		5L
#define ERROR_INVALID_HANDLE	6L
#define ERROR_NOT_ENOUGH_MEMORY	8L
#define ERROR_WRITE_PROTECT		19L
#define ERROR_NOT_READY			21L
#define ERROR_SHARING_VIOLATION	32L
#define ERROR_HANDLE_DISK_FULL	39L
#define ERROR_DISK_FULL			112L
#define ERROR_ALREADY_EXISTS	183L
#define ERROR_IO_PENDING		997L

#define WAIT_OBJECT_0		0x00000000L
#define WAIT_ABANDONED		0x00000080L
#define WAIT_TIMEOUT		0x00000102L
#define WAIT_FAILED			((DWORD)0xFFFFFFFF)

#define THREAD_PRIORITY_LOWEST			(-2)
#define THREAD_PRIORITY_BELOW_NORMAL	(-1)
#define THREAD_PRIORITY_NORMAL			0
#define THREAD_PRIORITY_ABOVE_NORMAL	1
#define THREAD_PRIORITY_HIGHEST			2
#define THREAD_PRIORITY_TIME_CRITICAL	15
#define NORMAL_PRIORITY_CLASS			0x00000020
#define HIGH_PRIORITY_CLASS				0x00000080
#define REALTIME_PRIORITY_CLASS			0x00000100

#define HKEY_CLASSES_ROOT	((HKEY)(ULONG_PTR)((LONG)0x80000000))
#define HKEY_CURRENT_USER	((HKEY)(ULONG_PTR)((LONG)0x80000001))
#define HKEY_LOCAL_MACHINE	((HKEY)(ULONG_PTR)((LONG)0x80000002))
#define HKEY_USERS			((HKEY)(ULONG_PTR)((LONG)0x80000003))
#define KEY_READ			0x20019
#define KEY_WRITE			0x20006
#define KEY_ALL_ACCESS		0xF003F
#define KEY_QUERY_VALUE		0x0001
#define KEY_SET_VALUE		0x0002
#define REG_NONE			0
#define REG_SZ				1
#define REG_BINARY			3
#define REG_DWORD			4
#define REG_OPTION_NON_VOLATILE 0x00000000L

/* Virtual key codes. */
#define VK_LBUTTON		0x01
#define VK_RBUTTON		0x02
#define VK_CANCEL		0x03
#define VK_MBUTTON		0x04
#define VK_BACK			0x08
#define VK_TAB			0x09
#define VK_CLEAR		0x0C
#define VK_RETURN		0x0D
#define VK_SHIFT		0x10
#define VK_CONTROL		0x11
#define VK_MENU			0x12
#define VK_PAUSE		0x13
#define VK_CAPITAL		0x14
#define VK_ESCAPE		0x1B
#define VK_SPACE		0x20
#define VK_PRIOR		0x21
#define VK_NEXT			0x22
#define VK_END			0x23
#define VK_HOME			0x24
#define VK_LEFT			0x25
#define VK_UP			0x26
#define VK_RIGHT		0x27
#define VK_DOWN			0x28
#define VK_SELECT		0x29
#define VK_PRINT		0x2A
#define VK_EXECUTE		0x2B
#define VK_SNAPSHOT		0x2C
#define VK_INSERT		0x2D
#define VK_DELETE		0x2E
#define VK_HELP			0x2F
#define VK_LWIN			0x5B
#define VK_RWIN			0x5C
#define VK_APPS			0x5D
#define VK_NUMPAD0		0x60
#define VK_NUMPAD1		0x61
#define VK_NUMPAD2		0x62
#define VK_NUMPAD3		0x63
#define VK_NUMPAD4		0x64
#define VK_NUMPAD5		0x65
#define VK_NUMPAD6		0x66
#define VK_NUMPAD7		0x67
#define VK_NUMPAD8		0x68
#define VK_NUMPAD9		0x69
#define VK_MULTIPLY		0x6A
#define VK_ADD			0x6B
#define VK_SEPARATOR	0x6C
#define VK_SUBTRACT		0x6D
#define VK_DECIMAL		0x6E
#define VK_DIVIDE		0x6F
#define VK_F1			0x70
#define VK_F2			0x71
#define VK_F3			0x72
#define VK_F4			0x73
#define VK_F5			0x74
#define VK_F6			0x75
#define VK_F7			0x76
#define VK_F8			0x77
#define VK_F9			0x78
#define VK_F10			0x79
#define VK_F11			0x7A
#define VK_F12			0x7B
#define VK_NUMLOCK		0x90
#define VK_SCROLL		0x91
#define VK_LSHIFT		0xA0
#define VK_RSHIFT		0xA1
#define VK_LCONTROL		0xA2
#define VK_RCONTROL		0xA3
#define VK_LMENU		0xA4
#define VK_RMENU		0xA5

/*
**	Functions (declared only; see the comment at the top of this file).
*/
#ifdef __cplusplus
extern "C" {
#endif

/* Windows and messages. */
ATOM WINAPI RegisterClass(WNDCLASS const * wc);
HWND WINAPI CreateWindow(LPCSTR cls, LPCSTR name, DWORD style, int x, int y, int w, int h, HWND parent, HMENU menu, HINSTANCE inst, LPVOID param);
HWND WINAPI CreateWindowEx(DWORD exstyle, LPCSTR cls, LPCSTR name, DWORD style, int x, int y, int w, int h, HWND parent, HMENU menu, HINSTANCE inst, LPVOID param);
BOOL WINAPI DestroyWindow(HWND);
BOOL WINAPI ShowWindow(HWND, int);
BOOL WINAPI UpdateWindow(HWND);
HWND WINAPI SetFocus(HWND);
HWND WINAPI GetFocus(void);
HWND WINAPI GetActiveWindow(void);
HWND WINAPI SetActiveWindow(HWND);
BOOL WINAPI SetForegroundWindow(HWND);
HWND WINAPI GetForegroundWindow(void);
HWND WINAPI FindWindow(LPCSTR cls, LPCSTR name);
BOOL WINAPI IsWindow(HWND);
BOOL WINAPI IsIconic(HWND);
BOOL WINAPI SetWindowPos(HWND, HWND, int, int, int, int, UINT);
BOOL WINAPI MoveWindow(HWND, int, int, int, int, BOOL);
BOOL WINAPI GetClientRect(HWND, LPRECT);
BOOL WINAPI GetWindowRect(HWND, LPRECT);
BOOL WINAPI InvalidateRect(HWND, RECT const *, BOOL);
BOOL WINAPI ValidateRect(HWND, RECT const *);
LONG WINAPI GetWindowLong(HWND, int);
LONG WINAPI SetWindowLong(HWND, int, LONG);
int WINAPI GetSystemMetrics(int);
LRESULT WINAPI DefWindowProc(HWND, UINT, WPARAM, LPARAM);
LRESULT WINAPI CallWindowProc(WNDPROC, HWND, UINT, WPARAM, LPARAM);
BOOL WINAPI PeekMessage(LPMSG, HWND, UINT, UINT, UINT);
BOOL WINAPI GetMessage(LPMSG, HWND, UINT, UINT);
BOOL WINAPI TranslateMessage(MSG const *);
LRESULT WINAPI DispatchMessage(MSG const *);
BOOL WINAPI PostMessage(HWND, UINT, WPARAM, LPARAM);
LRESULT WINAPI SendMessage(HWND, UINT, WPARAM, LPARAM);
void WINAPI PostQuitMessage(int);
BOOL WINAPI WaitMessage(void);
int WINAPI MessageBox(HWND, LPCSTR text, LPCSTR caption, UINT type);
INT_PTR WINAPI DialogBox(HINSTANCE, LPCSTR tmpl, HWND parent, DLGPROC);
UINT WINAPI RegisterWindowMessage(LPCSTR);
BOOL WINAPI MessageBeep(UINT);
HDC WINAPI BeginPaint(HWND, LPPAINTSTRUCT);
BOOL WINAPI EndPaint(HWND, PAINTSTRUCT const *);
HDC WINAPI GetDC(HWND);
int WINAPI ReleaseDC(HWND, HDC);
UINT_PTR WINAPI SetTimer(HWND, UINT_PTR, UINT, TIMERPROC);
BOOL WINAPI KillTimer(HWND, UINT_PTR);
SHORT WINAPI GetAsyncKeyState(int);
SHORT WINAPI GetKeyState(int);
BOOL WINAPI GetKeyboardState(BYTE *);
UINT WINAPI MapVirtualKey(UINT, UINT);
int WINAPI ToAscii(UINT, UINT, BYTE const *, LPWORD, UINT);
BOOL WINAPI GetCursorPos(LPPOINT);
BOOL WINAPI SetCursorPos(int, int);
int WINAPI ShowCursor(BOOL);
HCURSOR WINAPI SetCursor(HCURSOR);
HCURSOR WINAPI LoadCursor(HINSTANCE, LPCSTR);
HICON WINAPI LoadIcon(HINSTANCE, LPCSTR);
HWND WINAPI SetCapture(HWND);
BOOL WINAPI ReleaseCapture(void);
BOOL WINAPI ClipCursor(RECT const *);
BOOL WINAPI ScreenToClient(HWND, LPPOINT);
BOOL WINAPI ClientToScreen(HWND, LPPOINT);
int WINAPI LoadString(HINSTANCE, UINT, LPSTR, int);
int WINAPIV wsprintf(LPSTR, LPCSTR, ...);
int WINAPI wvsprintf(LPSTR, LPCSTR, va_list);
BOOL WINAPI SystemParametersInfo(UINT, UINT, PVOID, UINT);

/* GDI. */
HGDIOBJ WINAPI GetStockObject(int);
HGDIOBJ WINAPI SelectObject(HDC, HGDIOBJ);
BOOL WINAPI DeleteObject(HGDIOBJ);
COLORREF WINAPI SetTextColor(HDC, COLORREF);
COLORREF WINAPI SetBkColor(HDC, COLORREF);
int WINAPI SetBkMode(HDC, int);
BOOL WINAPI TextOut(HDC, int, int, LPCSTR, int);
HPALETTE WINAPI CreatePalette(LOGPALETTE const *);
HPALETTE WINAPI SelectPalette(HDC, HPALETTE, BOOL);
UINT WINAPI RealizePalette(HDC);
UINT WINAPI GetSystemPaletteEntries(HDC, UINT, UINT, LPPALETTEENTRY);
HBRUSH WINAPI CreateSolidBrush(COLORREF);
int WINAPI FillRect(HDC, RECT const *, HBRUSH);
int WINAPI GetDeviceCaps(HDC, int);

/* Kernel: process, threads, synchronisation, time. */
HMODULE WINAPI GetModuleHandle(LPCSTR);
DWORD WINAPI GetModuleFileName(HMODULE, LPSTR, DWORD);
HMODULE WINAPI LoadLibrary(LPCSTR);
BOOL WINAPI FreeLibrary(HMODULE);
FARPROC WINAPI GetProcAddress(HMODULE, LPCSTR);
DWORD WINAPI GetLastError(void);
void WINAPI SetLastError(DWORD);
void WINAPI Sleep(DWORD);
DWORD WINAPI GetTickCount(void);
BOOL WINAPI QueryPerformanceCounter(LARGE_INTEGER *);
BOOL WINAPI QueryPerformanceFrequency(LARGE_INTEGER *);
void WINAPI GetLocalTime(LPSYSTEMTIME);
void WINAPI GetSystemTime(LPSYSTEMTIME);
BOOL WINAPI CloseHandle(HANDLE);
HANDLE WINAPI CreateThread(LPSECURITY_ATTRIBUTES, SIZE_T, LPTHREAD_START_ROUTINE, LPVOID, DWORD, LPDWORD);
HANDLE WINAPI GetCurrentThread(void);
DWORD WINAPI GetCurrentThreadId(void);
HANDLE WINAPI GetCurrentProcess(void);
DWORD WINAPI GetCurrentProcessId(void);
BOOL WINAPI SetThreadPriority(HANDLE, int);
int WINAPI GetThreadPriority(HANDLE);
BOOL WINAPI SetPriorityClass(HANDLE, DWORD);
DWORD WINAPI SuspendThread(HANDLE);
DWORD WINAPI ResumeThread(HANDLE);
BOOL WINAPI TerminateThread(HANDLE, DWORD);
void WINAPI ExitProcess(UINT);
HANDLE WINAPI CreateEvent(LPSECURITY_ATTRIBUTES, BOOL, BOOL, LPCSTR);
HANDLE WINAPI CreateMutex(LPSECURITY_ATTRIBUTES, BOOL, LPCSTR);
BOOL WINAPI ReleaseMutex(HANDLE);
BOOL WINAPI SetEvent(HANDLE);
BOOL WINAPI ResetEvent(HANDLE);
DWORD WINAPI WaitForSingleObject(HANDLE, DWORD);
DWORD WINAPI WaitForMultipleObjects(DWORD, HANDLE const *, BOOL, DWORD);
void WINAPI InitializeCriticalSection(LPCRITICAL_SECTION);
void WINAPI DeleteCriticalSection(LPCRITICAL_SECTION);
void WINAPI EnterCriticalSection(LPCRITICAL_SECTION);
void WINAPI LeaveCriticalSection(LPCRITICAL_SECTION);
void WINAPI OutputDebugString(LPCSTR);
BOOL WINAPI GetVersionEx(LPOSVERSIONINFO);
DWORD WINAPI GetVersion(void);
void WINAPI GetSystemInfo(LPSYSTEM_INFO);
void WINAPI GlobalMemoryStatus(LPMEMORYSTATUS);
LPSTR WINAPI GetCommandLine(void);

/* Kernel: memory. */
HGLOBAL WINAPI GlobalAlloc(UINT, SIZE_T);
HGLOBAL WINAPI GlobalReAlloc(HGLOBAL, SIZE_T, UINT);
HGLOBAL WINAPI GlobalFree(HGLOBAL);
LPVOID WINAPI GlobalLock(HGLOBAL);
BOOL WINAPI GlobalUnlock(HGLOBAL);
SIZE_T WINAPI GlobalSize(HGLOBAL);
HLOCAL WINAPI LocalAlloc(UINT, SIZE_T);
HLOCAL WINAPI LocalFree(HLOCAL);
LPVOID WINAPI VirtualAlloc(LPVOID, SIZE_T, DWORD, DWORD);
BOOL WINAPI VirtualFree(LPVOID, SIZE_T, DWORD);
BOOL WINAPI VirtualLock(LPVOID, SIZE_T);
BOOL WINAPI VirtualUnlock(LPVOID, SIZE_T);
BOOL WINAPI VirtualProtect(LPVOID, SIZE_T, DWORD, PDWORD);
BOOL WINAPI IsBadReadPtr(void const *, UINT_PTR);
BOOL WINAPI IsBadWritePtr(LPVOID, UINT_PTR);

/* Kernel: files. */
HANDLE WINAPI CreateFile(LPCSTR, DWORD, DWORD, LPSECURITY_ATTRIBUTES, DWORD, DWORD, HANDLE);
BOOL WINAPI ReadFile(HANDLE, LPVOID, DWORD, LPDWORD, LPOVERLAPPED);
BOOL WINAPI WriteFile(HANDLE, LPCVOID, DWORD, LPDWORD, LPOVERLAPPED);
DWORD WINAPI SetFilePointer(HANDLE, LONG, PLONG, DWORD);
DWORD WINAPI GetFileSize(HANDLE, LPDWORD);
BOOL WINAPI GetFileTime(HANDLE, LPFILETIME, LPFILETIME, LPFILETIME);
BOOL WINAPI SetFileTime(HANDLE, FILETIME const *, FILETIME const *, FILETIME const *);
BOOL WINAPI FlushFileBuffers(HANDLE);
BOOL WINAPI SetEndOfFile(HANDLE);
BOOL WINAPI DeleteFile(LPCSTR);
BOOL WINAPI CopyFile(LPCSTR, LPCSTR, BOOL);
BOOL WINAPI MoveFile(LPCSTR, LPCSTR);
DWORD WINAPI GetFileAttributes(LPCSTR);
BOOL WINAPI SetFileAttributes(LPCSTR, DWORD);
HANDLE WINAPI FindFirstFile(LPCSTR, LPWIN32_FIND_DATA);
BOOL WINAPI FindNextFile(HANDLE, LPWIN32_FIND_DATA);
BOOL WINAPI FindClose(HANDLE);
BOOL WINAPI CreateDirectory(LPCSTR, LPSECURITY_ATTRIBUTES);
BOOL WINAPI RemoveDirectory(LPCSTR);
DWORD WINAPI GetCurrentDirectory(DWORD, LPSTR);
BOOL WINAPI SetCurrentDirectory(LPCSTR);
UINT WINAPI GetDriveType(LPCSTR);
DWORD WINAPI GetLogicalDrives(void);
DWORD WINAPI GetLogicalDriveStrings(DWORD, LPSTR);
BOOL WINAPI GetVolumeInformation(LPCSTR, LPSTR, DWORD, LPDWORD, LPDWORD, LPDWORD, LPSTR, DWORD);
BOOL WINAPI GetDiskFreeSpace(LPCSTR, LPDWORD, LPDWORD, LPDWORD, LPDWORD);
UINT WINAPI GetWindowsDirectory(LPSTR, UINT);
UINT WINAPI GetSystemDirectory(LPSTR, UINT);
DWORD WINAPI GetTempPath(DWORD, LPSTR);
UINT WINAPI SetErrorMode(UINT);
BOOL WINAPI FileTimeToSystemTime(FILETIME const *, LPSYSTEMTIME);
BOOL WINAPI FileTimeToLocalFileTime(FILETIME const *, LPFILETIME);
BOOL WINAPI SystemTimeToFileTime(SYSTEMTIME const *, LPFILETIME);
BOOL WINAPI FileTimeToDosDateTime(FILETIME const *, LPWORD, LPWORD);
BOOL WINAPI DosDateTimeToFileTime(WORD, WORD, LPFILETIME);
DWORD WINAPI GetPrivateProfileString(LPCSTR, LPCSTR, LPCSTR, LPSTR, DWORD, LPCSTR);
UINT WINAPI GetPrivateProfileInt(LPCSTR, LPCSTR, INT, LPCSTR);
BOOL WINAPI WritePrivateProfileString(LPCSTR, LPCSTR, LPCSTR, LPCSTR);

/* Registry. */
LONG WINAPI RegOpenKeyEx(HKEY, LPCSTR, DWORD, REGSAM, PHKEY);
LONG WINAPI RegOpenKey(HKEY, LPCSTR, PHKEY);
LONG WINAPI RegCreateKey(HKEY, LPCSTR, PHKEY);
LONG WINAPI RegCreateKeyEx(HKEY, LPCSTR, DWORD, LPSTR, DWORD, REGSAM, LPSECURITY_ATTRIBUTES, PHKEY, LPDWORD);
LONG WINAPI RegQueryValueEx(HKEY, LPCSTR, LPDWORD, LPDWORD, LPBYTE, LPDWORD);
LONG WINAPI RegSetValueEx(HKEY, LPCSTR, DWORD, DWORD, BYTE const *, DWORD);
LONG WINAPI RegDeleteValue(HKEY, LPCSTR);
LONG WINAPI RegCloseKey(HKEY);

/* Shell and misc. */
HINSTANCE WINAPI ShellExecute(HWND, LPCSTR, LPCSTR, LPCSTR, LPCSTR, INT);
UINT WINAPI WinExec(LPCSTR, UINT);
int WINAPI MulDiv(int, int, int);
LONG WINAPI InterlockedIncrement(LONG volatile *);
LONG WINAPI InterlockedDecrement(LONG volatile *);
LONG WINAPI InterlockedExchange(LONG volatile *, LONG);

#ifdef __cplusplus
}
#endif

#define ZeroMemory(d, n)		memset((d), 0, (n))
#define FillMemory(d, n, c)		memset((d), (c), (n))
#define CopyMemory(d, s, n)		memcpy((d), (s), (n))
#define MoveMemory(d, s, n)		memmove((d), (s), (n))

/* The multimedia API (timeGetTime, waveOut*, mci*) comes with windows.h, as in the SDK. */
#include <mmsystem.h>

#endif /* RA_COMPAT_WINDOWS_H */
