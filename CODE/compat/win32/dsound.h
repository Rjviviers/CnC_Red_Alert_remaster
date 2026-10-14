/*
**	Command & Conquer Red Alert(tm) - multiplayer port. GPLv3 or later, see LICENSE.md.
**	Minimal DirectSound (DirectX 5) declarations for non-Windows hosts
**	(session S06a, ADR-0014). Declarations only; see windows.h in this directory.
**	DirectSound is replaced by SDL audio in session S09 (ADR-0004).
*/

#ifndef RA_COMPAT_DSOUND_H
#define RA_COMPAT_DSOUND_H

#include <windows.h>
#include <mmsystem.h>

struct IDirectSound;
struct IDirectSoundBuffer;
typedef struct IDirectSound * LPDIRECTSOUND;
typedef struct IDirectSoundBuffer * LPDIRECTSOUNDBUFFER;
typedef LPDIRECTSOUNDBUFFER * LPLPDIRECTSOUNDBUFFER;

#pragma pack(push, 8)
typedef struct _DSCAPS {
	DWORD dwSize;
	DWORD dwFlags;
	DWORD dwMinSecondarySampleRate;
	DWORD dwMaxSecondarySampleRate;
	DWORD dwPrimaryBuffers;
	DWORD dwMaxHwMixingAllBuffers;
	DWORD dwMaxHwMixingStaticBuffers;
	DWORD dwMaxHwMixingStreamingBuffers;
	DWORD dwFreeHwMixingAllBuffers;
	DWORD dwFreeHwMixingStaticBuffers;
	DWORD dwFreeHwMixingStreamingBuffers;
	DWORD dwMaxHw3DAllBuffers;
	DWORD dwMaxHw3DStaticBuffers;
	DWORD dwMaxHw3DStreamingBuffers;
	DWORD dwFreeHw3DAllBuffers;
	DWORD dwFreeHw3DStaticBuffers;
	DWORD dwFreeHw3DStreamingBuffers;
	DWORD dwTotalHwMemBytes;
	DWORD dwFreeHwMemBytes;
	DWORD dwMaxContigFreeHwMemBytes;
	DWORD dwUnlockTransferRateHwBuffers;
	DWORD dwPlayCpuOverheadSwBuffers;
	DWORD dwReserved1;
	DWORD dwReserved2;
} DSCAPS, *LPDSCAPS;

typedef struct _DSBCAPS {
	DWORD dwSize;
	DWORD dwFlags;
	DWORD dwBufferBytes;
	DWORD dwUnlockTransferRate;
	DWORD dwPlayCpuOverhead;
} DSBCAPS, *LPDSBCAPS;

typedef struct _DSBUFFERDESC {
	DWORD dwSize;
	DWORD dwFlags;
	DWORD dwBufferBytes;
	DWORD dwReserved;
	LPWAVEFORMATEX lpwfxFormat;
} DSBUFFERDESC, *LPDSBUFFERDESC;
typedef DSBUFFERDESC const * LPCDSBUFFERDESC;
#pragma pack(pop)

DECLARE_INTERFACE_(IDirectSound, IUnknown)
{
	STDMETHOD(CreateSoundBuffer)(THIS_ LPCDSBUFFERDESC, LPDIRECTSOUNDBUFFER *, IUnknown *) PURE;
	STDMETHOD(GetCaps)(THIS_ LPDSCAPS) PURE;
	STDMETHOD(DuplicateSoundBuffer)(THIS_ LPDIRECTSOUNDBUFFER, LPDIRECTSOUNDBUFFER *) PURE;
	STDMETHOD(SetCooperativeLevel)(THIS_ HWND, DWORD) PURE;
	STDMETHOD(Compact)(THIS) PURE;
	STDMETHOD(GetSpeakerConfig)(THIS_ LPDWORD) PURE;
	STDMETHOD(SetSpeakerConfig)(THIS_ DWORD) PURE;
	STDMETHOD(Initialize)(THIS_ GUID const *) PURE;
};

