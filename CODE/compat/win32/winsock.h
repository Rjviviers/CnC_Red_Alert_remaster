/*
**	Command & Conquer Red Alert(tm) - multiplayer port. GPLv3 or later, see LICENSE.md.
**	Minimal WinSock 1.1 declarations for non-Windows hosts (session S06a, ADR-0014).
**	The BSD socket calls come from the real POSIX headers; only the WinSock-specific
**	names are added. The WinSock-only functions (WSAStartup, WSAAsyncSelect, ...) are
**	declared, never defined: session S12 replaces this code with portable sockets.
*/

#ifndef RA_COMPAT_WINSOCK_H
#define RA_COMPAT_WINSOCK_H

#include <windows.h>

#if defined(RA_PACK_STRUCTS)
#pragma pack(push, 8)
#endif
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#if defined(RA_PACK_STRUCTS)
#pragma pack(pop)
#endif

typedef UINT_PTR SOCKET;
typedef unsigned char u_char;
typedef unsigned short u_short;
typedef unsigned int u_int;
typedef unsigned long u_long;

#define INVALID_SOCKET	((SOCKET)(~0))
#define SOCKET_ERROR	(-1)

typedef struct sockaddr SOCKADDR;
typedef struct sockaddr * PSOCKADDR;
typedef struct sockaddr * LPSOCKADDR;
typedef struct sockaddr_in SOCKADDR_IN;
typedef struct sockaddr_in * PSOCKADDR_IN;
typedef struct sockaddr_in * LPSOCKADDR_IN;
typedef struct in_addr IN_ADDR;
typedef struct in_addr * PIN_ADDR;
typedef struct in_addr * LPIN_ADDR;
typedef struct hostent HOSTENT;
typedef struct hostent * PHOSTENT;
typedef struct hostent * LPHOSTENT;

#define WSADESCRIPTION_LEN	256
#define WSASYS_STATUS_LEN	128
#pragma pack(push, 8)
typedef struct WSAData {
	WORD wVersion;
	WORD wHighVersion;
	char szDescription[WSADESCRIPTION_LEN + 1];
	char szSystemStatus[WSASYS_STATUS_LEN + 1];
	unsigned short iMaxSockets;
	unsigned short iMaxUdpDg;
	char * lpVendorInfo;
} WSADATA, *LPWSADATA;
#pragma pack(pop)

/* WSAAsyncSelect network events. */
#define FD_READ		0x01
#define FD_WRITE	0x02
#define FD_OOB		0x04
#define FD_ACCEPT	0x08
#define FD_CONNECT	0x10
#define FD_CLOSE	0x20

#define WSAMAKESELECTREPLY(event, error)	MAKELONG(event, error)
#define WSAGETSELECTEVENT(lParam)			LOWORD(lParam)
#define WSAGETSELECTERROR(lParam)			HIWORD(lParam)
#define WSAGETASYNCERROR(lParam)			HIWORD(lParam)
#define WSAGETASYNCBUFLEN(lParam)			LOWORD(lParam)

#define WSABASEERR			10000
#define WSAEINTR			(WSABASEERR+4)
#define WSAEBADF			(WSABASEERR+9)
#define WSAEACCES			(WSABASEERR+13)
#define WSAEFAULT			(WSABASEERR+14)
#define WSAEINVAL			(WSABASEERR+22)
#define WSAEMFILE			(WSABASEERR+24)
#define WSAEWOULDBLOCK		(WSABASEERR+35)
#define WSAEINPROGRESS		(WSABASEERR+36)
#define WSAEALREADY			(WSABASEERR+37)
#define WSAENOTSOCK			(WSABASEERR+38)
#define WSAEMSGSIZE			(WSABASEERR+40)
#define WSAEADDRINUSE		(WSABASEERR+48)
#define WSAEADDRNOTAVAIL	(WSABASEERR+49)
#define WSAENETDOWN			(WSABASEERR+50)
#define WSAENETUNREACH		(WSABASEERR+51)
#define WSAECONNRESET		(WSABASEERR+54)
#define WSAENOBUFS			(WSABASEERR+55)
#define WSAENOTCONN			(WSABASEERR+57)
#define WSAETIMEDOUT		(WSABASEERR+60)
#define WSAECONNREFUSED		(WSABASEERR+61)
#define WSAHOST_NOT_FOUND	(WSABASEERR+1001)
#define WSANOTINITIALISED	(WSABASEERR+93)

#ifndef FIONBIO
#define FIONBIO				0x8004667E
#endif

#ifdef __cplusplus
extern "C" {
#endif
int WINAPI WSAStartup(WORD, LPWSADATA);
int WINAPI WSACleanup(void);
int WINAPI WSAGetLastError(void);
void WINAPI WSASetLastError(int);
int WINAPI WSAAsyncSelect(SOCKET, HWND, UINT, LONG);
HANDLE WINAPI WSAAsyncGetHostByName(HWND, UINT, char const *, char *, int);
HANDLE WINAPI WSAAsyncGetHostByAddr(HWND, UINT, char const *, int, int, char *, int);
int WINAPI WSACancelAsyncRequest(HANDLE);
int WINAPI closesocket(SOCKET);
int WINAPI ioctlsocket(SOCKET, long, u_long *);
#ifdef __cplusplus
}
#endif

#endif /* RA_COMPAT_WINSOCK_H */
