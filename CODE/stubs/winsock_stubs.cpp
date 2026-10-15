/*
**	Command & Conquer Red Alert(tm) - multiplayer port. GPLv3 or later, see LICENSE.md.
**
**	STUB (session S10b). REPLACED IN S12, which rewrites the UDP transport with polled
**	sockets (docs/design/lan-multiplayer.md). See docs/codebase/stubs.md.
**
**	Why a stub: the old transport (CODE/WSPROTO.CPP, CODE/WSPUDP.CPP) is driven by
**	WSAAsyncSelect window messages sent to MainWindow. There is no Win32 window any more,
**	so those messages can never arrive, and emulating them is wasted work when S12
**	replaces the transport anyway.
**
**	Behaviour: WSAStartup fails, so TCPIPClass/UDPInterfaceClass::Init returns false and
**	the game reports that the network is not available. LAN play does not work until S12;
**	that is expected. Everything else is a harmless no-op.
**
**	Only for non-Windows hosts. On Windows the game links the real Winsock (ws2_32).
*/

#if !defined(_WIN32)

#pragma pack(push, 8)
#include <unistd.h>
#pragma pack(pop)

#include <windows.h>
#include <winsock.h>

/* WSASYSNOTREADY: "the underlying network subsystem is not ready". */
static int const STUB_WSA_ERROR = WSABASEERR + 91;

int WINAPI WSAStartup(WORD, LPWSADATA data)
{
	if (data) memset(data, 0, sizeof(*data));
	return STUB_WSA_ERROR;
}

int WINAPI WSACleanup(void)
{
	return 0;
}

int WINAPI WSAGetLastError(void)
{
	return STUB_WSA_ERROR;
}

int WINAPI WSAAsyncSelect(SOCKET, HWND, UINT, LONG)
{
	return SOCKET_ERROR;
}

int WINAPI WSACancelAsyncRequest(HANDLE)
{
	return 0;
}

/* The only call that does real work: sockets are file descriptors here. */
int WINAPI closesocket(SOCKET s)
{
	return close((int)s) == 0 ? 0 : SOCKET_ERROR;
}

#endif /* !_WIN32 */
