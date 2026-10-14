/*
**	Command & Conquer Red Alert(tm) - multiplayer port. GPLv3 or later, see LICENSE.md.
**	Minimal Windows multimedia (winmm) declarations for non-Windows hosts
**	(session S06a, ADR-0014). Declarations only; see windows.h in this directory.
*/

#ifndef RA_COMPAT_MMSYSTEM_H
#define RA_COMPAT_MMSYSTEM_H

#include <windows.h>

typedef UINT MMRESULT;
typedef UINT MMVERSION;
typedef DWORD MCIERROR;
typedef UINT MCIDEVICEID;

#define MMSYSERR_NOERROR	0
#define MMSYSERR_ERROR		1
#define TIMERR_NOERROR		0
#define TIMERR_NOCANDO		97
#define TIME_ONESHOT		0x0000
#define TIME_PERIODIC		0x0001
#define TIME_CALLBACK_FUNCTION 0x0000

#pragma pack(push, 8)
typedef struct timecaps_tag {UINT wPeriodMin; UINT wPeriodMax;} TIMECAPS, *PTIMECAPS, *LPTIMECAPS;

/* WAVEFORMATEX is byte-packed in the SDK (pshpack1 in mmreg.h/mmsystem.h). */
#pragma pack(push, 1)
typedef struct tWAVEFORMATEX {
	WORD wFormatTag;
	WORD nChannels;
	DWORD nSamplesPerSec;
	DWORD nAvgBytesPerSec;
	WORD nBlockAlign;
	WORD wBitsPerSample;
	WORD cbSize;
} WAVEFORMATEX, *PWAVEFORMATEX, *LPWAVEFORMATEX;
typedef WAVEFORMATEX const * LPCWAVEFORMATEX;
typedef struct waveformat_tag {
	WORD wFormatTag;
	WORD nChannels;
	DWORD nSamplesPerSec;
	DWORD nAvgBytesPerSec;
	WORD nBlockAlign;
} WAVEFORMAT, *LPWAVEFORMAT;
typedef struct pcmwaveformat_tag {WAVEFORMAT wf; WORD wBitsPerSample;} PCMWAVEFORMAT, *LPPCMWAVEFORMAT;
#pragma pack(pop)
#define WAVE_FORMAT_PCM 1

typedef struct _MMIOINFO {
	DWORD dwFlags;
	DWORD fccIOProc;
	void * pIOProc;
	UINT wErrorRet;
	void * htask;
	LONG cchBuffer;
	char * pchBuffer;
	char * pchNext;
	char * pchEndRead;
	char * pchEndWrite;
	LONG lBufOffset;
	LONG lDiskOffset;
	DWORD adwInfo[3];
	DWORD dwReserved1;
	DWORD dwReserved2;
	HMMIO hmmio;
} MMIOINFO, *LPMMIOINFO;
#pragma pack(pop)

#define MMIO_READ		0x00000000L
#define MMIO_WRITE		0x00000001L
#define MMIO_READWRITE	0x00000002L
#define MMIO_CREATE		0x00001000L

typedef void (CALLBACK TIMECALLBACK)(UINT uTimerID, UINT uMsg, DWORD_PTR dwUser, DWORD_PTR dw1, DWORD_PTR dw2);
typedef TIMECALLBACK * LPTIMECALLBACK;

#ifdef __cplusplus
extern "C" {
#endif
DWORD WINAPI timeGetTime(void);
MMRESULT WINAPI timeGetDevCaps(LPTIMECAPS, UINT);
MMRESULT WINAPI timeBeginPeriod(UINT);
MMRESULT WINAPI timeEndPeriod(UINT);
MMRESULT WINAPI timeSetEvent(UINT, UINT, LPTIMECALLBACK, DWORD_PTR, UINT);
MMRESULT WINAPI timeKillEvent(UINT);
HMMIO WINAPI mmioOpen(LPSTR, LPMMIOINFO, DWORD);
MMRESULT WINAPI mmioClose(HMMIO, UINT);
LONG WINAPI mmioWrite(HMMIO, char const *, LONG);
LONG WINAPI mmioRead(HMMIO, char *, LONG);
MCIERROR WINAPI mciSendCommand(MCIDEVICEID, UINT, DWORD_PTR, DWORD_PTR);
BOOL WINAPI mciGetErrorString(MCIERROR, LPSTR, UINT);
#ifdef __cplusplus
}
#endif

#endif /* RA_COMPAT_MMSYSTEM_H */