DECLARE_INTERFACE_(IDirectSoundBuffer, IUnknown)
{
	STDMETHOD(GetCaps)(THIS_ LPDSBCAPS) PURE;
	STDMETHOD(GetCurrentPosition)(THIS_ LPDWORD, LPDWORD) PURE;
	STDMETHOD(GetFormat)(THIS_ LPWAVEFORMATEX, DWORD, LPDWORD) PURE;
	STDMETHOD(GetVolume)(THIS_ LPLONG) PURE;
	STDMETHOD(GetPan)(THIS_ LPLONG) PURE;
	STDMETHOD(GetFrequency)(THIS_ LPDWORD) PURE;
	STDMETHOD(GetStatus)(THIS_ LPDWORD) PURE;
	STDMETHOD(Initialize)(THIS_ LPDIRECTSOUND, LPCDSBUFFERDESC) PURE;
	STDMETHOD(Lock)(THIS_ DWORD, DWORD, LPVOID *, LPDWORD, LPVOID *, LPDWORD, DWORD) PURE;
	STDMETHOD(Play)(THIS_ DWORD, DWORD, DWORD) PURE;
	STDMETHOD(SetCurrentPosition)(THIS_ DWORD) PURE;
	STDMETHOD(SetFormat)(THIS_ LPCWAVEFORMATEX) PURE;
	STDMETHOD(SetVolume)(THIS_ LONG) PURE;
	STDMETHOD(SetPan)(THIS_ LONG) PURE;
	STDMETHOD(SetFrequency)(THIS_ DWORD) PURE;
	STDMETHOD(Stop)(THIS) PURE;
	STDMETHOD(Unlock)(THIS_ LPVOID, DWORD, LPVOID, DWORD) PURE;
	STDMETHOD(Restore)(THIS) PURE;
};

extern "C" HRESULT WINAPI DirectSoundCreate(GUID const *, LPDIRECTSOUND *, IUnknown *);

#define _FACDS				0x878
#define MAKE_DSHRESULT(i)	MAKE_HRESULT(1, _FACDS, i)
#define DS_OK				0
#define DSERR_ALLOCATED		MAKE_DSHRESULT(10)
#define DSERR_INVALIDPARAM	E_INVALIDARG
#define DSERR_BUFFERLOST	MAKE_DSHRESULT(150)
#define DSERR_PRIOLEVELNEEDED	MAKE_DSHRESULT(70)
#define DSERR_NODRIVER		MAKE_DSHRESULT(120)
#define DSERR_OUTOFMEMORY	E_OUTOFMEMORY

#define DSSCL_NORMAL		0x00000001
#define DSSCL_PRIORITY		0x00000002
#define DSSCL_EXCLUSIVE		0x00000003
#define DSSCL_WRITEPRIMARY	0x00000004

#define DSBCAPS_PRIMARYBUFFER	0x00000001
#define DSBCAPS_STATIC			0x00000002
#define DSBCAPS_LOCHARDWARE		0x00000004
#define DSBCAPS_LOCSOFTWARE		0x00000008
#define DSBCAPS_CTRLFREQUENCY	0x00000020
#define DSBCAPS_CTRLPAN			0x00000040
#define DSBCAPS_CTRLVOLUME		0x00000080
#define DSBCAPS_CTRLDEFAULT		0x000000E0
#define DSBCAPS_STICKYFOCUS		0x00004000
#define DSBCAPS_GETCURRENTPOSITION2	0x00010000
#define DSBCAPS_GLOBALFOCUS		0x00008000

#define DSBPLAY_LOOPING		0x00000001
#define DSBSTATUS_PLAYING	0x00000001
#define DSBSTATUS_BUFFERLOST	0x00000002
#define DSBSTATUS_LOOPING	0x00000004
#define DSBLOCK_FROMWRITECURSOR	0x00000001
#define DSBVOLUME_MIN		-10000
#define DSBVOLUME_MAX		0

#endif /* RA_COMPAT_DSOUND_H */
